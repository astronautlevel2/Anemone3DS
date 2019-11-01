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

#include "sprites.h"

#include <time.h>

C3D_RenderTarget* top;
C3D_RenderTarget* bottom;
C2D_TextBuf staticBuf, dynamicBuf;
static C2D_TextBuf widthBuf;

static C2D_SpriteSheet spritesheet;
static C2D_Sprite sprite_shuffle, sprite_shuffle_no_bgm, sprite_installed, sprite_start, sprite_select;

C2D_Text text[TEXT_AMOUNT];
static const char * mode_switch_char[MODE_AMOUNT] = {
    "S",
    "T",
};

void init_screens(void)
{
    init_colors();
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    spritesheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
    C2D_SpriteFromSheet(&sprite_shuffle, spritesheet, sprites_shuffle_idx);
    C2D_SpriteSetDepth(&sprite_shuffle, 0.6f);
    C2D_SpriteFromSheet(&sprite_shuffle_no_bgm, spritesheet, sprites_shuffle_no_bgm_idx);
    C2D_SpriteSetDepth(&sprite_shuffle_no_bgm, 0.6f);

    C2D_SpriteFromSheet(&sprite_installed, spritesheet, sprites_installed_idx);
    C2D_SpriteSetDepth(&sprite_installed, 0.6f);

    C2D_SpriteFromSheet(&sprite_start, spritesheet, sprites_start_idx);
    C2D_SpriteSetDepth(&sprite_start, 0.5f);
    C2D_SpriteFromSheet(&sprite_select, spritesheet, sprites_select_idx);
    C2D_SpriteSetDepth(&sprite_select, 0.5f);

    staticBuf = C2D_TextBufNew(4096);
    dynamicBuf = C2D_TextBufNew(4096);
    widthBuf = C2D_TextBufNew(4096);

    C2D_TextParse(&text[TEXT_VERSION], staticBuf, VERSION);

    C2D_TextParse(&text[TEXT_THEME_MODE], staticBuf, "Theme mode");
    C2D_TextParse(&text[TEXT_SPLASH_MODE], staticBuf, "Splash mode");

    C2D_TextParse(&text[TEXT_NO_THEME_FOUND], staticBuf, "No theme found");
    C2D_TextParse(&text[TEXT_NO_SPLASH_FOUND], staticBuf, "No splash found");

    C2D_TextParse(&text[TEXT_DOWNLOAD_FROM_QR], staticBuf, "Press \uE005 to download from QR");

    C2D_TextParse(&text[TEXT_SWITCH_TO_SPLASHES], staticBuf, "Or \uE004 to switch to splashes");
    C2D_TextParse(&text[TEXT_SWITCH_TO_THEMES], staticBuf, "Or \uE004 to switch to themes");

    C2D_TextParse(&text[TEXT_OR_START_TO_QUIT], staticBuf, "Or        to quit");

    C2D_TextParse(&text[TEXT_BY_AUTHOR], staticBuf, "By ");
    C2D_TextParse(&text[TEXT_SELECTED], staticBuf, "Selected:");
    C2D_TextParse(&text[TEXT_SELECTED_SHORT], staticBuf, "Sel.:");

    C2D_TextParse(&text[TEXT_THEMEPLAZA_THEME_MODE], staticBuf, "ThemePlaza Theme mode");
    C2D_TextParse(&text[TEXT_THEMEPLAZA_SPLASH_MODE], staticBuf, "ThemePlaza Splash mode");

    C2D_TextParse(&text[TEXT_SEARCH], staticBuf, "Search...");
    C2D_TextParse(&text[TEXT_PAGE], staticBuf, "Page:");

    C2D_TextParse(&text[TEXT_ERROR_QUIT], staticBuf, "Press \uE000 to quit.");
    C2D_TextParse(&text[TEXT_ERROR_CONTINUE], staticBuf, "Press \uE000 to continue.");

    C2D_TextParse(&text[TEXT_CONFIRM_YES_NO], staticBuf, "\uE000 Yes   \uE001 No");

    C2D_TextParse(&text[TEXT_INSTALL_LOADING_THEMES], staticBuf, "Loading themes, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_SPLASHES], staticBuf, "Loading splashes, please wait...");
    C2D_TextParse(&text[TEXT_INSTALL_LOADING_ICONS], staticBuf, "Loading icons, please wait...");

    C2D_TextParse(&text[TEXT_INSTALL_SPLASH], staticBuf, "Installing a splash...");
    C2D_TextParse(&text[TEXT_INSTALL_SPLASH_DELETE], staticBuf, "Deleting installed splash...");

    C2D_TextParse(&text[TEXT_INSTALL_SINGLE], staticBuf, "Installing a single theme...");
    C2D_TextParse(&text[TEXT_INSTALL_SHUFFLE], staticBuf, "Installing shuffle themes...");
    C2D_TextParse(&text[TEXT_INSTALL_BGM], staticBuf, "Installing BGM-only theme...");
    C2D_TextParse(&text[TEXT_INSTALL_NO_BGM], staticBuf, "Installing theme without BGM...");

    C2D_TextParse(&text[TEXT_INSTALL_DOWNLOAD], staticBuf, "Downloading...");
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
    C2D_TextBufDelete(widthBuf);
    C2D_TextBufDelete(dynamicBuf);
    C2D_TextBufDelete(staticBuf);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void set_screen(C3D_RenderTarget * screen)
{
    C2D_SceneBegin(screen);
}

void start_frame(void)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top, colors[COLOR_BACKGROUND]);
    C2D_TargetClear(bottom, colors[COLOR_BACKGROUND]);
}

