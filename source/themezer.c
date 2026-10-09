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

#include "themezer.h"
#include "http.h"
#include "conversion.h"
#include "loading.h"

extern int __stacksize__;

static const char * themezer_kind[REMOTE_MODE_AMOUNT] = {
    "themes",
    "splashes",
    "badges",
};

char * themezer_page_url(RemoteMode mode, json_int_t page, const char * escaped_search)
{
    char * url = NULL;
    asprintf(&url, THEMEZER_BASE_URL "/%s?page=%" JSON_INTEGER_FORMAT "&q=%s", themezer_kind[mode], page, escaped_search ? escaped_search : "");
    return url;
}

static char * json_strdup(json_t * item, const char * key)
{
    const char * value = json_string_value(json_object_get(item, key));
    return value ? strdup(value) : NULL;
}

static void set_text(u16 * dest, size_t max_chars, json_t * item, const char * key, const char * fallback)
{
    const char * text = json_string_value(json_object_get(item, key));
    if (text == NULL || text[0] == '\0')
        text = fallback;
    utf8_to_utf16(dest, (const u8 *)text, min(strlen(text), max_chars));
}

/*
 * { "pages": 3, "items": [ { "id": "...", "name": "...", "author": "...", "description": "...",
 *   "iconUrl": "...", "previewUrl": "...", "downloadUrl": "...", "audioUrl": "...", "filename": "..." } ] }
 */
bool themezer_load_page(Entry_List_s * list, json_t * root)
{
    json_t * items = json_object_get(root, "items");
    if (!json_is_array(items) || json_array_size(items) == 0)
        return false;

    const size_t item_count = json_array_size(items);
    Entry_s * entries = calloc(item_count, sizeof(Entry_s));
    if (entries == NULL)
        return false;

    int count = 0;
    size_t i = 0;
    json_t * item = NULL;
    json_array_foreach(items, i, item)
    {
        if (!json_is_object(item))
            continue;

        char id_buf[0x20] = {0};
        json_t * id = json_object_get(item, "id");
        const char * id_str = json_string_value(id);
        if (id_str == NULL && json_is_integer(id))
        {
            snprintf(id_buf, sizeof(id_buf), "%" JSON_INTEGER_FORMAT, json_integer_value(id));
            id_str = id_buf;
        }

        const char * download_url = json_string_value(json_object_get(item, "downloadUrl"));
        if (id_str == NULL || download_url == NULL)
            continue;

        Entry_s * entry = &entries[count++];
        // only identifies the entry (e.g. to reuse a loaded preview), Themezer entries aren't cached on the SD card
        utf8_to_utf16(entry->path, (const u8 *)id_str, 0x105);

        set_text(entry->name, 0x40, item, "name", "No name");
        set_text(entry->author, 0x40, item, "author", "Unknown author");
        set_text(entry->desc, 0x80, item, "description", "No description");

        entry->remote_download_url = strdup(download_url);
        entry->remote_preview_url = json_strdup(item, "previewUrl");
        entry->remote_icon_url = json_strdup(item, "iconUrl");
        entry->remote_audio_url = json_strdup(item, "audioUrl");
        entry->remote_filename = json_strdup(item, "filename");

        // shown until the icon is downloaded
        entry->placeholder_color = C2D_Color32(rand() % 255, rand() % 255, rand() % 255, 255);
    }

    if (count == 0)
    {
        free(entries);
        return false;
    }

    free_remote_entries(list);
    list->entries = entries;
    list->entries_count = count;
    list->entries_loaded = count;

    json_t * pages = json_object_get(root, "pages");
    list->tp_page_count = json_is_integer(pages) && json_integer_value(pages) > 0 ? json_integer_value(pages) : 1;
    return true;
}

static Thread icon_thread = NULL;
static volatile bool icons_running = false;
static Handle icons_mutex = 0;
static Entry_List_s * icons_list = NULL;
static CURL * icons_curl = NULL;

