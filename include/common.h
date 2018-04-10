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

#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define POS() DEBUG("%s (line %d)...\n", __func__, __LINE__)

#define DEBUGPOS(...) \
        POS(); \
        DEBUG(__VA_ARGS__)

#define FASTSCROLL_WAIT 1.5e8

typedef enum {
    MODE_THEMES = 0,
    MODE_SPLASHES,

    MODE_AMOUNT,
} EntryMode;

extern const char * main_paths[MODE_AMOUNT];
extern const int entries_per_screen_v[MODE_AMOUNT];
extern const int entries_per_screen_h[MODE_AMOUNT];
extern const int entry_size[MODE_AMOUNT];
extern bool quit;

enum TextureID {
    TEXTURE_FONT_RESERVED = 0, // used by pp2d for the font
    TEXTURE_ARROW,
    TEXTURE_ARROW_SIDE,
    TEXTURE_SHUFFLE,
    TEXTURE_INSTALLED,
    TEXTURE_PREVIEW_ICON,
    TEXTURE_SORT,
    TEXTURE_DOWNLOAD,
    TEXTURE_BROWSE,
    TEXTURE_LIST,
    TEXTURE_EXIT,
    TEXTURE_BATTERY_0,
    TEXTURE_BATTERY_1,
    TEXTURE_BATTERY_2,
    TEXTURE_BATTERY_3,
    TEXTURE_BATTERY_4,
    TEXTURE_BATTERY_5,
    TEXTURE_BATTERY_CHARGE,
    TEXTURE_QR,
    TEXTURE_PREVIEW,
    TEXTURE_REMOTE_PREVIEW,
    TEXTURE_SELECT_BUTTON,
    TEXTURE_START_BUTTON,

    // always the last
    TEXTURE_REMOTE_ICONS,
    TEXTURE_ICON = TEXTURE_REMOTE_ICONS + 24, 
};

#endif