void end_frame(void)
{
    C2D_TextBufClear(dynamicBuf);
    C2D_TextBufClear(widthBuf);
    C3D_FrameEnd(0);
}

static void draw_image(int image_id, float x, float y)
{
    C2D_DrawImageAt(C2D_SpriteSheetGetImage(spritesheet, image_id), x, y, 0.6f, NULL, 1.0f, 1.0f);
}

static void get_text_dimensions(const char * text, float scaleX, float scaleY, float * width, float * height)
{
    C2D_Text c2d_text;
    C2D_TextParse(&c2d_text, widthBuf, text);
    C2D_TextGetDimensions(&c2d_text, scaleX, scaleY, width, height);
}

static void draw_c2d_text(float x, float y, float z, float scaleX, float scaleY, Color color, C2D_Text * text)
{
    C2D_DrawText(text, C2D_WithColor, x, y, z, scaleX, scaleY, color);
}

void draw_text(float x, float y, float z, float scaleX, float scaleY, Color color, const char * text)
{
    C2D_Text c2d_text;
    C2D_TextParse(&c2d_text, dynamicBuf, text);
    C2D_TextOptimize(&c2d_text);
    C2D_DrawText(&c2d_text, C2D_WithColor, x, y, z, scaleX, scaleY, color);
}

static void draw_c2d_text_center(gfxScreen_t target, float y, float z, float scaleX, float scaleY, Color color, C2D_Text * text)
{
    float width = 0;
    C2D_TextGetDimensions(text, scaleX, scaleY, &width, NULL);
    float offset = (target == GFX_TOP ? 400 : 320)/2 - width/2;

    C2D_DrawText(text, C2D_WithColor, offset, y, z, scaleX, scaleY, color);
}

void draw_text_center(gfxScreen_t target, float y, float z, float scaleX, float scaleY, Color color, const char * text)
{
    C2D_Text text_arr[MAX_LINES];
    float offsets_arr[MAX_LINES];
    int actual_lines = 0;
    const char * end = text - 1;

    do {
        end = C2D_TextParseLine(&text_arr[actual_lines], dynamicBuf, end + 1, actual_lines);
        actual_lines++;
    } while(*end == '\n');

    for(int i = 0; i < actual_lines; i++)
    {
        C2D_TextOptimize(&text_arr[i]);
        float width = 0;
        C2D_TextGetDimensions(&text_arr[i], scaleX, scaleY, &width, NULL);
        offsets_arr[i] = (target == GFX_TOP ? 400 : 320)/2 - width/2;
    }

    for(int i = 0; i < actual_lines; i++)
    {
        C2D_DrawText(&text_arr[i], C2D_WithColor, offsets_arr[i], y, z, scaleX, scaleY, color);
    }
}

