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
                else if (json_is_array(value) && !strcmp(key, "Red Color") && json_array_size(value) == 4)
                {
                    if (json_is_integer(json_array_get(value, 0)) && json_is_integer(json_array_get(value, 1))
                        && json_is_integer(json_array_get(value, 2)) && json_is_integer(json_array_get(value, 3)))
                    {
                        u8 r = min(255, json_integer_value(json_array_get(value, 0)));
                        u8 g = min(255, json_integer_value(json_array_get(value, 1)));
                        u8 b = min(255, json_integer_value(json_array_get(value, 2)));
                        u8 a = min(255, json_integer_value(json_array_get(value, 3)));

                        config.red_color = C2D_Color32(r, g, b, a);
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

    if (config.red_color == 0)
        config.red_color = C2D_Color32(229, 66, 66, 255);
    
    if (config.yellow_color == 0)
        config.yellow_color = C2D_Color32(239, 220, 11, 255);

    if (json_buf) free(json_buf);
}