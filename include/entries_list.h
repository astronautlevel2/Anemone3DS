/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
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

#ifndef ENTRIES_LIST_H
#define ENTRIES_LIST_H

#include "common.h"
#include <jansson.h>

typedef enum {
    SORT_NONE,

    SORT_NAME,
    SORT_AUTHOR,
    SORT_PATH,
} SortMode;

typedef struct {
    u16 path[0x106];
    bool is_zip;
    bool in_shuffle;
    bool no_bgm_shuffle;
    bool installed;
    u32 placeholder_color; // doubles as not-info-loaded when == 0

    json_int_t tp_download_id;
    u16 name[0x41];
    u16 desc[0x81];
    u16 author[0x41];
} Entry_s;

typedef struct {
    Tex3DS_SubTexture subtex;
    u16 x, y;
} Entry_Icon_s;

typedef struct {
    Entry_s * entries;
    int entries_count;
    int entries_capacity;

    C3D_Tex icons_texture;
    Entry_Icon_s * icons_info;

    int previous_scroll;
    int scroll;

    int previous_selected;
    int selected_entry;

    int shuffle_count;

    EntryMode mode;
    int entries_per_screen_v;
    int entries_per_screen_h;
    int entries_loaded;
    int entry_size;

    SortMode current_sort;

    json_int_t tp_current_page;
    json_int_t tp_page_count;
    char * tp_search;
    const char* loading_path;
} Entry_List_s;

void sort_by_name(Entry_List_s * list);
void sort_by_author(Entry_List_s * list);
void sort_by_filename(Entry_List_s * list);

void delete_entry(Entry_s * entry, bool is_file);
// assumes list has been memset to 0
typedef enum InstallType_e InstallType;
Result load_entries(const char * loading_path, Entry_List_s * list, const InstallType loading_screen);
u32 load_data(const char * filename, const Entry_s * entry, char ** buf);
C2D_Image get_icon_at(Entry_List_s * list, size_t index);

// assumes list doesn't have any elements yet
void list_init_capacity(Entry_List_s * list, const int init_capacity);
// assumes list has been inited with a non zero capacity
ssize_t list_add_entry(Entry_List_s * list);

#endif
