#include <ctype.h>

#include "remote_internal.h"
#include "unicode.h"
#include "conversion.h"

static void load_remote_smdh(Entry_s * entry, C3D_Tex * into_tex, const Entry_Icon_s * icon_info, bool ignore_cache)
{
    bool not_cached = true;
    char * smdh_buf = NULL;
    u32 smdh_size = load_data("/info.smdh", entry, &smdh_buf);

    not_cached = (smdh_size != sizeof(Icon_s)) || ignore_cache;

    if (not_cached)
    {
        free(smdh_buf);
        smdh_buf = NULL;
        char * api_url = NULL;
        asprintf(&api_url, THEMEPLAZA_SMDH_FORMAT, entry->tp_download_id);
        Result res = http_get(api_url, NULL, &smdh_buf, &smdh_size, INSTALL_NONE, "application/octet-stream");
        free(api_url);
        if (R_FAILED(res))
        {
            free(smdh_buf);
            return;
        }
    }

    if (smdh_size != sizeof(Icon_s))
    {
        free(smdh_buf);
        smdh_buf = NULL;
    }

    Icon_s * smdh = (Icon_s *)smdh_buf;

    u16 fallback_name[0x81] = { 0 };
    utf8_to_utf16(fallback_name, (u8 *)"No name", 0x80);

    parse_smdh(smdh, entry, fallback_name);

    if (smdh_buf != NULL)
    {
        copy_texture_data(into_tex, smdh->big_icon, icon_info);
        if (not_cached)
        {
            ensure_remote_cache_directory(entry);
            u16 path[0x107] = { 0 };
            strucat(path, entry->path);
            struacat(path, "/info.smdh");
            remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, smdh_size);
            buf_to_file(smdh_size, fsMakePath(PATH_UTF16, path), ArchiveSD, smdh_buf);
        }
        free(smdh_buf);
    }
}

void remote_legacy_load_entries(Entry_List_s * list, json_t * ids_array, bool ignore_cache, InstallType type)
{
    if (loading_cancel_requested())
        return;

    free_remote_entries(list);
    free(list->entries);
    list->entries_count = json_array_size(ids_array);
    list->entries = calloc(list->entries_count, sizeof(Entry_s));
    list->entries_loaded = list->entries_count;

    size_t i = 0;
    json_t * id = NULL;
    json_array_foreach(ids_array, i, id)
    {
        if (loading_cancel_requested())
            return;

        draw_loading_bar(i, list->entries_count, type);
        Entry_s * current_entry = &list->entries[i];
        current_entry->tp_download_id = json_integer_value(id);

        char * entry_path = NULL;
        asprintf(&entry_path, THEMEPLAZA_CACHE_PATH_FORMAT, current_entry->tp_download_id);
        utf8_to_utf16(current_entry->path, (u8 *)entry_path, 0x106);
        free(entry_path);

        load_remote_smdh(current_entry, &list->icons_texture, &list->icons_info[i], ignore_cache);
    }
}

void remote_legacy_handle_page_json(Entry_List_s * list, json_t * root, json_int_t page, bool ignore_cache, InstallType loading_screen)
{
    const char * key;
    json_t * value;

    json_object_foreach(root, key, value)
    {
        if (loading_cancel_requested())
            return;

        if (json_is_true(value) && !strcmp(key, THEMEPLAZA_JSON_SUCCESS))
            last_page = page;
        else if (json_is_integer(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_COUNT))
            list->tp_page_count = json_integer_value(value);
        else if (json_is_array(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_IDS))
            remote_legacy_load_entries(list, value, ignore_cache, loading_screen);
        else if (json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_ERROR_MESSAGE)
            && !strcmp(json_string_value(value), THEMEPLAZA_JSON_ERROR_MESSAGE_NOT_FOUND))
        {
            throw_error(language.remote.no_results, ERROR_LEVEL_WARNING);
            if (list->tp_search) free(list->tp_search);
            asprintf(&list->tp_search, "%s", last_search);
            list->tp_current_page = last_page;
        }
    }
}