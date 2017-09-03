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

#ifndef THEMES_H
#define THEMES_H

#include "common.h"

typedef struct {
    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];
    
    u32 placeholder_color;
    ssize_t icon_id;
    
    bool has_preview;
    int preview_offset;
    
    u16 path[0x106];
    bool is_zip;
    
    bool in_shuffle;
} Theme_s;

void load_theme_preview(Theme_s *theme);
Result get_themes(Theme_s **themes_list, int *theme_count);
Result single_install(Theme_s theme);
Result shuffle_install(Theme_s *themes_list, int theme_count);
Result bgm_install(Theme_s bgm_to_install);

#endif