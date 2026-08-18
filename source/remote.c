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
#include <curl/curl.h>
#include <malloc.h>

#include "remote.h"
#include "remote_internal.h"
#include "loading.h"
#include "fs.h"
#include "unicode.h"
#include "music.h"
#include "themes.h"
#include "splashes.h"
#include "urls.h"
#include "conversion.h"
#include "ui_strings.h"
#include "sprites.h"

char *last_search = NULL;
json_int_t last_page = 1;

static const char * remote_provider_names[REMOTE_PROVIDER_AMOUNT] = {
    "ThemePlaza",
    "Themezer",
};

// forward declaration of special case used only here
// TODO: replace this travesty with a proper handler
static Result http_get_with_not_found_flag(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types, bool not_found_is_error);
static bool mime_type_is_acceptable(const char *acceptable_mime_types, const char *mime_type);
static bool remote_browser(RemoteMode mode, RemoteProvider provider);
static void restart_remote_icon_loader_if_needed(Entry_List_s * list, RemoteProvider provider);

static int curl_progress_cb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    (void)ultotal;
    (void)ulnow;

    InstallType install_type = *(InstallType *)clientp;
    hidScanInput();
    if (hidKeysDown() & KEY_B)
    {
        set_loading_cancel_requested(true);
        return 1;
    }

    if (install_type != INSTALL_NONE && dltotal > 0.0)
        draw_loading_bar((u32)dlnow, (u32)dltotal, install_type);

    return loading_cancel_requested() ? 1 : 0;
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

const char * get_remote_provider_name(RemoteProvider provider)
{
    if (provider >= REMOTE_PROVIDER_AMOUNT)
        return "Remote";

    return remote_provider_names[provider];
}

bool is_remote_provider_v2(RemoteProvider provider)
{
    return provider != REMOTE_PROVIDER_THEMEPLAZA;
}

static void draw_remote_provider_card(RemoteProvider provider, float x, float y, bool selected)
{
    const float width = 144.0f;
    const float height = 96.0f;
    const float icon_x = x + 48.0f;
    const float icon_y = y + 8.0f;
    const Color border = selected ? colors[COLOR_ACCENT] : C2D_Color32(68, 68, 68, 255);
    const Color fill = selected ? C2D_Color32(35, 35, 35, 235) : C2D_Color32(20, 20, 20, 220);
    const int icon_id = provider == REMOTE_PROVIDER_THEMEPLAZA ? sprites_themeplaza_icon_idx : sprites_themezer_icon_idx;

    C2D_DrawRectSolid(x - 2, y - 2, 0.55f, width + 4, height + 4, border);
    C2D_DrawRectSolid(x, y, 0.56f, width, height, fill);
    draw_image(icon_id, icon_x, icon_y);

    const char * label = provider == REMOTE_PROVIDER_THEMEPLAZA ? "Theme Plaza" : "Themezer";
    const float label_y = y + 66.0f;
    float label_width = 0.0f;
    get_text_dimensions(label, 0.62f, 0.62f, &label_width, NULL);
    const float label_x = x + ((width - label_width) / 2.0f);

    draw_text(label_x - 1.0f, label_y, 0.7f, 0.62f, 0.62f, colors[COLOR_BACKGROUND], label);
    draw_text(label_x + 1.0f, label_y, 0.7f, 0.62f, 0.62f, colors[COLOR_BACKGROUND], label);
    draw_text(label_x, label_y - 1.0f, 0.7f, 0.62f, 0.62f, colors[COLOR_BACKGROUND], label);
    draw_text(label_x, label_y + 1.0f, 0.7f, 0.62f, 0.62f, colors[COLOR_BACKGROUND], label);
    draw_text(label_x, label_y, 0.7f, 0.62f, 0.62f, colors[COLOR_WHITE_BACKGROUND], label);
}

bool select_remote_provider(RemoteProvider * provider)
{
    if (provider == NULL)
        return false;

    RemoteProvider current = *provider;
    if (current >= REMOTE_PROVIDER_AMOUNT)
        current = REMOTE_PROVIDER_THEMEPLAZA;

    while (aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_B)
            return false;

        if (kDown & (KEY_LEFT | KEY_L))
            current = REMOTE_PROVIDER_THEMEPLAZA;
        else if (kDown & (KEY_RIGHT | KEY_R))
            current = REMOTE_PROVIDER_THEMEZER;
        else if (kDown & KEY_TOUCH)
        {
            touchPosition touch = { 0 };
            hidTouchRead(&touch);

            if (BETWEEN(14, touch.px, 158) && BETWEEN(78, touch.py, 174))
            {
                *provider = REMOTE_PROVIDER_THEMEPLAZA;
                return true;
            }
            else if (BETWEEN(166, touch.px, 310) && BETWEEN(78, touch.py, 174))
            {
                *provider = REMOTE_PROVIDER_THEMEZER;
                return true;
            }
        }

        if (kDown & KEY_A)
        {
            *provider = current;
            return true;
        }

        draw_base_interface();
        draw_text_center(GFX_TOP, 98, 0.5f, 0.8f, 0.8f, colors[COLOR_WHITE_BACKGROUND], "Select a source");
        draw_text_center(GFX_TOP, 126, 0.5f, 0.62f, 0.62f, colors[COLOR_WHITE_BACKGROUND], "\uE000 Select   \uE001 Cancel");

        set_screen(bottom);
        draw_remote_provider_card(REMOTE_PROVIDER_THEMEPLAZA, 14, 78, current == REMOTE_PROVIDER_THEMEPLAZA);
        draw_remote_provider_card(REMOTE_PROVIDER_THEMEZER, 166, 78, current == REMOTE_PROVIDER_THEMEZER);

        end_frame();
    }

    return false;
}

