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

#include "draw.h"
#include "text.h"

static gfxScreen_t current_screen;
static C2D_SpriteSheet spritesheet;
static C3D_RenderTarget* top;
static C3D_RenderTarget* bottom;
static C2D_TextBuf width_buf, dynamic_buf;
static u32 current_clear_color = COLOR_THEME_BG;

void init_screens()
{
    DEBUG("gfxInitDefault\n");
    gfxInitDefault();
    DEBUG("C3D_Init\n");
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    DEBUG("C2D_Init\n");
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    DEBUG("C2D_Prepare\n");
    C2D_Prepare();

    DEBUG("C2D_SpriteSheetLoad\n");
    spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    if(!spritesheet)
        svcBreak(USERBREAK_PANIC);

    DEBUG("C2D_CreateScreenTarget\n");
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    DEBUG("C2D_TextBufNew\n");
    dynamic_buf = C2D_TextBufNew(0x1000);
    width_buf = C2D_TextBufNew(0x1000);

    DEBUG("init_text\n");
    init_text();
}

void exit_screens()
{
    exit_text();

    C2D_TextBufDelete(width_buf);
    C2D_TextBufDelete(dynamic_buf);

    C2D_SpriteSheetFree(spritesheet);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void start_frame(s64 background_color)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    if(background_color >= 0)
        current_clear_color = background_color & 0xFFFFFFFF;
    C2D_TargetClear(top, current_clear_color);
    C2D_TargetClear(bottom, current_clear_color);

    C2D_TextBufClear(dynamic_buf);
    C2D_TextBufClear(width_buf);
}

void switch_screen(gfxScreen_t screen)
{
    if(current_screen == screen)
        return;

    current_screen = screen;
    if(screen == GFX_TOP)
        C2D_SceneBegin(top);
    else if(screen == GFX_BOTTOM)
        C2D_SceneBegin(bottom);
    else
        svcBreak(USERBREAK_ASSERT);
}

void end_frame()
{
    C3D_FrameEnd(0);
}

void draw_image(int image_id, float x, float y, float z, C2D_ImageTint* tint, float scale_X, float scale_Y)
{
    C2D_DrawImageAt(C2D_SpriteSheetGetImage(spritesheet, image_id), x, y, z, tint, scale_X, scale_Y);
}

void get_text_dimensions(TextType type, int id, float* width, float* height, float scale_X, float scale_Y)
{
    C2D_TextGetDimensions(&static_texts[type][id], scale_X, scale_Y, width, height);
}

void get_text_dimensions(const char * text, float * width, float * height, float scale_X, float scale_Y)
{
    C2D_Text c2d_text;
    C2D_TextParse(&c2d_text, width_buf, text);
    C2D_TextGetDimensions(&c2d_text, scale_X, scale_Y, width, height);
}

void draw_text_centered(C2D_Text* text, u32 color, float y, float z, float scale_X, float scale_Y)
{
    // Citro2D does this internally and all we need is width, so save a function call
    float x = ((current_screen == GFX_TOP ? 400 : 320) - (text->width * scale_X)) / 2.0f;
    C2D_DrawText(text, C2D_WithColor, x, y, z, scale_X, scale_Y, color);
}

void draw_text_centered(TextType type, int id, u32 color, float y, float z, float scale_X, float scale_Y)
{
    draw_text_centered(&static_texts[type][id], color, y, z, scale_X, scale_Y);
}

void draw_text(C2D_Text* text, u32 color, float x, float y, float z, float scale_X, float scale_Y)
{
    C2D_DrawText(text, C2D_WithColor, x, y, z, scale_X, scale_Y, color);
}

void draw_text(TextType type, int id, u32 color, float x, float y, float z, float scale_X, float scale_Y)
{
    draw_text(&static_texts[type][id], color, x, y, z, scale_X, scale_Y);
}

void draw_text(const char* text, u32 color, float x, float y, float z, float scale_X, float scale_Y)
{
    C2D_Text actual_text;
    C2D_TextParse(&actual_text, dynamic_buf, text);
    C2D_TextOptimize(&actual_text);
    draw_text(&actual_text, color, x, y, z, scale_X, scale_Y);
}

float get_text_width(TextType type, int id, float scale_X)
{
    return static_texts[type][id].width * scale_X;
}