void draw_base_interface(void)
{
    start_frame();
    set_screen(top);

    C2D_DrawRectSolid(0, 0, 0.5f, 400, 23, colors[COLOR_ACCENT]);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char string_hours[3] = {0};
    sprintf(string_hours, "%.2i", tm.tm_hour);

    C2D_Text hours, separator, minutes;
    C2D_TextParse(&hours, dynamicBuf, string_hours);
    C2D_TextOptimize(&hours);

    if(tm.tm_sec % 2 == 1)
    {
        C2D_TextParse(&separator, dynamicBuf, ":");
        C2D_TextOptimize(&separator);
    }

    char string_minutes[3] = {0};
    sprintf(string_minutes, "%.2i", tm.tm_min);

    C2D_TextParse(&minutes, dynamicBuf, string_minutes);
    C2D_TextOptimize(&minutes);

    C2D_DrawText(&hours, C2D_WithColor, 7, 2, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE]);
    if(tm.tm_sec % 2 == 1)
        C2D_DrawText(&separator, C2D_WithColor, 28, 1, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE]);
    C2D_DrawText(&minutes, C2D_WithColor, 34, 2, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE]);

    #ifndef CITRA_MODE
    u8 battery_charging = 0;
    PTMU_GetBatteryChargeState(&battery_charging);
    u8 battery_status = 0;
    PTMU_GetBatteryLevel(&battery_status);
    draw_image(sprites_battery0_idx + battery_status, 357, 2);

    if(battery_charging)
        draw_image(sprites_charging_idx, 357, 2);
    #endif

    set_screen(bottom);

    C2D_DrawRectSolid(0, 0, 0.5f, 320, 24, colors[COLOR_ACCENT]);
    C2D_DrawRectSolid(0, 216, 0.5f, 320, 24, colors[COLOR_ACCENT]);
    C2D_DrawText(&text[TEXT_VERSION], C2D_WithColor, 7, 219, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE]);

    set_screen(top);
}

void throw_error(char* error, ErrorLevel level)
{
    Text bottom_text = TEXT_AMOUNT;
    Color text_color = COLOR_WHITE;

    switch(level)
    {
        case ERROR_LEVEL_ERROR:
            bottom_text = TEXT_ERROR_QUIT;
            text_color = COLOR_RED;
            break;
        case ERROR_LEVEL_WARNING:
            bottom_text = TEXT_ERROR_CONTINUE;
            text_color = COLOR_YELLOW;
            break;
        default:
            return;
    }

    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();

        draw_base_interface();
        draw_text_center(GFX_TOP, 100, 0.5f, 0.6f, 0.6f, colors[text_color], error);
        draw_c2d_text_center(GFX_TOP, 150, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE], &text[bottom_text]);
        end_frame();

        if(kDown & KEY_A) break;
    }
}

bool draw_confirm(const char* conf_msg, Entry_List_s* list)
{
    while(aptMainLoop())
    {
        Instructions_s instructions = {0};
        draw_interface(list, instructions);
        set_screen(top);
        draw_text_center(GFX_TOP, BUTTONS_Y_LINE_1, 0.5f, 0.7f, 0.7f, colors[COLOR_YELLOW], conf_msg);
        draw_c2d_text_center(GFX_TOP, BUTTONS_Y_LINE_3, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE], &text[TEXT_CONFIRM_YES_NO]);
        end_frame();

        hidScanInput();
        u32 kDown = hidKeysDown();
        if(kDown & KEY_A) return true;
        if(kDown & KEY_B) return false;
    }

    return false;
}

void draw_preview(C2D_Image preview, int preview_offset)
{
    start_frame();
    set_screen(top);
    C2D_DrawImageAt(preview, -preview_offset, 0, 0.5f, NULL, 1.0f, 1.0f);
    set_screen(bottom);
    C2D_DrawImageAt(preview, -(preview_offset+40), -240, 0.5f, NULL, 1.0f, 1.0f);
}

