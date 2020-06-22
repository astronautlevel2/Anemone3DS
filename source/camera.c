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

#include "camera.h"

#include "quirc/quirc.h"

#include "draw.h"
#include "fs.h"
#include "loading.h"
#include "remote.h"

#include <archive.h>
#include <archive_entry.h>

void exit_qr(qr_data *data)
{
    DEBUG("Exiting QR\n");
    svcSignalEvent(data->cancel);
    while(!data->finished)
        svcSleepThread(1000000);
    svcCloseHandle(data->cancel);
    data->capturing = false;

    free(data->camera_buffer);
    free(data->tex);
    quirc_destroy(data->context);
}

void capture_cam_thread(void *arg) 
{
    qr_data *data = (qr_data *) arg;
    Handle events[3] = {0};
    events[0] = data->cancel;
    u32 transferUnit;

    u16 *buffer = calloc(1, 400 * 240 * sizeof(u16));
    camInit();
    CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetFrameRate(SELECT_OUT1, FRAME_RATE_30);
    CAMU_SetNoiseFilter(SELECT_OUT1, true);
    CAMU_SetAutoExposure(SELECT_OUT1, true);
    CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);
    CAMU_Activate(SELECT_OUT1);
    CAMU_GetBufferErrorInterruptEvent(&events[2], PORT_CAM1);
    CAMU_SetTrimming(PORT_CAM1, false);
    CAMU_GetMaxBytes(&transferUnit, 400, 240);
    CAMU_SetTransferBytes(PORT_CAM1, transferUnit, 400, 240);
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_SetReceiving(&events[1], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), (s16) transferUnit);
    CAMU_StartCapture(PORT_CAM1);
    svcSignalEvent(data->started);
    bool cancel = false;
    while (!cancel) 
    {
        s32 index = 0;
        svcWaitSynchronizationN(&index, events, 3, false, U64_MAX);
        switch(index) {
            case 0:
                DEBUG("Cancel event received\n");
                cancel = true;
                break;
            case 1:
                svcCloseHandle(events[1]);
                events[1] = 0;
                svcWaitSynchronization(data->mutex, U64_MAX);
                memcpy(data->camera_buffer, buffer, 400 * 240 * sizeof(u16));
                GSPGPU_FlushDataCache(data->camera_buffer, 400 * 240 * sizeof(u16));
                svcReleaseMutex(data->mutex);
                CAMU_SetReceiving(&events[1], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), transferUnit);
                break;
            case 2:
                svcCloseHandle(events[1]);
                events[1] = 0;
                CAMU_ClearBuffer(PORT_CAM1);
                CAMU_SetReceiving(&events[1], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), transferUnit);
                CAMU_StartCapture(PORT_CAM1);
                break;
            default:
                break;
        }
    }

    CAMU_StopCapture(PORT_CAM1);

    bool busy = false;
    while(R_SUCCEEDED(CAMU_IsBusy(&busy, PORT_CAM1)) && busy) {
        svcSleepThread(1000000);
    }

    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);
    camExit();
    free(buffer);
    for(int i = 1; i < 3; i++) {
        if(events[i] != 0) {
            svcCloseHandle(events[i]);
            events[i] = 0;
        }
    }
    svcCloseHandle(data->mutex);
    data->finished = true;
}

void update_ui(void *arg)
{
    qr_data* data = (qr_data*) arg;

    while (!data->finished)
    {
        draw_base_interface();

        // Untiled texture loading code adapted from FBI
        svcWaitSynchronization(data->mutex, U64_MAX);
        for(u32 x = 0; x < 400 && !data->finished; x++) {
            for(u32 y = 0; y < 256 && !data->finished; y++) {
                u32 dstPos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3))) * sizeof(u16);
                u32 srcPos = (y * 400 + x) * sizeof(u16);

                memcpy(&((u8*) data->image.tex->data)[dstPos], &((u8*) data->camera_buffer)[srcPos], sizeof(u16));
            }
        }

        svcReleaseMutex(data->mutex);

        if (data->finished)
        {
            end_frame();
            break; 
       }

        C2D_DrawImageAt(data->image, 0.0f, 0.0f, 0.4f, NULL, 1.0f, 1.0f);

        set_screen(bottom);
        draw_text_center(GFX_BOTTOM, 4, 0.5, 0.5, 0.5, colors[COLOR_WHITE], "Press \uE005 To Quit");
        end_frame();
    }
    data->closed = true;
}

bool start_capture_cam(qr_data *data) 
{
    data->mutex = 0;
    data->cancel = 0;
    svcCreateEvent(&data->cancel, RESET_STICKY);
    svcCreateMutex(&data->mutex, false);
    if(threadCreate(capture_cam_thread, data, 0x10000, 0x1A, 1, true) == NULL)
    {
        throw_error("Capture cam thread creation failed\nPlease report this to the developers", ERROR_LEVEL_ERROR);
        return false;
    }
    svcWaitSynchronization(data->started, U64_MAX);
    if(threadCreate(update_ui, data, 0x10000, 0x1A, 1, true) == NULL)
    {
        exit_qr(data);
        return false;
    }
    return true;
}

