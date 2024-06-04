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

#ifndef LOADING_H
#define LOADING_H

#include "common.h"
#include "entries_list.h"
#include "music.h"
#include <jansson.h>

// These values assume a horizontal orientation
#define TOP_SCREEN_WIDTH 400
#define BOTTOM_SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_COLOR_DEPTH 4

enum ICON_IDS_OFFSET {
    ICONS_ABOVE = 0,
    ICONS_VISIBLE,
    ICONS_UNDER,

    ICONS_OFFSET_AMOUNT,
};

typedef struct {
    u8 _padding1[4 + 2 + 2];

    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];

    u8 _padding2[0x2000 - 0x200 + 0x30 + 0x8];
    u16 small_icon[24 * 24];

    u16 big_icon[48 * 48];
} Icon_s;

typedef struct {
    void ** thread_arg;
    volatile bool run_thread;
} Thread_Arg_s;

void copy_texture_data(C3D_Tex * texture, const u16 * src, const Entry_Icon_s * current_icon);
void parse_smdh(Icon_s * icon, Entry_s * entry, const u16 * fallback_name);


bool load_preview_from_buffer(char * row_pointers, u32 size, C2D_Image * preview_image, int * preview_offset, int height);
bool load_preview(const Entry_List_s * list, C2D_Image * preview_image, int * preview_offset);
void free_preview(C2D_Image preview_image);
Result load_audio(const Entry_s *, audio_s *);
Result load_audio_ogg(const Entry_s * entry, audio_ogg_s * audio);
void load_icons_first(Entry_List_s * current_list, bool silent);
void handle_scrolling(Entry_List_s * list);
void load_icons_thread(void * void_arg);

#endif
