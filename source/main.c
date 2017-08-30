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

#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include <string.h>
#include <time.h>

#include "pp2d/pp2d/pp2d.h"
#include "fs.h"
#include "themes.h"
#include "unicode.h"

#define TEXTURE_ARROW           1
#define TEXTURE_SHUFFLE_BLACK   2
#define TEXTURE_SHUFFLE_WHITE   3
#define MAX_THEMES      4

int init_services(void)
{
    cfguInit();
    srvInit();  
    hidInit();
    fsInit();   
    ptmSysmInit();
    open_archives();
    pp2d_init();
    pp2d_set_screen_color(GFX_TOP, ABGR8(255, 32, 28, 35));
    pp2d_set_screen_color(GFX_BOTTOM, ABGR8(255, 32, 28, 35));
    pp2d_load_texture_png(TEXTURE_ARROW, "romfs:/arrow.png");
    pp2d_load_texture_png(TEXTURE_SHUFFLE_BLACK, "romfs:/shuffle_black.png");
    pp2d_load_texture_png(TEXTURE_SHUFFLE_WHITE, "romfs:/shuffle_white.png");
    return 0;
}

int de_init_services(void)
{
    gfxExit();
    cfguExit();
    srvExit();
    hidExit();
    fsExit();
    ptmSysmExit();
    close_archives();
    return 0;
}

void format_time(char *time_string)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(time_string, "%s%.2i:%.2i", time_string, tm.tm_hour, tm.tm_min);
}

Result MCUHWC_GetBatteryLevel(u8 *out) // Code taken from daedreth's fork of lpp-3ds
{
    #define TRY(expr) if(R_FAILED(res = (expr))) { svcCloseHandle(mcuhwcHandle); return res; }
    Result res;
    Handle mcuhwcHandle;
    TRY(srvGetServiceHandle(&mcuhwcHandle, "mcu::HWC"));
    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x50000;
    TRY(svcSendSyncRequest(mcuhwcHandle));
    *out = (u8) cmdbuf[2];
    svcCloseHandle(mcuhwcHandle);
    return cmdbuf[1];
    #undef TRY
}

int main(void)
{
    init_services();

    int theme_count = get_number_entries("/Themes");
    theme **themes_list = calloc(theme_count, sizeof(theme));
    scan_themes(themes_list, theme_count);

    u32 color_accent = RGBA8(55, 122, 168, 255);
    u32 color_white = RGBA8(255, 255, 255, 255);
    u32 cursor_color = RGBA8(200, 200, 200, 255);
    u32 color_black = RGBA8(0, 0, 0, 255);

    int cursor_pos = 1;
    int top_pos = 0;
    
    while(aptMainLoop())
    {
        theme *current_theme = themes_list[cursor_pos + top_pos - 1];
        hidScanInput();
        u32 kDown = hidKeysDown();

        pp2d_begin_draw(GFX_TOP);
        pp2d_draw_rectangle(0, 0, 400, 23, color_accent);
        pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, color_white, "Theme mode");

        wchar_t title[0x40] = {0};
        utf16_to_utf32((u32*)title, current_theme->name, 0x40);
        pp2d_draw_wtext(20, 30, 0.7, 0.7, color_white, title);

        if (current_theme->has_preview)
        {
            pp2d_draw_texture_scale(current_theme->preview_id, 220, 35, 0.4, 0.4);
        }
        
        char time_string[6] = {0};
        format_time(time_string);
        pp2d_draw_text(7, 2, 0.6, 0.6, color_white, time_string);

        u8 battery_val;
        MCUHWC_GetBatteryLevel(&battery_val);
        pp2d_draw_textf(350, 2, 0.6, 0.6, color_white, "%i%%", battery_val);

        pp2d_draw_on(GFX_BOTTOM);
        pp2d_draw_rectangle(0, 0, 320, 24, color_accent);
        pp2d_draw_rectangle(0, 216, 320, 24, color_accent);
        pp2d_draw_rectangle(0, 24 + (48 * (cursor_pos-1)), 320, 48, cursor_color);
        
        if (top_pos > 0)
        {
            pp2d_draw_texture(TEXTURE_ARROW, 155, 6);
        }

        if (top_pos + MAX_THEMES < theme_count)
        {
            pp2d_draw_texture_flip(TEXTURE_ARROW, 155, 224, VERTICAL);
        }

        for (int i = 0; i < MAX_THEMES; i++)
        {
            if (i + top_pos == theme_count) break;
            wchar_t name[0x40] = {0};
            utf16_to_utf32((u32*)name, themes_list[i+top_pos]->name, 0x40);
            if (cursor_pos-1 == i) pp2d_draw_wtext(50, 40 + (48 * i), 0.55, 0.55, color_black, name);
            else pp2d_draw_wtext(50, 40 + (48 * i), 0.55, 0.55, color_white, name);
            if (themes_list[i+top_pos]->selected)
            {
                if (cursor_pos-1 == i) pp2d_draw_texture(TEXTURE_SHUFFLE_BLACK, 280, 32 + (48 * i));
                else pp2d_draw_texture(TEXTURE_SHUFFLE_WHITE, 280, 32 + (48 * i));
            }
        }

        if (kDown & KEY_A)
        {
            single_install(*current_theme);
        }

        if (kDown & KEY_B)
        {
            current_theme->selected = !(current_theme->selected);
        }

        if (kDown & KEY_SELECT)
        {
            shuffle_install(themes_list, theme_count);
        }

        if (kDown & KEY_DOWN) 
        {
            if (cursor_pos < MAX_THEMES && cursor_pos < theme_count) cursor_pos++; 
            else if (cursor_pos + top_pos < theme_count) top_pos++;
        }

        if (kDown & KEY_UP)
        {
            if (cursor_pos > 1) cursor_pos--;
            else if (top_pos > 0) top_pos--;
        }

        if (kDown & KEY_START)
        {
            // close_archives();
            // PTMSYSM_ShutdownAsync(0);
            // ptmSysmExit();
            break;
        }

        pp2d_end_draw();
    }

    de_init_services();
    return 0;
}