bool browse_remote_provider(RemoteProvider provider, RemoteMode mode)
{
    switch (provider)
    {
    case REMOTE_PROVIDER_THEMEPLAZA:
    case REMOTE_PROVIDER_THEMEZER:
        return remote_browser(mode, provider);
    default:
        return false;
    }
}

static void restart_remote_icon_loader_if_needed(Entry_List_s * list, RemoteProvider provider)
{
    if (is_remote_provider_v2(provider))
    remote_v2_start_icon_thread(list, false);
}

static void free_icons(Entry_List_s * list)
{
    if (list != NULL)
    {
        C3D_TexDelete(&list->icons_texture);
        free(list->icons_info);
    }
}

static void free_remote_entry_fields(Entry_s * entry)
{
    free(entry->remote_id);
    free(entry->remote_download_url);
    free(entry->remote_preview_url);
    free(entry->remote_icon_url);
    free(entry->remote_audio_url);
    free(entry->remote_filename);

    entry->remote_id = NULL;
    entry->remote_download_url = NULL;
    entry->remote_preview_url = NULL;
    entry->remote_icon_url = NULL;
    entry->remote_audio_url = NULL;
    entry->remote_filename = NULL;
}

void free_remote_entries(Entry_List_s * list)
{
    if (list == NULL || list->entries == NULL)
        return;

    for (int i = 0; i < list->entries_count; ++i)
        free_remote_entry_fields(&list->entries[i]);
}

void ensure_remote_cache_directory(const Entry_s * entry)
{
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_UTF16, entry->path), FS_ATTRIBUTE_DIRECTORY);
}

void set_remote_text_field(u16 * dest, size_t max_chars, const char * value, const char * fallback)
{
    const char * text = value ? value : fallback;
    utf8_to_utf16(dest, (u8 *)text, min((int)strlen(text), (int)max_chars));
}

void copy_linear_rgb565_texture_data(C3D_Tex * texture, const u16 * src, const Entry_Icon_s * icon_info, u32 width, u32 height)
{
    u16 * dest = (u16 *)texture->data;
    for (u32 y = 0; y < height; ++y)
    {
        for (u32 x = 0; x < width; ++x)
        {
            const u32 tex_x = icon_info->x + x;
            const u32 tex_y = icon_info->y + y;
            const u32 dst = (((((tex_y >> 3) * (texture->width >> 3)) + (tex_x >> 3)) << 6)
                + ((tex_x & 1) | ((tex_y & 1) << 1) | ((tex_x & 2) << 1) | ((tex_y & 2) << 2)
                | ((tex_x & 4) << 2) | ((tex_y & 4) << 3)));

            dest[dst] = src[(y * width) + x];
        }
    }

    GSPGPU_InvalidateDataCache(texture->data, texture->size);
}

/* Unnecessary with ThemePlaza providing smdh files for badges
static void load_remote_metadata(Entry_s * entry)
{
    char *page_json = NULL;
    char *api_url = NULL;
    asprintf(&api_url, THEMEPLAZA_QUERY_ENTRY_INFO, entry->tp_download_id);
    u32 json_len;
    Result res = http_get(api_url, NULL, &page_json, &json_len, INSTALL_NONE, "application/json");
    free(api_url);
    if (R_FAILED(res))
    {
        free(page_json);
        return;
    }

    if (json_len)
    {
        json_error_t error;
        json_t *root = json_loadb(page_json, json_len, 0, &error);
        if (root)
        {
            const char *key;
            json_t *value;
            json_object_foreach(root, key, value)
            {
                if (json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_TITLE))
                    utf8_to_utf16(entry->name, (u8 *) json_string_value(value), min(json_string_length(value), 0x41));
                else if (json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_AUTHOR))
                    utf8_to_utf16(entry->author, (u8 *) json_string_value(value), min(json_string_length(value), 0x41));
                else if (json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_DESC))
                    utf8_to_utf16(entry->desc, (u8 *) json_string_value(value), min(json_string_length(value), 0x81));
            }
        }
    }
}
*/ 
static void load_remote_list(Entry_List_s * list, json_int_t page, RemoteMode mode, RemoteProvider provider, bool ignore_cache)
{
    if (is_remote_provider_v2(provider))
        remote_v2_stop_icon_thread();

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
    char * api_url = NULL;
    if (is_remote_provider_v2(provider))
        asprintf(&api_url, THEMEZER_PAGE_FORMAT, remote_v2_get_kind_path(mode), page, list->tp_search);
    else
        asprintf(&api_url, THEMEPLAZA_PAGE_FORMAT, page, mode + 1, list->tp_search);
    u32 json_len;
    Result res = http_get(api_url, NULL, &page_json, &json_len, INSTALL_NONE, "application/json");
    free(api_url);
    if (R_FAILED(res))
    {
        free(page_json);
        return;
    }

    if (json_len)
    {
        if (loading_cancel_requested())
        {
            free_remote_entries(list);
            free(list->entries);
            list->entries = NULL;
            list->entries_count = 0;
            list->entries_loaded = 0;
            free(page_json);
            return;
        }

        list->tp_current_page = page;
        list->mode = (EntryMode) mode;
        list->remote_provider = provider;
        if (is_remote_provider_v2(provider))
            last_page = page;

        json_error_t error;
        json_t * root = json_loadb(page_json, json_len, 0, &error);
        if (root)
        {
            if (provider == REMOTE_PROVIDER_THEMEPLAZA)
                remote_legacy_handle_page_json(list, root, page, ignore_cache, loading_screen);
            else if (is_remote_provider_v2(provider))
                remote_v2_handle_page_json(list, root, page, ignore_cache, loading_screen);
        }
        else
            DEBUG("json error on line %d: %s\n", error.line, error.text);

        if (loading_cancel_requested())
        {
            free_remote_entries(list);
            free(list->entries);
            list->entries = NULL;
            list->entries_count = 0;
            list->entries_loaded = 0;
            json_decref(root);
            free(page_json);
            return;
        }

        json_decref(root);

        if (is_remote_provider_v2(provider))
            remote_v2_start_icon_thread(list, ignore_cache);
    }
    else
        throw_error(language.remote.check_wifi, ERROR_LEVEL_WARNING);

    free(page_json);
}

