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
#include "draw.h"
#include "fs.h"
#include "themes.h"
#include "pp2d/pp2d/pp2d.h"

u32 transfer_size;
Handle event;
struct quirc* context;

void init_qr(void)
{
    camInit();
    CAMU_SetSize(SELECT_OUT1_OUT2, SIZE_CTR_TOP_LCD, CONTEXT_A);
    CAMU_SetOutputFormat(SELECT_OUT1_OUT2, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetFrameRate(SELECT_OUT1_OUT2, FRAME_RATE_10);

    CAMU_SetNoiseFilter(SELECT_OUT1_OUT2, true);
    CAMU_SetAutoExposure(SELECT_OUT1_OUT2, true);
    CAMU_SetAutoWhiteBalance(SELECT_OUT1_OUT2, true);
    CAMU_SetTrimming(PORT_CAM1, false);
    CAMU_SetTrimming(PORT_CAM2, false);

    CAMU_GetMaxBytes(&transfer_size, 400, 240);
    CAMU_SetTransferBytes(PORT_BOTH, transfer_size, 400, 240);

    CAMU_Activate(SELECT_OUT1_OUT2);
    event = 0;

    CAMU_ClearBuffer(PORT_BOTH);
    CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2);
    CAMU_StartCapture(PORT_BOTH);

    context = quirc_new();
    quirc_resize(context, 400, 240);
}

void exit_qr(void)
{
    CAMU_StopCapture(PORT_BOTH);
    CAMU_Activate(SELECT_NONE);
    camExit();
    quirc_destroy(context);
}

void scan_qr(u16 *buf)
{
    int w;
    int h;

    u8 *image = (u8*) quirc_begin(context, &w, &h);

    for (ssize_t x = 0; x < w; x++)
    {
        for (ssize_t y = 0; y < h; y++)
        {
            u16 px = buf[y * 400 + x];
            image[y * w + x] = (u8)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
        }
    }

    quirc_end(context);

    if (quirc_count(context) > 0)
    {
        struct quirc_code code;
        struct quirc_data data;
        quirc_extract(context, 0, &code);
        if (!quirc_decode(&code, &data))
        {
            qr_mode = false;
            exit_qr();

            u8 *download;
            http_get((char*)data.payload, &download);
        }
    }
}

void take_picture(void)
{
    u16 *buf = malloc(sizeof(u16) * 400 * 240 * 4);
    if (buf == NULL) return;
    CAMU_SetReceiving(&event, buf, PORT_CAM1, 240 * 400 * 2, transfer_size);
    svcWaitSynchronization(event, U64_MAX);
    svcCloseHandle(event);
    pp2d_begin_draw(GFX_TOP);
    u32 *rgba8_buf = malloc(240 * 400 * sizeof(u32));
    if (rgba8_buf == NULL) return;
    for (int i = 0; i < 240 * 400; i++)
    {
        rgba8_buf[i] = RGB565_TO_ABGR8(buf[i]);
    }
    pp2d_load_texture_memory(TEXTURE_QR, rgba8_buf, 400, 240);
    pp2d_draw_texture(TEXTURE_QR, 0, 0);
    pp2d_draw_rectangle(0, 216, 400, 24, RGBA8(55, 122, 168, 255));
    pp2d_draw_text_center(GFX_TOP, 220, 0.5, 0.5, RGBA8(255, 255, 255, 255), "Press \uE005 To Quit");
    pp2d_draw_rectangle(0, 0, 400, 24, RGBA8(55, 122, 168, 255));
    pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, RGBA8(255, 255, 255, 255), "Press \uE004 To Scan");
    pp2d_end_draw();
    free(rgba8_buf);
    pp2d_free_texture(TEXTURE_QR);
    hidScanInput();
    u32 kDown = hidKeysDown();
    if (kDown & KEY_L)
    {
        CAMU_StopCapture(PORT_BOTH);
        CAMU_Activate(SELECT_NONE);
        scan_qr(buf);
        CAMU_Activate(SELECT_OUT1_OUT2);
        CAMU_StartCapture(PORT_BOTH);
    }
    if (kDown & KEY_R)
    {
        exit_qr();
        qr_mode = false;
    }
    free(buf);
}

/*
Putting this in camera because I'm too lazy to make a network.c
This'll probably get refactored later
*/
Result http_get(char *url, u8 **data)
{
    Result ret;
    httpcContext context;
    char *new_url = NULL;
    u32 status_code;
    u32 content_size = 0;
    u32 read_size = 0;
    u32 size = 0;
    u8 *last_buf;

    do {
        ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
        ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
        ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        ret = httpcAddRequestHeaderField(&context, "User-Agent", "Anemone3DS/1.1.0");
        ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
		draw_theme_install(DOWNLOADING);
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

    *data = malloc(0x1000);
    if (*data == NULL)
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
        free(*data);
    }

    char *filename;
    filename = strtok(content_disposition, "\"");
    filename = strtok(NULL, "\"");

    char *illegal_characters = "\"?;:/\\+";
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
        ret = httpcDownloadData(&context, (*data) + size, 0x1000, &read_size);
        size += read_size;

        if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
        {
            last_buf = *data;
            *data = realloc(data, size + 0x1000);
            if (data == NULL)
            {
                httpcCloseContext(&context);
                free(last_buf);
                return ret;
            }
        }
    } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

    last_buf = *data;
    *data = realloc(*data, size);
    if (*data == NULL)
    {
        httpcCloseContext(&context);
        free(last_buf);
        return -1;
    }

    return 0;
}