static void draw_install_handler(InstallType type)
{
    if(type != INSTALL_NONE)
    {
        C2D_Text * install_text = &text[type];
        draw_c2d_text_center(GFX_TOP, 120.0f, 0.5f, 0.8f, 0.8f, colors[COLOR_WHITE], install_text);
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
    set_screen(bottom);
    double percent = 100*((double)current/(double)max);
    u32 width = (u32)percent;
    width *= 2;
    C2D_DrawRectSolid(60-1, 110-1, 0.5f, 200+2, 20+2, colors[COLOR_CURSOR]);
    C2D_DrawRectSolid(60, 110, 0.5f, width, 20, colors[COLOR_ACCENT]);
    end_frame();
}

static void draw_instructions(Instructions_s instructions)
{
    if(instructions.info_line != NULL)
        draw_text_center(GFX_TOP, BUTTONS_Y_INFO, 0.5, 0.55, 0.55, colors[COLOR_WHITE], instructions.info_line);

    const int y_lines[BUTTONS_INFO_LINES-1] = {
        BUTTONS_Y_LINE_1,
        BUTTONS_Y_LINE_2,
        BUTTONS_Y_LINE_3,
    };

    for(int i = 0; i < BUTTONS_INFO_LINES-1; i++)
    {
        if(instructions.instructions[i][0] != NULL)
            draw_text_wrap_scaled(BUTTONS_X_LEFT, y_lines[i], 0.5, colors[COLOR_WHITE], instructions.instructions[i][0], 0.6, 0, BUTTONS_X_RIGHT-2);
        if(instructions.instructions[i][1] != NULL)
            draw_text_wrap_scaled(BUTTONS_X_RIGHT, y_lines[i], 0.5, colors[COLOR_WHITE], instructions.instructions[i][1], 0.6, 0, BUTTONS_X_MAX-2);
    }

    C2D_ImageTint white_tint;
    C2D_PlainImageTint(&white_tint, colors[COLOR_WHITE], 1.0f);

    const char * start_line = instructions.instructions[BUTTONS_INFO_LINES-1][0];
    if(start_line != NULL)
    {
        C2D_SpriteSetPos(&sprite_start, BUTTONS_X_LEFT-10, BUTTONS_Y_LINE_4 + 3);
        C2D_DrawSpriteTinted(&sprite_start, &white_tint);
        draw_text_wrap_scaled(BUTTONS_X_LEFT+26, BUTTONS_Y_LINE_4, 0.5, colors[COLOR_WHITE], start_line, 0.6, 0, BUTTONS_X_RIGHT-2);
    }

    const char * select_line = instructions.instructions[BUTTONS_INFO_LINES-1][1];
    if(select_line != NULL)
    {
        C2D_SpriteSetPos(&sprite_select, BUTTONS_X_RIGHT-10, BUTTONS_Y_LINE_4 + 3);
        C2D_DrawSpriteTinted(&sprite_select, &white_tint);
        draw_text_wrap_scaled(BUTTONS_X_RIGHT+26, BUTTONS_Y_LINE_4, 0.5, colors[COLOR_WHITE], select_line, 0.6, 0, BUTTONS_X_MAX-2);
    }
}

void draw_text_wrap(float x, float y, float z, float scaleX, float scaleY, Color color, const char * text, float max_width)
{
    // sanity check
    if(max_width <= 0)
        return;

    int length = strlen(text) + 1;
    char result[length]; // of note is that, if `text` has no spaces in it and needs to be wrapped, this can and will overflow (!!)
    memset(result, 0, length);
    int idx = 0;

    float current_width = 0;

    while(*text)
    {
        ssize_t consumed;
        u32 codepoint;

        if(*text == '\n')
            current_width = 0;

        if(*text == '\r')
        {
            text++;
            continue;
        }

        if((consumed = decode_utf8(&codepoint, (unsigned char*)text)) == -1)
            break;

        float character_width = scaleX * (fontGetCharWidthInfo(NULL, fontGlyphIndexFromCodePoint(NULL, codepoint))->charWidth);
        if((current_width += character_width) > max_width)
        {
            char* last_space = NULL;
            for(int i = idx; i >= 0; i--)
            {
                if(result[i] == ' ')
                {
                    last_space = &result[i];
                    break;
                }
            }
            if(last_space != NULL)
                *last_space = '\n';
            else
                result[idx++] = '\n';
            current_width = 0;
        }
        memcpy(&result[idx], text, consumed);
        idx += consumed;
        text += consumed;
    }

    draw_text(x, y, z, scaleX, scaleY, color, result);
}

void draw_text_wrap_scaled(float x, float y, float z, Color color, const char * text, float max_scale, float min_scale, float max_width)
{
    // sanity check
    if(max_scale < 0 || min_scale < 0)
        return;

    float width = 0;
    get_text_dimensions(text, max_scale, max_scale, &width, NULL);
    float scale = 0;

    if(width < max_width)
    {
        draw_text(x, y, z, max_scale, max_scale, color, text);
    }
    else if((scale = max_width / width) >= min_scale)
    {
        draw_text(x, y, z, scale, scale, color, text);
    }
    else
    {
        draw_text_wrap(x, y, z, min_scale, min_scale, color, text, max_width);
    }
}

static void draw_entry_info(Entry_s * entry)
{
    char author[0x41] = {0};
    utf16_to_utf8((u8*)author, entry->author, 0x40);
    draw_c2d_text(20, 35, 0.5, 0.5, 0.5, colors[COLOR_WHITE], &text[TEXT_BY_AUTHOR]);
    float width = 0;
    C2D_TextGetDimensions(&text[TEXT_BY_AUTHOR], 0.5, 0.5, &width, NULL);
    draw_text(20+width, 35, 0.5, 0.5, 0.5, colors[COLOR_WHITE], author);

    char title[0x41] = {0};
    utf16_to_utf8((u8*)title, entry->name, 0x40);
    draw_text(20, 50, 0.5, 0.7, 0.7, colors[COLOR_WHITE], title);

    char description[0x81] = {0};
    utf16_to_utf8((u8*)description, entry->desc, 0x80);
    draw_text_wrap(20, 70, 0.5, 0.5, 0.5, colors[COLOR_WHITE], description, 363);
}

void draw_grid_interface(Entry_List_s* list, Instructions_s instructions)
{
    draw_base_interface();
    EntryMode current_mode = list->mode;

    C2D_Text* mode_string[MODE_AMOUNT] = {
        &text[TEXT_THEMEPLAZA_THEME_MODE],
        &text[TEXT_THEMEPLAZA_SPLASH_MODE],
    };

    draw_c2d_text_center(GFX_TOP, 4, 0.5f, 0.5f, 0.5f, colors[COLOR_WHITE], mode_string[current_mode]);

    draw_instructions(instructions);

    int selected_entry = list->selected_entry;
    Entry_s * current_entry = &list->entries[selected_entry];
    draw_entry_info(current_entry);

    set_screen(bottom);

    draw_c2d_text(7, 3, 0.5f, 0.6f, 0.6f, colors[COLOR_WHITE], &text[TEXT_SEARCH]);

    draw_image(sprites_list_idx, 320-96, 0);
    draw_image(sprites_exit_idx, 320-72, 0);
    draw_image(sprites_preview_idx, 320-48, 0);

    draw_text(320-24+2.5, -3, 0.6, 1.0f, 0.9f, colors[COLOR_WHITE], mode_switch_char[!current_mode]);

    draw_image(sprites_arrow_left_idx, 3, 114);
    draw_image(sprites_arrow_right_idx, 308, 114);

    for(int i = list->scroll; i < (list->entries_loaded + list->scroll); i++)
    {
        if(i >= list->entries_count) break;

        current_entry = &list->entries[i];

        char name[0x41] = {0};
        utf16_to_utf8((u8*)name, current_entry->name, 0x40);

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
            C2D_Image * image = list->icons[i];
            C2D_DrawImageAt(*image, horizontal_offset, vertical_offset, 0.5f, NULL, 1.0f, 1.0f);
        }
        else
            C2D_DrawRectSolid(horizontal_offset, vertical_offset, 0.5f, list->entry_size, list->entry_size, current_entry->placeholder_color);

        if(i == selected_entry)
        {
            unsigned int border_width = 3;
            C2D_DrawRectSolid(horizontal_offset, vertical_offset, 0.5f, border_width, list->entry_size, colors[COLOR_CURSOR]);
            C2D_DrawRectSolid(horizontal_offset, vertical_offset, 0.5f, list->entry_size, border_width, colors[COLOR_CURSOR]);
            C2D_DrawRectSolid(horizontal_offset, vertical_offset+list->entry_size-border_width, 0.5f, list->entry_size, border_width, colors[COLOR_CURSOR]);
            C2D_DrawRectSolid(horizontal_offset+list->entry_size-border_width, vertical_offset, 0.5f, border_width, list->entry_size, colors[COLOR_CURSOR]);
        }
    }

    char entries_count_str[0x20] = {0};
    sprintf(entries_count_str, "/%"  JSON_INTEGER_FORMAT, list->tp_page_count);
    float x = 316;
    float width = 0;
    get_text_dimensions(entries_count_str, 0.6, 0.6, &width, NULL);
    x -= width;
    draw_text(x, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], entries_count_str);

    char selected_entry_str[0x20] = {0};
    sprintf(selected_entry_str, "%"  JSON_INTEGER_FORMAT, list->tp_current_page);
    get_text_dimensions(selected_entry_str, 0.6, 0.6, &width, NULL);
    x -= width;
    draw_text(x, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], selected_entry_str);

    draw_c2d_text(176, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], &text[TEXT_PAGE]);
}

