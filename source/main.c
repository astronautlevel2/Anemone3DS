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
#include "instructions.h"
#include "pp2d/pp2d/pp2d.h"
#include <time.h>

#define FASTSCROLL_WAIT 1e8

static bool homebrew = false;
static bool installed_themes = false;
static Thread_Arg_s arg = {0};
static Handle update_icons_handle;
static Thread iconLoadingThread;
static Entry_List_s lists[MODE_AMOUNT] = {0};

int __stacksize__ = 64 * 1024;
Result archive_result;

const char * main_paths[MODE_AMOUNT] = {
    "/Themes/",
    "/Splashes/",
};

static void init_services(void)
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

static void exit_services(void)
{
    close_archives();
    cfguExit();
    ptmuExit();
    httpcExit();
    acExit();
}

static void exit_thread(void)
{
    if(arg.run_thread)
    {
        DEBUG("exiting thread\n");
        arg.run_thread = false;
        svcSignalEvent(update_icons_handle);
        threadJoin(iconLoadingThread, U64_MAX);
        threadFree(iconLoadingThread);
    }
}

void exit_function(void)
{
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        Entry_List_s * current_list = &lists[i];
        free(current_list->entries);
        current_list->entries = NULL;
    }

    exit_thread();
    svcCloseHandle(update_icons_handle);
    exit_screens();
    exit_services();

    if(installed_themes)
    {
        if(homebrew)
        {
            APT_HardwareResetAsync();
        }
        else
        {
            srvPublishToSubscriber(0x202, 0);
        }
    }
}

static void handle_scrolling(Entry_List_s * list)
{
    // Scroll the menu up or down if the selected theme is out of its bounds
    //----------------------------------------------------------------
    if(list->entries_count > ENTRIES_PER_SCREEN)
    {
        if(list->entries_count > ENTRIES_PER_SCREEN*2 && list->previous_scroll < ENTRIES_PER_SCREEN && list->selected_entry >= list->entries_count - ENTRIES_PER_SCREEN)
        {
            list->scroll = list->entries_count - ENTRIES_PER_SCREEN;
        }
        else if(list->entries_count > ENTRIES_PER_SCREEN*2 && list->selected_entry < ENTRIES_PER_SCREEN && list->previous_selected >= list->entries_count - ENTRIES_PER_SCREEN)
        {
            list->scroll = 0;
        }
        else if(list->selected_entry == list->previous_selected+1 && list->selected_entry == list->scroll+ENTRIES_PER_SCREEN)
        {
            list->scroll++;
        }
        else if(list->selected_entry == list->previous_selected-1 && list->selected_entry == list->scroll-1)
        {
            list->scroll--;
        }
        else if(list->selected_entry == list->previous_selected+ENTRIES_PER_SCREEN || list->selected_entry >= list->scroll + ENTRIES_PER_SCREEN)
        {
            list->scroll += ENTRIES_PER_SCREEN;
        }
        else if(list->selected_entry == list->previous_selected-ENTRIES_PER_SCREEN || list->selected_entry < list->scroll)
        {
            list->scroll -= ENTRIES_PER_SCREEN;
        }

        if(list->scroll < 0)
            list->scroll = 0;
        if(list->scroll > list->entries_count - ENTRIES_PER_SCREEN)
            list->scroll = list->entries_count - ENTRIES_PER_SCREEN;
    }
    //----------------------------------------------------------------
}

static void change_selected(Entry_List_s * list, int change_value)
{
    if(abs(change_value) >= list->entries_count) return;

    list->selected_entry += change_value;
    if(list->selected_entry < 0)
        list->selected_entry += list->entries_count;
    list->selected_entry %= list->entries_count;
}

static void start_thread(void)
{
    if(arg.run_thread)
    {
        DEBUG("starting thread\n");
        iconLoadingThread = threadCreate(load_icons_thread, &arg, __stacksize__, 0x3f, -2, false);
    }
}