static u16 previous_path_preview[0x106];

static bool is_remote_v2_entry(const Entry_s * entry)
{
    return entry->remote_id != NULL;
}

static bool load_remote_preview(const Entry_s * entry, C2D_Image * preview_image, int * preview_offset, u32 height)
{
    const bool use_cache = !is_remote_v2_entry(entry);
    bool fetched_preview = false;

    if (use_cache && !memcmp(&previous_path_preview, entry->path, 0x106 * sizeof(u16))) return true;

    char * preview_png = NULL;
    u32 preview_size = use_cache ? load_data("/preview.png", entry, &preview_png) : 0;

    if (!use_cache || !preview_size)
    {
        free(preview_png);
        preview_png = NULL;
        fetched_preview = true;

        char * preview_url = NULL;
        if (entry->remote_preview_url)
            preview_url = strdup(entry->remote_preview_url);
        else
            asprintf(&preview_url, THEMEPLAZA_PREVIEW_FORMAT, entry->tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_PREVIEW);
        Result res = http_get(preview_url, NULL, &preview_png, &preview_size, INSTALL_LOADING_REMOTE_PREVIEW, "image/png");
        free(preview_url);
        if (R_FAILED(res))
            return false;
    }

    if (!preview_size)
    {
        free(preview_png);
        return false;
    }

    char * preview_buf = malloc(preview_size);
    u32 preview_buf_size = preview_size;
    memcpy(preview_buf, preview_png, preview_size);

    if (!(preview_buf_size = png_to_abgr(&preview_buf, preview_buf_size, &height)))
    {
        free(preview_buf);
        return false;
    }

    bool ret = load_preview_from_buffer(preview_buf, preview_buf_size, preview_image, preview_offset, height);
    free(preview_buf);

    if (ret && use_cache && fetched_preview) // only save the preview if it loaded correctly - isn't corrupted
    {
        ensure_remote_cache_directory(entry);

        u16 path[0x107] = { 0 };
        strucat(path, entry->path);
        struacat(path, "/preview.png");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, preview_size);
        buf_to_file(preview_size, fsMakePath(PATH_UTF16, path), ArchiveSD, preview_png);
    }

    free(preview_png);

    return ret;
}

static u16 previous_path_bgm[0x106];

static Result load_remote_bgm(const Entry_s * entry, char ** bgm_ogg, u32 * bgm_size)
{
    const bool use_cache = !is_remote_v2_entry(entry);
    bool fetched_bgm = false;

    *bgm_ogg = NULL;
    *bgm_size = 0;

    if (use_cache && !memcmp(&previous_path_bgm, entry->path, 0x106 * sizeof(u16)))
    {
        *bgm_size = load_data("/bgm.ogg", entry, bgm_ogg);
        if (*bgm_size)
            return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
        free(*bgm_ogg);
        *bgm_ogg = NULL;
    }

    if (use_cache)
        *bgm_size = load_data("/bgm.ogg", entry, bgm_ogg);

    if (!use_cache || !*bgm_size)
    {
        free(*bgm_ogg);
        *bgm_ogg = NULL;
        fetched_bgm = true;

        char * bgm_url = NULL;
        if (entry->remote_audio_url)
            bgm_url = strdup(entry->remote_audio_url);
        else
            asprintf(&bgm_url, THEMEPLAZA_BGM_FORMAT, entry->tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_BGM);

        Result res = http_get_with_not_found_flag(bgm_url, NULL, bgm_ogg, bgm_size, INSTALL_LOADING_REMOTE_BGM, "application/ogg, audio/ogg", false);
        free(bgm_url);
        if (R_FAILED(res))
            return res;
        if (R_SUMMARY(res) == RS_NOTFOUND && R_MODULE(res) == RM_FILE_SERVER)
            return res;

        if (use_cache && fetched_bgm)
        {
            ensure_remote_cache_directory(entry);

            u16 path[0x107] = { 0 };
            strucat(path, entry->path);
            struacat(path, "/bgm.ogg");
            remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, *bgm_size);
            buf_to_file(*bgm_size, fsMakePath(PATH_UTF16, path), ArchiveSD, *bgm_ogg);
        }
    }

    if (use_cache && *bgm_size)
        memcpy(&previous_path_bgm, entry->path, 0x106 * sizeof(u16));

    return *bgm_size ? MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS)
        : MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
}

