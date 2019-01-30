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

#ifndef DRAW_H
#define DRAW_H

#include "common.h"
#include "text_enums.h"
#include "colors.h"

static constexpr float BARS_SIZE = 24.0f;
extern bool have_ptmu;

void init_screens();
void exit_screens();

void start_frame(s64 background_color);
void switch_screen(gfxScreen_t screen);
void end_frame();

void draw_image(int image_id, float x, float y, float z, C2D_ImageTint* tint = nullptr, float scale_X = 1.0f, float scale_Y = 1.0f);

void get_text_dimensions(TextType type, int id, float* width, float* height, float scale_X, float scale_Y);
void get_text_dimensions(const char* text, float* width, float* height, float scale_X, float scale_Y);
inline void get_text_dimensions(const std::string& text, float* width, float* height, float scale_X, float scale_Y)
{
    get_text_dimensions(text.c_str(), width, height, scale_X, scale_Y);
}

void draw_text_centered(C2D_Text* text, u32 color, float y, float z, float scale_X, float scale_Y);
void draw_text_centered(TextType type, int id, u32 color, float y, float z, float scale_X, float scale_Y);

float draw_text_wrap(const char* text, u32 color, float max_x, float x, float y, float z, float scale_X, float scale_Y);
inline float draw_text_wrap(const std::string& text, u32 color, float max_x, float x, float y, float z, float scale_X, float scale_Y)
{
    return draw_text_wrap(text.c_str(), color, max_x, x, y, z, scale_X, scale_Y);
}

void draw_text(C2D_Text* text, u32 color, float x, float y, float z, float scale_X, float scale_Y);
void draw_text(TextType type, int id, u32 color, float x, float y, float z, float scale_X, float scale_Y);
void draw_text(const char* text, u32 color, float x, float y, float z, float scale_X, float scale_Y);
inline void draw_text(const std::string& text, u32 color, float x, float y, float z, float scale_X, float scale_Y)
{
    draw_text(text.c_str(), color, x, y, z, scale_X, scale_Y);
}

float get_text_width(TextType type, int id, float scale_X);

void draw_install(InstallType type);
void draw_loading_bar(u32 current, u32 max, InstallType type);

void draw_error(ErrorLevel level, ErrorType type);

void draw_basic_interface();

#endif
