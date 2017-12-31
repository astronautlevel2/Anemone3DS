/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2017 Alex Taber ("astronautlevel"), Dawid Eckert ("daedreth")
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
#include "pp2d/pp2d/pp2d.h"

#include "draw.h"
#include "fs.h"
#include "loading.h"

/*
static u32 transfer_size;
static Handle event;
static struct quirc* context;
static u16 * camera_buf = NULL;
*/

void exit_qr(qr_data *data)
{
    svcSignalEvent(data->cancel);
    while(!data->finished)
       svcSleepThread(1000000);
    data->capturing = false;

    free(data->camera_buffer);
    free(data->texture_buffer);
    quirc_destroy(data->context);
    free(data);
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
    bool cancel = false;
    while (!cancel) 
    {
        s32 index = 0;
        svcWaitSynchronizationN(&index, events, 3, false, U64_MAX);
        switch(index) {
            case 0:
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
    for(int i = 0; i < 3; i++) {
        if(events[i] != 0) {
            svcCloseHandle(events[i]);
            events[i] = 0;
        }
    }
    svcCloseHandle(data->mutex);
    data->finished = true;
}

bool start_capture_cam(qr_data *data) 
{
    data->mutex = 0;
    data->cancel = 0;
    svcCreateEvent(&data->cancel, RESET_STICKY);
    svcCreateMutex(&data->mutex, false);
    if(threadCreate(capture_cam_thread, data, 0x10000, 0x1A, 1, true) == NULL)
        return false;
    return true;
}

void update_qr(qr_data *data, EntryMode current_mode)
{
    hidScanInput();
    if (hidKeysDown() & KEY_R) {
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
    for (int i = 0; i < 240 * 400; i++)
    {
        data->texture_buffer[i] = RGB565_TO_ABGR8(data->camera_buffer[i]);
    }
    draw_base_interface();
    pp2d_free_texture(TEXTURE_QR);
    pp2d_load_texture_memory(TEXTURE_QR, data->texture_buffer, 400, 240);

    pp2d_draw_texture(TEXTURE_QR, 0, 0);
    pp2d_draw_rectangle(0, 216, 400, 24, RGBA8(55, 122, 168, 255));
    pp2d_draw_text_center(GFX_TOP, 220, 0.5, 0.5, RGBA8(255, 255, 255, 255), "Press \uE005 To Quit");
    pp2d_end_draw();
    
    int w;
    int h;
    u8 *image = (u8*) quirc_begin(data->context, &w, &h);
    svcWaitSynchronization(data->mutex, U64_MAX);
    for (ssize_t x = 0; x < w; x++) {
        for (ssize_t y = 0; y < h; y++) {
            u16 px = data->camera_buffer[y * 400 + x];
            image[y * w + x] = (u8)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
        }
    }
    svcReleaseMutex(data->mutex);
    quirc_end(data->context);
    if(quirc_count(data->context) > 0)
    {
        struct quirc_code code;
        struct quirc_data scan_data;
        quirc_extract(data->context, 0, &code);
        if (!quirc_decode(&code, &scan_data))
        {
            exit_qr(data);
            http_get((char*)scan_data.payload, main_paths[current_mode]);
        }   
    }
    
}

void init_qr(EntryMode current_mode)
{
    qr_data *data = calloc(1, sizeof(qr_data));
    data->capturing = false;
    data->finished = false;
    data->context = quirc_new();
    quirc_resize(data->context, 400, 240);

    data->camera_buffer = calloc(1, 400 * 240 * sizeof(u16));
    data->texture_buffer = calloc(1, 400 * 240 * sizeof(u32));

    while (!data->finished) update_qr(data, current_mode);
}

/*
Putting this in camera because I'm too lazy to make a network.c
This'll probably get refactored later
*/
Result http_get(char *url, const char *path)
{
    Result ret;
    httpcContext context;
    char *new_url = NULL;
    u32 status_code;
    u32 content_size = 0;
    u32 read_size = 0;
    u32 size = 0;
    u8 *buf;
    u8 *last_buf;

    do {
        ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
        ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
        ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        ret = httpcAddRequestHeaderField(&context, "User-Agent", USER_AGENT);
        ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
        draw_install(INSTALL_DOWNLOAD);

        ret = httpcBeginRequest(&context);
        if (ret != 0)
        {
            httpcCloseContext(&context);
            if (new_url != NULL) free(new_url);
            return ret;
        }

        ret = httpcGetResponseStatusCode(&context, &status_code);
        if(ret!=0){
            httpcCloseContext(&context);
            if(new_url!=NULL) free(new_url);
            return ret;
        }

        if ((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308))
        {
            if (new_url == NULL) new_url = malloc(0x1000);
            ret = httpcGetResponseHeader(&context, "Location", new_url, 0x1000);
            url = new_url;
            httpcCloseContext(&context);
        }
    } while ((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308));

    if (status_code != 200)
    {
        httpcCloseContext(&context);
        if (new_url != NULL) free(new_url);
        return ret;
    }

    ret = httpcGetDownloadSizeState(&context, NULL, &content_size);
    if (ret != 0)
    {
        httpcCloseContext(&context);
        if (new_url != NULL) free(new_url);
        return ret;
    }

    buf = malloc(0x1000);
    if (buf == NULL)
    {
        httpcCloseContext(&context);
        free(new_url);
        return -2;
    }

    char *content_disposition = malloc(1024);
    ret = httpcGetResponseHeader(&context, "Content-Disposition", content_disposition, 1024);
    if (ret != 0)
    {
        free(content_disposition);
        free(new_url);
        free(buf);
        return ret;
    }

    char *filename;
    filename = strtok(content_disposition, "\"");
    filename = strtok(NULL, "\"");

    char *illegal_characters = "\"?;:/\\+";
    if(!filename)
    {
        free(content_disposition);
        free(new_url);
        free(buf);
        throw_error("Target is not valid!", ERROR_LEVEL_WARNING);
        return -1;
    }
    for (size_t i = 0; i < strlen(filename); i++)
    {
        for (size_t n = 0; n < strlen(illegal_characters); n++)
        {
            if (filename[i] == illegal_characters[n]) 
            {
                filename[i] = '-';
            }
        }
    }

    do {
        ret = httpcDownloadData(&context, buf + size, 0x1000, &read_size);
        size += read_size;

        if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
        {
            last_buf = buf;
            buf = realloc(buf, size + 0x1000);
            if (buf == NULL)
            {
                httpcCloseContext(&context);
                free(content_disposition);
                free(new_url);
                free(last_buf);
                return ret;
            }
        }
    } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

    last_buf = buf;
    buf = realloc(buf, size);
    if (buf == NULL)
    {
        httpcCloseContext(&context);
        free(content_disposition);
        free(new_url);
        free(last_buf);
        return -1;
    }

    char path_to_file[0x106] = {0};
    strcpy(path_to_file, path);
    strcat(path_to_file, filename);
    char * extension = strrchr(path_to_file, '.');
    if (extension == NULL || strcmp(extension, ".zip"))
        strcat(path_to_file, ".zip");

    remake_file(path_to_file, ArchiveSD, size);
    buf_to_file(size, path_to_file, ArchiveSD, (char*)buf);

    free(content_disposition);
    free(new_url);
    free(buf);

    return 0;
}