static void download_remote_entry(Entry_s * entry, RemoteMode mode, RemoteProvider provider)
{
    char * download_url = NULL;
    if (entry->remote_download_url)
        download_url = strdup(entry->remote_download_url);
    else
        asprintf(&download_url, THEMEPLAZA_DOWNLOAD_FORMAT, entry->tp_download_id);

    char * zip_buf = NULL;
    char * filename = NULL;
    draw_install(INSTALL_DOWNLOAD);
    u32 zip_size;
    if(R_FAILED(http_get(download_url, &filename, &zip_buf, &zip_size, INSTALL_DOWNLOAD, "application/zip")))
    {
        free(download_url);
        free(filename);
        return;
    }
    free(download_url);

    if (filename == NULL && entry->remote_filename != NULL)
        filename = strdup(entry->remote_filename);
    if (filename == NULL)
        filename = strdup("download.zip");

    u16 saved_path[0x106] = { 0 };
    save_zip_to_sd(filename, zip_size, zip_buf, mode, provider, saved_path);

    if (saved_path[0] != 0 && (mode == REMOTE_MODE_THEMES || mode == REMOTE_MODE_SPLASHES))
    {
        if (draw_confirm_no_interface("Do you want to install it now?"))
        {
            Entry_s installed_entry = { 0 };
            memcpy(installed_entry.path, saved_path, sizeof(installed_entry.path));
            installed_entry.is_zip = true;

            if (mode == REMOTE_MODE_THEMES)
            {
                aptSetHomeAllowed(false);
                draw_install(INSTALL_SINGLE);
                (void)theme_install(&installed_entry);
            }
            else
            {
                draw_install(INSTALL_SPLASH);
                splash_install(&installed_entry, SPLASH_INSTALL_NORMAL);
            }
        }
    }

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
            load_remote_list(list, newpage, (RemoteMode) list->mode, list->remote_provider, false);
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
        load_remote_list(list, 1, (RemoteMode) list->mode, list->remote_provider, false);
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

    bool preview_mode = false;
    int preview_offset = 0;
    audio_ogg_s * audio = NULL;

    Entry_List_s list = { 0 };
    Entry_List_s * current_list = &list;
    current_list->tp_search = strdup("");
    current_list->remote_provider = provider;
    last_search = strdup("");
    last_page = 1;

    if (is_remote_provider_v2(provider))
    {
        Result init_res = remote_v2_init_session();
        if (R_FAILED(init_res))
            return false;
    }

    list.entries_per_screen_v = entries_per_screen_v[mode];
    list.entries_per_screen_h = entries_per_screen_h[mode];
    list.entry_size = entry_size[mode];
    current_list->entries_loaded = current_list->entries_per_screen_v * current_list->entries_per_screen_h;

    const int x_component = max(current_list->entries_per_screen_h, current_list->entries_per_screen_v);
    const int y_component = min(current_list->entries_per_screen_h, current_list->entries_per_screen_v);
    C3D_TexInit(&current_list->icons_texture,
        next_or_equal_power_of_2(x_component * current_list->entry_size),
        next_or_equal_power_of_2(y_component * current_list->entry_size * ICONS_OFFSET_AMOUNT),
        GPU_RGB565);
    C3D_TexSetFilter(&current_list->icons_texture, GPU_NEAREST, GPU_NEAREST);
    const int entries_icon_count = x_component * y_component * ICONS_OFFSET_AMOUNT;
    current_list->icons_info = calloc(entries_icon_count, sizeof(Entry_Icon_s));

    const float inv_width = 1.0f / current_list->icons_texture.width;
    const float inv_height = 1.0f / current_list->icons_texture.height;
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

    load_remote_list(current_list, 1, mode, provider, false);
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
            if (mode == REMOTE_MODE_BADGES) draw_preview(preview, -40, 0.625f);
            else draw_preview(preview, preview_offset, 1.0f);
            
        }
        else
        {
            Instructions_s instructions = is_remote_provider_v2(provider)
                ? language.remote_v2_instructions[mode]
                : language.remote_instructions[mode];
            if (extra_mode)
                instructions = is_remote_provider_v2(provider)
                    ? language.remote_v2_extra_instructions[mode]
                    : language.remote_extra_instructions[mode];

            if (is_remote_provider_v2(provider))
                remote_v2_lock_texture();
            draw_grid_interface(current_list, instructions, extra_mode);
            if (is_remote_provider_v2(provider))
                remote_v2_unlock_texture();
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
                load_remote_list(current_list, 1, mode, provider, false);
            }
            else if (kDown & KEY_R)
            {
                extra_mode = false;
                mode = mode + 1;
                mode = mode % REMOTE_MODE_AMOUNT;
                free(current_list->tp_search);
                current_list->tp_search = strdup("");
                load_remote_list(current_list, 1, mode, provider, false);
            }
            else if (kDown & KEY_DUP)
            {
                extra_mode = false;
                jump_menu(current_list);
            }
            else if (kDown & KEY_DRIGHT)
            {
                if (is_remote_provider_v2(provider))
                    continue;
                extra_mode = false;
                load_remote_list(current_list, current_list->tp_current_page, mode, provider, true);
            }
            else if (kDown & KEY_DDOWN)
            {
                if (is_remote_provider_v2(provider))
                    continue;
                extra_mode = false;
                search_menu(current_list);
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
                if (is_remote_provider_v2(provider))
                    remote_v2_stop_icon_thread();
                u32 height = mode == REMOTE_MODE_BADGES ? 1024 : 480;
                preview_mode = load_remote_preview(current_entry, &preview, &preview_offset, height);
                if (mode == REMOTE_MODE_THEMES && dspfirm)
                {
                    audio = calloc(1, sizeof(audio_ogg_s));
                    if (is_remote_v2_entry(current_entry))
                    {
                        char * bgm_ogg = NULL;
                        u32 bgm_size = 0;
                        Result bgm_res = load_remote_bgm(current_entry, &bgm_ogg, &bgm_size);
                        if (R_FAILED(bgm_res))
                        {
                            free(bgm_ogg);
                            free(audio);
                            audio = NULL;
                        }
                        else
                        {
                            Result audio_res = load_audio_ogg_buffer(bgm_ogg, bgm_size, audio);
                            bgm_ogg = NULL;
                            if (R_FAILED(audio_res))
                                audio = NULL;
                        }
                    }
                    else
                    {
                        Result bgm_res = load_remote_bgm(current_entry, &(char *){0}, &(u32){0});
                        (void)bgm_res;
                        if (R_FAILED(load_audio_ogg(current_entry, audio))) audio = NULL;
                    }
                    if (audio != NULL) play_audio_ogg(audio);
                }
                restart_remote_icon_loader_if_needed(current_list, provider);
            }
            else
            {
                preview_mode = false;
                if (mode == REMOTE_MODE_THEMES && audio != NULL)
                {
                    stop_audio_ogg(&audio);
                }
            }
            continue;
        }
        else if (kDown & KEY_B)
        {
            if (preview_mode)
            {
                preview_mode = false;
                if (mode == REMOTE_MODE_THEMES && audio != NULL)
                {
                    stop_audio_ogg(&audio);
                }
            }
            else
                break;
        }

        if (preview_mode)
            goto touch;

        if (kDown & KEY_A)
        {
            if (is_remote_provider_v2(provider))
                remote_v2_stop_icon_thread();
            download_remote_entry(current_entry, mode, provider);
            restart_remote_icon_loader_if_needed(current_list, provider);
            downloaded = true;
        }
        else if (kDown & KEY_X)
        {
            extra_mode = true;
        }
        else if (kDown & KEY_L)
        {
            load_remote_list(current_list, current_list->tp_current_page - 1, mode, provider, false);
        }
        else if (kDown & KEY_R)
        {
            load_remote_list(current_list, current_list->tp_current_page + 1, mode, provider, false);
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
                    if (mode == REMOTE_MODE_THEMES && audio)
                    {
                        stop_audio_ogg(&audio);
                    }
                    continue;
                }

                if (y < 24)
                {
                    if (BETWEEN(2, x, 26))
                    {
                        break;
                    }
                    else if (BETWEEN(26, x, 196))
                    {
                        search_menu(current_list);
                    }
                    else if (BETWEEN(TOOLBAR_REMOTE_FILTER_X, x, TOOLBAR_REMOTE_MODE_X))
                    {
                        extra_mode = true;
                    }
                    else if (BETWEEN(320 - 24, x, 320))
                    {
                        mode++;
                        mode %= REMOTE_MODE_AMOUNT;

                        free(current_list->tp_search);
                        current_list->tp_search = strdup("");

                        load_remote_list(current_list, 1, mode, provider, false);
                    }
                }
                else if (BETWEEN(216, y, 240))
                {
                    if (BETWEEN(2, x, 26))
                    {
                        goto toggle_preview;
                    }
                    else if (BETWEEN(26, x, 50))
                    {
                        if (is_remote_provider_v2(provider))
                            remote_v2_stop_icon_thread();
                        download_remote_entry(current_entry, mode, provider);
                        restart_remote_icon_loader_if_needed(current_list, provider);
                        downloaded = true;
                    }
                }
                else if (BETWEEN(240 - 24, y, 240) && BETWEEN(176, x, 320))
                {
                    jump_menu(current_list);
                }
                else
                {
                    if (BETWEEN(0, x, border))
                    {
                        load_remote_list(current_list, current_list->tp_current_page - 1, mode, provider, false);
                    }
                    else if (BETWEEN(320 - border, x, 320))
                    {
                        load_remote_list(current_list, current_list->tp_current_page + 1, mode, provider, false);
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

    if (audio)
    {
        stop_audio_ogg(&audio);
    }

    free_preview(preview);

    if (is_remote_provider_v2(provider))
        remote_v2_cleanup_session();

    free_icons(current_list);
    free_remote_entries(current_list);
    free(current_list->entries);
    free(current_list->tp_search);
    free(last_search);

    return downloaded;
}

typedef struct header
{
    char ** filename; // pointer to location for filename; if NULL, no filename is parsed
    u32 file_size; // if == 0, fall back to chunked read
    Result result_code;
} header;

typedef enum ParseResult
{
    SUCCESS, // 200/203 (203 indicates a successful request with a transformation applied by a proxy)
    REDIRECT, // 301/302/307/308
    HTTPC_ERROR,
    SERVER_IS_MISBEHAVING,
    SEE_OTHER = 303, // Theme Plaza returns these
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_UNACCEPTABLE = 406, // like 204, usually doesn't happen
    HTTP_PROXY_UNAUTHORIZED = 407,
    HTTP_GONE = 410,
    HTTP_URI_TOO_LONG = 414,
    HTTP_IM_A_TEAPOT = 418, // Note that a combined coffee/tea pot that is temporarily out of coffee should instead return 503.
    HTTP_UPGRADE_REQUIRED = 426, // the 3DS doesn't support HTTP/2, so we can't upgrade - inform and return
    HTTP_LEGAL_REASONS = 451,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
} ParseResult;

/*static SwkbdCallbackResult fat32filter(void * user, const char ** ppMessage, const char * text, size_t textlen)
{
    (void)textlen;
    (void)user;
    *ppMessage = "Input must not contain:\n><\"?;:/\\+,.|[=]";
    if(strpbrk(text, "><\"?;:/\\+,.|[=]"))
    {
        DEBUG("illegal filename: %s\n", text);
        return SWKBD_CALLBACK_CONTINUE;
    }

    return SWKBD_CALLBACK_OK;
}*/

static bool mime_type_is_acceptable(const char *acceptable_mime_types, const char *mime_type);

// the good paths for this function return SUCCESS, ABORTED, or REDIRECT;
// all other paths are failures
static ParseResult parse_header(struct header * out, httpcContext * context, const char * mime)
{
    // status code
    u32 status_code;

    out->result_code = httpcGetResponseStatusCode(context, &status_code);
    if (R_FAILED(out->result_code))
    {
        DEBUG("httpcGetResponseStatusCode\n");
        return HTTPC_ERROR;
    }

    DEBUG("HTTP %lu\n", status_code);
    switch (status_code)
    {
    case 301:
    case 302:
    case 307:
    case 308:
        return REDIRECT;
    case 200:
    case 203:
        break;
    default:
        return (ParseResult)status_code;
    }

    char content_buf[1024] = {0};

    // Content-Type

    if (mime)
    {
        out->result_code = httpcGetResponseHeader(context, "Content-Type", content_buf, 1024);
        if (R_FAILED(out->result_code))
        {
            return HTTPC_ERROR;
        }

        if (!mime_type_is_acceptable(mime, content_buf))
        {
            return SERVER_IS_MISBEHAVING;
        }
    }

    // Content-Length

    out->result_code = httpcGetDownloadSizeState(context, NULL, &out->file_size);
    if (R_FAILED(out->result_code))
    {
        DEBUG("httpcGetDownloadSizeState\n");
        return HTTPC_ERROR; // no need to free, program dies anyway
    }

    // Content-Disposition

    if (out->filename)
    {
        bool present = 1;
        out->result_code = httpcGetResponseHeader(context, "Content-Disposition", content_buf, 1024);
        if (R_FAILED(out->result_code))
        {
            if (out->result_code == (long)0xD8A0A028L)
                present = 0;
            else
            {
                DEBUG("httpcGetResponseHeader\n");
                return HTTPC_ERROR;
            }
        }

        // content_buf: Content-Disposition: attachment; ... filename=<filename>;? ...

        if (present)
        {
            char * filename = strstr(content_buf, "filename="); // filename=<filename>;? ...
            // in the extreme fringe case that filename is missing:
            if (filename != NULL)
            {
                filename = strpbrk(filename, "=") + 1; // <filename>;?
                char * end = strpbrk(filename, ";");
                if (end)
                    *end = '\0'; // <filename>

                // safe to assume the filename is quoted
                // (if it isn't, then we already have a null-terminated string <filename>)
                if (filename[0] == '"')
                {
                    filename[strlen(filename) - 1] = '\0';
                    filename++;
                }

                *out->filename = malloc(strlen(filename) + 1);
                strcpy(*out->filename, filename);
            }
            else
            {
                *out->filename = NULL;
            }
        }
        else
        {
            *out->filename = NULL;
        }
    }
    return SUCCESS;
}


/*
 * call example: written = http_get("url", &filename, &buffer_to_download_to, &filesize, INSTALL_DOWNLOAD, "application/json");
 */
Result http_get(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types)
{
    return http_get_with_not_found_flag(url, filename, buf, size, install_type, acceptable_mime_types, true);
}

/* 
 * curl functions modified from Universal-Updater download.cpp
 */
typedef struct {
    void *soc_buffer;
    CURL *handle;
} curl_session;

static curl_session fallback_curl_session = {0};

static bool init_fallback_curl_session(void)
{
    if (fallback_curl_session.handle != NULL)
        return true;

    fallback_curl_session.soc_buffer = memalign(0x1000, 0x100000);
    if (fallback_curl_session.soc_buffer == NULL)
        return false;

    Result ret = socInit((u32 *)fallback_curl_session.soc_buffer, 0x100000);
    if (R_FAILED(ret))
    {
        free(fallback_curl_session.soc_buffer);
        fallback_curl_session.soc_buffer = NULL;
        return false;
    }

    fallback_curl_session.handle = curl_easy_init();
    if (fallback_curl_session.handle == NULL)
    {
        socExit();
        free(fallback_curl_session.soc_buffer);
        fallback_curl_session.soc_buffer = NULL;
        return false;
    }

    return true;
}

static size_t handle_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    curl_data *data = (curl_data *) userdata;
    const size_t bsz = size * nmemb;

    if (data->result_sz == 0 || !(data->result_buf))
    {
        data->result_sz = 0x1000;
        data->result_buf = (char *) malloc(data->result_sz);
    }

    bool need_realloc = false;
    while (data->result_written + bsz > data->result_sz)
    {
        data->result_sz <<= 1;
        need_realloc = true;
    }

    if (need_realloc)
    {
        char *new_buf = (char *)realloc(data->result_buf, data->result_sz);
        if (!new_buf) return 0;

        data->result_buf = new_buf;
    }

    memcpy(data->result_buf + data->result_written, ptr, bsz);
    data->result_written += bsz;
    return bsz;
}

