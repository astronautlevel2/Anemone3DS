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

#ifndef COLORS_H
#define COLORS_H

#include "common.h"
#include "text_enums.h"

static constexpr u32 COLOR_WHITE = C2D_Color32f(1,1,1,1);
static constexpr u32 COLOR_BLACK = C2D_Color32f(0,0,0,1);

static constexpr u32 COLOR_BARS = C2D_Color32(15,16,24,255);
static constexpr u32 COLOR_CURSOR = C2D_Color32(200, 200, 200, 255);

static constexpr u32 COLOR_THEME_BG = C2D_Color32(20,32,50,255);
static constexpr u32 COLOR_SPLASH_BG = C2D_Color32(26,42,67,255);
static constexpr u32 COLOR_BADGE_BG = C2D_Color32(32,52,83,255);

static constexpr u32 COLOR_INFO = COLOR_WHITE;
static constexpr u32 COLOR_WARNING = C2D_Color32(239, 220, 11, 255);
static constexpr u32 COLOR_ERROR = C2D_Color32(229, 66, 66, 255);
static constexpr u32 COLOR_ERROR_CRITICAL = C2D_Color32(200, 10, 10, 255);

static constexpr u32 COLOR_ERROR_TEXT[ERROR_LEVEL_AMOUNT] = {
    COLOR_INFO,
    COLOR_WARNING,
    COLOR_ERROR,
    COLOR_ERROR_CRITICAL,
};

#endif
