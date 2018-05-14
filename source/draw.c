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
#include "unicode.h"
#include "colors.h"

#include <time.h>

C3D_RenderTarget* top;
C3D_RenderTarget* bottom;
C2D_TextBuf staticBuf, dynamicBuf;

C2D_Text text[TEXT_AMOUNT];

void init_screens(void)
{
    init_colors();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    staticBuf = C2D_TextBufNew(4096);
    dynamicBuf = C2D_TextBufNew(4096);

    C2D_TextParse(&text[TEXT_VERSION], staticBuf, VERSION);

    C2D_TextParse(&text[TEXT_THEME_MODE], staticBuf, "Theme mode");
    C2D_TextParse(&text[TEXT_SPLASH_MODE], staticBuf, "Splash mode");

    C2D_TextParse(&text[TEXT_NO_THEME_FOUND], staticBuf, "No themes found");
    C2D_TextParse(&text[TEXT_NO_SPLASH_FOUND], staticBuf, "No splashes found");

    C2D_TextParse(&text[TEXT_SELECTED], staticBuf, "Selected:");

    C2D_TextParse(&text[TEXT_THEMEPLAZA_THEME_MODE], staticBuf, "ThemePlaza Theme mode");
    C2D_TextParse(&text[TEXT_THEMEPLAZA_THEME_MODE], staticBuf, "ThemePlaza Splash mode");

    C2D_TextParse(&text[TEXT_SEARCH], staticBuf, "Search...");
    C2D_TextParse(&text[TEXT_PAGE], staticBuf, "Page:");

    C2D_TextParse(&text[TEXT_ERROR_QUIT], staticBuf, "Press \uE000 to quit.");
    C2D_TextParse(&text[TEXT_ERROR_CONTINUE], staticBuf, "Press \uE000 to continue.");

    C2D_TextParse(&text[TEXT_INSTALL_LOADING_THEMES], staticBuf, "Loading themes, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_SPLASHES], staticBuf, "Loading splashes, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_ICONS], staticBuf, "Loading icons, please wait...");

    C2D_TextParse(&text[TEXT_INSTALL_SPLASH], staticBuf, "Installing a splash...");
    C2D_TextParse(&text[TEXT_INSTALL_SPLASH_DELETE], staticBuf, "Deleting installed splash...");

    C2D_TextParse(&text[TEXT_INSTALL_SINGLE], staticBuf, "Installing a single theme...");
    C2D_TextParse(&text[TEXT_INSTALL_SHUFFLE], staticBuf, "Installing shuffle themes...");
    C2D_TextParse(&text[TEXT_INSTALL_BGM], staticBuf, "Installing BGM-only theme...");
    C2D_TextParse(&text[TEXT_INSTALL_NO_BGM], staticBuf, "Installing theme without BGM...");

    C2D_TextParse(&text[TEXT_INSTALL_DOWNLOAD], staticBuf, "Installing BGM-only theme...");
    C2D_TextParse(&text[TEXT_INSTALL_CHECKING_DOWNLOAD], staticBuf, "Checking downloaded file...");
    C2D_TextParse(&text[TEXT_INSTALL_ENTRY_DELETE], staticBuf, "Deleting from SD...");

    C2D_TextParse(&text[TEXT_INSTALL_LOADING_REMOTE_THEMES], staticBuf, "Downloading theme list, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_REMOTE_SPLASHES], staticBuf, "Downloading splash list, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_REMOTE_PREVIEW], staticBuf, "Downloading preview, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_REMOTE_BGM], staticBuf, "Downloading BGM, please wait...");

    for(int i = 0; i < TEXT_AMOUNT; i++)
        C2D_TextOptimize(&text[i]);
}

