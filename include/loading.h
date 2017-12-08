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

#ifndef LOADING_H
#define LOADING_H

#include "common.h"

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
    ssize_t icon_id;

    u16 path[0x106];
    bool is_zip;

    bool in_shuffle;
} Entry_s;

typedef struct {
    Entry_s * entries;
    int entries_count;

    ssize_t icon_id_start;

    int scroll;
    int selected_entry;

    int shuffle_count;
} Entry_List_s;

void delete_entry(Entry_s entry);
Result load_entries(const char * loading_path, Entry_List_s * list);
bool load_preview(Entry_List_s list, int * preview_offset);
u32 load_data(char * filename, Entry_s entry, char ** buf);

#endif