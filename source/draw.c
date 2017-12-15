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
#include "unicode.h"

#include "pp2d/pp2d/pp2d.h"

#include <time.h>

enum Colors {
    COLOR_BACKGROUND = ABGR8(255, 32, 28, 35), //silver-y black
    COLOR_ACCENT = RGBA8(55, 122, 168, 255),
    COLOR_WHITE = RGBA8(255, 255, 255, 255),
    COLOR_CURSOR = RGBA8(200, 200, 200, 255),
    COLOR_BLACK = RGBA8(0, 0, 0, 255),
    COLOR_RED = RGBA8(200, 0, 0, 255),
    COLOR_YELLOW = RGBA8(239, 220, 11, 255),
};

void init_screens(void)
{
    pp2d_init();

    pp2d_set_screen_color(GFX_TOP, COLOR_BACKGROUND);
    pp2d_set_screen_color(GFX_BOTTOM, COLOR_BACKGROUND);

    pp2d_load_texture_png(TEXTURE_ARROW, "romfs:/arrow.png");
    pp2d_load_texture_png(TEXTURE_SHUFFLE, "romfs:/shuffle.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_0, "romfs:/battery0.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_1, "romfs:/battery1.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_2, "romfs:/battery2.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_3, "romfs:/battery3.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_4, "romfs:/battery4.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_5, "romfs:/battery5.png");
    pp2d_load_texture_png(TEXTURE_BATTERY_CHARGE, "romfs:/charging.png");
    pp2d_load_texture_png(TEXTURE_SELECT_BUTTON, "romfs:/select.png");
    pp2d_load_texture_png(TEXTURE_START_BUTTON, "romfs:/start.png");
}

void exit_screens(void)
{
    pp2d_exit();
}

static void draw_base_interface(void)
{
    pp2d_begin_draw(GFX_TOP, GFX_LEFT);
    pp2d_draw_rectangle(0, 0, 400, 23, COLOR_ACCENT);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    pp2d_draw_textf(7, 2, 0.6, 0.6, COLOR_WHITE, "%.2i", tm.tm_hour);
    pp2d_draw_text(28, 1, 0.6, 0.6, COLOR_WHITE, (tm.tm_sec % 2 == 1) ? ":" : " ");
    pp2d_draw_textf(34, 2, 0.6, 0.6, COLOR_WHITE, "%.2i", tm.tm_min);

    #ifndef CITRA_MODE
    u8 battery_charging = 0;
    PTMU_GetBatteryChargeState(&battery_charging);
    u8 battery_status = 0;
    PTMU_GetBatteryLevel(&battery_status);
    pp2d_draw_texture(TEXTURE_BATTERY_0 + battery_status, 357, 2);

    if(battery_charging)
        pp2d_draw_texture(TEXTURE_BATTERY_CHARGE, 357, 2);
    #endif

    pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);
    pp2d_draw_rectangle(0, 0, 320, 24, COLOR_ACCENT);
    pp2d_draw_rectangle(0, 216, 320, 24, COLOR_ACCENT);
    pp2d_draw_text(7, 219, 0.6, 0.6, COLOR_WHITE, VERSION);

    pp2d_draw_on(GFX_TOP, GFX_LEFT);
}

static void draw_text_center(gfxScreen_t target, float y, float scaleX, float scaleY, u32 color, const char* text)
{
    char * _text = strdup(text);
    float prevY = y;
    int offset = 0;
    while(true)
    {
        char *nline = strchr(_text+offset, '\n');
        int nlinepos = 0;
        if(nline != NULL)
        {
            nlinepos = nline-_text;
            _text[nlinepos] = '\0';
        }
        pp2d_draw_text_center(target, prevY, scaleX, scaleY, color, _text+offset);
        if(nline == NULL) break;
        else
        {
            prevY += pp2d_get_text_height(_text+offset, scaleX, scaleY);
            _text[nlinepos] = '\n';
            offset = nlinepos+1;
        }
    }
    free(_text);
}

void throw_error(char* error, ErrorLevel level)
{
    switch(level)
    {
        case ERROR_LEVEL_ERROR:
            while(aptMainLoop())
            {
                hidScanInput();
                u32 kDown = hidKeysDown();
                draw_base_interface();
                draw_text_center(GFX_TOP, 100, 0.6, 0.6, COLOR_RED, error);
                pp2d_draw_wtext_center(GFX_TOP, 150, 0.6, 0.6, COLOR_WHITE, L"Press \uE000 to shut down.");
                pp2d_end_draw();
                if(kDown & KEY_A) break;
            }
            break;
        case ERROR_LEVEL_WARNING:
            while(aptMainLoop())
            {
                hidScanInput();
                u32 kDown = hidKeysDown();
                draw_base_interface();
                draw_text_center(GFX_TOP, 100, 0.6, 0.6, COLOR_YELLOW, error);
                pp2d_draw_wtext_center(GFX_TOP, 150, 0.6, 0.6, COLOR_WHITE, L"Press \uE000 to continue.");
                pp2d_end_draw();
                if(kDown & KEY_A) break;
            }
            break;
    }
}

