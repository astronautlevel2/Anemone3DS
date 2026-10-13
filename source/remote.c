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

#include <ctype.h>
#include <malloc.h>

#include "remote.h"
#include "loading.h"
#include "fs.h"
#include "unicode.h"
#include "music.h"
#include "urls.h"
#include "conversion.h"
#include "ui_strings.h"
#include "remote_api.h"

char *last_search = NULL;
json_int_t last_page = 1;

static void free_icons(Entry_List_s * list)
{
    if (list != NULL)
    {
        C3D_TexDelete(&list->icons_texture);
        free(list->icons_info);
    }
}

static void load_page(Entry_List_s * list, json_t * root, json_int_t page, RemoteMode mode)
{
    if (remote_api_load_page(list, root))
    {
        list->tp_current_page = page;
        list->mode = (EntryMode) mode;
        last_page = page;
    }
    else
    {
        // the current page stays as it was, only the search has to be put back
        throw_error(language.remote.no_results, ERROR_LEVEL_WARNING);
        free(list->tp_search);
        asprintf(&list->tp_search, "%s", last_search);
    }
}

static void load_remote_list(Entry_List_s * list, json_int_t page, RemoteMode mode)
{
    remote_icons_stop();

    if (page > list->tp_page_count)
        page = 1;
    if (page <= 0)
        page = list->tp_page_count;

    list->selected_entry = 0;

    InstallType loading_screen = INSTALL_NONE;
    if (mode == REMOTE_MODE_THEMES)
        loading_screen = INSTALL_LOADING_REMOTE_THEMES;
    else if (mode == REMOTE_MODE_SPLASHES)
        loading_screen = INSTALL_LOADING_REMOTE_SPLASHES;
    else if (mode == REMOTE_MODE_BADGES)
        loading_screen = INSTALL_LOADING_REMOTE_BADGES;
    draw_install(loading_screen);

    char * page_json = NULL;
    char * api_url = remote_api_page_url(list->remote_provider, mode, page, list->tp_search);
    u32 json_len;
    Result res = http_get(api_url, NULL, &page_json, &json_len, INSTALL_NONE, "application/json");
    free(api_url);
    if (R_SUCCEEDED(res) && json_len)
    {
        json_error_t error;
        json_t * root = json_loadb(page_json, json_len, 0, &error);
        if (root)
            load_page(list, root, page, mode);
        else
            DEBUG("json error on line %d: %s\n", error.line, error.text);

        json_decref(root);
    }
    else if (R_SUCCEEDED(res))
        throw_error(language.remote.check_wifi, ERROR_LEVEL_WARNING);

    free(page_json);
    remote_icons_start(list);
}

static bool load_remote_preview(const Entry_s * entry, C2D_Image * preview_image, Preview_Layout_s * preview_layout)
{
    if (entry->remote_preview_url == NULL)
        return false;

    char * preview = NULL;
    u32 preview_size = 0;
    draw_install(INSTALL_LOADING_REMOTE_PREVIEW);
    Result res = http_get(entry->remote_preview_url, NULL, &preview, &preview_size, INSTALL_LOADING_REMOTE_PREVIEW, "image/png");
    u32 height = 0;
    if (R_FAILED(res) || !preview_size || !(preview_size = png_to_abgr(&preview, preview_size, &height)))
    {
        free(preview);
        return false;
    }

    bool ret = load_preview_from_buffer(preview, preview_size, preview_image, preview_layout, height);
    free(preview);
    return ret;
}

static void download_remote_entry(Entry_s * entry, RemoteMode mode)
{
    char * zip_buf = NULL;
    char * filename = NULL;
    draw_install(INSTALL_DOWNLOAD);
    u32 zip_size;
    if(R_FAILED(http_get(entry->remote_download_url, &filename, &zip_buf, &zip_size, INSTALL_DOWNLOAD, "application/zip")))
    {
        free(filename);
        return;
    }

    if (filename == NULL && entry->remote_filename != NULL)
        filename = strdup(entry->remote_filename);

    save_zip_to_sd(filename, zip_size, zip_buf, mode);
    free(filename);
    free(zip_buf);
}