static size_t curl_parse_header(char *buffer, size_t size, size_t nitems, void *userdata)
{
    curl_header *header = (curl_header *) userdata;
    for (size_t i = 0; i < size * nitems; ++i)
    {
        if (buffer[i] == '\n' || buffer[i] == '\r')
        {
            buffer[i] = '\0';
            break;
        }
    }

    if (!strncasecmp(buffer, "Content-Type: ", 14))
    {
        header->mime_type = malloc(strlen(buffer) - 13);
        strncpy(header->mime_type, buffer + 14, strlen(buffer) - 14);
        header->mime_type[strlen(buffer) - 14] = '\0';
    } else if (!strncasecmp(buffer, "Content-Disposition: ", 21))
    {
        header->filename = malloc(strlen(buffer) - 20);
        memcpy(header->filename, buffer + 21, strlen(buffer) - 21);
        header->filename[strlen(buffer) - 21] = '\0';
    } 

    return nitems * size;
}

static bool mime_type_is_acceptable(const char *acceptable_mime_types, const char *mime_type)
{
    if (!acceptable_mime_types || !mime_type)
        return true;

    char mime_base[128] = {0};
    size_t mime_len = strcspn(mime_type, "; \t\r\n");
    if (mime_len >= sizeof(mime_base))
        mime_len = sizeof(mime_base) - 1;
    memcpy(mime_base, mime_type, mime_len);
    mime_base[mime_len] = '\0';

    const char *cursor = acceptable_mime_types;
    while (*cursor)
    {
        while (*cursor == ' ' || *cursor == ',' || *cursor == ';')
            ++cursor;

        const char *token_start = cursor;
        while (*cursor && *cursor != ' ' && *cursor != ',' && *cursor != ';')
            ++cursor;

        size_t token_len = cursor - token_start;
        if (token_len == 0)
            continue;

        if (token_len == mime_len)
        {
            bool match = true;
            for (size_t i = 0; i < mime_len; ++i)
            {
                if (tolower((unsigned char)token_start[i]) != tolower((unsigned char)mime_base[i]))
                {
                    match = false;
                    break;
                }
            }

            if (match)
                return true;
        }
    }

    return false;
}

