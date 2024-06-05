/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include "music.h"
#include "loading.h"

// BCSTM Player adapted from BCSTM-Player by tobid7
// https://github.com/NPI-D7/BCSTM-Player/blob/main/source/bcstm.hpp

u32 read32(char *music_buf, ssize_t *cursor)
{
    u32 ret;
    memcpy(&ret, music_buf + *cursor, 4);
    *cursor += 4;
    return ret;
}

u16 read16(char *music_buf, ssize_t *cursor)
{
    u16 ret;
    memcpy(&ret, music_buf + *cursor, 2);
    *cursor += 2;
    return ret;
}

u8 read8(char *music_buf, ssize_t *cursor)
{
    u8 ret;
    memcpy(&ret, music_buf + *cursor, 1);
    *cursor += 1;
    return ret;
}

Result update_audio_ogg(audio_ogg_s * audio) 
{
    u32 size = audio->wave_buf[audio->buf_pos].nsamples * 4 - audio->data_read;
    DEBUG("<update_audio> Audio Size: %ld\n", size);
    if (audio->wave_buf[audio->buf_pos].status == NDSP_WBUF_DONE) // only run if the current selected buffer has already finished playing
    { 
        DEBUG("<update_audio> Attempting ov_read\n");
        int bitstream;
        u32 read = ov_read(&audio->vf, (char *)audio->wave_buf[audio->buf_pos].data_vaddr + audio->data_read, size, &bitstream); // read 1 vorbis packet into wave buffer
        DEBUG("<update_audio> ov_read successful\n");

        if (read <= 0) // EoF or error
        { 
            ov_clear(&audio->vf);
            if (read == 0) // EoF
            { 
                ov_open(fmemopen(audio->filebuf, audio->filesize, "rb"), &audio->vf, NULL, 0); // Reopen file. Don't need to reinit channel stuff since it's all the same as before
            } else // Error :(
            { 
                DEBUG("<update_audio> Vorbis play error: %ld\n", read);
                ndspChnReset(0);
                return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_NO_DATA);
            }
        } else 
        {
            audio->data_read += read;
            if (read == size) {
                audio->data_read = 0;
                ndspChnWaveBufAdd(0, &audio->wave_buf[audio->buf_pos]); // Add buffer to ndsp 
                audio->buf_pos = 1 - audio->buf_pos; // switch to other buffer to load and prepare it while the current one is playing
            }
        }
    }
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
}

void thread_audio_ogg(void * data) {
    audio_ogg_s * audio = (audio_ogg_s *)data;
    while(!audio->stop) {
        update_audio_ogg(audio);
    }
    ndspChnWaveBufClear(0);
    ndspChnReset(0);
    ov_clear(&audio->vf);
    free(audio->filebuf);
    linearFree((void *)audio->wave_buf[0].data_vaddr);
    linearFree((void *)audio->wave_buf[1].data_vaddr);
}

void play_audio_ogg(audio_ogg_s * audio) {
    audio->playing_thread = threadCreate(thread_audio_ogg, audio, 0x1000, 0x3F, 1, false);
}

void stop_audio_ogg(audio_ogg_s ** audio_ptr) {
    audio_ogg_s * audio = *audio_ptr;
    if(audio->playing_thread)
    {
        audio->stop = true;
        threadJoin(audio->playing_thread, U64_MAX);
        threadFree(audio->playing_thread);
    }
    free(audio);
    *audio_ptr = NULL;
}

