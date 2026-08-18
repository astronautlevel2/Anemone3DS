#ifndef REMOTE_INTERNAL_H
#define REMOTE_INTERNAL_H

#include "remote.h"
#include "loading.h"
#include "fs.h"
#include "ui_strings.h"

extern int __stacksize__;
extern char *last_search;
extern json_int_t last_page;

void free_remote_entries(Entry_List_s * list);
void ensure_remote_cache_directory(const Entry_s * entry);
void set_remote_text_field(u16 * dest, size_t max_chars, const char * value, const char * fallback);

void remote_legacy_handle_page_json(Entry_List_s * list, json_t * root, json_int_t page, bool ignore_cache, InstallType loading_screen);
void remote_legacy_load_entries(Entry_List_s * list, json_t * ids_array, bool ignore_cache, InstallType type);

Result remote_v2_init_session(void);
void remote_v2_cleanup_session(void);
void remote_v2_lock_texture(void);
void remote_v2_unlock_texture(void);
void remote_v2_stop_icon_thread(void);
void remote_v2_start_icon_thread(Entry_List_s * list, bool ignore_cache);
const char * remote_v2_get_kind_path(RemoteMode mode);
void remote_v2_handle_page_json(Entry_List_s * list, json_t * root, json_int_t page, bool ignore_cache, InstallType loading_screen);
void remote_v2_load_entries(Entry_List_s * list, json_t * items_array, bool ignore_cache, InstallType type);
Result curl_http_get(const char * url, char ** filename, char ** buf, u32 * size, const char * acceptable_mime_types, InstallType install_type);

#endif