/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2024 Contributors in CONTRIBUTORS.md
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

#include "config.h"

Config_s config;

void load_config(void)
{
    Handle test_handle;
    Result res;
    memset(&config, 0, sizeof(Config_s));
    char *json_buf = NULL;
    u32 json_len = file_to_buf(fsMakePath(PATH_ASCII, "/3ds/" APP_TITLE "/config.json"), ArchiveSD, &json_buf);
    if (json_len)
    {
        json_error_t error;
        json_t *root = json_loadb(json_buf, json_len, 0, &error);
        if (root)
        {
            const char *key;
            json_t *value;
            json_object_foreach(root, key, value)
            {
                if (json_is_array(value) && !strcmp(key, "Accent Color") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.accent_color = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "Background Color") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.background_color = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "White Color Background") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.white_color_background = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "White Color Accent") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.white_color_accent = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "Cursor Color") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.cursor_color = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "Black Color") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.black_color = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "Red Color Background") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.red_color_background = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "Red Color Accent") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.red_color_accent = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_array(value) && !strcmp(key, "Yellow Color") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.yellow_color = C2D_Color32(r, g, b, a);
                    }
                }
                else if (json_is_string(value) && !strcmp(key, "Themes Path"))
                {
                    bool need_slash = json_string_value(value)[strlen(json_string_value(value)) - 1] != '/';
                    char *theme_path = calloc(1, strlen(json_string_value(value)) + 1 + (need_slash ? 1 : 0));
                    memcpy(theme_path, json_string_value(value), strlen(json_string_value(value)));
                    if (need_slash) theme_path[strlen(json_string_value(value))] = '/';
                    if (R_SUCCEEDED(res = FSUSER_OpenDirectory(&test_handle, ArchiveSD, fsMakePath(PATH_ASCII, theme_path))))
                    {
                        main_paths[REMOTE_MODE_THEMES] = theme_path;
                        FSDIR_Close(test_handle);
                    } else
                    {
                        DEBUG("Failed test - reverting to default. Err 0x%08lx\n", res);
                        free(theme_path);
                    }
                }
                else if (json_is_string(value) && !strcmp(key, "Splashes Path"))
                {
                    bool need_slash = json_string_value(value)[strlen(json_string_value(value)) - 1] != '/';
                    char *splash_path = calloc(1, strlen(json_string_value(value)) + 1 + (need_slash ? 1 : 0));
                    memcpy(splash_path, json_string_value(value), strlen(json_string_value(value)));
                    if (need_slash) splash_path[strlen(json_string_value(value))] = '/';
                    if (R_SUCCEEDED(res = FSUSER_OpenDirectory(&test_handle, ArchiveSD, fsMakePath(PATH_ASCII, splash_path))))
                    {
                        main_paths[REMOTE_MODE_SPLASHES] = splash_path;
                        FSDIR_Close(test_handle);
                    } else
                    {
                        DEBUG("Failed test - reverting to default. Err 0x%08lx\n", res);
                        free(splash_path);
                    }
                }
                else if (json_is_string(value) && !strcmp(key, "Badges Path"))
                {
                    bool need_slash = json_string_value(value)[strlen(json_string_value(value)) - 1] != '/';
                    char *badge_path = calloc(1, strlen(json_string_value(value)) + 1 + (need_slash ? 1 : 0));
                    memcpy(badge_path, json_string_value(value), strlen(json_string_value(value)));
                    if (need_slash) badge_path[strlen(json_string_value(value))] = '/';
                    if (R_SUCCEEDED(res = FSUSER_OpenDirectory(&test_handle, ArchiveSD, fsMakePath(PATH_ASCII, badge_path))))
                    {
                        main_paths[REMOTE_MODE_BADGES] = badge_path;
                        FSDIR_Close(test_handle);
                    } else
                    {
                        DEBUG("Failed test - reverting to default. Err 0x%08lx\n", res);
                        free(badge_path);
                    }
                }
            }
        } else
        {
            DEBUG(error.text);
        }
    }
    if (config.accent_color == 0)
        config.accent_color = C2D_Color32(12, 58, 111, 255);

    if (config.background_color == 0)
        config.background_color = C2D_Color32(35, 28, 32, 255); //silver-y black

    if (config.white_color_background == 0)
        config.white_color_background = C2D_Color32(255, 255, 255, 255);

    if (config.white_color_accent == 0)
        config.white_color_accent = C2D_Color32(255, 255, 255, 255);

    if (config.cursor_color == 0)
        config.cursor_color = C2D_Color32(200, 200, 200, 255);

    if (config.black_color == 0)
        config.black_color = C2D_Color32(0, 0, 0, 255);

    if (config.red_color_background == 0)
        config.red_color_background = C2D_Color32(229, 66, 66, 255);
    
    if (config.red_color_accent == 0)
        config.red_color_accent = C2D_Color32(229, 66, 66, 255);

    if (config.yellow_color == 0)
        config.yellow_color = C2D_Color32(239, 220, 11, 255);

    if (json_buf) free(json_buf);
}