void draw_interface(Entry_List_s* list, Instructions_s instructions)
{
    draw_base_interface();
    EntryMode current_mode = list->mode;

    C2D_Text* mode_string[MODE_AMOUNT] = {
        &text[TEXT_THEME_MODE],
        &text[TEXT_SPLASH_MODE],
    };

    draw_c2d_text_center(GFX_TOP, 4, 0.5f, 0.5f, 0.5f, colors[COLOR_WHITE], mode_string[current_mode]);

    if(list->entries == NULL)
    {
        C2D_Text* mode_found_string[MODE_AMOUNT] = {
            &text[TEXT_NO_THEME_FOUND],
            &text[TEXT_NO_SPLASH_FOUND],
        };

        draw_c2d_text_center(GFX_TOP, 80, 0.5f, 0.7f, 0.7f, colors[COLOR_YELLOW], mode_found_string[current_mode]);
        draw_c2d_text_center(GFX_TOP, 110, 0.5f, 0.7f, 0.7f, colors[COLOR_YELLOW], &text[TEXT_DOWNLOAD_FROM_QR]);

        C2D_Text* mode_switch_string[MODE_AMOUNT] = {
            &text[TEXT_SWITCH_TO_SPLASHES],
            &text[TEXT_SWITCH_TO_THEMES],
        };

        draw_c2d_text_center(GFX_TOP, 140, 0.5f, 0.7f, 0.7f, colors[COLOR_YELLOW], mode_switch_string[current_mode]);
        draw_c2d_text_center(GFX_TOP, 170, 0.5f, 0.7f, 0.7f, colors[COLOR_YELLOW], &text[TEXT_OR_START_TO_QUIT]);

        C2D_ImageTint yellow_tint;
        C2D_PlainImageTint(&yellow_tint, colors[COLOR_YELLOW], 1.0f);
        C2D_SpriteSetPos(&sprite_start, 162, 173);
        C2D_SpriteSetScale(&sprite_start, 1.25f, 1.4f);
        C2D_DrawSpriteTinted(&sprite_start, &yellow_tint);
        C2D_SpriteSetScale(&sprite_start, 1.0f, 1.0f);

        set_screen(bottom);

        draw_image(sprites_sort_idx, 320-144, 0);
        draw_image(sprites_download_idx, 320-120, 0);
        draw_image(sprites_browse_idx, 320-96, 0);
        draw_image(sprites_exit_idx, 320-72, 0);
        draw_image(sprites_preview_idx, 320-48, 0);

        draw_text(320-24+2.5, -3, 0.6, 1.0f, 0.9f, colors[COLOR_WHITE], mode_switch_char[!current_mode]);

        return;
    }

    draw_instructions(instructions);

    int selected_entry = list->selected_entry;
    Entry_s * current_entry = &list->entries[selected_entry];
    draw_entry_info(current_entry);

    set_screen(bottom);

    if(current_mode == MODE_THEMES)
    {
        char * shuffle_count_string = NULL;
        asprintf(&shuffle_count_string, "Shuffle: %i/10", list->shuffle_count);
        draw_text(7, 3, 0.6, 0.6, 0.6f, list->shuffle_count <= 10 && list->shuffle_count >= 2 ? colors[COLOR_WHITE] : colors[COLOR_RED], shuffle_count_string);
        free(shuffle_count_string);
    }

    draw_image(sprites_sort_idx, 320-144, 0);
    draw_image(sprites_download_idx, 320-120, 0);
    draw_image(sprites_browse_idx, 320-96, 0);
    draw_image(sprites_exit_idx, 320-72, 0);
    draw_image(sprites_preview_idx, 320-48, 0);

    draw_text(320-24+2.5, -3, 0.6, 1.0f, 0.9f, colors[COLOR_WHITE], mode_switch_char[!current_mode]);

    // Show arrows if there are themes out of bounds
    //----------------------------------------------------------------
    if(list->scroll > 0)
        draw_image(sprites_arrow_up_idx, 152, 4);
    if(list->scroll + list->entries_loaded < list->entries_count)
        draw_image(sprites_arrow_down_idx, 152, 220);

    for(int i = list->scroll; i < (list->entries_loaded + list->scroll); i++)
    {
        if(i >= list->entries_count) break;

        current_entry = &list->entries[i];

        char name[0x41] = {0};
        utf16_to_utf8((u8*)name, current_entry->name, 0x40);

        int vertical_offset = i - list->scroll;
        int horizontal_offset = 0;
        horizontal_offset *= list->entry_size;
        vertical_offset *= list->entry_size;
        vertical_offset += 24;

        u32 font_color = colors[COLOR_WHITE];

        if(i == selected_entry)
        {
            font_color = colors[COLOR_BLACK];
            C2D_DrawRectSolid(0, vertical_offset, 0.5f, 320, list->entry_size, colors[COLOR_CURSOR]);
        }

        draw_text(list->entry_size+6, vertical_offset + 16, 0.5f, 0.55, 0.55, font_color, name);

        C2D_ImageTint tint;
        C2D_PlainImageTint(&tint, font_color, 1.0f);

        if(current_entry->no_bgm_shuffle)
        {
            C2D_SpriteSetPos(&sprite_shuffle_no_bgm, 320-24-4, vertical_offset);
            C2D_DrawSpriteTinted(&sprite_shuffle_no_bgm, &tint);
        }
        else if(current_entry->in_shuffle)
        {
            C2D_SpriteSetPos(&sprite_shuffle, 320-24-4, vertical_offset);
            C2D_DrawSpriteTinted(&sprite_shuffle, &tint);
        }

        if(current_entry->installed)
        {
            C2D_SpriteSetPos(&sprite_installed, 320-24-4, vertical_offset + 22);
            C2D_DrawSpriteTinted(&sprite_installed, &tint);
        }

        if(!current_entry->placeholder_color)
        {
            C2D_Image * image = NULL;
            if(list->entries_count > list->entries_loaded*ICONS_OFFSET_AMOUNT)
                image = list->icons[ICONS_VISIBLE*list->entries_loaded + (i - list->scroll)];
            else
                image = list->icons[i];
            C2D_DrawImageAt(*image, horizontal_offset, vertical_offset, 0.5f, NULL, 1.0f, 1.0f);
        }
        else
        {
            C2D_DrawRectSolid(horizontal_offset, vertical_offset, 0.5f, list->entry_size, list->entry_size, current_entry->placeholder_color);
        }
    }

    char entries_count_str[0x20] = {0};
    sprintf(entries_count_str, "/%i", list->entries_count);
    float x = 316;
    float width = 0;
    get_text_dimensions(entries_count_str, 0.6, 0.6, &width, NULL);
    x -= width;
    draw_text(x, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], entries_count_str);

    char selected_entry_str[0x20] = {0};
    sprintf(selected_entry_str, "%i", selected_entry + 1);
    get_text_dimensions(selected_entry_str, 0.6, 0.6, &width, NULL);
    x -= width;
    draw_text(x, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], selected_entry_str);

    if(list->entries_count < 10000)
        draw_c2d_text(176, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], &text[TEXT_SELECTED]);
    else
        draw_c2d_text(176, 219, 0.5, 0.6, 0.6, colors[COLOR_WHITE], &text[TEXT_SELECTED_SHORT]);
}
