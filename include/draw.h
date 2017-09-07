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

#include "themes.h"
#include "splashes.h"
#include "camera.h"

void init_screens(void);
void exit_screens(void);

void draw_themext_error(void);
void draw_qr(void);
void draw_base_interface(void);
void draw_theme_install(int install_type);
void draw_theme_interface(Theme_s * themes_list, int theme_count, int selected_theme, bool preview_mode, int shuffle_theme_count);
void draw_splash_install(int install_type);
void draw_splash_interface(Splash_s *splashes_list, int splash_count, int selected_splash, bool preview_mode);
void throw_error(char* error, int error_type);
#endif