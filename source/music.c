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
#include "loading.h"

// Play a given audio struct
Result update_audio(audio_s *audio) 
{
    svcCreateEvent(&audio->finished, RESET_STICKY);

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
                ov_open(fmemopen(audio->filebuf, audio->filesize, "rb"), &audio->vf, NULL, 0); // Reopen file. Don't need to reinit channel stuff since it's all the same as before
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
    while(!audio->stop) {
        update_audio(audio);
    }
    svcSignalEvent(audio->finished);
    free(audio->filebuf);
    free(audio);
}

void play_audio(audio_s *audio) {
    threadCreate(thread_audio, audio, 0x1000, 0x3F, 1, true);
}