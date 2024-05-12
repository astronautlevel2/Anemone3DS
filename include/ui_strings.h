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

#ifndef UISTRINGS_H
#define UISTRINGS_H

#include "colors.h"
#include "draw.h"

#define CAMERA_STRINGS 5
#define DRAW_STRINGS 36
#define FS_STRINGS 8
#define LOADING_STRINGS 1
#define MAIN_STRINGS 14
#define REMOTE_STRINGS 29
#define SPLASHES_STRINGS 2
#define THEMES_STRINGS 6

typedef struct {
    Instructions_s normal_instructions[MODE_AMOUNT];
    Instructions_s install_instructions;
    Instructions_s extra_instructions[3];
    char *camera[CAMERA_STRINGS];
    char *draw[DRAW_STRINGS];
    char *fs[FS_STRINGS];
    char *loading[LOADING_STRINGS];
    char *main[MAIN_STRINGS];
    char *remote[REMOTE_STRINGS];
    char *splashes[SPLASHES_STRINGS];
    char *themes[THEMES_STRINGS];
} Language_s;

typedef enum {
    LANGUAGE_EN,

    LANGUAGE_AMOUNT,
} Language_Name;

extern Language_s languages[LANGUAGE_AMOUNT];
extern Language_s language;

#endif
