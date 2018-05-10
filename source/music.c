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

// Play a given audio struct
Result play_audio(audio_s *audio) 
{
    long size = audio->wave_buf[audio->buf_pos].nsamples * 4 - audio->data_read;
    char size_info[50] = {0};
    sprintf(size_info, "Audio Size: %ld\n", size);
    DEBUG(size_info);
    if (audio->wave_buf[audio->buf_pos].status == NDSP_WBUF_DONE) // only run if the current selected buffer has already finished playing
    { 
        size_t read = ov_read(&audio->vf, (char*)audio->wave_buf[audio->buf_pos].data_vaddr + audio->data_read, size, NULL); // read 1 vorbis packet into wave buffer

        if (read <= 0) // EoF or error
        { 
            ov_clear(&audio->vf);
            if (read == 0) // EoF
            { 
                ov_open(fopen(audio->filename, "rb"), &audio->vf, NULL, 0); // Reopen file. Don't need to reinit channel stuff since it's all the same as before
            } else // Error :(
            { 
                char error[100] = {0};
                sprintf(error, "Vorbis play error: %d\n", read);
                DEBUG(error);
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

void thread_audio(void* data) {
    audio_s *audio = (audio_s*)data;
    DEBUG(audio->filename);
    while(!audio->stop) {
        play_audio(audio);
    }
    free(audio);
}

// Initialize the audio struct
Result load_audio(u16 *entry_path, audio_s *audio) 
{
    u16 path[0x106] = {0};
    strucat(path, entry_path);
    struacat(path, "/bgm.ogg");
    
    ssize_t len = strulen(path, 0x106);
    char *cpath = calloc(sizeof(char), len*sizeof(u16));
    utf16_to_utf8((u8*)cpath, path, len*sizeof(u16));
    audio->filename = cpath;

    audio->mix[0] = audio->mix[1] = 1.0f; // Determines volume for the 12 (?) different outputs. See http://smealum.github.io/ctrulib/channel_8h.html#a30eb26f1972cc3ec28370263796c0444

    ndspChnSetInterp(0, NDSP_INTERP_LINEAR); 
    ndspChnSetRate(0, 44100);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); // Tremor outputs ogg files in 16 bit PCM stereo
	ndspChnSetMix(0, audio->mix); // See mix comment above

    FILE *file = fopen(cpath, "rb");
    if(file != NULL) 
    {
        int e = ov_open(file, &audio->vf, NULL, 0);
        if (e < 0) 
        {
            char error[50];
            sprintf(error, "Vorbis: %d\n", e);
            DEBUG(error);
            return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_NO_DATA);
        }

        vorbis_info *vi = ov_info(&audio->vf, -1);
        ndspChnSetRate(0, vi->rate);// Set sample rate to what's read from the ogg file

        audio->wave_buf[0].nsamples = audio->wave_buf[1].nsamples = vi->rate / 4; // 4 bytes per sample, samples = rate (bytes) / 4
        audio->wave_buf[0].status = audio->wave_buf[1].status = NDSP_WBUF_DONE; // Used in play to stop from writing to current buffer
        audio->wave_buf[0].data_vaddr = linearAlloc(BUF_TO_READ); // Most vorbis packets should only be 4 KB at most (?) Possibly dangerous assumption
        audio->wave_buf[1].data_vaddr = linearAlloc(BUF_TO_READ);
        DEBUG("Success!");
        threadCreate(thread_audio, audio, 0x1000, 0x3F, 1, true);
        return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
    } else {
        DEBUG("File not found!\n");
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }
}