int init_audio(audio_s *audio)
{
    u32 magic = read32(audio->music_buf, &audio->cursor);
    DEBUG("Loading music, music_size: %d, magic: 0x%08lx\n", audio->music_size, magic);
    audio->is_little_endian = read16(audio->music_buf, &audio->cursor) == 0xFEFF;
    audio->info_offset = 0;
    audio->data_offset = 0;

    if (magic != 0x4D545343) // CSTM
    {
        free(audio->music_buf);
        return -1;
    }

    audio->cursor = 0x10;
    u16 sbc = read16(audio->music_buf, &audio->cursor);
    audio->cursor += 2;

    for (u16 i = 0; i < sbc; ++i)
    {
        u16 sec = read16(audio->music_buf, &audio->cursor);
        audio->cursor += 2;
        u32 off = read32(audio->music_buf, &audio->cursor);
        audio->cursor += 4;
        DEBUG("Reading sbc: %04x, %08lx\n", sec, off);
        if (sec == 0x4000) // Info block
            audio->info_offset = off;
        if (sec == 0x4002) // Data block
            audio->data_offset = off;
    }

    DEBUG("Info offset: 0x%08lx, data offset: 0x%08lx\n", audio->info_offset, audio->data_offset);
    
    if (audio->data_offset == 0 || audio->info_offset == 0)
    {
        free(audio->music_buf);
        return -2;
    }

    audio->cursor = audio->info_offset + 0x20;

    if (read8(audio->music_buf, &audio->cursor) != 2) // Encoding - 2 is DSP_ADPCM
    {
        free(audio->music_buf);
        return -3;
    }

    audio->is_looping = read8(audio->music_buf, &audio->cursor);
    audio->channel_count = read8(audio->music_buf, &audio->cursor);

    if (audio->channel_count > 2)
    {
        free(audio->music_buf);
        return -4;
    }

    DEBUG("Channel count: %d, is looping: %d\n", audio->channel_count, audio->is_looping);
    
    audio->cursor = audio->info_offset + 0x24;
    audio->sample_rate = read32(audio->music_buf, &audio->cursor);
    u32 _loop_pos = read32(audio->music_buf, &audio->cursor);
    u32 _loop_end = read32(audio->music_buf, &audio->cursor);
    audio->num_blocks = read32(audio->music_buf, &audio->cursor);
    audio->block_size = read32(audio->music_buf, &audio->cursor);
    audio->block_samples = read32(audio->music_buf, &audio->cursor);
    audio->cursor += 4;
    audio->last_block_samples = read32(audio->music_buf, &audio->cursor);
    audio->last_block_size = read32(audio->music_buf, &audio->cursor);

    DEBUG("sample_rate: %lu, loop_start: %lu, loop_end: %lu, num_blocks: %lu, block_size: %lu, block_samples: %lu, last_block_samples: %lu, last_block_size: %lu\n", 
        audio->sample_rate, _loop_pos, _loop_end, audio->num_blocks, audio->block_size, audio->block_samples, audio->last_block_samples, audio->last_block_size);

    audio->loop_start = _loop_pos / audio->block_samples;
    audio->loop_end = (_loop_end % audio->block_samples ? audio->num_blocks : _loop_end / audio->block_samples);

    while (read32(audio->music_buf, &audio->cursor) != 0x4102); // find channel info header
    audio->cursor += read32(audio->music_buf, &audio->cursor) + audio->channel_count * 8 - 12;

    for (u8 i = 0; i < audio->channel_count; ++i)
    {
        memcpy(audio->adpcm_coefs[i], audio->music_buf + audio->cursor, sizeof(unsigned short) * 16);
        audio->cursor += sizeof(unsigned short) * 16;
        memcpy(&(audio->adpcm_data[i][0]), audio->music_buf + audio->cursor, sizeof(ndspAdpcmData));
        audio->cursor += sizeof(ndspAdpcmData);
        memcpy(&(audio->adpcm_data[i][1]), audio->music_buf + audio->cursor, sizeof(ndspAdpcmData));
        audio->cursor += sizeof(ndspAdpcmData);
        audio->cursor += 2; // skip padding
    }

    audio->cursor = audio->data_offset + 0x20;

    return 0;
}

int start_play(audio_s *audio) {
    audio->current_block = 0;
    for (u8 i = 0; i < audio->channel_count; ++i)
    {
        audio->channel[i] = 0;
        while (audio->channel[i] < 24 && ((audio->active_channels >> audio->channel[i]) & 1)) {
            audio->channel[i]++;
        }
        if (audio->channel[i] == 24) {
            return -1;
        }
        audio->active_channels |= 1 << audio->channel[i];
        ndspChnWaveBufClear(audio->channel[i]);
        
        static float mix[16];
        ndspChnSetFormat(audio->channel[i], NDSP_FORMAT_ADPCM | NDSP_3D_SURROUND_PREPROCESSED);
        ndspChnSetRate(audio->channel[i], audio->sample_rate);
        
        if (audio->channel_count == 1)
        {
            mix[0] = mix[1] = 0.5f;
        } else if (audio->channel_count == 2)
        {
            if (i == 0)
            {
                mix[0] = 0.8f;
                mix[1] = 0.0f;
                mix[2] = 0.2f;
                mix[3] = 0.0f;
            } else
            {
                mix[0] = 0.0f;
                mix[1] = 0.8f;
                mix[2] = 0.0f;
                mix[3] = 0.2f;
            }
        }
        ndspChnSetMix(audio->channel[i], mix);
        ndspChnSetAdpcmCoefs(audio->channel[i], audio->adpcm_coefs[i]);

        for (u8 j = 0; j < BUFFER_COUNT; ++j)
        {
            memset(&(audio->wave_buf[i][j]), 0, sizeof(ndspWaveBuf));
            audio->wave_buf[i][j].status = NDSP_WBUF_DONE;
            audio->buffer_data[i][j] = linearAlloc(audio->block_size);
        }
    }

    return 0;
}

