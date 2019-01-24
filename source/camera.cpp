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
#include "file.h"
#include "draw.h"
#include "network.h"

static bool inited_light = false;
static LightLock camera_buffer_mutex, draw_mutex;
static LightEvent started_camera_event, camera_received_event;
static Handle cancel_event;
static u16* camera_buffer = nullptr;
static volatile bool finished = false;

static void capture_cam_thread(void* void_arg)
{
    DEBUG("capture_cam_thread start\n");
    Handle events[3] = {0};
    events[0] = cancel_event;
    u32 transferUnit;

    u16* buffer = new(std::nothrow) u16[400 * 240];
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
    CAMU_SetReceiving(&events[1], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), (s16)transferUnit);
    CAMU_StartCapture(PORT_CAM1);

    LightEvent_Signal(&started_camera_event);
    bool cancel = false;
    while(!cancel) 
    {
        s32 index = 0;
        svcWaitSynchronizationN(&index, events, 3, false, U64_MAX);
        switch(index)
        {
            case 0:
                DEBUG("capture_cam_thread Cancel event received\n");
                cancel = true;
                break;
            case 1:
                DEBUG("capture_cam_thread camera event received\n");
                svcCloseHandle(events[1]);
                events[1] = 0;

                LightLock_Lock(&camera_buffer_mutex);
                memcpy(camera_buffer, buffer, 400 * 240 * sizeof(u16));

                DEBUG("capture_cam_thread signalling draw event\n");
                LightEvent_Signal(&camera_received_event);

                DEBUG("capture_cam_thread releasing camera buffer mutex\n");
                LightLock_Unlock(&camera_buffer_mutex);

                CAMU_SetReceiving(&events[1], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), transferUnit);
                break;
            case 2:
                DEBUG("capture_cam_thread camera error event received\n");
                svcCloseHandle(events[1]);
                events[1] = 0;
                CAMU_ClearBuffer(PORT_CAM1);
                CAMU_SetReceiving(&events[1], buffer, PORT_CAM1, 400 * 240 * sizeof(u16), transferUnit);
                CAMU_StartCapture(PORT_CAM1);
                break;
            default:
                DEBUG("capture_cam_thread wtaf\n");
                break;
        }
    }

    CAMU_StopCapture(PORT_CAM1);

    bool busy = false;
    while(R_SUCCEEDED(CAMU_IsBusy(&busy, PORT_CAM1)) && busy)
        svcSleepThread(1 * 1000 * 1000);

    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);
    camExit();
    delete[] buffer;
    svcCloseHandle(events[1]);
    svcCloseHandle(events[2]);

    finished = true;
    LightLock_Lock(&camera_buffer_mutex);
    LightEvent_Signal(&camera_received_event);
    LightLock_Unlock(&camera_buffer_mutex);
    DEBUG("capture_cam_thread end\n");
}

static void update_ui_thread(void* void_arg)
{
    DEBUG("update_ui_thread start\n");
    C3D_Tex* tex = new(std::nothrow) C3D_Tex;
    C3D_TexInit(tex, 512, 256, GPU_RGB565);
    C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);

    static const Tex3DS_SubTexture subt3x = { 400, 240, 0.0f, 1.0f, 400.0f / 512.0f, 1.0f - (240.0f / 512.0f)};
    C2D_Image image = {tex, &subt3x};

    while(!finished)
    {
        DEBUG("update_ui_thread waiting for draw event\n");
        // Untiled texture loading code adapted from FBI
        LightEvent_Wait(&camera_received_event);
        DEBUG("update_ui_thread received draw event\n");
        LightLock_Lock(&camera_buffer_mutex);
        DEBUG("update_ui_thread tiling captured image\n");
        for(u32 x = 0; x < 400; x++)
        {
            for(u32 y = 0; y < 240; y++)
            {
                u32 dstPos = ((((y >> 3) * (512 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
                u32 srcPos = (y * 400) + x;

                memcpy(static_cast<u16*>(tex->data) + dstPos, camera_buffer + srcPos, sizeof(u16));
            }
        }
        LightLock_Unlock(&camera_buffer_mutex);

        LightLock_Lock(&draw_mutex);
        DEBUG("update_ui_thread drawing captured image\n");
        start_frame(-1);
        switch_screen(GFX_TOP);
        C2D_DrawImageAt(image, 0.0f, 0.0f, 0.4f, NULL, 1.0f, 1.0f);

        switch_screen(GFX_BOTTOM);
        C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, 320.0f, BARS_SIZE, COLOR_BARS);
        C2D_DrawRectSolid(0.0f, 240.0f - BARS_SIZE, 0.0f, 320.0f, BARS_SIZE, COLOR_BARS);
        // draw_text_center(GFX_BOTTOM, 4, 0.5, 0.5, 0.5, colors[COLOR_WHITE], "Press \uE005 To Quit");
        end_frame();
        LightLock_Unlock(&draw_mutex);
    }

    C3D_TexDelete(tex);
    delete tex;
    DEBUG("update_ui_thread end\n");
}

QrMenu::QrMenu()
{
    DEBUG("QrMenu::QrMenu\n");
    draw_install(INSTALL_ENTERING_QR);
    if(R_SUCCEEDED(camInit()))
    {
        camExit();
        u32 out;
        ACU_GetWifiStatus(&out);
        if(out)
        {
            finished = false;

            this->context = quirc_new();
            quirc_resize(this->context, 400, 240);
            std::fill(this->downloaded_any.begin(), this->downloaded_any.end(), false);

            camera_buffer = new(std::nothrow) u16[400 * 240];
            std::fill(camera_buffer, camera_buffer + (400 * 240), 0);

            if(!inited_light)
            {
                inited_light = true;
                LightLock_Init(&camera_buffer_mutex);
                LightLock_Init(&draw_mutex);
                LightEvent_Init(&camera_received_event, RESET_ONESHOT);
                LightEvent_Init(&started_camera_event, RESET_ONESHOT);
            }
            svcCreateEvent(&cancel_event, RESET_STICKY);
            this->update_ui = this->capture_cam = NULL;
            if((this->capture_cam = threadCreate(capture_cam_thread, nullptr, 0x10000, 0x1A, 1, false)) == NULL)
                return;
            LightEvent_Wait(&started_camera_event);
            if((this->update_ui = threadCreate(update_ui_thread, nullptr, 0x10000, 0x1A, 1, false)) == NULL)
                return;

            const KeysActions normal_actions_down{
                {KEY_B, [](){ return RETURN_CHANGE_TO_LIST_MODE; }},
            };

            const KeysActions normal_actions_held{};

            this->current_actions.push({normal_actions_down, normal_actions_held});

            static const Instructions normal_actions_instructions{
                INSTRUCTIONS_NONE,
                INSTRUCTION_B_FOR_GOING_BACK,
                INSTRUCTIONS_NONE,
                INSTRUCTIONS_NONE,
                INSTRUCTIONS_NONE,
                INSTRUCTIONS_NONE,
                INSTRUCTIONS_NONE,
                INSTRUCTIONS_NONE,
            };

            this->instructions_stack.push(&normal_actions_instructions);

            this->ready = true;
        }
        else
        {
            draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_NO_WIFI_QR);
        }
    }
    else
    {
        if(running_from_hax)
            draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_NO_QR_HBL);
        else
            draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_OTHER_QR_ERROR);
    }
}