Result curl_http_get(const char * url, char ** out_filename, char ** buf, u32 * size, const char * acceptable_mime_types, InstallType install_type)
{
    DEBUG("attempting curl_http_get\n");
    curl_data data = {0};
    curl_header header = {0};
    if (!init_fallback_curl_session())
    {
        return -1;
    }

    CURL *handle = fallback_curl_session.handle;
    curl_easy_reset(handle);

    curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &install_type);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, curl_progress_cb);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(handle, CURLOPT_FRESH_CONNECT, 0L);
    curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 0L);
    curl_easy_setopt(handle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, handle_data);
    // curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(handle, CURLOPT_STDERR, stderr);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, curl_parse_header);

    struct curl_slist *list = NULL;
    char mime_string[512] = {0};
    sprintf(mime_string, "Accept:%s", acceptable_mime_types);
    list = curl_slist_append(list, mime_string);

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);

    CURLcode cres = curl_easy_perform(handle);
    curl_easy_reset(handle);
    char *newbuf = (char *) realloc(data.result_buf, data.result_written + 1);
    data.result_buf = newbuf;
    data.result_buf[data.result_written] = 0;
    if (cres != CURLE_OK)
    {
        const bool was_canceled = cres == CURLE_ABORTED_BY_CALLBACK || loading_cancel_requested();
        free(data.result_buf);
        if (header.mime_type) free(header.mime_type);
        if (header.filename) free(header.filename);
        if (list) curl_slist_free_all(list);
        if (was_canceled)
            return MAKERESULT(RL_TEMPORARY, RS_CANCELED, RM_APPLICATION, RD_CANCEL_REQUESTED);
        return -1;
    }
    
    DEBUG("Mime Type: %s\n", header.mime_type);
    DEBUG("Acceptable Mime Types: %s\n", acceptable_mime_types);
    if (header.mime_type)
    {
        if (!mime_type_is_acceptable(acceptable_mime_types, header.mime_type))
        {
            free(data.result_buf);
            if (header.mime_type) free(header.mime_type);
            if (header.filename) free(header.filename);
            if (list) curl_slist_free_all(list);
            return -2;
        }
    } 

    DEBUG("Content-Disposition: %s\n", header.filename);
    if (out_filename)
    {
        if (header.filename)
        {
            char *filename = strstr(header.filename, "filename=");
            if (filename)
            {
                filename = strpbrk(filename, "=") + 1;
                char *end = strpbrk(filename, ";");
                if (end)
                    *end = '\0';

                if (filename[0] == '"')
                {
                    filename[strlen(filename) - 1] = '\0';
                    filename++;
                }

                *out_filename = malloc(0x100);
                strcpy(*out_filename, filename);
            } else {
                *out_filename = NULL;
            }
        } else {
            *out_filename = NULL;
        }
    }

    *buf = data.result_buf;
    *size = data.result_written;

    if (header.mime_type) free(header.mime_type);
    if (header.filename) free(header.filename);
    if (list) curl_slist_free_all(list);

    return 0;
}