static void draw_install_internal(InstallType type)
{
    switch_screen(GFX_TOP);
    static constexpr float install_text_scale = 0.8f;
    float height;
    C2D_TextGetDimensions(&static_texts[TEXT_INSTALL][type], install_text_scale, install_text_scale, nullptr, &height);
    draw_text_centered(TEXT_INSTALL, type, COLOR_WHITE, (240.0f - height)/2.0f, 0.5f, install_text_scale, install_text_scale);
}

void draw_install(InstallType type)
{
    start_frame(-1);
    draw_basic_interface();
    draw_install_internal(type);
    end_frame();
}

void draw_loading_bar(u32 current, u32 max, InstallType type)
{
    start_frame(-1);
    draw_basic_interface();
    double percent = 100.0*(current/static_cast<double>(max));
    u32 width = (u32)percent;
    width *= 2;
    C2D_DrawRectSolid(60-4, 110-4, 0.1f, 200+8, 20+8, COLOR_WHITE);
    C2D_DrawRectSolid(60-2, 110-2, 0.1f, 200+4, 20+4, current_clear_color);
    C2D_DrawRectSolid(60, 110, 0.1f, width, 20, COLOR_WHITE);
    draw_install_internal(type);
    end_frame();
}

void draw_error(ErrorLevel level, ErrorType type)
{
    DEBUG("Throwing error level %d, type %d\n", level, type);
    while(aptMainLoop() && running)
    {
        start_frame(-1);
        draw_basic_interface();

        // /*
        float height;
        static constexpr float error_text_scale = 0.8f;
        C2D_Text* instructions_text = &static_texts[TEXT_GENERAL][level == ERROR_LEVEL_CRITICAL ? TEXT_ERROR_QUIT : TEXT_ERROR_CONTINUE];
        C2D_TextGetDimensions(instructions_text, error_text_scale, error_text_scale, nullptr, &height);
        draw_text_centered(instructions_text, COLOR_ERROR_TEXT[level], (240.0f - height)/2.0f, 0.5f, error_text_scale, error_text_scale);

        switch_screen(GFX_TOP);

        C2D_TextGetDimensions(&static_texts[TEXT_ERROR][type], error_text_scale, error_text_scale, nullptr, &height);
        draw_text_centered(TEXT_ERROR, type, COLOR_ERROR_TEXT[level], (240.0f - height)/2.0f, 0.5f, error_text_scale, error_text_scale);
        // */

        end_frame();

        hidScanInput();
        u32 kDown = hidKeysDown();

        if(kDown & KEY_START)
        {
            power_pressed = false;
            running = false;
            break;
        }
        else if(kDown & KEY_A)
        {
            if(level == ERROR_LEVEL_CRITICAL)
            {
                power_pressed = false;
                running = false;
            }
            break;
        }
    }
}

void draw_basic_interface()
{
    switch_screen(GFX_TOP);
    C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, 400.0f, BARS_SIZE, COLOR_BARS);
    C2D_DrawRectSolid(0.0f, 240.0f - BARS_SIZE, 0.0f, 400.0f, BARS_SIZE, COLOR_BARS);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char string_hours[3] = {0};
    sprintf(string_hours, "%.2i", tm.tm_hour);

    C2D_Text hours, separator, minutes;
    C2D_TextParse(&hours, dynamic_buf, string_hours);
    C2D_TextOptimize(&hours);

    C2D_TextParse(&separator, dynamic_buf, ":");
    C2D_TextOptimize(&separator);

    char string_minutes[3] = {0};
    sprintf(string_minutes, "%.2i", tm.tm_min);

    C2D_TextParse(&minutes, dynamic_buf, string_minutes);
    C2D_TextOptimize(&minutes);

    C2D_DrawText(&hours, C2D_WithColor, 28 - hours.width*0.6f, 3, 0.1f, 0.6f, 0.6f, COLOR_WHITE);
    if(tm.tm_sec % 2 == 1)
        C2D_DrawText(&separator, C2D_WithColor, 28, 2, 0.1f, 0.6f, 0.6f, COLOR_WHITE);
    C2D_DrawText(&minutes, C2D_WithColor, 28 + separator.width*0.6f, 3, 0.1f, 0.6f, 0.6f, COLOR_WHITE);

    switch_screen(GFX_BOTTOM);
    C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, 320.0f, BARS_SIZE, COLOR_BARS);
    C2D_DrawRectSolid(0.0f, 240.0f - BARS_SIZE, 0.0f, 320.0f, BARS_SIZE, COLOR_BARS);

    draw_text(TEXT_GENERAL, TEXT_VERSION, COLOR_WHITE, 4, 219, 0.1f, 0.6f, 0.6f);
}
