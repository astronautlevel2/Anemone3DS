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

#include "fs.h"
#include "loading.h"
#include "themes.h"
#include "splashes.h"
#include "draw.h"
#include "camera.h"
#include "pp2d/pp2d/pp2d.h"
#include <time.h>

#define FASTSCROLL_WAIT 1e8

static bool homebrew = false;
int __stacksize__ = 64 * 1024;
Result archive_result;

const char * main_paths[MODE_AMOUNT] = {
    "/Themes/",
    "/Splashes/",
};

void init_services(void)
{
    consoleDebugInit(debugDevice_SVC);
    cfguInit();
    ptmuInit();
    acInit();
    httpcInit(0);
    archive_result = open_archives();
    if(envIsHomebrew())
    {
        s64 out;
        svcGetSystemInfo(&out, 0x10000, 0);
        homebrew = !out;
    }
}

void exit_services(void)
{
    close_archives();
    cfguExit();
    ptmuExit();
    httpcExit();
    acExit();
}

void exit_function(void)
{
    exit_screens();
    exit_services();

    if(homebrew)
    {
        APT_HardwareResetAsync();
    }
    else
    {
        srvPublishToSubscriber(0x202, 0);
    }
}

void change_selected(Entry_List_s * list, int change_value)
{
    list->selected_entry += change_value;
    if(change_value < 0 && list->selected_entry < 0)
        list->selected_entry = list->entries_count - 1;
    else
        list->selected_entry %= list->entries_count;
}

void load_lists(Entry_List_s * lists)
{
    DEBUG("origin: %u\n", TEXTURE_ICON);

    ssize_t last_icon_id = TEXTURE_ICON;
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        Entry_List_s * current_list = &lists[i];
        last_icon_id += current_list->entries_count;
        free(current_list->entries);
        memset(current_list, 0, sizeof(Entry_List_s));
    }
    pp2d_free_texture(last_icon_id);

    DEBUG("max: %u\n", last_icon_id);

    ssize_t icon_id_start = TEXTURE_ICON;
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        Entry_List_s * current_list = &lists[i];
        current_list->icon_id_start = icon_id_start;
        load_entries(main_paths[i], current_list);
        icon_id_start += current_list->entries_count;
    }

    DEBUG("end: %u\n", icon_id_start);
}

