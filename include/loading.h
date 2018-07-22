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

#ifndef LOADING_H
#define LOADING_H

#include "common.h"
#include "music.h"
#include <jansson.h>

enum ICON_IDS_OFFSET {
    ICONS_ABOVE = 0,
    ICONS_VISIBLE,
    ICONS_UNDER,

    ICONS_OFFSET_AMOUNT,
};

typedef enum {
    SORT_NONE,

    SORT_NAME,
    SORT_AUTHOR,
    SORT_PATH,
} SortMode;

typedef struct {
    u8 _padding1[4 + 2 + 2];

    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];

    u8 _padding2[0x2000 - 0x200 + 0x30 + 0x8];
    u16 small_icon[24*24];

    u16 big_icon[48*48];
} Icon_s;

typedef struct {
    u16 name[0x41];
    u16 desc[0x81];
    u16 author[0x41];

    u32 placeholder_color;

    u16 path[0x106];
    bool is_zip;

    bool in_shuffle;
    bool no_bgm_shuffle;
    bool installed;

    json_int_t tp_download_id;
} Entry_s;

typedef struct {
    Entry_s * entries;
    int entries_count;

    C2D_Image ** icons;

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
} Entry_List_s;

typedef struct {
    void ** thread_arg;
    volatile bool run_thread;
} Thread_Arg_s;

C2D_Image * loadTextureIcon(Icon_s *icon);
void parse_smdh(Icon_s *icon, Entry_s * entry, const u16 * fallback_name);

void sort_by_name(Entry_List_s * list);
void sort_by_author(Entry_List_s * list);
void sort_by_filename(Entry_List_s * list);

void delete_entry(Entry_s * entry, bool is_file);
Result load_entries(const char * loading_path, Entry_List_s * list);
bool load_preview_from_buffer(void * buf, u32 size, C2D_Image * preview_image, int * preview_offset);
bool load_preview(Entry_List_s list, C2D_Image * preview_image, int * preview_offset);
void free_preview(C2D_Image preview_image);
Result load_audio(Entry_s, audio_s *);
void load_icons_first(Entry_List_s * current_list, bool silent);
void handle_scrolling(Entry_List_s * list);
void load_icons_thread(void * void_arg);
u32 load_data(char * filename, Entry_s entry, char ** buf);

#endif