static SwkbdCallbackResult
jump_menu_callback(void * page_number, const char ** ppMessage, const char * text, size_t textlen)
{
    (void)textlen;
    int typed_value = atoi(text);
    if (typed_value > *(json_int_t *)page_number)
    {
        *ppMessage = language.remote.new_page_big;
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if (typed_value == 0)
    {
        *ppMessage = language.remote.new_page_zero;
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

static void jump_menu(Entry_List_s * list)
{
    if (list == NULL) return;

    char numbuf[64] = { 0 };

    SwkbdState swkbd;

    sprintf(numbuf, "%"
    JSON_INTEGER_FORMAT, list->tp_page_count);
    int max_chars = strlen(numbuf);
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, max_chars);

    sprintf(numbuf, "%"
    JSON_INTEGER_FORMAT, list->tp_current_page);
    swkbdSetInitialText(&swkbd, numbuf);

    sprintf(numbuf, language.remote.jump_page);
    swkbdSetHintText(&swkbd, numbuf);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, language.remote.cancel, false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, language.remote.jump, true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, max_chars);
    swkbdSetFilterCallback(&swkbd, jump_menu_callback, &list->tp_page_count);

    memset(numbuf, 0, sizeof(numbuf));
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if (button == SWKBD_BUTTON_CONFIRM)
    {
        json_int_t newpage = (json_int_t)atoi(numbuf);
        if (newpage != list->tp_current_page)
            load_remote_list(list, newpage, (RemoteMode) list->mode);
    }
}

static void search_menu(Entry_List_s * list)
{
    const int max_chars = 256;
    char * search = calloc(max_chars + 1, sizeof(char));

    SwkbdState swkbd;

    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, max_chars);
    swkbdSetHintText(&swkbd, language.remote.tags);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, language.remote.cancel, false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, language.remote.search, true);
    swkbdSetValidation(&swkbd, SWKBD_NOTBLANK, 0, max_chars);

    SwkbdButton button = swkbdInputText(&swkbd, search, max_chars);
    if (button == SWKBD_BUTTON_CONFIRM)
    {
        free(last_search);
        asprintf(&last_search, "%s", list->tp_search);
        free(list->tp_search);
        list->tp_search = url_escape(search);
        DEBUG("Search escaped: %s -> %s\n", search, list->tp_search);
        load_remote_list(list, 1, (RemoteMode) list->mode);
    }
    free(search);
}

static void change_selected(Entry_List_s * list, int change_value)
{
    if (abs(change_value) >= list->entries_count) return;

    int newval = list->selected_entry + change_value;

    if (abs(change_value) == 1)
    {
        if (newval < 0)
            newval += list->entries_per_screen_h;
        if (newval / list->entries_per_screen_h != list->selected_entry / list->entries_per_screen_h)
            newval += list->entries_per_screen_h * (-change_value);
        newval %= list->entries_count;
    }
    else
    {
        if (newval < 0)
            newval += list->entries_per_screen_h * list->entries_per_screen_v;
        newval %= list->entries_count;
    }
    list->selected_entry = newval;
}

