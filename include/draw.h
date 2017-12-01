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

#ifndef DRAW_H
#define DRAW_H

#include "common.h"
#include "loading.h"

typedef enum {
    INSTALL_SPLASH,
    INSTALL_SPLASH_DELETE,

    INSTALL_SINGLE,
    INSTALL_SHUFFLE,
    INSTALL_BGM,

    INSTALL_DOWNLOAD,
} InstallType;

typedef enum {
    ERROR_LEVEL_ERROR,
    ERROR_LEVEL_WARNING,
} ErrorLevel;

#define BUTTONS_START_Y 140
#define BUTTONS_STEP 25

enum {
    BUTTONS_Y_PREVIEW = BUTTONS_START_Y,

    BUTTONS_Y_LINE_1 = BUTTONS_START_Y + BUTTONS_STEP*1,
    BUTTONS_Y_LINE_2 = BUTTONS_START_Y + BUTTONS_STEP*2,
    BUTTONS_Y_LINE_3 = BUTTONS_START_Y + BUTTONS_STEP*3,

    BUTTONS_X_LEFT = 25,
    BUTTONS_X_RIGHT = 195,
} ButtonPos;

void init_screens(void);
void exit_screens(void);

void draw_themext_error(void);
void throw_error(char* error, ErrorLevel level);

void draw_preview(int preview_offset);

void draw_install(InstallType type);

void draw_interface(Entry_List_s* list, EntryMode current_mode);

#endif