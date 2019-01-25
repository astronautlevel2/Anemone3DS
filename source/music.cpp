/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2018 Contributors in CONTRIBUTORS.md
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

#define BUF_TO_READ 0x1000 // How much data should be buffered at a time

static void decode_thread(void* arg)
{
    MusicBase* this_ = static_cast<MusicBase*>(arg);

    FILE* fh = fmemopen(this_->buf, this_->size, "rb");
    if(!fh)
    {
        DEBUG("MusicBase fmemopen failed!\n");
        LightEvent_Signal(&this_->ready_or_not_event);
        return;
    }

    OggVorbis_File vf;
    int e = ov_open(fh, &vf, NULL, 0);  // Takes ownership of FILE except if failure
    if(e < 0) 
    {
        DEBUG("<decode_thread> Vorbis: %d\n", e);
        fclose(fh);
        LightEvent_Signal(&this_->ready_or_not_event);
        return;
    }

    float mix[12];
    mix[0] = mix[1] = 1.0f;
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR); 
    ndspChnSetMix(0, mix); // See mix comment above

    vorbis_info* vi = ov_info(&vf, -1);
    DEBUG("rate: %ld\n", vi->rate);
    ndspChnSetRate(0, vi->rate);// Set sample rate to what's read from the ogg file
    if(vi->channels == 2)
    {
        DEBUG("<decode_thread> Using stereo\n");
        ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); // 2 channels == Stereo
    }
    else
    {
        DEBUG("<decode_thread> Invalid number of channels\n");
        ov_clear(&vf);
        LightEvent_Signal(&this_->ready_or_not_event);
        return;
    }

    ndspWaveBuf wave_buf[2];
    memset(wave_buf, 0, sizeof(wave_buf));
    wave_buf[0].nsamples = wave_buf[1].nsamples = BUF_TO_READ / 2;
    wave_buf[0].status = wave_buf[1].status = NDSP_WBUF_DONE;  // Used in play to stop from writing to current buffer
    // Most vorbis packets should only be 4 KiB at most (?) Possibly dangerous assumption
    wave_buf[0].data_vaddr = linearAlloc(BUF_TO_READ * 2);
    wave_buf[1].data_vaddr = linearAlloc(BUF_TO_READ * 2);
    DEBUG("vaddr 1: %p\n", wave_buf[0].data_vaddr);
    DEBUG("vaddr 2: %p\n", wave_buf[1].data_vaddr);
    DEBUG("<decode_thread> start success!\n");

    long data_read = 0;
    int selected_buf = 0;

    this_->ready = true;
    LightEvent_Signal(&this_->ready_or_not_event);

    while(!LightEvent_TryWait(&this_->stop_event))
    {
        long size = BUF_TO_READ * 2 - data_read;
        if(wave_buf[selected_buf].status == NDSP_WBUF_DONE) // only run if the current selected buffer has already finished playing
        {
            int bitstream;
            long read_size = ov_read(&vf, ((char*)wave_buf[selected_buf].data_vaddr) + data_read, size, &bitstream); // read 1 vorbis packet into wave buffer

            if(read_size <= 0) // EoF or error
            { 
                if(read_size == 0) // EoF
                { 
                    DEBUG("looping audio...\n");
                    ov_clear(&vf);
                    fh = fmemopen(this_->buf, this_->size, "rb");
                    ov_open(fh, &vf, NULL, 0); // Reopen file. Don't need to reinit channel stuff since it's all the same as before
                }
                else // Error :(
                { 
                    DEBUG("<decode_thread> Vorbis play error: %ld\n", read_size);
                    ndspChnReset(0);
                    break;
                }
            }
            else 
            {
                data_read += read_size;
                if(read_size == size)
                {
                    data_read = 0;
                    ndspChnWaveBufAdd(0, &wave_buf[selected_buf]); // Add buffer to ndsp 
                    selected_buf = selected_buf == 0 ? 1 : 0; // switch to other buffer to load and prepare it while the current one is playing
                }
            }
        }
    }

    DEBUG("out of loop\n");
    ndspChnWaveBufClear(0);
    DEBUG("ndspChnWaveBufClear\n");
    ov_clear(&vf);
    DEBUG("ov_clear\n");
    while((wave_buf[0].status != NDSP_WBUF_DONE || wave_buf[0].status != NDSP_WBUF_FREE) || (wave_buf[1].status != NDSP_WBUF_DONE || wave_buf[1].status != NDSP_WBUF_FREE))
        svcSleepThread(10 * 1000 * 1000);

    DEBUG("free\n");
    linearFree(const_cast<void*>(wave_buf[0].data_vaddr));
    linearFree(const_cast<void*>(wave_buf[1].data_vaddr));
}

MusicBase::MusicBase(void* buf, u32 size) : buf(buf), size(size)
{
    if(have_sound)
    {
        LightEvent_Init(&this->stop_event, RESET_STICKY);
        LightEvent_Init(&this->ready_or_not_event, RESET_ONESHOT);
        this->bgm_thread = threadCreate(decode_thread, this, 0x10000, 0x3F, 1, false);
        LightEvent_Wait(&this->ready_or_not_event);
    }
}

MusicBase::~MusicBase()
{
    if(have_sound)
    {
        DEBUG("signalling event...\n");
        LightEvent_Signal(&this->stop_event);
        if(this->bgm_thread)
        {
            threadJoin(this->bgm_thread, U64_MAX);
            threadFree(this->bgm_thread);
        }
    }
}

MusicFile::MusicFile(std::unique_ptr<char[]>& ogg_buf, u32 ogg_size) : MusicBase(ogg_buf.get(), ogg_size), ogg_buf(std::move(ogg_buf))
{

}

MusicDownloaded::MusicDownloaded(std::unique_ptr<u8[]>& ogg_buf, u32 ogg_size) : MusicBase(ogg_buf.get(), ogg_size), ogg_buf(std::move(ogg_buf))
{

}