static Result http_get_with_not_found_flag(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types, bool not_found_is_error)
{
    const char *zip_not_available = language.remote.zip_not_found;
    Result ret;
    httpcContext context;
    char redirect_url[0x824] = {0};
    char new_url[0x824] = {0};

    struct header _header = { .filename = filename };

    DEBUG("Original URL: %s\n", url);

redirect: // goto here if we need to redirect
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(ret))
    {
        httpcCloseContext(&context);
        DEBUG("httpcOpenContext %.8lx\n", ret);
        return ret;
    }

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    httpcAddRequestHeaderField(&context, "User-Agent", USER_AGENT);
    httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
    if (acceptable_mime_types)
        httpcAddRequestHeaderField(&context, "Accept", acceptable_mime_types);

    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret))
    {
        httpcCloseContext(&context);
        DEBUG("httpcBeginRequest %.8lx\n", ret);
        return ret;
    }

#define ERROR_BUFFER_SIZE 0x80
    char err_buf[ERROR_BUFFER_SIZE];
    Result res;
    ParseResult parse = parse_header(&_header, &context, acceptable_mime_types);
    switch (parse)
    {
    case SUCCESS:
        break;
    case HTTPC_ERROR:
        DEBUG("httpc error %lx\n", _header.result_code);
        switch (_header.result_code)
        {
        case 0xd8a0a028: // bad zip file
        case 0xd8a0a03c: // SSL failure
            // try curl?
            res = curl_http_get(url, filename, buf, size, acceptable_mime_types, install_type);
            if (R_SUCCEEDED(res))
            {
                httpcCloseContext(&context);
                return res;
            }

            if (R_SUMMARY(res) == RS_CANCELED)
            {
                httpcCancelConnection(&context);
                httpcCloseContext(&context);
                return res;
            }

            if(_header.result_code == 0xd8a0a028)
            {
                snprintf(err_buf, ERROR_BUFFER_SIZE, zip_not_available);
            }
            else
            {
                snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_ssl_error, _header.result_code);
            }
            quit = false;
            break;
        case 0xd8a0a049:
            // Timeout (bad wifi/proxy)
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_timeout);
            quit = false;
            break;
        case 0xd8a0a046:
            // poor reception/no wifi/custom dns set
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_no_network);
            quit = false;
            break;
        default:
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.generic_httpc_error, _header.result_code);
            quit = true;
            break;
        }
        throw_error(err_buf, ERROR_LEVEL_ERROR);
        httpcCloseContext(&context);
        return _header.result_code;
    case SEE_OTHER:
        if (strstr(url, THEMEPLAZA_BASE_URL))
        {
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http303_tp);
            goto error;
        }
        else
        {
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http303);
            goto error;
        }
    case REDIRECT:
        httpcGetResponseHeader(&context, "Location", redirect_url, 0x824);
        httpcCloseContext(&context);
        if (*redirect_url == '/') // if relative URL
        {
            strcpy(new_url, url);
            char * last_slash = strchr(strchr(strchr(new_url, '/') + 1, '/') + 1, '/');
            if (last_slash) *last_slash = '\0'; // prevents a NULL deref in case the original domain was not /-delimited
            strncat(new_url, redirect_url, 0x824 - strlen(new_url));
            url = new_url;
        }
        else
        {
            url = redirect_url;
        }
        DEBUG("HTTP Redirect: %s %s\n", new_url, *redirect_url == '/' ? "relative" : "absolute");
        goto redirect;
    case SERVER_IS_MISBEHAVING:
        DEBUG("Server is misbehaving (provided resource with incorrect MIME)\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, zip_not_available);
        goto error;
    case HTTP_NOT_FOUND:
        if (!not_found_is_error)
            goto not_found_non_error;
        [[fallthrough]];
    case HTTP_GONE:
        const char * http_error = parse == HTTP_NOT_FOUND ? "404 Not Found" : "410 Gone";
        DEBUG("HTTP %s; URL: %s\n", http_error, url);
        if (strstr(url, THEMEPLAZA_BASE_URL) && parse == HTTP_NOT_FOUND)
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http404);
        else
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_err_url, http_error);
        goto error;
    case HTTP_UNACCEPTABLE:
        DEBUG("HTTP 406 Unacceptable; Accept: %s\n", acceptable_mime_types);
        snprintf(err_buf, ERROR_BUFFER_SIZE, zip_not_available);
        goto error;
    case HTTP_UNAUTHORIZED:
    case HTTP_FORBIDDEN:
    case HTTP_PROXY_UNAUTHORIZED:
        DEBUG("HTTP %u: device not authenticated\n", parse);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_errcode_generic, parse == HTTP_UNAUTHORIZED
            ? language.remote.http401
            : parse == HTTP_FORBIDDEN
            ? language.remote.http403
            : language.remote.http407);
        goto error;
    case HTTP_URI_TOO_LONG:
        DEBUG("HTTP 414; URL is too long, maybe too many redirects?\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http414);
        goto error;
    case HTTP_IM_A_TEAPOT:
        DEBUG("HTTP 418 I'm a teapot\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http418);
        goto error;
    case HTTP_UPGRADE_REQUIRED:
        DEBUG("HTTP 426; HTTP/2 required\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http426);
        goto error;
    case HTTP_LEGAL_REASONS:
        DEBUG("HTTP 451; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http451);
        goto error;
    case HTTP_INTERNAL_SERVER_ERROR:
        DEBUG("HTTP 500; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http500);
        goto error;
    case HTTP_BAD_GATEWAY:
        DEBUG("HTTP 502; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http502);
        goto error;
    case HTTP_SERVICE_UNAVAILABLE:
        DEBUG("HTTP 503; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http503);
        goto error;
    case HTTP_GATEWAY_TIMEOUT:
        DEBUG("HTTP 504; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http504);
        goto error;
    default:
        DEBUG("HTTP %u; URL: %s\n", parse, url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_unexpected, parse);
        goto error;
    }

    goto no_error;
error:
    throw_error(err_buf, ERROR_LEVEL_WARNING);
    res = httpcCloseContext(&context);
    if (R_FAILED(res)) return res;
    return MAKERESULT(RL_TEMPORARY, RS_CANCELED, RM_APPLICATION, RD_NO_DATA);
not_found_non_error:
    res = httpcCloseContext(&context);
    if (R_FAILED(res)) return res;
    return MAKERESULT(RL_SUCCESS, RS_NOTFOUND, RM_FILE_SERVER, RD_NO_DATA);
no_error:;
    u32 chunk_size;
    if (_header.file_size)
        // the only reason we chunk this at all is for the download bar;
        // in terms of efficiency, allocating the full size
        // would avoid 3 reallocs whenever the server isn't lying
        chunk_size = _header.file_size / 4;
    else
        chunk_size = 0x80000;

    *buf = NULL;
    char * new_buf;
    *size = 0;
    u32 read_size = 0;

    do
    {
        hidScanInput();
        if (hidKeysDown() & KEY_B)
        {
            httpcCancelConnection(&context);
            httpcCloseContext(&context);
            free(*buf);
            *buf = NULL;
            *size = 0;
            return MAKERESULT(RL_TEMPORARY, RS_CANCELED, RM_APPLICATION, RD_CANCEL_REQUESTED);
        }

        new_buf = realloc(*buf, *size + chunk_size);
        if (new_buf == NULL)
        {
            httpcCloseContext(&context);
            free(*buf);
            DEBUG("realloc failed in http_get - file possibly too large?\n");
            return MAKERESULT(RL_FATAL, RS_INTERNAL, RM_KERNEL, RD_OUT_OF_MEMORY);
        }
        *buf = new_buf;

        // download exactly chunk_size bytes and toss them into buf.
        // size contains the current offset into buf.
        ret = httpcDownloadData(&context, (u8 *)(*buf) + *size, chunk_size, &read_size);
        /* FIXME: I have no idea why this doesn't work, but it causes problems. Look into it later
        if (R_FAILED(ret))
        {
            httpcCloseContext(&context);
            free(*buf);
            DEBUG("download failed in http_get\n");
            return ret;
        }
        */
        *size += read_size;

        if (_header.file_size && install_type != INSTALL_NONE)
            draw_loading_bar(*size, _header.file_size, install_type);
    } while (ret == (Result)HTTPC_RESULTCODE_DOWNLOADPENDING);
    httpcCloseContext(&context);

    // shrink to size
    new_buf = realloc(*buf, *size);
    if (new_buf == NULL)
    {
        httpcCloseContext(&context);
        free(*buf);
        DEBUG("shrinking realloc failed\n"); // 何？
        return MAKERESULT(RL_FATAL, RS_INTERNAL, RM_KERNEL, RD_OUT_OF_MEMORY);
    }
    *buf = new_buf;

    DEBUG("size: %lu\n", *size);
    if (filename) { DEBUG("filename: %s\n", *filename); }
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
 }