QrMenu::~QrMenu()
{
    DEBUG("Exiting QR\n");
    svcSignalEvent(cancel_event);
    if(this->update_ui)
    {
        threadJoin(this->update_ui, U64_MAX);
        threadFree(this->update_ui);
    }
    if(this->capture_cam)
    {
        threadJoin(this->capture_cam, U64_MAX);
        threadFree(this->capture_cam);
    }
    svcCloseHandle(cancel_event);

    delete[] camera_buffer;
    camera_buffer = nullptr;

    quirc_destroy(this->context);
}

void QrMenu::calculate_new_scroll()
{

}

void QrMenu::draw()
{

}

void QrMenu::scan()
{
    int w;
    int h;
    u8* image = (u8*) quirc_begin(this->context, &w, &h);
    for (ssize_t x = 0; x < w && x < 400; x++) {
        for (ssize_t y = 0; y < h && y < 240; y++) {
            u16 px = camera_buffer[y * 400 + x];
            image[y * w + x] = (u8)(((((px >> 11) & 0x1F) << 3) + (((px >> 5) & 0x3F) << 2) + ((px & 0x1F) << 3)) / 3);
        }
    }
    quirc_end(this->context);

    if(quirc_count(this->context) > 0)
    {
        struct quirc_code code;
        struct quirc_data scan_data;
        quirc_extract(this->context, 0, &code);
        if(!quirc_decode(&code, &scan_data))
        {
            LightLock_Lock(&draw_mutex);
            draw_install(INSTALL_DOWNLOAD);
            char* filename = nullptr;
            const auto& [zip_buf, zip_size] = download_data((char*)scan_data.payload, INSTALL_DOWNLOAD, &filename);
            if(zip_size != 0)
            {
                draw_install(INSTALL_CHECKING_DOWNLOAD);

                if(check_file_is_zip(zip_buf.get(), zip_size))
                {
                    MenuType mode = MODE_BADGES;

                    if(mode == MODE_BADGES)
                    {

                    }
                    else
                    {
                        char path_to_file[0x107] = {0};
                        const char* main_paths[] = {
                            "/Themes",
                            "/Splashes",
                        };
                        sprintf(path_to_file, "%s/%s", main_paths[mode], filename);
                        char* extension = strrchr(path_to_file, '.');
                        if (extension == NULL || strcmp(extension, ".zip"))
                            strcat(path_to_file, ".zip");

                        remake_file(fsMakePath(PATH_ASCII, path_to_file), SD_CARD, zip_size, zip_buf.get());
                    }

                    this->downloaded_any[mode] = true;
                }
                else
                {
                    draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_DOWNLOADED_NOT_ZIP);
                }
            }
            else
            {
                draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_DOWNLOAD_FAILED);
            }
            LightLock_Unlock(&draw_mutex);

            if(filename)
                delete[] filename;
        }
    }
}
