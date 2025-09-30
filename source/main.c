/*
*   This file is part of Anemone3DS
*   Copyright (C) 2015-2020 Contributors in CONTRIBUTORS.md
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
#include "music.h"
#include "remote.h"
#include "ui_strings.h"
#include "badges.h"
#include <time.h>

bool quit = false;
bool dspfirm = false;
static audio_s * audio = NULL;
static bool homebrew = false;
static bool installed_themes = false;
bool home_displayed = false;
u64 time_home_pressed = 0;

static Thread iconLoadingThread = {0};
static Thread_Arg_s iconLoadingThread_arg = {0};
static Handle update_icons_mutex;
static bool released = false;

static Thread install_check_threads[MODE_AMOUNT] = {0};
static Thread_Arg_s install_check_threads_arg[MODE_AMOUNT] = {0};

static Entry_List_s lists[MODE_AMOUNT] = {0};

Language_s language = {0};

int __stacksize__ = 64 * 1024;
Result archive_result;
Result badge_archive_result;
u32 old_time_limit;

const char * main_paths[REMOTE_MODE_AMOUNT] = {
    "/Themes/",
    "/Splashes/",
    "/Badges/"
};

const int entries_per_screen_v[MODE_AMOUNT] = {
    4,
    4,
};
const int entries_per_screen_h[MODE_AMOUNT] = { //for themeplaza browser
    6,
    6,
};
const int entry_size[MODE_AMOUNT] = {
    48,
    48,
};

static void init_services(void)
{
    consoleDebugInit(debugDevice_SVC);
    cfguInit();
    ptmuInit();
    acInit();
    dspfirm = !ndspInit();
    APT_GetAppCpuTimeLimit(&old_time_limit);
    APT_SetAppCpuTimeLimit(30);
    httpcInit(0);
    init_sd();
    archive_result = open_archives();
    badge_archive_result = open_badge_extdata();
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
    ndspExit();
}

static void stop_install_check(void)
{
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        install_check_threads_arg[i].run_thread = false;
    }
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        if(install_check_threads[i] == NULL)
            continue;

        threadJoin(install_check_threads[i], U64_MAX);
        threadFree(install_check_threads[i]);
        install_check_threads[i] = NULL;
    }
}

static inline void wait_scroll(void)
{
    released = true;
    svcReleaseMutex(update_icons_mutex);
    svcSleepThread(FASTSCROLL_WAIT);
}

static void exit_thread(void)
{
    if(iconLoadingThread_arg.run_thread)
    {
        DEBUG("exiting thread\n");
        iconLoadingThread_arg.run_thread = false;
        svcReleaseMutex(update_icons_mutex);
        svcWaitSynchronization(update_icons_mutex, U64_MAX);
        threadJoin(iconLoadingThread, U64_MAX);
        threadFree(iconLoadingThread);
        iconLoadingThread = NULL;
    }
}

void free_lists(void)
{
    stop_install_check();
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        Entry_List_s * const current_list = &lists[i];
        C3D_TexDelete(&current_list->icons_texture);
        free(current_list->icons_info);
        free(current_list->entries);
        memset(current_list, 0, sizeof(Entry_List_s));
    }
    exit_thread();
}

void exit_function(bool power_pressed)
{
    if(audio)
    {
        stop_audio(&audio);
    }
    free_lists();
    svcCloseHandle(update_icons_mutex);
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

static u32 next_or_equal_power_of_2(u32 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static void load_lists(Entry_List_s * lists)
{
    free_lists();
    for(int i = 0; i < MODE_AMOUNT; i++)
    {
        InstallType loading_screen = INSTALL_NONE;
        if(i == MODE_THEMES)
            loading_screen = INSTALL_LOADING_THEMES;
        else if(i == MODE_SPLASHES)
            loading_screen = INSTALL_LOADING_SPLASHES;

        draw_install(loading_screen);

        Entry_List_s * const current_list = &lists[i];
        current_list->mode = i;
        current_list->entries_per_screen_v = entries_per_screen_v[i];
        current_list->entries_per_screen_h = 1;
        current_list->entries_loaded = current_list->entries_per_screen_v * current_list->entries_per_screen_h;
        current_list->entry_size = entry_size[i];

        const int x_component = max(current_list->entries_per_screen_h, current_list->entries_per_screen_v);
        const int y_component = min(current_list->entries_per_screen_h, current_list->entries_per_screen_v);
        // A texture must have power of 2 dimensions (not necessarily the same)
        // so, get the power of two greater than or equal to:
        // - the size of the largest length (row or column) of icons for the width
        // - the size of all of those lengths to fit the total for the height
        C3D_TexInit(&current_list->icons_texture,
            next_or_equal_power_of_2(x_component * current_list->entry_size),
            next_or_equal_power_of_2(y_component * current_list->entry_size * ICONS_OFFSET_AMOUNT),
            GPU_RGB565);
        C3D_TexSetFilter(&current_list->icons_texture, GPU_NEAREST, GPU_NEAREST);

        const float inv_width = 1.0f / current_list->icons_texture.width;
        const float inv_height = 1.0f / current_list->icons_texture.height;
        current_list->icons_info = (Entry_Icon_s *)calloc(x_component * y_component * ICONS_OFFSET_AMOUNT, sizeof(Entry_Icon_s));
        for(int j = 0; j < y_component * ICONS_OFFSET_AMOUNT; ++j)
        {
            const int index = j * x_component;
            for(int h = 0; h < x_component; ++h)
            {
                Entry_Icon_s * const icon_info = &current_list->icons_info[index + h];
                icon_info->x = h * current_list->entry_size;
                icon_info->y = j * current_list->entry_size;
                icon_info->subtex.width = current_list->entry_size;
                icon_info->subtex.height = current_list->entry_size;
                icon_info->subtex.left = icon_info->x * inv_width;
                icon_info->subtex.top = 1.0f - (icon_info->y * inv_height);
                icon_info->subtex.right = icon_info->subtex.left + (icon_info->subtex.width * inv_width);
                icon_info->subtex.bottom = icon_info->subtex.top - (icon_info->subtex.height * inv_height);
            }
        }

        Result res = load_entries(main_paths[i], current_list, loading_screen);
        if(R_SUCCEEDED(res))
        {
            if(current_list->entries_count > current_list->entries_loaded * ICONS_OFFSET_AMOUNT)
                iconLoadingThread_arg.run_thread = true;

            DEBUG("total: %i\n", current_list->entries_count);

            sort_by_name(current_list);
            load_icons_first(current_list, false);

            void (*install_check_function)(void *) = NULL;
            if(i == MODE_THEMES)
                install_check_function = themes_check_installed;
            else if(i == MODE_SPLASHES)
                install_check_function = splash_check_installed;

            Thread_Arg_s * current_arg = &install_check_threads_arg[i];
            current_arg->run_thread = true;
            current_arg->thread_arg = (void **)current_list;

            if(install_check_function != NULL)
            {
                install_check_threads[i] = threadCreate(install_check_function, current_arg, __stacksize__, 0x3f, -2, false);
                svcSleepThread(1e8);
            }
        }
    }
    start_thread();
}

static SwkbdCallbackResult jump_menu_callback(void * entries_count, const char ** ppMessage, const char * text, size_t textlen)
{
    (void)textlen;
    int typed_value = atoi(text);
    if(typed_value > *(int *)entries_count)
    {
        *ppMessage = language.main.position_too_big;
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if(typed_value == 0)
    {
        *ppMessage = language.main.position_zero;
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

    sprintf(numbuf, language.main.jump_q);
    swkbdSetHintText(&swkbd, numbuf);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, language.main.cancel, false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, language.main.jump, true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, max_chars);
    swkbdSetFilterCallback(&swkbd, jump_menu_callback, &list->entries_count);

    memset(numbuf, 0, sizeof(numbuf));
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        list->selected_entry = atoi(numbuf) - 1;
        wait_scroll();
    }
}

static void change_selected(Entry_List_s * list, int change_value)
{
    if(abs(change_value) >= list->entries_count) return;

    int newval = list->selected_entry + change_value;

    if(newval < 0)
        newval += list->entries_count;
    newval %= list->entries_count;

    list->selected_entry = newval;
}

static void toggle_shuffle(Entry_List_s * list)
{
    Entry_s * current_entry = &list->entries[list->selected_entry];
    if(current_entry->in_shuffle)
    {
        if(current_entry->no_bgm_shuffle)
        {
            current_entry->in_shuffle = false;
            current_entry->no_bgm_shuffle = false;
            list->shuffle_count--;
        }
        else
        {
            current_entry->no_bgm_shuffle = true;
        }
    }
    else
    {
        current_entry->in_shuffle = true;
        list->shuffle_count++;
    }
}

int main(void)
{
    srand(time(NULL));
    init_services();
    const CFG_Language lang = get_system_language();
    language = init_strings(lang);
    init_screens();

    svcCreateMutex(&update_icons_mutex, true);

    static Entry_List_s * current_list = NULL;
    void * iconLoadingThread_args_void[] = {
        &current_list,
        &update_icons_mutex,
    };
    iconLoadingThread_arg.thread_arg = iconLoadingThread_args_void;
    iconLoadingThread_arg.run_thread = false;

    #ifndef CITRA_MODE
    if(R_SUCCEEDED(archive_result))
        load_lists(lists);
    #else
    load_lists(lists);
    #endif

    EntryMode current_mode = MODE_THEMES;

    bool preview_mode = false;
    int preview_offset = 0;

    bool install_mode = false;
    DrawMode draw_mode = DRAW_MODE_LIST;
    bool extra_mode = false;
    int extra_index = 1;
    C2D_Image preview = {0};

    while(aptMainLoop())
    {
        if(quit)
        {
            free_preview(preview);
            exit_function(false);
            return 0;
        }

        if (aptCheckHomePressRejected() && !home_displayed)
        {
            time_home_pressed = svcGetSystemTick() / CPU_TICKS_PER_MSEC;
            home_displayed = true;
        }

        #ifndef CITRA_MODE
        if(R_FAILED(archive_result) && current_mode == MODE_THEMES)
        {
            throw_error(language.main.no_theme_extdata, ERROR_LEVEL_ERROR);
            quit = true;
            continue;
        }
        #endif

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        current_list = &lists[current_mode];

        Instructions_s instructions = language.normal_instructions[current_mode];
        if(install_mode)
            instructions = language.install_instructions;
        if(extra_mode)
        {
            instructions = language.extra_instructions[extra_index];
        }

        if(preview_mode)
        {
            draw_preview(preview, preview_offset, 1.0f);
        }
        else {
            if(!iconLoadingThread_arg.run_thread)
            {
                handle_scrolling(current_list);
                current_list->previous_scroll = current_list->scroll;
            }
            else
            {
                if(!released)
                {
                    svcReleaseMutex(update_icons_mutex);
                    released = true;
                }
                svcWaitSynchronization(update_icons_mutex, U64_MAX);
            }

            draw_interface(current_list, instructions, draw_mode);

            svcSleepThread(1e7);
            released = false;
        }

        if (home_displayed)
        {
            u64 cur_time = svcGetSystemTick() / CPU_TICKS_PER_MSEC;
            draw_home(time_home_pressed, cur_time);
            if (cur_time - time_home_pressed > 2000) home_displayed = false;
        }

        end_frame();

        if(kDown & KEY_START) quit = true;

        if(current_list->entries_count == 0)
        {
            if (kDown & KEY_R)
            {
                goto enable_qr;
            } else if (kDown & KEY_L)
            {
                goto switch_mode;
            } else if (kDown & KEY_TOUCH)
            {
                touchPosition touch = {0};
                hidTouchRead(&touch);

                u16 x = touch.px;
                u16 y = touch.py;
                if (y < 24)
                {
                    if(BETWEEN(320-24, x, 320))
                    {
                        goto switch_mode;
                    } else if(BETWEEN(320-48, x, 320-24))
                    {
                        quit = true;
                        continue;
                    } else if(BETWEEN(320-72, x, 320-48))
                    {
                        goto browse_themeplaza;
                    } else if(BETWEEN(320-96, x, 320-72))
                    {
                        goto enable_qr;
                    }
                }
            }
        }
        else if(!install_mode && !extra_mode)
        {
            if(!preview_mode && kDown & KEY_L) //toggle between splashes and themes
            {
                switch_mode:
                current_mode++;
                current_mode %= MODE_AMOUNT;
                continue;
            }
            else if(!preview_mode && kDown & KEY_R) //toggle QR mode
            {
                enable_qr:
                draw_base_interface();
                draw_text_center(GFX_TOP, 100, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE_BACKGROUND], language.main.loading_qr);
                end_frame();
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
                        throw_error(language.main.no_wifi, ERROR_LEVEL_WARNING);
                    }
                }
                else
                {
                    if(homebrew)
                        throw_error(language.main.qr_homebrew, ERROR_LEVEL_WARNING);
                    else
                        throw_error(language.main.camera_broke, ERROR_LEVEL_WARNING);
                }

                continue;
            }
            else if(kDown & KEY_Y && current_list->entries != NULL) //toggle preview mode
            {
                toggle_preview:
                if(!preview_mode)
                {
                    preview_mode = load_preview(current_list, &preview, &preview_offset);
                    if(preview_mode)
                    {
                        end_frame();
                        draw_preview(preview, preview_offset, 1.0f);
                        end_frame();
                        if(current_mode == MODE_THEMES && dspfirm)
                        {
                            audio = calloc(1, sizeof(audio_s));
                            Result r = load_audio(&current_list->entries[current_list->selected_entry], audio);
                            if (R_SUCCEEDED(r)) play_audio(audio);
                            else audio = NULL;
                        }
                    }
                }
                else
                {
                    preview_mode = false;
                    if(current_mode == MODE_THEMES && audio)
                    {
                        stop_audio(&audio);
                    }
                }
                continue;
            }
            else if(preview_mode && kDown & (KEY_B | KEY_TOUCH))
            {
                preview_mode = false;
                if(current_mode == MODE_THEMES && audio)
                {
                    stop_audio(&audio);
                }
                continue;
            }
        }

        int selected_entry = current_list->selected_entry;
        Entry_s * current_entry = &current_list->entries[selected_entry];

        if(preview_mode || current_list->entries == NULL)
            goto touch;


        if(install_mode)
        {
            if ((kDown | kHeld) & KEY_TOUCH)
            {
                touchPosition touch = {0};
                hidTouchRead(&touch);
                u16 x = touch.px;
                u16 y = touch.py;

                if (kDown & KEY_TOUCH)
                {
                    if (y < 24)
                    {
                        if (BETWEEN(320-24, x, 320))
                        {
                            goto install_theme_single;
                        } else if (BETWEEN(320-48, x, 320-24))
                        {
                            goto install_theme_shuffle;
                        } else if (BETWEEN(320-72, x, 320-48))
                        {
                            goto install_theme_no_bgm;
                        } else if (BETWEEN(320-96, x, 320-72))
                        {
                            goto install_theme_bgm_only;
                        } else if (BETWEEN(2, x, 26))
                        {
                            goto install_leave;
                        }
                    }
                }
            }

            if(kDown & KEY_B)
            {
                install_leave:
                install_mode = false;
                draw_mode = DRAW_MODE_LIST;
            }
            else if(kDown & KEY_DLEFT)
            {
                install_theme_bgm_only:
                install_mode = false;
                draw_mode = DRAW_MODE_LIST;
                aptSetHomeAllowed(false);
                draw_install(INSTALL_BGM);
                if(R_SUCCEEDED(bgm_install(current_entry)))
                {
                    for(int i = 0; i < current_list->entries_count; i++)
                    {
            #define BETWEEN(min, x, max) (min < x && x < max)
                        Entry_s * theme = &current_list->entries[i];
                        if(theme == current_entry)
                            theme->installed = true;
                        else
                            theme->installed = false;
                    }
                    installed_themes = true;
                }
            }
            else if(kDown & KEY_DUP)
            {
                install_theme_single:
                install_mode = false;
                draw_mode = DRAW_MODE_LIST;
                aptSetHomeAllowed(false);
                draw_install(INSTALL_SINGLE);
                if(R_SUCCEEDED(theme_install(current_entry)))
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
            else if(kDown & KEY_DRIGHT)
            {
                install_theme_no_bgm:
                install_mode = false;
                draw_mode = DRAW_MODE_LIST;
                aptSetHomeAllowed(false);
                draw_install(INSTALL_NO_BGM);
                if(R_SUCCEEDED(no_bgm_install(current_entry)))
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
            else if(kDown & KEY_DDOWN)
            {
                install_theme_shuffle:
                install_mode = false;
                draw_mode = DRAW_MODE_LIST;
                if(current_list->shuffle_count > MAX_SHUFFLE_THEMES)
                {
                    throw_error(language.main.too_many_themes, ERROR_LEVEL_WARNING);
                }
                else if(current_list->shuffle_count < 2)
                {
                    throw_error(language.main.not_enough_themes, ERROR_LEVEL_WARNING);
                }
                else
                {
                    aptSetHomeAllowed(false);
                    draw_install(INSTALL_SHUFFLE);
                    Result res = shuffle_install(current_list);
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
            continue;
        }
        else if(extra_mode)
        {
            if((kDown | kHeld) & KEY_TOUCH)
            {
                touchPosition touch = {0};
                hidTouchRead(&touch);
                u16 x = touch.px;
                u16 y = touch.py;
                if (kDown & KEY_TOUCH)
                {
                    if (y < 24)
                    {
                        if (BETWEEN(320-24, x, 320))
                        {
                            goto browse_themeplaza;
                        } else if (BETWEEN(320-48, x, 320-24))
                        {
                            goto dump_single;
                        } else if (BETWEEN(320-72, x, 320-48))
                        {
                            switch (current_list->current_sort)
                            {
                                case SORT_NAME:
                                    goto sort_author;
                                    break;
                                case SORT_AUTHOR:
                                    goto sort_path;
                                    break;
                                case SORT_PATH:
                                    goto sort_name;
                                    break;
                                default:
                                    break;
                            }
                        } else if (BETWEEN(320-96, x, 320-72))
                        {
                            goto badge_install;
                        } else if (BETWEEN(2, x, 26))
                        {
                            extra_mode = false;
                            extra_index = 1;
                            draw_mode = DRAW_MODE_LIST;
                        }
                    }
                }
            }
            else if(extra_index == 1)
            {
                if(kDown & KEY_B)
                {
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                }
                else if(kDown & KEY_DLEFT)
                {
                    browse_themeplaza:
                    if(themeplaza_browser((RemoteMode) current_mode))
                    {
                        current_mode = MODE_THEMES;
                        load_lists(lists);
                    }
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if(kDown & KEY_DUP)
                {
                    jump:
                    jump_menu(current_list);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;

                }
                else if(kDown & KEY_DDOWN)
                {
                    load_icons_first(current_list, false);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if (kDown & KEY_DRIGHT)
                {
                    badge_install:
                    extra_mode = false;
                    extra_index = 1;
                    draw_mode = DRAW_MODE_LIST;
                    draw_install(INSTALL_BADGES);
                    install_badges();
                }
                else if (kDown & KEY_R)
                {
                    extra_index = 2;
                }
                else if(kDown & KEY_L)
                {
                    extra_index = 0;
                }
            }
            else if(extra_index == 0)
            {
                if(kDown & KEY_DLEFT)
                {
                    sort_path:
                    sort_by_filename(current_list);
                    load_icons_first(current_list, false);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if(kDown & KEY_DUP)
                {
                    sort_name:
                    sort_by_name(current_list);
                    load_icons_first(current_list, false);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if(kDown & KEY_DDOWN)
                {
                    sort_author:
                    sort_by_author(current_list);
                    load_icons_first(current_list, false);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if (kDown & KEY_B)
                {
                    extra_index = 1;
                }
            }
            else if(extra_index == 2)
            {
                if(kDown & KEY_DUP)
                {
                    dump_single:
                    draw_install(INSTALL_DUMPING_THEME);
                    Result res = dump_current_theme();
                    if (R_FAILED(res)) DEBUG("Dump theme result: %lx\n", res);
                    else load_lists(lists);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if(kDown & KEY_DDOWN)
                {
                    draw_install(INSTALL_DUMPING_ALL_THEMES);
                    Result res = dump_all_themes();
                    if (R_FAILED(res)) DEBUG("Dump all themes result: %lx\n", res);
                    else load_lists(lists);
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if(kDown & KEY_DLEFT)
                {
                    draw_install(INSTALL_DUMPING_BADGES);
                    extract_badges();
                    extra_mode = false;
                    draw_mode = DRAW_MODE_LIST;
                    extra_index = 1;
                }
                else if(kDown & KEY_B)
                {
                    extra_index = 1;
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
                    draw_mode = DRAW_MODE_INSTALL;
                    break;
                case MODE_SPLASHES:
                    draw_install(INSTALL_SPLASH);
                    splash_install(current_entry);
                    for(int i = 0; i < current_list->entries_count; i++)
                    {
                        Entry_s * splash = &current_list->entries[i];
                        if(splash == current_entry)
                            splash->installed = true;
                        else
                            splash->installed = false;
                    }
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
                    toggle_shuffle(current_list);
                    break;
                case MODE_SPLASHES:
                    if(draw_confirm(language.main.uninstall_confirm, current_list, draw_mode))
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
            extra_mode = true;
            draw_mode = DRAW_MODE_EXTRA;
        }
        else if(kDown & KEY_SELECT)
        {
            if(draw_confirm(language.main.delete_confirm, current_list, draw_mode))
            {
                draw_install(INSTALL_ENTRY_DELETE);
                delete_entry(current_entry, current_entry->is_zip);
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
            change_selected(current_list, -current_list->entries_per_screen_v);
        }
        else if(kDown & KEY_RIGHT)
        {
            change_selected(current_list, current_list->entries_per_screen_v);
        }

        // Fast scroll using circle pad
        else if(kHeld & KEY_CPAD_UP)
        {
            change_selected(current_list, -1);
            wait_scroll();
        }
        else if(kHeld & KEY_CPAD_DOWN)
        {
            change_selected(current_list, 1);
            wait_scroll();
        }
        else if(kHeld & KEY_CPAD_LEFT)
        {
            change_selected(current_list, -current_list->entries_per_screen_v);
            wait_scroll();
        }
        else if(kHeld & KEY_CPAD_RIGHT)
        {
            change_selected(current_list, current_list->entries_per_screen_v);
            wait_scroll();
        }

        // Movement using the touchscreen
        touch:
        if((kDown | kHeld) & KEY_TOUCH)
        {
            touchPosition touch = {0};
            hidTouchRead(&touch);

            u16 x = touch.px;
            u16 y = touch.py;

            u16 arrowStartX = 136;
            u16 arrowEndX = arrowStartX+16;


            if(kDown & KEY_TOUCH)
            {
                if(y < 24)
                {
                    if(BETWEEN(320-144, x, 320-120))
                    {
                        if (current_mode == MODE_THEMES)
                        {
                            toggle_shuffle(current_list);
                        }
                    }
                    else if(BETWEEN(2, x, 26))
                    {
                        extra_mode = true;
                        draw_mode = DRAW_MODE_EXTRA;
                    }
                    else if(BETWEEN(320-120, x, 320-96))
                    {
                        if (current_mode == MODE_THEMES)
                        {
                            install_mode = true;
                            draw_mode = DRAW_MODE_INSTALL;
                        } else if (current_mode == MODE_SPLASHES)
                        {
                            draw_install(INSTALL_SPLASH);
                            splash_install(current_entry);
                            for(int i = 0; i < current_list->entries_count; i++)
                            {
                                Entry_s * splash = &current_list->entries[i];
                                if(splash == current_entry)
                                    splash->installed = true;
                                else
                                    splash->installed = false;
                            }
                        }
                    }
                    else if(BETWEEN(320-96, x, 320-72))
                    {
                        goto enable_qr;
                    }
                    else if(BETWEEN(320-72, x, 320-48))
                    {
                        quit = true;
                    }
                    else if(BETWEEN(320-48, x, 320-24))
                    {
                        goto toggle_preview;
                    }
                    else if(BETWEEN(320-24, x, 320))
                    {
                        goto switch_mode;
                    }
                }
                else if(y >= 216)
                {
                    if(current_list->entries != NULL && BETWEEN(arrowStartX, x, arrowEndX) && current_list->scroll > 0)
                    {
                        change_selected(current_list, -current_list->entries_per_screen_v);
                    }
                    else if(current_list->entries != NULL && BETWEEN(arrowStartX + 16, x, arrowEndX + 16) && current_list->scroll < current_list->entries_count - current_list->entries_per_screen_v)
                    {
                        change_selected(current_list, current_list->entries_per_screen_v);
                    }
                    else if(current_list->entries != NULL && BETWEEN(176, x, 320))
                    {
                        goto jump;
                    }
                }
            }
            else
            {
                if(current_list->entries != NULL && BETWEEN(24, y, 216))
                {
                    for(int i = 0; i < current_list->entries_loaded; i++)
                    {
                        u16 miny = 24 + current_list->entry_size * i;
                        u16 maxy = miny + current_list->entry_size;
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

    free_preview(preview);
    // aptSetHomeAllowed(true);
    exit_function(true);

    return 0;
}