void draw_preview(int preview_offset)
{
    pp2d_begin_draw(GFX_TOP, GFX_LEFT);
    pp2d_draw_texture_part(TEXTURE_PREVIEW, 0, 0, preview_offset, 0, 400, 240);
    pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);
    pp2d_draw_texture_part(TEXTURE_PREVIEW, 0, 0, 40 + preview_offset, 240, 320, 240);
}

void draw_install(InstallType type)
{
    draw_base_interface();
    switch(type)
    {
        case INSTALL_SINGLE:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Installing a single theme...");
            break;
        case INSTALL_SHUFFLE:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Installing shuffle themes...");
            break;
        case INSTALL_BGM:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Installing BGM-only theme...");
            break;
        case INSTALL_DOWNLOAD:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Downloading...");
            break;
        case INSTALL_SPLASH:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Installing a splash...");
            break;
        case INSTALL_SPLASH_DELETE:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Deleting installed splash...");
            break;
        case INSTALL_ENTRY_DELETE:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Deleting from SD...");
            break;
        case INSTALL_NO_BGM:
            pp2d_draw_text_center(GFX_TOP, 120, 0.8, 0.8, COLOR_WHITE, "Installing theme without BGM...");
            break;
        default:
            break;
    }
    pp2d_end_draw();
}