void exit_screens(void)
{
    C2D_TextBufDelete(dynamicBuf);
    C2D_TextBufDelete(staticBuf);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void set_screen(C3D_RenderTarget * screen)
{
    C2D_TargetClear(screen, colors[COLOR_BACKGROUND]);
    C2D_SceneBegin(screen);
}

void start_frame(void)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void end_frame(void)
{
    C3D_FrameEnd(0);
}

static void draw_text_center(gfxScreen_t target, float y, float z, float scaleX, float scaleY, Color color, C2D_Text * text)
{
    /*
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
        // pp2d_draw_text_center(target, prevY, scaleX, scaleY, color, _text+offset);
        if(nline == NULL) break;
        else
        {
            prevY += pp2d_get_text_height(_text+offset, scaleX, scaleY);
            _text[nlinepos] = '\n';
            offset = nlinepos+1;
        }
    }
    */
}

void draw_base_interface(void)
{
    start_frame();
    set_screen(top);
    // pp2d_draw_rectangle(0, 0, 400, 23, colors[COLOR_ACCENT]);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);


    // pp2d_draw_textf(7, 2, 0.6, 0.6, colors[COLOR_WHITE], "%.2i", tm.tm_hour);
    // pp2d_draw_text(28, 1, 0.6, 0.6, colors[COLOR_WHITE], (tm.tm_sec % 2 == 1) ? ":" : " ");
    // pp2d_draw_textf(34, 2, 0.6, 0.6, colors[COLOR_WHITE], "%.2i", tm.tm_min);

    #ifndef CITRA_MODE
    u8 battery_charging = 0;
    PTMU_GetBatteryChargeState(&battery_charging);
    u8 battery_status = 0;
    PTMU_GetBatteryLevel(&battery_status);
    // pp2d_draw_texture(TEXTURE_BATTERY_0 + battery_status, 357, 2);

    // if(battery_charging)
        // pp2d_draw_texture(TEXTURE_BATTERY_CHARGE, 357, 2);
    #endif

    set_screen(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 24, colors[COLOR_ACCENT]);
    C2D_DrawRectSolid(0, 216, 0.5f, 320, 24, colors[COLOR_ACCENT]);
    // pp2d_draw_text(7, 219, 0.6, 0.6, colors[COLOR_WHITE], VERSION);

    set_screen(top);
}

void throw_error(char* error, ErrorLevel level)
{
    C2D_Text error_text_1, error_text_2;

    bool second_line = *C2D_TextParseLine(&error_text_1, dynamicBuf, error, 0) == '\0';
    C2D_TextOptimize(&error_text_1);
    float scale = 0.5f;
    float error_text_x_pos_1 = 200 - (error_text_1.width*scale/2);
    float error_text_x_pos_2 = 0;

    if(second_line)
    {
        C2D_TextParseLine(&error_text_2, dynamicBuf, error, 1);
        C2D_TextOptimize(&error_text_2);
        error_text_x_pos_2 = 200 - (error_text_2.width*scale/2);
    }

    float bottom_text_scale = 0.6f;
    float bottom_text_x_pos = 0;

    switch(level)
    {
        case ERROR_LEVEL_ERROR:
            bottom_text_x_pos = 200 - (text[TEXT_ERROR_QUIT].width*bottom_text_scale/2);
            while(true)
            {
                hidScanInput();
                u32 kDown = hidKeysDown();

                draw_base_interface();
                C2D_DrawText(&error_text_1, C2D_AtBaseline | C2D_WithColor, error_text_x_pos_1, 100.0f, 0.5f, scale, scale, colors[COLOR_RED]);
                if(second_line)
                    C2D_DrawText(&error_text_2, C2D_AtBaseline | C2D_WithColor, error_text_x_pos_2, 125.0f, 0.5f, scale, scale, colors[COLOR_RED]);

                C2D_DrawText(&text[TEXT_ERROR_QUIT], 0, bottom_text_x_pos, 150.0f, 0.5f, bottom_text_scale, bottom_text_scale);
                end_frame();

                if(kDown & KEY_A) break;
            }
            break;
        case ERROR_LEVEL_WARNING:
            bottom_text_x_pos = 200 - (text[TEXT_ERROR_CONTINUE].width*bottom_text_scale/2);
            while(true)
            {
                hidScanInput();
                u32 kDown = hidKeysDown();

                draw_base_interface();
                C2D_DrawText(&error_text_1, C2D_AtBaseline | C2D_WithColor, error_text_x_pos_1, 100.0f, 0.5f, 0.5f, 0.5f, colors[COLOR_YELLOW]);
                if(second_line)
                    C2D_DrawText(&error_text_2, C2D_AtBaseline | C2D_WithColor, error_text_x_pos_2, 100.0f, 0.5f, 0.5f, 0.5f, colors[COLOR_YELLOW]);

                C2D_DrawText(&text[TEXT_ERROR_CONTINUE], 0, bottom_text_x_pos, 150.0f, 0.5f, bottom_text_scale, bottom_text_scale);
                end_frame();

                if(kDown & KEY_A) break;
            }
            break;
    }
    C2D_TextBufClear(dynamicBuf);
}

bool draw_confirm(const char* conf_msg, Entry_List_s* list)
{
    while(aptMainLoop())
    {
        Instructions_s instructions = {0};
        draw_interface(list, instructions);
        // pp2d_draw_on(GFX_TOP, GFX_LEFT);
        draw_text_center(GFX_TOP, BUTTONS_Y_LINE_1, 0.5f, 0.7, 0.7, colors[COLOR_YELLOW], conf_msg);
        // pp2d_draw_wtext_center(GFX_TOP, BUTTONS_Y_LINE_3, 0.6, 0.6, colors[COLOR_WHITE], L"\uE000 Yes   \uE001 No");
        // pp2d_end_draw();

        hidScanInput();
        u32 kDown = hidKeysDown();
        if(kDown & KEY_A) return true;
        if(kDown & KEY_B) return false;
    }

    return false;
}

void draw_preview(ssize_t previewID, int preview_offset)
{
    // pp2d_begin_draw(GFX_TOP, GFX_LEFT);
    // pp2d_draw_texture_part(previewID, 0, 0, preview_offset, 0, 400, 240);
    // pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);
    // pp2d_draw_texture_part(previewID, 0, 0, 40 + preview_offset, 240, 320, 240);
}

static void draw_install_handler(InstallType type)
{
    if(type != INSTALL_NONE)
    {
        C2D_Text * install_text = &text[type];
        draw_text_center(GFX_TOP, 120.0f, 0.5f, 0.8f, 0.8f, colors[COLOR_WHITE], install_text);
    }
}

void draw_install(InstallType type)
{
    draw_base_interface();
    draw_install_handler(type);
    end_frame();
}

void draw_loading_bar(u32 current, u32 max, InstallType type)
{
    draw_base_interface();
    draw_install_handler(type);
    // pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);
    double percent = 100*((double)current/(double)max);
    u32 width = (u32)percent;
    width *= 2;
    C2D_DrawRectSolid(60-1, 110-1, 0.5f, 200+2, 20+2, colors[COLOR_CURSOR]);
    C2D_DrawRectSolid(60, 110, 0.5f, width, 20, colors[COLOR_ACCENT]);
    end_frame();
}

static void draw_instructions(Instructions_s instructions)
{
    // pp2d_draw_on(GFX_TOP, GFX_LEFT);

    if(instructions.info_line != NULL)
        // pp2d_draw_wtext_center(GFX_TOP, BUTTONS_Y_INFO, 0.55, 0.55, instructions.info_line_color, instructions.info_line);

    const int y_lines[BUTTONS_INFO_LINES-1] = {
        BUTTONS_Y_LINE_1,
        BUTTONS_Y_LINE_2,
        BUTTONS_Y_LINE_3,
    };

    for(int i = 0; i < BUTTONS_INFO_LINES-1; i++)
    {
        if(instructions.instructions[i][0] != NULL)
            // pp2d_draw_wtext(BUTTONS_X_LEFT, y_lines[i], 0.6, 0.6, colors[COLOR_WHITE, instructions.instructions[i][0]);
        if(instructions.instructions[i][1] != NULL)
            // pp2d_draw_wtext(BUTTONS_X_RIGHT, y_lines[i], 0.6, 0.6, colors[COLOR_WHITE, instructions.instructions[i][1]);
    }

    const wchar_t * start_line = instructions.instructions[BUTTONS_INFO_LINES-1][0];
    if(start_line != NULL)
    {
        // pp2d_draw_texture(TEXTURE_START_BUTTON, BUTTONS_X_LEFT-10, BUTTONS_Y_LINE_4 + 3);
        // pp2d_draw_wtext(BUTTONS_X_LEFT+26, BUTTONS_Y_LINE_4, 0.6, 0.6, colors[COLOR_WHITE, start_line);
    }

    const wchar_t * select_line = instructions.instructions[BUTTONS_INFO_LINES-1][1];
    if(select_line != NULL)
    {
        // pp2d_draw_texture(TEXTURE_SELECT_BUTTON, BUTTONS_X_RIGHT-10, BUTTONS_Y_LINE_4 + 3);
        // pp2d_draw_wtext(BUTTONS_X_RIGHT+26, BUTTONS_Y_LINE_4, 0.6, 0.6, colors[COLOR_WHITE, select_line);
    }
}

static void draw_entry_info(Entry_s * entry)
{
    float wrap = 363;

    wchar_t author[0x41] = {0};
    utf16_to_utf32((u32*)author, entry->author, 0x40);
    // pp2d_draw_text(20, 35, 0.5, 0.5, colors[COLOR_WHITE, "By ");
    // pp2d_draw_wtext_wrap(40, 35, 0.5, 0.5, colors[COLOR_WHITE, wrap, author);

    wchar_t title[0x41] = {0};
    utf16_to_utf32((u32*)title, entry->name, 0x40);
    // pp2d_draw_wtext_wrap(20, 50, 0.7, 0.7, colors[COLOR_WHITE, wrap, title);

    int width = (int)pp2d_get_wtext_width(title, 0.7, 0.7);
    int height = (int)pp2d_get_wtext_height(title, 0.7, 0.7);
    int count = ((width - (width % (int)wrap))/wrap) + 1;

    wchar_t description[0x81] = {0};
    utf16_to_utf32((u32*)description, entry->desc, 0x80);
    // pp2d_draw_wtext_wrap(20, 50+count*height, 0.5, 0.5, colors[COLOR_WHITE, wrap, description);
}

void draw_grid_interface(Entry_List_s* list, Instructions_s instructions)
{
    draw_base_interface();
    EntryMode current_mode = list->mode;

    const char* mode_string[MODE_AMOUNT] = {
        "ThemePlaza Theme mode",
        "ThemePlaza Splash mode",
    };

    // pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, colors[COLOR_WHITE, mode_string[current_mode]);

    // draw_instructions(instructions);

    int selected_entry = list->selected_entry;
    Entry_s * current_entry = &list->entries[selected_entry];
    draw_entry_info(current_entry);

    // pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);

    // pp2d_draw_text(7, 3, 0.6, 0.6, colors[COLOR_WHITE, "Search...");

    // pp2d_draw_texture_blend(TEXTURE_LIST, 320-96, 0, colors[COLOR_WHITE);
    // pp2d_draw_texture_blend(TEXTURE_EXIT, 320-72, 0, colors[COLOR_WHITE);
    // pp2d_draw_texture_blend(TEXTURE_PREVIEW_ICON, 320-48, 0, colors[COLOR_WHITE);
    // pp2d_draw_textf(320-24+2.5, -3, 1, 1, colors[COLOR_WHITE, "%c", mode_string[!list->mode][11]);

    // pp2d_draw_texture(TEXTURE_ARROW_SIDE, 3, 114);
    // pp2d_draw_texture_flip(TEXTURE_ARROW_SIDE, 308, 114, HORIZONTAL);

    for(int i = list->scroll; i < (list->entries_loaded + list->scroll); i++)
    {
        if(i >= list->entries_count) break;

        current_entry = &list->entries[i];

        wchar_t name[0x41] = {0};
        utf16_to_utf32((u32*)name, current_entry->name, 0x40);

        int vertical_offset = 0;
        int horizontal_offset = i - list->scroll;
        vertical_offset = horizontal_offset/list->entries_per_screen_h;
        horizontal_offset %= list->entries_per_screen_h;

        horizontal_offset *= list->entry_size;
        vertical_offset *= list->entry_size;
        vertical_offset += 24;
        horizontal_offset += 16;

        if(!current_entry->placeholder_color)
        {
            ssize_t id = list->icons_ids[i];
            // pp2d_draw_texture(id, horizontal_offset, vertical_offset);
        }
        else
            // pp2d_draw_rectangle(horizontal_offset, vertical_offset, list->entry_size, list->entry_size, current_entry->placeholder_color);

        if(i == selected_entry)
        {
            unsigned int border_width = 3;
            // pp2d_draw_rectangle(horizontal_offset, vertical_offset, border_width, list->entry_size, colors[COLOR_CURSOR);
            // pp2d_draw_rectangle(horizontal_offset, vertical_offset, list->entry_size, border_width, colors[COLOR_CURSOR);
            // pp2d_draw_rectangle(horizontal_offset, vertical_offset+list->entry_size-border_width, list->entry_size, border_width, colors[COLOR_CURSOR);
            // pp2d_draw_rectangle(horizontal_offset+list->entry_size-border_width, vertical_offset, border_width, list->entry_size, colors[COLOR_CURSOR);
        }
    }

    char entries_count_str[0x20] = {0};
    sprintf(entries_count_str, "/%"  JSON_INTEGER_FORMAT, list->tp_page_count);
    float x = 316;
    x -= pp2d_get_text_width(entries_count_str, 0.6, 0.6);
    // pp2d_draw_text(x, 219, 0.6, 0.6, colors[COLOR_WHITE, entries_count_str);

    char selected_entry_str[0x20] = {0};
    sprintf(selected_entry_str, "%"  JSON_INTEGER_FORMAT, list->tp_current_page);
    x -= pp2d_get_text_width(selected_entry_str, 0.6, 0.6);
    // pp2d_draw_text(x, 219, 0.6, 0.6, colors[COLOR_WHITE, selected_entry_str);

    // pp2d_draw_text(176, 219, 0.6, 0.6, colors[COLOR_WHITE, "Page:");
}

void draw_interface(Entry_List_s* list, Instructions_s instructions)
{
    draw_base_interface();
    EntryMode current_mode = list->mode;

    const char* mode_string[MODE_AMOUNT] = {
        "Theme mode",
        "Splash mode",
    };

    // pp2d_draw_text_center(GFX_TOP, 4, 0.5, 0.5, colors[COLOR_WHITE, mode_string[current_mode]);

    if(list->entries == NULL)
    {
        const char* mode_found_string[MODE_AMOUNT] = {
            "No themes found",
            "No splashes found",
        };
        // pp2d_draw_text_center(GFX_TOP, 80, 0.7, 0.7, colors[COLOR_YELLOW, mode_found_string[current_mode]);
        // pp2d_draw_text_center(GFX_TOP, 110, 0.7, 0.7, colors[COLOR_YELLOW, "Press \uE005 to download from QR");
        const char* mode_switch_string[MODE_AMOUNT] = {
            "Or \uE004 to switch to splashes",
            "Or \uE004 to switch to themes",
        };
        // pp2d_draw_text_center(GFX_TOP, 140, 0.7, 0.7, colors[COLOR_YELLOW, mode_switch_string[current_mode]);
        // pp2d_draw_text_center(GFX_TOP, 170, 0.7, 0.7, colors[COLOR_YELLOW, "Or        to quit");
        // pp2d_texture_select(TEXTURE_START_BUTTON, 162, 173);
        // pp2d_texture_blend(colors[COLOR_YELLOW);
        // pp2d_texture_scale(1.25, 1.4);
        // pp2d_texture_draw();

        // pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);

        // pp2d_draw_texture_blend(TEXTURE_SORT, 320-144, 0, colors[COLOR_WHITE);
        // pp2d_draw_texture_blend(TEXTURE_DOWNLOAD, 320-120, 0, colors[COLOR_WHITE);
        // pp2d_draw_texture_blend(TEXTURE_BROWSE, 320-96, 0, colors[COLOR_WHITE);
        // pp2d_draw_texture_blend(TEXTURE_EXIT, 320-72, 0, colors[COLOR_WHITE);
        // pp2d_draw_texture_blend(TEXTURE_PREVIEW_ICON, 320-48, 0, colors[COLOR_WHITE);
        // pp2d_draw_textf(320-24+2.5, -3, 1, 1, colors[COLOR_WHITE, "%c", mode_string[!list->mode][0]);

        return;
    }

    // draw_instructions(instructions);

    int selected_entry = list->selected_entry;
    Entry_s * current_entry = &list->entries[selected_entry];
    draw_entry_info(current_entry);

    // pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);

    switch(current_mode)
    {
        case MODE_THEMES:
            // pp2d_draw_textf(7, 3, 0.6, 0.6, list->shuffle_count <= 10 && list->shuffle_count >= 2 ? colors[COLOR_WHITE : colors[COLOR_RED, "Shuffle: %i/10", list->shuffle_count);
            break;
        default:
            break;
    }

    // pp2d_draw_texture_blend(TEXTURE_SORT, 320-144, 0, colors[COLOR_WHITE);
    // pp2d_draw_texture_blend(TEXTURE_DOWNLOAD, 320-120, 0, colors[COLOR_WHITE);
    // pp2d_draw_texture_blend(TEXTURE_BROWSE, 320-96, 0, colors[COLOR_WHITE);
    // pp2d_draw_texture_blend(TEXTURE_EXIT, 320-72, 0, colors[COLOR_WHITE);
    // pp2d_draw_texture_blend(TEXTURE_PREVIEW_ICON, 320-48, 0, colors[COLOR_WHITE);
    // pp2d_draw_textf(320-24+2.5, -3, 1, 1, colors[COLOR_WHITE, "%c", mode_string[!list->mode][0]);

    // Show arrows if there are themes out of bounds
    //----------------------------------------------------------------
    if(list->scroll > 0)
        // pp2d_draw_texture(TEXTURE_ARROW, 152, 4);
    if(list->scroll + list->entries_loaded < list->entries_count)
        // pp2d_draw_texture_flip(TEXTURE_ARROW, 152, 220, VERTICAL);

    for(int i = list->scroll; i < (list->entries_loaded + list->scroll); i++)
    {
        if(i >= list->entries_count) break;

        current_entry = &list->entries[i];

        wchar_t name[0x41] = {0};
        utf16_to_utf32((u32*)name, current_entry->name, 0x40);

        int vertical_offset = i - list->scroll;
        int horizontal_offset = 0;
        horizontal_offset *= list->entry_size;
        vertical_offset *= list->entry_size;
        vertical_offset += 24;

        u32 font_color = colors[COLOR_WHITE];

        if(i == selected_entry)
        {
            font_color = colors[COLOR_BLACK];
            // pp2d_draw_rectangle(0, vertical_offset, 320, list->entry_size, colors[COLOR_CURSOR);
        }

        // pp2d_draw_wtext(list->entry_size+6, vertical_offset + 16, 0.55, 0.55, font_color, name);

        if(current_entry->no_bgm_shuffle)
            // pp2d_draw_texture_blend(TEXTURE_SHUFFLE_NO_BGM, 320-24-4, vertical_offset, font_color);
        else if(current_entry->in_shuffle)
            // pp2d_draw_texture_blend(TEXTURE_SHUFFLE, 320-24-4, vertical_offset, font_color);

        if(current_entry->installed)
            // pp2d_draw_texture_blend(TEXTURE_INSTALLED, 320-24-4, vertical_offset + 22, font_color);

        if(!current_entry->placeholder_color)
        {
            ssize_t id = 0;
            if(list->entries_count > list->entries_loaded*ICONS_OFFSET_AMOUNT)
                id = list->icons_ids[ICONS_VISIBLE*list->entries_loaded + (i - list->scroll)];
            else
                id = list->icons_ids[i];
            // pp2d_draw_texture(id, horizontal_offset, vertical_offset);
        }
        else
            // pp2d_draw_rectangle(horizontal_offset, vertical_offset, list->entry_size, list->entry_size, current_entry->placeholder_color);
    }

    char entries_count_str[0x20] = {0};
    sprintf(entries_count_str, "/%i", list->entries_count);
    float x = 316;
    x -= pp2d_get_text_width(entries_count_str, 0.6, 0.6);
    // pp2d_draw_text(x, 219, 0.6, 0.6, colors[COLOR_WHITE], entries_count_str);

    char selected_entry_str[0x20] = {0};
    sprintf(selected_entry_str, "%i", selected_entry + 1);
    x -= pp2d_get_text_width(selected_entry_str, 0.6, 0.6);
    // pp2d_draw_text(x, 219, 0.6, 0.6, colors[COLOR_WHITE, selected_entry_str);

    // pp2d_draw_text(176, 219, 0.6, 0.6, colors[COLOR_WHITE, list->entries_count < 1000 ? "Selected:" : "Sel.:");
}
