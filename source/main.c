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
#include "badges.h"
#include "draw.h"
#include "camera.h"
#include "instructions.h"
#include "pp2d/pp2d/pp2d.h"
#include <time.h>

#define FASTSCROLL_WAIT 1.5e8

static bool homebrew = false;
static bool installed_themes = false;

static Thread iconLoadingThread = {0};
static Thread_Arg_s iconLoadingThread_arg = {0};
static Handle update_icons_handle;

static Thread installCheckThreads[MODE_AMOUNT] = {0};
static Thread_Arg_s installCheckThreads_arg[MODE_AMOUNT] = {0};

static Entry_List_s lists[MODE_AMOUNT] = {0};

int __stacksize__ = 64 * 1024;
u32 old_time_limit;

const char * main_paths[MODE_AMOUNT] = {
    "/Themes/",
    "/Splashes/",
    "/Badges/",
};
const int entries_per_screen[MODE_AMOUNT] = {
    4,
    4,
    3,
};
const int entry_size[MODE_AMOUNT] = {
    48,
    48,
    64,
};

static void init_services(void)
{
    consoleDebugInit(debugDevice_SVC);
    cfguInit();
    ptmuInit();
    acInit();
    APT_GetAppCpuTimeLimit(&old_time_limit);
    APT_SetAppCpuTimeLimit(30);
    httpcInit(0);
    open_archives();
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
    if (old_time_limit != UINT32_MAX) APT_SetAppCpuTimeLimit(old_time_limit);
    httpcExit();
    acExit();
}

static void stop_install_check(void)
{
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        if(installCheckThreads_arg[i].run_thread)
        {
            installCheckThreads_arg[i].run_thread = false;
        }
    }
}

static void exit_thread(void)
{
    if(iconLoadingThread_arg.run_thread)
    {
        DEBUG("exiting thread\n");
        iconLoadingThread_arg.run_thread = false;
        svcSignalEvent(update_icons_handle);
        threadJoin(iconLoadingThread, U64_MAX);
        threadFree(iconLoadingThread);
    }
}

void free_lists(void)
{
    stop_install_check();
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        Entry_List_s * current_list = &lists[i];
        free(current_list->entries);
        free(current_list->icons_ids);
        memset(current_list, 0, sizeof(Entry_List_s));
    }
    exit_thread();
}

void exit_function(bool power_pressed)
{
    free_lists();
    svcCloseHandle(update_icons_handle);
    exit_screens();
    exit_services();

    if(!power_pressed && installed_themes)
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

static void start_thread(void)
{
    if(iconLoadingThread_arg.run_thread)
    {
        DEBUG("starting thread\n");
        iconLoadingThread = threadCreate(load_icons_thread, &iconLoadingThread_arg, __stacksize__, 0x38, -2, false);
    }
}

static void load_lists(Entry_List_s * lists)
{
    ssize_t texture_id_offset = TEXTURE_ICON;

    free_lists();
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        InstallType loading_screen = INSTALL_NONE;
        if(i == MODE_THEMES)
            loading_screen = INSTALL_LOADING_THEMES;
        else if(i == MODE_SPLASHES)
            loading_screen = INSTALL_LOADING_SPLASHES;
        else if(i == MODE_BADGES)
        {
            loading_screen = INSTALL_LOADING_BADGES;
            if(R_FAILED(badgeResult))
                break;
        }

        draw_install(loading_screen);
        draw_install(loading_screen);

        Entry_List_s * current_list = &lists[i];
        current_list->mode = i;
        current_list->entries_per_screen = entries_per_screen[i];
        current_list->entry_size = entry_size[i];
        Result res = load_entries(main_paths[i], current_list);
        if(R_SUCCEEDED(res))
        {
            if(current_list->entries_count > current_list->entries_per_screen*ICONS_OFFSET_AMOUNT)
                iconLoadingThread_arg.run_thread = true;

            DEBUG("total: %i\n", current_list->entries_count);

            current_list->texture_id_offset = texture_id_offset;
            load_icons_first(current_list, false);

            texture_id_offset += current_list->entries_per_screen*ICONS_OFFSET_AMOUNT;

            void (*install_check_function)(void*) = NULL;
            if(i == MODE_THEMES)
                install_check_function = themes_check_installed;
            else if(i == MODE_SPLASHES)
                install_check_function = splash_check_installed;

            Thread_Arg_s * current_arg = &installCheckThreads_arg[i];
            current_arg->run_thread = true;
            current_arg->thread_arg = (void**)current_list;

            if(install_check_function != NULL)
            {
                installCheckThreads[i] = threadCreate(install_check_function, current_arg, __stacksize__, 0x3f, -2, true);
                svcSleepThread(1e8);
            }
        }
    }
    start_thread();
}