int main(void)
{
    srand(time(NULL));
    init_services();
    init_screens();

    Entry_List_s lists[MODE_AMOUNT] = {0};
    load_lists(lists);

    EntryMode current_mode = MODE_THEMES;

    bool preview_mode = false;
    int preview_offset = 0;

    bool qr_mode = false;
    bool install_mode = false;

    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        Entry_List_s * current_list = &lists[current_mode];

        if(qr_mode) take_picture();
        else if(preview_mode) draw_preview(preview_offset);
        else draw_interface(current_list, current_mode, install_mode);
        pp2d_end_draw();

        if(kDown & KEY_START) break;

        if(R_FAILED(archive_result) && current_mode == MODE_THEMES)
        {
            throw_error("Theme extdata does not exist!\nSet a default theme from the home menu.", ERROR_LEVEL_ERROR);
            continue;
        }

        if(!install_mode)
        {
            if(!preview_mode && !qr_mode && kDown & KEY_L) //toggle between splashes and themes
            {
                current_mode++;
                current_mode %= MODE_AMOUNT;
                continue;
            }
            else if(!preview_mode && kDown & KEY_R) //toggle QR mode
            {
                u32 out;
                ACU_GetWifiStatus(&out);
                if(out)
                {
                    qr_mode = !qr_mode;
                    if(qr_mode)
                        init_qr();
                    else
                        exit_qr();
                }
                else
                {
                    throw_error("Please connect to Wi-Fi before scanning QR", ERROR_LEVEL_WARNING);
                }
                continue;
            }
            else if(!qr_mode && kDown & KEY_Y) //toggle preview mode
            {
                if(!preview_mode)
                    preview_mode = load_preview(*current_list, &preview_offset);
                else
                    preview_mode = false;
                continue;
            }
            else if(qr_mode && kDown & KEY_L) //scan a QR code while in QR mode
            {
                CAMU_StopCapture(PORT_BOTH);
                CAMU_Activate(SELECT_NONE);
                qr_mode = !scan_qr(current_mode);
                CAMU_Activate(SELECT_OUT1_OUT2);
                CAMU_StartCapture(PORT_BOTH);

                if(!qr_mode)
                    load_lists(lists);
                continue;
            }
        }

        if(qr_mode || preview_mode || current_list->entries == NULL)
            continue;

        int selected_entry = current_list->selected_entry;
        Entry_s * current_entry = &current_list->entries[selected_entry];

        if(install_mode)
        {
            if(kUp & KEY_A)
                install_mode = false;
            if(!install_mode)
            {
                if((kDown | kHeld) & KEY_DLEFT)
                {
                    draw_install(INSTALL_BGM);
                    bgm_install(*current_entry);
                }
                else if((kDown | kHeld) & KEY_DUP)
                {
                    draw_install(INSTALL_SINGLE);
                    theme_install(*current_entry);
                }
                else if((kDown | kHeld) & KEY_DRIGHT)
                {
                    draw_install(INSTALL_NO_BGM);
                    no_bgm_install(*current_entry);
                }
                else if((kDown | kHeld) & KEY_DDOWN)
                {
                    if(current_list->shuffle_count > MAX_SHUFFLE_THEMES)
                    {
                        throw_error("You have too many Shuffle selected.", ERROR_LEVEL_WARNING);
                    }
                    else if(current_list->shuffle_count == 0)
                    {
                        throw_error("You dont have any Shuffle selected.", ERROR_LEVEL_WARNING);
                    }
                    else
                    {
                        draw_install(INSTALL_SHUFFLE);
                        Result res = shuffle_install(*current_list);
                        if(R_FAILED(res)) DEBUG("shuffle install result: %lx\n", res);
                        else current_list->shuffle_count = 0;
                    }
                }
            }
            continue;
        }

        // Actions

        if(kDown & KEY_A)
        {
            switch(current_mode)
            {
                case MODE_THEMES:
                    install_mode = true;
                    break;
                case MODE_SPLASHES:
                    draw_install(INSTALL_SPLASH);
                    splash_install(*current_entry);
                    break;
                default:
                    break;
            }
        }
        else if(kDown & KEY_B)
        {
            switch(current_mode)
            {
                case MODE_THEMES:
                    if(current_entry->in_shuffle) current_list->shuffle_count--;
                    else current_list->shuffle_count++;
                    current_entry->in_shuffle = !current_entry->in_shuffle;
                    break;
                case MODE_SPLASHES:
                    draw_install(INSTALL_SPLASH_DELETE);
                    splash_delete();
                    break;
                default:
                    break;
            }
        }
        else if(kDown & KEY_X)
        {
            switch(current_mode)
            {
                case MODE_THEMES:
                    break;
                case MODE_SPLASHES:
                    break;
                default:
                    break;
            }
        }
        else if(kDown & KEY_SELECT)
        {
            draw_install(INSTALL_ENTRY_DELETE);
            delete_entry(*current_entry);
            load_lists(lists);
        }

        // Movement in the UI
        else if(kDown & KEY_UP)
        {
            change_selected(current_list, -1);
        }
        else if(kDown & KEY_DOWN) 
        {
            change_selected(current_list, 1);
        }
        // Quick moving
        else if(kDown & KEY_LEFT) 
        {
            change_selected(current_list, -ENTRIES_PER_SCREEN);
        }
        else if(kDown & KEY_RIGHT)
        {
            change_selected(current_list, ENTRIES_PER_SCREEN);
        }

        // Fast scroll using circle pad
        else if(kHeld & KEY_CPAD_UP)
        {
            change_selected(current_list, -1);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_DOWN)
        {
            change_selected(current_list, 1);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_LEFT)
        {
            change_selected(current_list, -ENTRIES_PER_SCREEN);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_RIGHT)
        {
            change_selected(current_list, ENTRIES_PER_SCREEN);
            svcSleepThread(FASTSCROLL_WAIT);
        }
    }

    exit_function();

    return 0;
}
