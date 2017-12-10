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

#ifndef COLORS_H
#define COLORS_H

#define ABGR8(a, b, g, r) ((((a)&0xFF)<<0) | (((b)&0xFF)<<8) | (((g)&0xFF)<<16) | (((r)&0xFF)<<24))
#define RGBA8(r, g, b, a) ((((r)&0xFF)<<0) | (((g)&0xFF)<<8) | (((b)&0xFF)<<16) | (((a)&0xFF)<<24))

typedef enum {
    COLOR_BACKGROUND = ABGR8(255, 32, 28, 35), //silver-y black
    COLOR_ACCENT = RGBA8(55, 122, 168, 255),
    COLOR_WHITE = RGBA8(255, 255, 255, 255),
    COLOR_CURSOR = RGBA8(200, 200, 200, 255),
    COLOR_BLACK = RGBA8(0, 0, 0, 255),
    COLOR_RED = RGBA8(200, 0, 0, 255),
    COLOR_YELLOW = RGBA8(239, 220, 11, 255),
} Color;

#endif