void fill_buffers(audio_s *audio)
{
    DEBUG("Filling buffers...\n");
    for (u8 bufIndex = 0; bufIndex < BUFFER_COUNT; ++bufIndex)
    {
        if (audio->wave_buf[0][bufIndex].status != NDSP_WBUF_DONE) continue;
        if (audio->channel_count == 2 && audio->wave_buf[1][bufIndex].status != NDSP_WBUF_DONE) continue;

        if (audio->is_looping && audio->current_block == audio->loop_end)
        {
            audio->current_block = audio->loop_start;
            audio->cursor = audio->data_offset + 0x20 + audio->block_size * audio->channel_count * audio->loop_start;
        }

        if (!audio->is_looping && audio->current_block == audio->loop_end)
        {
            audio->stop = true;
            return;
        }

        for (u8 channelIndex = 0; channelIndex < audio->channel_count; ++channelIndex)
        {
            ndspWaveBuf *buf = &(audio->wave_buf[channelIndex][bufIndex]);
            memset(buf, 0, sizeof(ndspWaveBuf));
            buf->data_adpcm = audio->buffer_data[channelIndex][bufIndex];
            memcpy(buf->data_adpcm, audio->music_buf + audio->cursor, (audio->current_block == audio->num_blocks - 1) ? audio->last_block_size : audio->block_size);
            audio->cursor += (audio->current_block == audio->num_blocks - 1) ? audio->last_block_size : audio->block_size;
            DSP_FlushDataCache(buf->data_adpcm, audio->block_size);

            if (audio->current_block == 0)
                buf->adpcm_data = &(audio->adpcm_data[channelIndex][0]);
            else if (audio->current_block == audio->loop_start)
                buf->adpcm_data = &(audio->adpcm_data[channelIndex][1]);

            if (audio->current_block == audio->num_blocks - 1)
                buf->nsamples = audio->last_block_samples;
            else
                buf->nsamples = audio->block_samples;

            ndspChnWaveBufAdd(audio->channel[channelIndex], buf);
        }
        audio->current_block++;
    }
}

// Play a given audio struct
void update_audio(audio_s * audio) 
{
    u32 current_time = svcGetSystemTick();
    if (current_time - audio->last_time > 1e8)
    {
        fill_buffers(audio);
        audio->last_time = current_time;
    }
}

void thread_audio(void * data) {
    audio_s * audio = (audio_s *)data;
    int res = init_audio(audio);
    if (res < 0)
    {
        return;
    }
    start_play(audio);
    while(!audio->stop) {
        update_audio(audio);
    }
    for (u8 i = 0; i < audio->channel_count; ++i)
    {
        ndspChnWaveBufClear(audio->channel[i]);
        ndspChnReset(audio->channel[i]);
        audio->active_channels &= ~(1 << audio->channel[i]);
        for (u8 j = 0; j < BUFFER_COUNT; ++j) {
            linearFree(audio->buffer_data[i][j]);
        }
    }
    free(audio->music_buf);
}

void play_audio(audio_s * audio) {
    audio->playing_thread = threadCreate(thread_audio, audio, 0x1000, 0x3F, 1, false);
}

void stop_audio(audio_s ** audio_ptr) {
    audio_s * audio = *audio_ptr;
    if(audio->playing_thread)
    {
        audio->stop = true;
        threadJoin(audio->playing_thread, U64_MAX);
        threadFree(audio->playing_thread);
    }
    free(audio);
    *audio_ptr = NULL;
}