void draw_interface(Entry_List_s* list, EntryMode current_mode, bool install_mode)
{
    draw_base_interface();

    const char* mode_string[MODE_AMOUNT] = {
        "Theme mode",
        "Splash mode",
    };

    pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, COLOR_WHITE, mode_string[current_mode]);

    if(list->entries == NULL)
    {
        const char* mode_found_string[MODE_AMOUNT] = {
            "No themes found",
            "No splashes found",
        };
        pp2d_draw_text_center(GFX_TOP, 80, 0.7, 0.7, COLOR_YELLOW, mode_found_string[current_mode]);
        pp2d_draw_text_center(GFX_TOP, 110, 0.7, 0.7, COLOR_YELLOW, "Press \uE005 to download from QR");
        const char* mode_switch_string[MODE_AMOUNT] = {
            "Or \uE004 to switch to splashes",
            "Or \uE004 to switch to themes",
        };
        pp2d_draw_text_center(GFX_TOP, 140, 0.7, 0.7, COLOR_YELLOW, mode_switch_string[current_mode]);
        pp2d_draw_text_center(GFX_TOP, 170, 0.7, 0.7, COLOR_YELLOW, "Or \uE045 to quit");
        return;
    }

    int selected_entry = list->selected_entry;
    Entry_s current_entry = list->entries[selected_entry];

    wchar_t title[0x41] = {0};
    utf16_to_utf32((u32*)title, current_entry.name, 0x40);
    pp2d_draw_wtext_wrap(20, 30, 0.7, 0.7, COLOR_WHITE, 380, title);

    wchar_t author[0x41] = {0};
    utf16_to_utf32((u32*)author, current_entry.author, 0x40);
    pp2d_draw_text(20, 50, 0.5, 0.5, COLOR_WHITE, "By: ");
    pp2d_draw_wtext_wrap(44, 50, 0.5, 0.5, COLOR_WHITE, 380, author);

    wchar_t description[0x81] = {0};
    utf16_to_utf32((u32*)description, current_entry.desc, 0x80);
    pp2d_draw_wtext_wrap(20, 65, 0.5, 0.5, COLOR_WHITE, 363, description);

    if(install_mode)
    {
        pp2d_draw_wtext(BUTTONS_X_LEFT, BUTTONS_Y_LINE_2, 0.6, 0.6, COLOR_WHITE, L"\uE079 Normal install");
        pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_2, 0.6, 0.6, COLOR_WHITE, L"\uE07A Shuffle install");

        pp2d_draw_wtext(BUTTONS_X_LEFT, BUTTONS_Y_LINE_3, 0.6, 0.6, COLOR_WHITE, L"\uE07B BGM-only install");
        pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_3, 0.6, 0.6, COLOR_WHITE, L"\uE07C No-BGM install");

        pp2d_draw_text_center(GFX_TOP, BUTTONS_Y_LINE_1, 0.55, 0.55, COLOR_WHITE, "Release \uE000 to cancel or hold \uE006 and release \uE000 to install");
    }
    else
    {
        switch(current_mode)
        {
            case MODE_THEMES:
                pp2d_draw_wtext(BUTTONS_X_LEFT, BUTTONS_Y_LINE_1, 0.6, 0.6, COLOR_WHITE, L"\uE000 Hold to install");
                pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_1, 0.6, 0.6, COLOR_WHITE, L"\uE001 Queue shuffle theme");

                pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_2, 0.6, 0.6, COLOR_WHITE, L"\uE003 Preview theme");

                pp2d_draw_wtext(BUTTONS_X_LEFT, BUTTONS_Y_LINE_3, 0.6, 0.6, COLOR_WHITE, L"\uE004 Switch to splashes");
                pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_3, 0.6, 0.6, COLOR_WHITE, L"\uE005 Scan QR code");
                break;
            case MODE_SPLASHES:
                pp2d_draw_wtext(BUTTONS_X_LEFT, BUTTONS_Y_LINE_1, 0.6, 0.6, COLOR_WHITE, L"\uE000 Install splash");
                pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_1, 0.6, 0.6, COLOR_WHITE, L"\uE001 Delete installed splash");

                pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_2, 0.6, 0.6, COLOR_WHITE, L"\uE003 Preview splash");

                pp2d_draw_wtext(BUTTONS_X_LEFT, BUTTONS_Y_LINE_3, 0.6, 0.6, COLOR_WHITE, L"\uE004 Switch to themes");
                pp2d_draw_wtext(BUTTONS_X_RIGHT, BUTTONS_Y_LINE_3, 0.6, 0.6, COLOR_WHITE, L"\uE005 Scan QR code");
                break;
            default:
                break;
        }
        pp2d_draw_texture(TEXTURE_SELECT_BUTTON, BUTTONS_X_RIGHT-10, BUTTONS_Y_LINE_4 + 3);
        pp2d_draw_wtext(BUTTONS_X_RIGHT+26, BUTTONS_Y_LINE_4, 0.6, 0.6, COLOR_WHITE, L"Delete from SD");
    }

    pp2d_draw_texture(TEXTURE_START_BUTTON, BUTTONS_X_LEFT-10, BUTTONS_Y_LINE_4 + 3);
    pp2d_draw_wtext(BUTTONS_X_LEFT+26, BUTTONS_Y_LINE_4, 0.6, 0.6, COLOR_WHITE, L"Exit");

    pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);

    switch(current_mode)
    {
        case MODE_THEMES:
            pp2d_draw_textf(7, 3, 0.6, 0.6, list->shuffle_count <= 10 ? COLOR_WHITE : COLOR_RED, "Selected: %i/10", list->shuffle_count);
            break;
        default:
            break;
    }

    // Scroll the menu up or down if the selected theme is out of its bounds
    //----------------------------------------------------------------
    for(int i = 0; i < list->entries_count; i++) {
        if(list->entries_count <= ENTRIES_PER_SCREEN) break;

        if(list->scroll > list->selected_entry)
            list->scroll--;

        if((i < list->selected_entry) && \
          ((list->selected_entry - list->scroll) >= ENTRIES_PER_SCREEN) && \
          (list->scroll != (i - ENTRIES_PER_SCREEN)))
            list->scroll++;
    }
    //----------------------------------------------------------------

    // Show arrows if there are themes out of bounds
    //----------------------------------------------------------------
    if(list->scroll > 0)
        pp2d_draw_texture(TEXTURE_ARROW, 155, 6);
    if(list->scroll + ENTRIES_PER_SCREEN < list->entries_count)
        pp2d_draw_texture_flip(TEXTURE_ARROW, 155, 224, VERTICAL);

    for(int i = list->scroll; i < (ENTRIES_PER_SCREEN + list->scroll); i++)
    {
        if(i >= list->entries_count) break;

        current_entry = list->entries[i];

        wchar_t name[0x41] = {0};
        utf16_to_utf32((u32*)name, current_entry.name, 0x40);

        int vertical_offset = 48 * (i - list->scroll);
        u32 font_color = COLOR_WHITE;

        if(i == list->selected_entry)
        {
            font_color = COLOR_BLACK;
            pp2d_draw_rectangle(0, 24 + vertical_offset, 320, 48, COLOR_CURSOR);
        }
        pp2d_draw_wtext(54, 40 + vertical_offset, 0.55, 0.55, font_color, name);
        if(!current_entry.placeholder_color)
            pp2d_draw_texture(current_entry.icon_id, 0, 24 + vertical_offset);
        else
            pp2d_draw_rectangle(0, 24 + vertical_offset, 48, 48, current_entry.placeholder_color);

        if(current_entry.in_shuffle)
            pp2d_draw_texture_blend(TEXTURE_SHUFFLE, 280, 32 + vertical_offset, font_color);
    }
}
