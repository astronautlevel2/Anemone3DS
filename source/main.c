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

#include "fs.h"
#include "themes.h"
#include "splashes.h"
#include "draw.h"
#include "common.h"

int init_services(void)
{
    cfguInit();
    ptmuInit();
    open_archives();
    return 0;
}

int exit_services(void)
{
    close_archives();
    cfguExit();
    ptmuExit();
    return 0;
}

int main(void)
{
    init_services();
    init_screens();

    
    int theme_count = 0;
    Theme_s * themes_list = NULL;
    Result res = get_themes(&themes_list, &theme_count);
    if (R_FAILED(res))
    {
        //don't need to worry about possible textures (icons, previews), that's freed by pp2d itself
        free(themes_list);
        themes_list = NULL;
    }
    int splash_count = 0;
    Splash_s *splashes_list = NULL;
    res = get_splashes(&splashes_list, &splash_count);
    if (R_FAILED(res))
    {
        //don't need to worry about possible textures (icons, previews), that's freed by pp2d itself
        free(splashes_list);
        splashes_list = NULL;
    }

    bool splash_mode = false;
    int selected_splash = 0;
    int selected_theme = 0;
    int previously_selected = 0;
    int shuffle_theme_count = 0;
    bool preview_mode = false;
    
    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        if (!splash_mode) draw_theme_interface(themes_list, theme_count, selected_theme, preview_mode);
        else draw_splash_interface(splashes_list, splash_count, selected_splash, preview_mode);
        
        if (kDown & KEY_START)
        {
            APT_HardwareResetAsync();
        }
        else if (kDown & KEY_L)
        {
            splash_mode = !splash_mode;
        }

        if (themes_list == NULL && !splash_mode)
            continue;
        
        if (splashes_list == NULL && splash_mode)
            continue;

        Theme_s * current_theme = &themes_list[selected_theme];
        Splash_s *current_splash = &splashes_list[selected_splash];
        
        if (kDown & KEY_Y)
        {
            if (!preview_mode)
            {
                if (!current_theme->has_preview)
                    load_theme_preview(current_theme);
                
                preview_mode = current_theme->has_preview;
            }
            else
                preview_mode = false;
        }

        //don't allow anything while the preview is up
        if (preview_mode)
            continue;
        
        // Actions
        else if (kDown & KEY_X)
        {
			if (splash_mode) {
				splash_delete();
			} else {
				draw_theme_install(BGM_INSTALL);
				bgm_install(*current_theme);
			}
        }
        else if (kDown & KEY_A)
        {
            if (splash_mode)
            {
                draw_splash_install();
                splash_install(*current_splash);
                svcSleepThread(5e8);
            } else {
                draw_theme_install(SINGLE_INSTALL);
                single_install(*current_theme);
            }
        }
        
        else if (kDown & KEY_B)
        {
            if (splash_mode)
            {

            } else {
                if (shuffle_theme_count < 10)
                {
                    if (current_theme->in_shuffle) shuffle_theme_count--;
                    else shuffle_theme_count++;
                    current_theme->in_shuffle = !(current_theme->in_shuffle);
                } else {
                    if (current_theme->in_shuffle) {
                        shuffle_theme_count--;
                        current_theme->in_shuffle = false;
                    } 
                }
            }
        }

        else if (kDown & KEY_SELECT)
        {
            if (splash_mode)
            {

            } else {
                if (shuffle_theme_count > 0)
                {
                    draw_theme_install(SHUFFLE_INSTALL);
                    shuffle_install(themes_list, theme_count);
                }
            }
        }

        // Movement in the UI
        else if (kDown & KEY_DOWN) 
        {
            if (splash_mode)
            {
                selected_splash++;
                if (selected_splash >= splash_count)
                    selected_splash = 0;
            } else {
                selected_theme++;
                if (selected_theme >= theme_count)
                    selected_theme = 0;
            }
        }
        else if (kDown & KEY_UP)
        {
            if (splash_mode)
            {
                selected_splash--;
                if (selected_splash < 0)
                    selected_splash = splash_count - 1;
            } else {
                selected_theme--;
                if (selected_theme < 0)
                    selected_theme = theme_count - 1;
            }
        }
        // Quick moving
        else if (kDown & KEY_LEFT) 
        {
            if (splash_mode) 
            {
                selected_splash -= 4;
                if (selected_splash < 0) selected_splash = 0;
            } else {
                selected_theme -= 4;
                if (selected_theme < 0) selected_theme = 0;
            }
        }
        else if (kDown & KEY_RIGHT)
        {
            if (splash_mode) 
            {
                selected_splash += 4;
                if (selected_splash >= splash_count) selected_splash = splash_count-1;
            } else {
                selected_theme += 4;
                if (selected_theme >= theme_count) selected_theme = theme_count-1;
            }
        }
        
        if (!splash_mode && selected_theme != previously_selected)
        {
            current_theme->has_preview = false;
            previously_selected = selected_theme;
        }
    }
}