static bool remote_browser(RemoteMode mode, RemoteProvider provider)
{
    bool downloaded = false;

    Parental_Restrictions_s restrictions = {0};
    Result res = load_parental_controls(&restrictions);
    if (R_SUCCEEDED(res))
    {
        if (restrictions.enable && restrictions.browser)
        {
            SwkbdState swkbd;
            char entered[5] = {0};
            swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, 4);
            swkbdSetFeatures(&swkbd, SWKBD_PARENTAL);

            swkbdInputText(&swkbd, entered, 5);
            SwkbdResult swkbd_res = swkbdGetResult(&swkbd);
            if (swkbd_res != SWKBD_PARENTAL_OK)
            {
                throw_error(language.remote.parental_fail, ERROR_LEVEL_WARNING);
                return downloaded;
            }
        }
    }

    if (R_FAILED(remote_icons_init()))
        return downloaded;

    bool preview_mode = false;
    Preview_Layout_s preview_layout = {0};

    Entry_List_s list = { 0 };
    Entry_List_s * current_list = &list;
    current_list->remote_provider = provider;
    current_list->tp_search = strdup("");
    last_search = strdup("");
    last_page = 1;

    list.entries_per_screen_v = entries_per_screen_v[mode];
    list.entries_per_screen_h = entries_per_screen_h[mode];
    list.entry_size = entry_size[mode];
    C3D_TexInit(&current_list->icons_texture, 512, 256, GPU_RGB565);
    C3D_TexSetFilter(&current_list->icons_texture, GPU_NEAREST, GPU_NEAREST);
    const int entries_icon_count = current_list->entries_per_screen_h * current_list->entries_per_screen_v;
    current_list->icons_info = calloc(entries_icon_count, sizeof(Entry_Icon_s));

    const float inv_width = 1.0f / current_list->icons_texture.width;
    const float inv_height = 1.0f / current_list->icons_texture.height;
    for(int i = 0; i < entries_icon_count; ++i)
    {
        Entry_Icon_s * const icon_info = &current_list->icons_info[i];
        // division by how many icons can fit horizontally
        const div_t d = div(i, (current_list->icons_texture.width / 48));
        icon_info->x = d.rem * current_list->entry_size;
        icon_info->y = d.quot * current_list->entry_size;
        icon_info->subtex.width = current_list->entry_size;
        icon_info->subtex.height = current_list->entry_size;
        icon_info->subtex.left = icon_info->x * inv_width;
        icon_info->subtex.top = 1.0f - (icon_info->y * inv_height);
        icon_info->subtex.right = icon_info->subtex.left + (icon_info->subtex.width * inv_width);
        icon_info->subtex.bottom = icon_info->subtex.top - (icon_info->subtex.height * inv_height);
    }

    load_remote_list(current_list, 1, mode);
    C2D_Image preview = { 0 };

    bool extra_mode = false;
    extern u64 time_home_pressed;
    extern bool home_displayed;

    while (aptMainLoop() && !quit)
    {
        if (current_list->entries == NULL)
            break;

        if (aptCheckHomePressRejected() && !home_displayed)
        {
            time_home_pressed = svcGetSystemTick() / CPU_TICKS_PER_MSEC;
            home_displayed = true;
        }

        if (preview_mode)
        {
            draw_preview(preview, &preview_layout);
            
        }
        else
        {
            Instructions_s instructions = language.remote_instructions[mode];
            if (extra_mode)
                instructions = language.remote_extra_instructions[mode];

            remote_icons_lock();
            draw_grid_interface(current_list, instructions, extra_mode);
            remote_icons_unlock();
        }

        if (home_displayed)
        {
            u64 cur_time = svcGetSystemTick() / CPU_TICKS_PER_MSEC;
            draw_home(time_home_pressed, cur_time);
            if (cur_time - time_home_pressed > 2000) home_displayed = false;
        }
        end_frame();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        if (kDown & KEY_START)
        {
        exit:
            quit = true;
            downloaded = false;
            break;
        }

        if (extra_mode)
        {
            if (kDown & KEY_B)
            {
                extra_mode = false;
            }
            else if (kDown & KEY_L)
            {
                extra_mode = false;
                mode = mode -1;
                if (mode > REMOTE_MODE_AMOUNT) mode = REMOTE_MODE_AMOUNT -1;
                free(current_list->tp_search);
                current_list->tp_search = strdup("");
                load_remote_list(current_list, 1, mode);
                mode = (RemoteMode) current_list->mode; // unchanged if the new mode couldn't be loaded
            }
            else if (kDown & KEY_R)
            {
                extra_mode = false;
                mode = mode + 1;
                mode = mode % REMOTE_MODE_AMOUNT;
                free(current_list->tp_search);
                current_list->tp_search = strdup("");
                load_remote_list(current_list, 1, mode);
                mode = (RemoteMode) current_list->mode;
            }
            else if (kDown & KEY_DUP)
            {
                extra_mode = false;
                remote_icons_stop();
                jump_menu(current_list);
                remote_icons_start(current_list);
            }
            else if (kDown & KEY_DDOWN)
            {
                extra_mode = false;
                remote_icons_stop();
                search_menu(current_list);
                remote_icons_start(current_list);
            }
            continue;
        }

        int selected_entry = current_list->selected_entry;
        Entry_s * current_entry = &current_list->entries[selected_entry];

        if (kDown & KEY_Y)
        {
        toggle_preview:
            if (!preview_mode)
            {
                remote_icons_stop();
                preview_mode = load_remote_preview(current_entry, &preview, &preview_layout);
                remote_icons_start(current_list);
            }
            else
            {
                preview_mode = false;
            }
            continue;
        }
        else if (kDown & KEY_B)
        {
            if (preview_mode)
                preview_mode = false;
            else
                break;
        }

        if (preview_mode)
            goto touch;

        if (kDown & KEY_A)
        {
            remote_icons_stop();
            download_remote_entry(current_entry, mode);
            remote_icons_start(current_list);
            downloaded = true;
        }
        else if (kDown & KEY_X)
        {
            extra_mode = true;
        }
        else if (kDown & KEY_L)
        {
            load_remote_list(current_list, current_list->tp_current_page - 1, mode);
        }
        else if (kDown & KEY_R)
        {
            load_remote_list(current_list, current_list->tp_current_page + 1, mode);
        }

            // Movement in the UI
        else if (kDown & KEY_UP)
        {
            change_selected(current_list, -current_list->entries_per_screen_h);
        }
        else if (kDown & KEY_DOWN)
        {
            change_selected(current_list, current_list->entries_per_screen_h);
        }
            // Quick moving
        else if (kDown & KEY_LEFT)
        {
            change_selected(current_list, -1);
        }
        else if (kDown & KEY_RIGHT)
        {
            change_selected(current_list, 1);
        }

    touch:
        if ((kDown | kHeld) & KEY_TOUCH)
        {
            touchPosition touch = { 0 };
            hidTouchRead(&touch);

            u16 x = touch.px;
            u16 y = touch.py;

#define BETWEEN(min, x, max) (min < x && x < max)

            int border = 16;
            if (kDown & KEY_TOUCH)
            {
                if (preview_mode)
                {
                    preview_mode = false;
                    continue;
                }

                if (y < 24)
                {
                    if (BETWEEN(0, x, 80))
                    {
                        remote_icons_stop();
                        search_menu(current_list);
                        remote_icons_start(current_list);
                    }
                    else if (BETWEEN(320 - 96, x, 320 - 72))
                    {
                        break;
                    }
                    else if (BETWEEN(320 - 72, x, 320 - 48))
                    {
                        goto exit;
                    }
                    else if (BETWEEN(320 - 48, x, 320 - 24))
                    {
                        goto toggle_preview;
                    }
                    else if (BETWEEN(320 - 24, x, 320))
                    {
                        mode++;
                        mode %= REMOTE_MODE_AMOUNT;

                        free(current_list->tp_search);
                        current_list->tp_search = strdup("");

                        load_remote_list(current_list, 1, mode);
                        mode = (RemoteMode) current_list->mode;
                    }
                }
                else if (BETWEEN(240 - 24, y, 240) && BETWEEN(176, x, 320))
                {
                    remote_icons_stop();
                    jump_menu(current_list);
                    remote_icons_start(current_list);
                }
                else
                {
                    if (BETWEEN(0, x, border))
                    {
                        load_remote_list(current_list, current_list->tp_current_page - 1, mode);
                    }
                    else if (BETWEEN(320 - border, x, 320))
                    {
                        load_remote_list(current_list, current_list->tp_current_page + 1, mode);
                    }
                }
            }
            else
            {
                if (BETWEEN(24, y, 240 - 24))
                {
                    if (BETWEEN(border, x, 320 - border))
                    {
                        x -= border;
                        x /= current_list->entry_size;
                        y -= 24;
                        y /= current_list->entry_size;
                        int new_selected = y * current_list->entries_per_screen_h + x;
                        if (new_selected < current_list->entries_count)
                            current_list->selected_entry = new_selected;
                    }
                }
            }
        }
    }

    free_preview(preview);
    remote_icons_exit();

    free_icons(current_list);
    free_remote_entries(current_list);
    free(current_list->tp_search);
    free(last_search);

    return downloaded;
}

