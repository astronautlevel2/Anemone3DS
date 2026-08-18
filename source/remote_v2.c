/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*/

#include <ctype.h>
#include <malloc.h>
#include <stdarg.h>

#include <curl/curl.h>

#include "remote_internal.h"
#include "unicode.h"
#include "conversion.h"

extern int __stacksize__;

static inline u64 remote_v2_now_ms(void)
{
    return svcGetSystemTick() / CPU_TICKS_PER_MSEC;
}

static void remote_v2_debug_elapsed(u64 start_ms, const char * fmt, ...)
{
    va_list args;
    DEBUG("remote_v2[+%llums]: ", (unsigned long long)(remote_v2_now_ms() - start_ms));
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

const char * remote_v2_kind_path[REMOTE_MODE_AMOUNT] = {
    "themes",
    "splashes",
    "badges",
};

typedef struct {
    Entry_List_s * list;
    Handle texture_mutex;
} RemoteV2_IconLoader_s;

static Thread remote_v2_icon_thread = {0};
static Thread_Arg_s remote_v2_icon_thread_arg = {0};
static RemoteV2_IconLoader_s remote_v2_icon_loader = {0};
static void * remote_v2_icon_thread_args[1] = {0};
static Handle remote_v2_icon_texture_mutex = 0;

typedef struct {
    void *soc_buffer;
    CURL *curl;
    struct curl_slist *headers;
} RemoteV2_IconHttpSession_s;

static bool remote_v2_icon_http_session_init(RemoteV2_IconHttpSession_s * session)
{
    (void)session;
    return true;
}

static void remote_v2_icon_http_session_cleanup(RemoteV2_IconHttpSession_s * session)
{
    (void)session;
}

static bool remote_v2_fetch_icon_png(RemoteV2_IconHttpSession_s * session, const char * url, char ** icon_png, u32 * icon_size, u64 start_ms)
{
    (void)session;

    remote_v2_debug_elapsed(start_ms, "icon fetch start %s\n", url);
    Result res = curl_http_get(url, NULL, icon_png, icon_size, "image/png", INSTALL_NONE);
    if (R_FAILED(res))
    {
        remote_v2_debug_elapsed(start_ms, "icon fetch failed %s (res=%08lx)\n", url, res);
        return false;
    }

    remote_v2_debug_elapsed(start_ms, "icon fetch end %s (%lu bytes)\n", url, *icon_size);
    return true;
}

static void copy_linear_rgb565_texture_data(C3D_Tex * texture, const u16 * src, const Entry_Icon_s * icon_info, u32 width, u32 height)
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

static bool load_remote_v2_icon(RemoteV2_IconHttpSession_s * session, Entry_s * entry, C3D_Tex * into_tex, const Entry_Icon_s * icon_info, Handle texture_mutex, u64 start_ms)
{
    char * icon_png = NULL;
    u32 icon_size = 0;

    if (entry->remote_icon_url == NULL)
        return false;

    remote_v2_debug_elapsed(start_ms, "load icon entry %s\n", entry->remote_id ? entry->remote_id : "(local)");

    if (!remote_v2_fetch_icon_png(session, entry->remote_icon_url, &icon_png, &icon_size, start_ms))
    {
        free(icon_png);
        return false;
    }

    if (!icon_size)
    {
        free(icon_png);
        return false;
    }

    char * icon_buf = malloc(icon_size);
    if (icon_buf == NULL)
    {
        free(icon_png);
        return false;
    }
    memcpy(icon_buf, icon_png, icon_size);

    u32 height = 48;
    if (!(icon_size = png_to_abgr(&icon_buf, icon_size, &height)))
    {
        free(icon_buf);
        free(icon_png);
        return false;
    }

    const u32 width = (icon_size / 4) / height;
    if (width != 48 || height != 48)
    {
        free(icon_buf);
        free(icon_png);
        return false;
    }

    u16 * icon_rgb565 = malloc(48 * 48 * sizeof(u16));
    if (icon_rgb565 == NULL)
    {
        free(icon_buf);
        free(icon_png);
        return false;
    }

    for (u32 i = 0; i < 48 * 48; ++i)
    {
        u8 * px = (u8 *)icon_buf + (i * 4);
        const u8 r = px[3];
        const u8 g = px[2];
        const u8 b = px[1];
        icon_rgb565[i] = (u16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    svcWaitSynchronization(texture_mutex, U64_MAX);
    copy_linear_rgb565_texture_data(into_tex, icon_rgb565, icon_info, width, height);
    svcReleaseMutex(texture_mutex);
    entry->placeholder_color = 0;

    remote_v2_debug_elapsed(start_ms, "icon uploaded %s\n", entry->remote_id ? entry->remote_id : "(local)");

    free(icon_rgb565);
    free(icon_buf);
    free(icon_png);
    return true;
}

static void load_remote_v2_icons_thread(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    RemoteV2_IconLoader_s * loader = (RemoteV2_IconLoader_s *)arg->thread_arg[0];
    RemoteV2_IconHttpSession_s session = {0};
    const u64 start_ms = remote_v2_now_ms();

    remote_v2_debug_elapsed(start_ms, "icon thread start\n");

    if (loader == NULL || loader->list == NULL || loader->list->entries == NULL)
    {
        arg->run_thread = false;
        return;
    }

    if (!remote_v2_icon_http_session_init(&session))
    {
        remote_v2_debug_elapsed(start_ms, "icon HTTP session init failed\n");
        arg->run_thread = false;
        return;
    }

    remote_v2_debug_elapsed(start_ms, "icon HTTP session ready\n");

    Entry_List_s * list = loader->list;
    const int icon_slot_count = list->entries_per_screen_v * list->entries_per_screen_h * ICONS_OFFSET_AMOUNT;
    const int load_count = min(list->entries_count, icon_slot_count);

    remote_v2_debug_elapsed(start_ms, "loading %d icons (slot cap %d)\n", load_count, icon_slot_count);

    for (int i = 0; arg->run_thread && i < load_count; ++i)
        load_remote_v2_icon(&session, &list->entries[i], &list->icons_texture, &list->icons_info[i], loader->texture_mutex, start_ms);

    remote_v2_icon_http_session_cleanup(&session);
    remote_v2_debug_elapsed(start_ms, "icon thread end\n");
    arg->run_thread = false;
}

static void reset_remote_v2_icon_state(void)
{
    remote_v2_icon_thread = NULL;
    remote_v2_icon_thread_arg.thread_arg = NULL;
    remote_v2_icon_thread_arg.run_thread = false;
    remote_v2_icon_loader.list = NULL;
    remote_v2_icon_loader.texture_mutex = 0;
    remote_v2_icon_thread_args[0] = NULL;
}

Result remote_v2_init_session(void)
{
    remote_v2_cleanup_session();
    return svcCreateMutex(&remote_v2_icon_texture_mutex, false);
}

void remote_v2_cleanup_session(void)
{
    remote_v2_stop_icon_thread();
    if (remote_v2_icon_texture_mutex != 0)
    {
        svcCloseHandle(remote_v2_icon_texture_mutex);
        remote_v2_icon_texture_mutex = 0;
    }
}

void remote_v2_lock_texture(void)
{
    if (remote_v2_icon_texture_mutex != 0)
        svcWaitSynchronization(remote_v2_icon_texture_mutex, U64_MAX);
}

void remote_v2_unlock_texture(void)
{
    if (remote_v2_icon_texture_mutex != 0)
        svcReleaseMutex(remote_v2_icon_texture_mutex);
}

void remote_v2_stop_icon_thread(void)
{
    if (remote_v2_icon_thread_arg.run_thread)
        remote_v2_icon_thread_arg.run_thread = false;

    if (remote_v2_icon_thread != NULL)
    {
        threadJoin(remote_v2_icon_thread, U64_MAX);
        threadFree(remote_v2_icon_thread);
    }

    reset_remote_v2_icon_state();
}

void remote_v2_start_icon_thread(Entry_List_s * list)
{
    remote_v2_stop_icon_thread();

    if (list == NULL || list->entries == NULL || list->entries_count <= 0 || remote_v2_icon_texture_mutex == 0)
        return;

    remote_v2_icon_loader.list = list;
    remote_v2_icon_loader.texture_mutex = remote_v2_icon_texture_mutex;

    remote_v2_icon_thread_args[0] = &remote_v2_icon_loader;
    remote_v2_icon_thread_arg.thread_arg = remote_v2_icon_thread_args;
    remote_v2_icon_thread_arg.run_thread = true;

    DEBUG("remote_v2: starting icon thread for %d entries\n", list->entries_count);

    remote_v2_icon_thread = threadCreate(load_remote_v2_icons_thread, &remote_v2_icon_thread_arg, __stacksize__, 0x38, -2, false);
    if (remote_v2_icon_thread == NULL)
    {
        DEBUG("remote_v2: icon thread create failed\n");
        remote_v2_icon_thread_arg.run_thread = false;
    }
}

const char * remote_v2_get_kind_path(RemoteMode mode)
{
    return remote_v2_kind_path[mode];
}

void remote_v2_load_entries(Entry_List_s * list, json_t * items_array, InstallType type)
{
    if (loading_cancel_requested())
        return;

    free_remote_entries(list);
    free(list->entries);
    list->entries_count = json_array_size(items_array);
    list->entries = calloc(list->entries_count, sizeof(Entry_s));
    list->entries_loaded = list->entries_per_screen_v * list->entries_per_screen_h;

    size_t i = 0;
    json_t * item = NULL;
    json_array_foreach(items_array, i, item)
    {
        if (loading_cancel_requested())
            return;

        draw_loading_bar(i, list->entries_count, type);

        if (!json_is_object(item))
            continue;

        Entry_s * current_entry = &list->entries[i];
        const char * remote_id = json_string_value(json_object_get(item, "id"));
        const char * name = json_string_value(json_object_get(item, "name"));
        const char * author = json_string_value(json_object_get(item, "author"));
        const char * description = json_string_value(json_object_get(item, "description"));
        const char * icon_url = json_string_value(json_object_get(item, "iconUrl"));
        const char * preview_url = json_string_value(json_object_get(item, "previewUrl"));
        const char * download_url = json_string_value(json_object_get(item, "downloadUrl"));
        const char * audio_url = json_string_value(json_object_get(item, "audioUrl"));
        const char * filename = json_string_value(json_object_get(item, "filename"));

        current_entry->remote_id = remote_id ? strdup(remote_id) : NULL;
        current_entry->remote_icon_url = icon_url ? strdup(icon_url) : NULL;
        current_entry->remote_preview_url = preview_url ? strdup(preview_url) : NULL;
        current_entry->remote_download_url = download_url ? strdup(download_url) : NULL;
        current_entry->remote_audio_url = audio_url ? strdup(audio_url) : NULL;
        current_entry->remote_filename = filename ? strdup(filename) : NULL;

        if (current_entry->remote_id)
        {
            utf8_to_utf16(current_entry->path, (u8 *)current_entry->remote_id, 0x106);
        }

        set_remote_text_field(current_entry->name, 0x40, name, "No name");
        set_remote_text_field(current_entry->author, 0x40, author, "Unknown author");
        set_remote_text_field(current_entry->desc, 0x80, description, "No description");
        current_entry->placeholder_color = C2D_Color32(rand() % 255, rand() % 255, rand() % 255, 255);
    }
}

void remote_v2_handle_page_json(Entry_List_s * list, json_t * root, json_int_t page, InstallType loading_screen)
{
    const char * key;
    json_t * value;

    last_page = page;

    json_object_foreach(root, key, value)
    {
        if (loading_cancel_requested())
            return;

        if (json_is_integer(value) && !strcmp(key, "pages"))
            list->tp_page_count = json_integer_value(value);
        else if (json_is_array(value) && !strcmp(key, "items"))
        {
            if (json_array_size(value) == 0)
            {
                throw_error(language.remote.no_results, ERROR_LEVEL_WARNING);
                if (list->tp_search) free(list->tp_search);
                asprintf(&list->tp_search, "%s", last_search);
                list->tp_current_page = last_page;
            }
            else
            {
                remote_v2_load_entries(list, value, loading_screen);
            }
        }
    }
}
