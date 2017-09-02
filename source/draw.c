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

#include "draw.h"

#include "pp2d/pp2d/pp2d.h"

#include <time.h>

enum Colors {
    COLOR_BACKGROUND = ABGR8(255, 32, 28, 35), //silver-y black
    COLOR_ACCENT = RGBA8(55, 122, 168, 255),
    COLOR_WHITE = RGBA8(255, 255, 255, 255),
    COLOR_CURSOR = RGBA8(200, 200, 200, 255),
    COLOR_BLACK = RGBA8(0, 0, 0, 255),
};

void init_screens(void)
{
    pp2d_init();
    
    pp2d_set_screen_color(GFX_TOP, COLOR_BACKGROUND);
    pp2d_set_screen_color(GFX_BOTTOM, COLOR_BACKGROUND);
    
    pp2d_load_texture_png(TEXTURE_ARROW, "romfs:/arrow.png");
    pp2d_load_texture_png(TEXTURE_SHUFFLE, "romfs:/shuffle.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_1, "romfs:/battery1.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_2, "romfs:/battery2.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_3, "romfs:/battery3.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_4, "romfs:/battery4.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_5, "romfs:/battery5.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_CHARGE, "romfs:/charging.png");
}

void exit_screens(void)
{
    pp2d_exit();
}

static int theme_vertical_scroll = 0;
static int splash_vertical_scroll = 0;

void draw_base_interface(void)
{
    pp2d_begin_draw(GFX_TOP);
    pp2d_draw_rectangle(0, 0, 400, 23, COLOR_ACCENT);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
        
    pp2d_draw_textf(7, 2, 0.6, 0.6, COLOR_WHITE, "%.2i", tm.tm_hour);
    pp2d_draw_text(28, 2, 0.6, 0.6, COLOR_WHITE, (tm.tm_sec % 2 == 1) ? ":" : " ");
    pp2d_draw_textf(34, 2, 0.6, 0.6, COLOR_WHITE, "%.2i", tm.tm_min);

    u8 battery_charging;
    PTMU_GetBatteryChargeState(&battery_charging);
    u8 battery_status;
    PTMU_GetBatteryLevel(&battery_status);
    pp2d_draw_texture(2 + battery_status, 357, 2);
    
    if (battery_charging)
        pp2d_draw_texture(TEXTURE_BATTERY_CHARGE, 357, 2);

    pp2d_draw_on(GFX_BOTTOM);
    pp2d_draw_rectangle(0, 0, 320, 24, COLOR_ACCENT);
    pp2d_draw_rectangle(0, 216, 320, 24, COLOR_ACCENT);
    pp2d_draw_on(GFX_TOP);
}

void draw_theme_install(int install_type)
{
    draw_base_interface();
    switch(install_type) 
    {
        case 0:
            pp2d_draw_text(20, 30, 0.7, 0.7, COLOR_WHITE, "Installing a single theme...");
            break;
        case 1:
            pp2d_draw_text(20, 30, 0.7, 0.7, COLOR_WHITE, "Installing a shuffle theme...");
            break;
        case 2:
            pp2d_draw_text(20, 30, 0.7, 0.7, COLOR_WHITE, "Installing BGM...");
            break;
        default:
            break;
    }
    pp2d_end_draw();
}