static void load_lists(Entry_List_s * lists)
{
    ssize_t texture_id_offset = TEXTURE_ICON;

    exit_thread();
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        InstallType loading_screen = INSTALL_NONE;
        if(i == MODE_THEMES)
            loading_screen = INSTALL_LOADING_THEMES;
        else if(i == MODE_SPLASHES)
            loading_screen = INSTALL_LOADING_SPLASHES;

        draw_install(loading_screen);

        Entry_List_s * current_list = &lists[i];
        free(current_list->entries);
        memset(current_list, 0, sizeof(Entry_List_s));
        load_entries(main_paths[i], current_list, i);

        if(current_list->entries_count > ENTRIES_PER_SCREEN*ICONS_OFFSET_AMOUNT)
            arg.run_thread = true;

        DEBUG("total: %i\n", current_list->entries_count);

        current_list->texture_id_offset = texture_id_offset;
        load_icons_first(current_list);

        texture_id_offset += ENTRIES_PER_SCREEN*ICONS_OFFSET_AMOUNT;
    }
    start_thread();
}

int main(void)
{
    srand(time(NULL));
    init_services();
    init_screens();

    static Entry_List_s * current_list = NULL;
    arg.thread_argument = (void*)&current_list;
    arg.update_request = &update_icons_handle;
    svcCreateEvent(&update_icons_handle, 0);

    load_lists(lists);

    EntryMode current_mode = MODE_THEMES;

    bool preview_mode = false;
    int preview_offset = 0;

    bool qr_mode = false;
    bool install_mode = false;

    while(aptMainLoop())
    {
        #ifndef CITRA_MODE
        if(R_FAILED(archive_result) && current_mode == MODE_THEMES)
        {
            throw_error("Theme extdata does not exist!\nSet a default theme from the home menu.", ERROR_LEVEL_ERROR);
            break;
        }
        #endif

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        current_list = &lists[current_mode];

        Instructions_s instructions = normal_instructions[current_mode];
        if(install_mode)
            instructions = install_instructions;

        if(qr_mode) take_picture();
        else if(preview_mode) draw_preview(preview_offset);
        else {
            handle_scrolling(current_list);
            svcSignalEvent(update_icons_handle);
            svcSleepThread(1e6);

            current_list->previous_scroll = current_list->scroll;
            current_list->previous_selected = current_list->selected_entry;

            draw_interface(current_list, instructions);
        }

        pp2d_end_draw();

        if(kDown & KEY_START) break;

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
            else if(qr_mode && kDown & KEY_B)
            {
                exit_qr();
                qr_mode = false;
                continue;
            }
            else if(preview_mode && kDown & KEY_B)
            {
                preview_mode = false;
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
                    installed_themes = true;
                }
                else if((kDown | kHeld) & KEY_DUP)
                {
                    draw_install(INSTALL_SINGLE);
                    theme_install(*current_entry);
                    installed_themes = true;
                }
                else if((kDown | kHeld) & KEY_DRIGHT)
                {
                    draw_install(INSTALL_NO_BGM);
                    no_bgm_install(*current_entry);
                    installed_themes = true;
                }
                else if((kDown | kHeld) & KEY_DDOWN)
                {
                    if(current_list->shuffle_count > MAX_SHUFFLE_THEMES)
                    {
                        throw_error("You have too many themes selected.", ERROR_LEVEL_WARNING);
                    }
                    else if(current_list->shuffle_count == 0)
                    {
                        throw_error("You don't have any themes selected.", ERROR_LEVEL_WARNING);
                    }
                    else
                    {
                        draw_install(INSTALL_SHUFFLE);
                        Result res = shuffle_install(*current_list);
                        if(R_FAILED(res)) DEBUG("shuffle install result: %lx\n", res);
                        else
                        {
                            current_list->shuffle_count = 0;
                            installed_themes = true;
                        }
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
                    if(draw_confirm("Are you sure you would like to delete\nthe installed splash?", current_list))
                    {
                        draw_install(INSTALL_SPLASH_DELETE);
                        splash_delete();
                    }
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
            if(draw_confirm("Are you sure you would like to delete this?", current_list))
            {
                draw_install(INSTALL_ENTRY_DELETE);
                delete_entry(*current_entry);
                load_lists(lists);
            }
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