void update_qr(qr_data *data)
{
    hidScanInput();
    if (hidKeysDown() & (KEY_R | KEY_B | KEY_TOUCH)) {
        exit_qr(data);
        return;
    }

    if (!data->capturing) {
        if(start_capture_cam(data))
            data->capturing = true;
        else {
            exit_qr(data);
            return;
        }

    }

    if (data->finished) {
        exit_qr(data);
        return;
    }

    int w;
    int h;
    u8 *image = (u8*) quirc_begin(data->context, &w, &h);
    for (ssize_t x = 0; x < w; x++) {
        for (ssize_t y = 0; y < h; y++) {
            u16 px = data->camera_buffer[y * 400 + x];
            image[y * w + x] = (u8)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
        }
    }
    quirc_end(data->context);
    if(quirc_count(data->context) > 0)
    {
        struct quirc_code code;
        struct quirc_data scan_data;
        quirc_extract(data->context, 0, &code);
        if (!quirc_decode(&code, &scan_data))
        {
            exit_qr(data);

            while (!data->finished) svcSleepThread(1000000);

            draw_install(INSTALL_DOWNLOAD);
            char * zip_buf = NULL;
            char * filename = NULL;
            u32 zip_size = http_get((char*)scan_data.payload, &filename, &zip_buf, INSTALL_DOWNLOAD);

            if(zip_size != 0)
            {
                draw_install(INSTALL_CHECKING_DOWNLOAD);

                struct archive *a = archive_read_new();
                archive_read_support_format_zip(a);

                int r = archive_read_open_memory(a, zip_buf, zip_size);
                archive_read_free(a);

                if(r == ARCHIVE_OK)
                {
                    EntryMode mode = MODE_AMOUNT;

                    char * buf = NULL;
                    do {
                        if(zip_memory_to_buf("body_LZ.bin", zip_buf, zip_size, &buf) != 0)
                        {
                            mode = MODE_THEMES;
                            break;
                        }

                        free(buf);
                        buf = NULL;
                        if(zip_memory_to_buf("splash.bin", zip_buf, zip_size, &buf) != 0)
                        {
                            mode = MODE_SPLASHES;
                            break;
                        }

                        free(buf);
                        buf = NULL;
                        if(zip_memory_to_buf("splashbottom.bin", zip_buf, zip_size, &buf) != 0)
                        {
                            mode = MODE_SPLASHES;
                            break;
                        }
                    }
                    while(false);

                    free(buf);
                    buf = NULL;

                    if(mode != MODE_AMOUNT)
                    {
                        char path_to_file[0x107] = {0};
                        sprintf(path_to_file, "%s%s", main_paths[mode], filename);
                        char * extension = strrchr(path_to_file, '.');
                        if (extension == NULL || strcmp(extension, ".zip"))
                            strcat(path_to_file, ".zip");

                        remake_file(fsMakePath(PATH_ASCII, path_to_file), ArchiveSD, zip_size);
                        buf_to_file(zip_size, fsMakePath(PATH_ASCII, path_to_file), ArchiveSD, zip_buf);
                        data->success = true;
                    }
                    else
                    {
                        throw_error("Zip downloaded is neither\na splash nor a theme.", ERROR_LEVEL_WARNING);
                    }
                }
                else
                {
                    throw_error("File downloaded isn't a zip.", ERROR_LEVEL_WARNING);
                }
                free(zip_buf);
            }
            else
            {
                throw_error("Download failed.", ERROR_LEVEL_WARNING);
            }

            free(filename);
        }   
    }

}

bool init_qr(void)
{
    qr_data *data = calloc(1, sizeof(qr_data));
    data->capturing = false;
    data->finished = false;
    data->closed = false;
    svcCreateEvent(&data->started, RESET_STICKY);
    
    data->context = quirc_new();
    quirc_resize(data->context, 400, 240);

    data->camera_buffer = calloc(1, 400 * 240 * sizeof(u16));

    data->tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    static const Tex3DS_SubTexture subt3x = { 512, 256, 0.0f, 1.0f, 1.0f, 0.0f };
    data->image = (C2D_Image){ data->tex, &subt3x };
    C3D_TexInit(data->image.tex, 512, 256, GPU_RGB565);
    C3D_TexSetFilter(data->image.tex, GPU_LINEAR, GPU_LINEAR);

    while (!data->finished) update_qr(data);
    bool success = data->success;
    while (!data->closed) svcSleepThread(1000000);
    svcCloseHandle(data->started);
    free(data);

    return success;
}