void draw_theme_interface(Theme_s * themes_list, int theme_count, int selected_theme, bool preview_mode)
{
    
    if (themes_list == NULL)
    {
        pp2d_begin_draw(GFX_TOP);
        pp2d_draw_text_center(GFX_TOP, 100, 1, 1, COLOR_WHITE, "NO THEMES FOUND");
        pp2d_end_draw();
        return;
    }
    
    Theme_s current_theme = themes_list[selected_theme];
    
    if (preview_mode)
    {
        if (current_theme.has_preview)
        {
            pp2d_begin_draw(GFX_TOP);
            pp2d_draw_texture_part(TEXTURE_PREVIEW, 0, 0, current_theme.preview_offset, 0, 400, 240);
            pp2d_draw_on(GFX_BOTTOM);
            pp2d_draw_texture_part(TEXTURE_PREVIEW, 0, 0, 40+current_theme.preview_offset, 240, 320, 240);
        }
    }
    else
    {
        draw_base_interface();
        pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, COLOR_WHITE, "Theme mode");
        wchar_t title[0x40] = {0};
        utf16_to_utf32((u32*)title, current_theme.name, 0x40);
        pp2d_draw_wtext(20, 30, 0.7, 0.7, COLOR_WHITE, title);
        wchar_t author[0x40] = {0};
        utf16_to_utf32((u32*)author, current_theme.author, 0x40);
        pp2d_draw_text(20, 50, 0.5, 0.5, COLOR_WHITE, "By: ");
        pp2d_draw_wtext(44, 50, 0.5, 0.5, COLOR_WHITE, author);
        wchar_t description[0x80] = {0};
        utf16_to_utf32((u32*)description, current_theme.desc, 0x80);
        pp2d_draw_wtext(20, 65, 0.5, 0.5, COLOR_WHITE, description);
        
        pp2d_draw_wtext(20, 150, 0.6, 0.6, COLOR_WHITE, L"\uE046 Install Shuffle Theme");
        pp2d_draw_wtext(200, 150, 0.6, 0.6, COLOR_WHITE, L"\uE004 Switch to Splashes");
        pp2d_draw_wtext(20, 180, 0.6, 0.6, COLOR_WHITE, L"\uE000 Install Theme");
        pp2d_draw_wtext(200, 180, 0.6, 0.6, COLOR_WHITE, L"\uE001 Queue Shuffle");
        pp2d_draw_wtext(20, 210, 0.6, 0.6, COLOR_WHITE, L"\uE002 Install BGM");
        pp2d_draw_wtext(200, 210, 0.6, 0.6, COLOR_WHITE, L"\uE003 Preview Theme");

        pp2d_draw_on(GFX_BOTTOM);

        // Scroll the menu up or down if the selected theme is out of its bounds
        //----------------------------------------------------------------
        for (int i = 0; i < theme_count; i++) {
            if (theme_count <= THEMES_PER_SCREEN)
                break;

            if (theme_vertical_scroll > selected_theme)
                theme_vertical_scroll--;

            if ((i < selected_theme) && \
               ((selected_theme - theme_vertical_scroll) >= THEMES_PER_SCREEN) && \
               (theme_vertical_scroll != ( - THEMES_PER_SCREEN)))
                theme_vertical_scroll++;
        }
        //----------------------------------------------------------------
        
        // Show arrows if there are themes out of bounds
        //----------------------------------------------------------------
        if (theme_vertical_scroll > 0)
            pp2d_draw_texture(TEXTURE_ARROW, 155, 6);
        if (theme_vertical_scroll + THEMES_PER_SCREEN < theme_count)
            pp2d_draw_texture_flip(TEXTURE_ARROW, 155, 224, VERTICAL);
        
        for (int i = theme_vertical_scroll; i < (THEMES_PER_SCREEN + theme_vertical_scroll); i++)
        {
            if (i >= theme_count)
                break;
            
            current_theme = themes_list[i];
            wchar_t name[0x80] = {0};
            utf16_to_utf32((u32*)name, current_theme.name, 0x80);
            
            int vertical_offset = 48 * (i-theme_vertical_scroll);
            u32 font_color = COLOR_WHITE;
            
            if (i == selected_theme)
            {
                font_color = COLOR_BLACK;
                pp2d_draw_rectangle(0, 24 + vertical_offset, 320, 48, COLOR_CURSOR);
            }
            pp2d_draw_wtext(54, 40 + vertical_offset, 0.55, 0.55, font_color, name);
            if (current_theme.has_icon)
                pp2d_draw_texture(current_theme.icon_id, 0, 24 + vertical_offset);
            
            if (current_theme.in_shuffle)
                pp2d_draw_texture_blend(TEXTURE_SHUFFLE, 280, 32 + vertical_offset, font_color);
        }
    }
    
    pp2d_end_draw();
}

void draw_splash_install(int install_type)
{
    draw_base_interface();
    switch (install_type)
    {
        case SINGLE_INSTALL:
            pp2d_draw_textf(20, 30, 0.7, 0.7, COLOR_WHITE, "Installing a splash...");
            break;
        case UNINSTALL:
            pp2d_draw_textf(20, 30, 0.7, 0.7, COLOR_WHITE, "Uninstalling a splash...");
            break;
        default: 
            break;
    }
    pp2d_end_draw();
}

void draw_splash_interface(Splash_s *splashes_list, int splash_count, int selected_splash, bool preview_mode)
{
    if (splashes_list == NULL)
    {
        pp2d_begin_draw(GFX_TOP);
        pp2d_draw_text_center(GFX_TOP, 100, 1, 1, COLOR_WHITE, "NO SPLASHES FOUND");
        pp2d_end_draw();
        return;
    }

    Splash_s current_splash = splashes_list[selected_splash];

    if (preview_mode)
    {
        // TODO: Splash Previews
    } else {
        draw_base_interface();
        pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, COLOR_WHITE, "Splash mode");

        pp2d_draw_wtext_center(GFX_TOP, 180, 0.7, 0.7, COLOR_WHITE, L"\uE000 Install Splash    \uE004 Switch to Themes");
		pp2d_draw_wtext_center(GFX_TOP, 210, 0.7, 0.7, COLOR_WHITE, L"\uE002 Delete current Splash");
        pp2d_draw_on(GFX_BOTTOM);
        for (int i = 0; i < splash_count; i++) {
            if (splash_count <= THEMES_PER_SCREEN)
                break;

            if (splash_vertical_scroll > selected_splash)
                splash_vertical_scroll--;

            if ((i < selected_splash) && \
               ((selected_splash - splash_vertical_scroll) >= THEMES_PER_SCREEN) && \
               (splash_vertical_scroll != ( - THEMES_PER_SCREEN)))
                splash_vertical_scroll++;
        }

        if (splash_vertical_scroll > 0)
            pp2d_draw_texture(TEXTURE_ARROW, 155, 6);
        if (splash_vertical_scroll + THEMES_PER_SCREEN < splash_count)
            pp2d_draw_texture_flip(TEXTURE_ARROW, 155, 224, VERTICAL);

        for (int i = splash_vertical_scroll; i < (THEMES_PER_SCREEN + splash_vertical_scroll); i++)
        {
            if (i >= splash_count)
                break;
            
            current_splash = splashes_list[i];
            wchar_t name[0x106] = {0};
            utf16_to_utf32((u32*)name, current_splash.name, 0x106);
            
            int vertical_offset = 48 * (i-splash_vertical_scroll);
            u32 font_color = COLOR_WHITE;
            
            if (i == selected_splash)
            {
                font_color = COLOR_BLACK;
                pp2d_draw_rectangle(0, 24 + vertical_offset, 320, 48, COLOR_CURSOR);
            }
            pp2d_draw_wtext(15, 40 + vertical_offset, 0.55, 0.55, font_color, name);
        }
    }
    pp2d_end_draw();
}