static SwkbdCallbackResult jump_menu_callback(void* entries_count, const char** ppMessage, const char* text, size_t textlen)
{
    int typed_value = atoi(text);
    if(typed_value > *(int*)entries_count)
    {
        *ppMessage = "The new position has to be\nsmaller or equal to the\nnumber of entries!";
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if(typed_value == 0)
    {
        *ppMessage = "The new position has to\nbe positive!";
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

static void jump_menu(Entry_List_s * list)
{
    if(list == NULL) return;

    char numbuf[64] = {0};

    SwkbdState swkbd;

    sprintf(numbuf, "%i", list->entries_count);
    int max_chars = strlen(numbuf);
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, max_chars);

    sprintf(numbuf, "%i", list->selected_entry);
    swkbdSetInitialText(&swkbd, numbuf);

    sprintf(numbuf, "Where do you want to jump to?\nMay cause icons to reload.");
    swkbdSetHintText(&swkbd, numbuf);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Jump", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, max_chars);
    swkbdSetFilterCallback(&swkbd, jump_menu_callback, &list->entries_count);

    memset(numbuf, 0, sizeof(numbuf));
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        list->selected_entry = atoi(numbuf) - 1;
        list->scroll = list->selected_entry;
        if(list->scroll >= list->entries_count - list->entries_per_screen)
            list->scroll = list->entries_count - list->entries_per_screen - 1;
    }
}

static void change_selected(Entry_List_s * list, int change_value)
{
    if(abs(change_value) >= list->entries_count) return;

    list->selected_entry += change_value;
    if(list->selected_entry < 0)
        list->selected_entry += list->entries_count;
    list->selected_entry %= list->entries_count;
}

static void toggle_shuffle(Entry_List_s * list)
{
    Entry_s * current_entry = &list->entries[list->selected_entry];
    if(current_entry->in_shuffle) list->shuffle_count--;
    else list->shuffle_count++;
    current_entry->in_shuffle = !current_entry->in_shuffle;
}

int main(void)
{
    srand(time(NULL));
    init_services();
    init_screens();

    svcCreateEvent(&update_icons_handle, RESET_ONESHOT);

    static Entry_List_s * current_list = NULL;
    void * iconLoadingThread_args_void[] = {
        &current_list,
        &update_icons_handle,
    };
    iconLoadingThread_arg.thread_arg = iconLoadingThread_args_void;
    iconLoadingThread_arg.run_thread = false;

    #ifndef CITRA_MODE
    if(R_SUCCEEDED(themeResult))
        load_lists(lists);
    #else
    load_lists(lists);
    #endif

    EntryMode current_mode = MODE_THEMES;

    bool preview_mode = false;
    int preview_offset = 0;

    bool qr_mode = false;
    bool install_mode = false;

    bool quit = false;

    while(aptMainLoop())
    {
        if(quit)
        {
            exit_function(false);
            return 0;
        }

        #ifndef CITRA_MODE
        if(R_FAILED(themeResult) && current_mode == MODE_THEMES)
        {
            throw_error("Theme extdata does not exist!\nSet a default theme from the home menu.", ERROR_LEVEL_ERROR);
            quit = true;
            continue;
        }
        if(R_FAILED(badgeResult) && current_mode == MODE_BADGES)
        {
            throw_error("Badge extdata does not exist!\nDownload and launch the Badge Arcade.", ERROR_LEVEL_WARNING);
            goto switch_mode;
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
            if(!iconLoadingThread_arg.run_thread)
            {
                handle_scrolling(current_list);
                current_list->previous_scroll = current_list->scroll;
            }
            else
            {
                svcSignalEvent(update_icons_handle);
                svcSleepThread(5e6);
            }

            draw_interface(current_list, instructions);
            svcSleepThread(1e7);
        }

        pp2d_end_draw();

        if(kDown & KEY_START) quit = true;

        if(!install_mode)
        {
            if(!preview_mode && !qr_mode && kDown & KEY_L) //toggle between splashes and themes
            {
                switch_mode:
                current_mode++;
                current_mode %= MODE_AMOUNT;
                continue;
            }
            else if(!qr_mode && !preview_mode && kDown & KEY_R) //toggle QR mode
            {
                enable_qr:
                if(R_SUCCEEDED(camInit()))
                {
                    camExit();
                    u32 out;
                    ACU_GetWifiStatus(&out);
                    if(out)
                    {

                        if(init_qr())
                        {
                            load_lists(lists);
                        }
                    }
                    else
                    {
                        throw_error("Please connect to Wi-Fi before scanning QRs", ERROR_LEVEL_WARNING);
                    }
                }
                else
                {
                    throw_error("Your camera seems to have a problem, unable to scan QRs.", ERROR_LEVEL_WARNING);
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
            else if(preview_mode && kDown & (KEY_B | KEY_TOUCH))
            {
                preview_mode = false;
                continue;
            }
        }

        if(qr_mode || preview_mode || current_list->entries == NULL)
            goto touch;

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
                    if(R_SUCCEEDED(bgm_install(*current_entry)))
                    {
                        for(int i = 0; i < current_list->entries_count; i++)
                        {
                            Entry_s * theme = &current_list->entries[i];
                            if(theme == current_entry)
                                theme->installed = true;
                            else
                                theme->installed = false;
                        }
                        installed_themes = true;
                    }
                }
                else if((kDown | kHeld) & KEY_DUP)
                {
                    draw_install(INSTALL_SINGLE);
                    if(R_SUCCEEDED(theme_install(*current_entry)))
                    {
                        for(int i = 0; i < current_list->entries_count; i++)
                        {
                            Entry_s * theme = &current_list->entries[i];
                            if(theme == current_entry)
                                theme->installed = true;
                            else
                                theme->installed = false;
                        }
                        installed_themes = true;
                    }
                }
                else if((kDown | kHeld) & KEY_DRIGHT)
                {
                    draw_install(INSTALL_NO_BGM);
                    if(R_SUCCEEDED(no_bgm_install(*current_entry)))
                    {
                        for(int i = 0; i < current_list->entries_count; i++)
                        {
                            Entry_s * theme = &current_list->entries[i];
                            if(theme == current_entry)
                                theme->installed = true;
                            else
                                theme->installed = false;
                        }
                        installed_themes = true;
                    }
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
                            for(int i = 0; i < current_list->entries_count; i++)
                            {
                                Entry_s * theme = &current_list->entries[i];
                                if(theme->in_shuffle)
                                {
                                    theme->in_shuffle = false;
                                    theme->installed = true;
                                }
                                else theme->installed = false;
                            }
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
                case MODE_BADGES:
                    draw_install(INSTALL_BADGE);
                    badge_install(*current_entry);
                default:
                    break;
            }
        }
        else if(kDown & KEY_B)
        {
            switch(current_mode)
            {
                case MODE_THEMES:
                    toggle_shuffle(current_list);
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
                    load_icons_first(current_list, false);
                    break;
                case MODE_SPLASHES:
                    load_icons_first(current_list, false);
                    break;
                case MODE_BADGES:
                    load_icons_first(current_list, false);
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
            change_selected(current_list, -current_list->entries_per_screen);
        }
        else if(kDown & KEY_RIGHT)
        {
            change_selected(current_list, current_list->entries_per_screen);
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
            change_selected(current_list, -current_list->entries_per_screen);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_RIGHT)
        {
            change_selected(current_list, current_list->entries_per_screen);
            svcSleepThread(FASTSCROLL_WAIT);
        }

        // Movement using the touchscreen
        touch:
        if((kDown | kHeld) & KEY_TOUCH)
        {
            touchPosition touch = {0};
            hidTouchRead(&touch);

            u16 x = touch.px;
            u16 y = touch.py;

            u16 arrowStartX = 152;
            u16 arrowEndX = arrowStartX+16;

            #define BETWEEN(min, x, max) (min < x && x < max)

            if(kDown & KEY_TOUCH)
            {
                if(y < 24)
                {
                    if(current_list->entries != NULL && BETWEEN(arrowStartX, x, arrowEndX) && current_list->scroll > 0)
                    {
                        change_selected(current_list, -current_list->entries_per_screen);
                    }
                    else if(BETWEEN(320-120, x, 320-96))
                    {
                        quit = true;
                    }
                    else if(BETWEEN(320-96, x, 320-72))
                    {
                        goto enable_qr;
                    }
                    else if(current_list->entries != NULL && BETWEEN(320-120, x, 320-96) && current_mode == MODE_THEMES)
                    {
                        toggle_shuffle(current_list);
                    }
                    else
                    {
                        for(int i = 0; i < MODE_AMOUNT; i++)
                        {
                            u16 minx = 320 - 24*(i+1);
                            u16 maxx = minx + 24;
                            if(BETWEEN(minx, x, maxx))
                            {
                                current_mode = i;
                                break;
                            }
                        }
                    }
                }
                else if(y >= 216)
                {
                    if(current_list->entries != NULL && BETWEEN(arrowStartX, x, arrowEndX) && current_list->scroll < current_list->entries_count - current_list->entries_per_screen)
                    {
                        change_selected(current_list, current_list->entries_per_screen);
                    }
                    else if(current_list->entries != NULL && BETWEEN(176, x, 320))
                    {
                        jump_menu(current_list);
                    }
                }
            }
            else
            {
                if(current_list->entries != NULL && BETWEEN(24, y, 216))
                {
                    for(int i = 0; i < current_list->entries_per_screen; i++)
                    {
                        u16 miny = 24 + 48*i;
                        u16 maxy = miny + 48;
                        if(BETWEEN(miny, y, maxy) && current_list->scroll + i < current_list->entries_count)
                        {
                            current_list->selected_entry = current_list->scroll + i;
                            break;
                        }
                    }
                }
            }
        }
    }

    exit_function(true);
    return 0;
}