Result themezer_icons_init(void)
{
    if (icons_mutex != 0)
        return 0;
    return svcCreateMutex(&icons_mutex, false);
}

void themezer_icons_exit(void)
{
    themezer_stop_icons();
    http_background_handle_free(icons_curl);
    icons_curl = NULL;
    if (icons_mutex != 0)
    {
        svcCloseHandle(icons_mutex);
        icons_mutex = 0;
    }
}

void themezer_lock_icons(void)
{
    if (icons_mutex != 0)
        svcWaitSynchronization(icons_mutex, U64_MAX);
}

void themezer_unlock_icons(void)
{
    if (icons_mutex != 0)
        svcReleaseMutex(icons_mutex);
}

static bool is_png(const char * buf, u32 size)
{
    return size >= 8 && !memcmp(buf, "\x89PNG\r\n\x1a\n", 8);
}

// 48x48 ABGR pixels to the tiled RGB565 layout of SMDH icons, which copy_texture_data expects
static void abgr_to_icon(const u8 * abgr, u16 * icon)
{
    for (u32 y = 0; y < 48; ++y)
    {
        for (u32 x = 0; x < 48; ++x)
        {
            const u8 * px = abgr + ((y * 48 + x) * 4);
            const u16 rgb565 = ((px[3] & 0xF8) << 8) | ((px[2] & 0xFC) << 3) | (px[1] >> 3);
            const u32 tile = (y / 8) * 6 + (x / 8);
            const u32 morton = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
            icon[tile * 64 + morton] = rgb565;
        }
    }
}

static void load_icon(Entry_List_s * list, int index)
{
    Entry_s * entry = &list->entries[index];
    if (entry->remote_icon_url == NULL || entry->placeholder_color == 0)
        return;

    char * buf = NULL;
    u32 size = 0;
    Result res = http_get_background(icons_curl, entry->remote_icon_url, &buf, &size, "image/png", &icons_running);
    // png_to_abgr shows an error dialog for bad data, which can't be drawn from this thread
    if (R_FAILED(res) || !is_png(buf, size))
    {
        free(buf);
        return;
    }

    u32 height = 0;
    const size_t abgr_size = png_to_abgr(&buf, size, &height);
    if (abgr_size != 48 * 48 * 4 || height != 48)
    {
        free(buf);
        return;
    }

    u16 icon[48 * 48];
    abgr_to_icon((const u8 *)buf, icon);
    free(buf);

    themezer_lock_icons();
    copy_texture_data(&list->icons_texture, icon, &list->icons_info[index]);
    entry->placeholder_color = 0;
    themezer_unlock_icons();
}

static void icons_thread(void * arg)
{
    (void)arg;
    Entry_List_s * list = icons_list;
    // the remote browser has one icon slot per visible entry
    const int icon_slots = list->entries_per_screen_v * list->entries_per_screen_h;
    const int count = min(list->entries_count, icon_slots);

    for (int i = 0; i < count && icons_running; ++i)
        load_icon(list, i);

    icons_running = false;
}

void themezer_start_icons(Entry_List_s * list)
{
    themezer_stop_icons();
    if (list == NULL || list->entries == NULL || list->entries_count <= 0 || icons_mutex == 0)
        return;

    if (icons_curl == NULL)
        icons_curl = http_background_handle();
    if (icons_curl == NULL)
        return;

    icons_list = list;
    icons_running = true;
    icon_thread = threadCreate(icons_thread, NULL, __stacksize__, 0x38, -2, false);
    if (icon_thread == NULL)
        icons_running = false;
}

void themezer_stop_icons(void)
{
    // also aborts the icon download in progress
    icons_running = false;
    if (icon_thread != NULL)
    {
        threadJoin(icon_thread, U64_MAX);
        threadFree(icon_thread);
        icon_thread = NULL;
    }
    icons_list = NULL;
}
