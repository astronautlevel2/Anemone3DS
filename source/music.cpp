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

#define BUF_TO_READ 40960 // How much data should be buffered at a time

static void decode_thread(void* arg)
{
    MusicBase* this_ = static_cast<MusicBase*>(arg);
    OggVorbis_File vf;
    int e = ov_open(this_->fh, &vf, NULL, 0);
    if(e < 0) 
    {
        DEBUG("<decode_thread> Vorbis: %d\n", e);
        return;
    }

    float mix[12];
    mix[0] = mix[1] = 1.0f;
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR); 
    ndspChnSetMix(0, mix); // See mix comment above

    vorbis_info* vi = ov_info(&vf, -1);
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
        return;
    }

    // Most vorbis packets should only be 4 KB at most (?) Possibly dangerous assumption
    void* buf_1 = linearAlloc(BUF_TO_READ);
    void* buf_2 = linearAlloc(BUF_TO_READ);

    ndspWaveBuf wave_buf[2];
    wave_buf[0].nsamples = wave_buf[1].nsamples = vi->rate / 4;  // 4 bytes per sample, samples = rate (bytes) / 4
    wave_buf[0].status = wave_buf[1].status = NDSP_WBUF_DONE;  // Used in play to stop from writing to current buffer
    wave_buf[0].data_vaddr = buf_1;
    wave_buf[1].data_vaddr = buf_2;
    DEBUG("<decode_thread> start success!\n");

    long data_read = 0;
    int selected_buf = 0;
    while(!LightEvent_TryWait(&this_->stop_event))
    {
        long size = wave_buf[selected_buf].nsamples * 4 - data_read;
        DEBUG("<decode_thread> Audio Size: %ld\n", size);
        if(wave_buf[selected_buf].status == NDSP_WBUF_DONE) // only run if the current selected buffer has already finished playing
        {
            DEBUG("<decode_thread> Attempting ov_read\n");
            int bitstream;
            long read_size = ov_read(&vf, (char*)wave_buf[selected_buf].data_vaddr + data_read, size, &bitstream); // read 1 vorbis packet into wave buffer
            DEBUG("<decode_thread> ov_read successful\n");

            if(read_size <= 0) // EoF or error
            { 
                if(read_size == 0) // EoF
                { 
                    ov_clear(&vf);
                    ov_open(this_->fh, &vf, NULL, 0); // Reopen file. Don't need to reinit channel stuff since it's all the same as before
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

    ov_clear(&vf);
    while(wave_buf[0].status != NDSP_WBUF_DONE || wave_buf[1].status != NDSP_WBUF_DONE)
        svcSleepThread(10 * 1000 * 1000);

    linearFree(buf_1);
    linearFree(buf_2);
}

MusicBase::MusicBase(FILE* fh) : fh(fh)
{
    if(have_sound)
    {
        if(fh)
        {
            LightEvent_Init(&this->stop_event, RESET_STICKY);
            this->bgm_thread = threadCreate(decode_thread, this, 0x10000, 0x3F, 1, false);
            this->ready = true;
        }
        else
        {
            DEBUG("MusicBase fmemopen failed!\n");
        }
    }
}

MusicBase::~MusicBase()
{
    if(this->fh)
    {
        LightEvent_Signal(&this->stop_event);
        if(this->bgm_thread)
        {
            threadJoin(this->bgm_thread, U64_MAX);
            threadFree(this->bgm_thread);
        }
        fclose(this->fh);
    }
}

MusicFile::MusicFile(std::unique_ptr<char[]>& ogg_buf, u32 ogg_size) : MusicBase(fmemopen(ogg_buf.get(), ogg_size, "rb")), ogg_buf(std::move(ogg_buf))
{

}

MusicDownloaded::MusicDownloaded(std::unique_ptr<u8[]>& ogg_buf, u32 ogg_size) : MusicBase(fmemopen(ogg_buf.get(), ogg_size, "rb")), ogg_buf(std::move(ogg_buf))
{

}