static bool select_remote_provider(RemoteProvider * provider)
{
    while (aptMainLoop() && !quit)
    {
        draw_remote_provider_picker(*provider);

        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            quit = true;
        else if (kDown & KEY_B)
            return false;
        else if (kDown & KEY_A)
            return true;
        else if (kDown & (KEY_LEFT | KEY_L))
            *provider = REMOTE_PROVIDER_THEMEPLAZA;
        else if (kDown & (KEY_RIGHT | KEY_R))
            *provider = REMOTE_PROVIDER_THEMEZER;
        else if (kDown & KEY_TOUCH)
        {
            touchPosition touch = {0};
            hidTouchRead(&touch);

            const bool card_row = touch.py >= PROVIDER_CARD_Y && touch.py < PROVIDER_CARD_Y + PROVIDER_CARD_HEIGHT;
            if (card_row && touch.px >= PROVIDER_CARD_THEMEPLAZA_X && touch.px < PROVIDER_CARD_THEMEPLAZA_X + PROVIDER_CARD_WIDTH)
            {
                *provider = REMOTE_PROVIDER_THEMEPLAZA;
                return true;
            }
            else if (card_row && touch.px >= PROVIDER_CARD_THEMEZER_X && touch.px < PROVIDER_CARD_THEMEZER_X + PROVIDER_CARD_WIDTH)
            {
                *provider = REMOTE_PROVIDER_THEMEZER;
                return true;
            }
        }
    }

    return false;
}

bool browse_remote(RemoteMode mode)
{
    // remembered while the app is open
    static RemoteProvider provider = REMOTE_PROVIDER_THEMEPLAZA;
    if (!select_remote_provider(&provider))
        return false;

    return remote_browser(mode, provider);
}
