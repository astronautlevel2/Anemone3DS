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
#include "draw.h"

int init_services(void)
{
    cfguInit();
    open_archives();
    return 0;
}

int exit_services(void)
{
    close_archives();
    cfguExit();
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
    
    int selected_theme = 0;
    bool preview_mode = false;
    
    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        draw_interface(themes_list, theme_count, selected_theme, preview_mode);
        
        if (kDown & KEY_START)
            break; //quit
        
        if (themes_list == NULL)
            continue;
        
        Theme_s * current_theme = &themes_list[selected_theme];
        
        if (kDown & KEY_Y)
        {
            if (!preview_mode)
            {
                if (current_theme->has_preview)
                    preview_mode = true;
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
            // install_bgm(current_theme);
        }
        else if (kDown & KEY_A)
        {
            single_install(*current_theme);
        }
        else if (kDown & KEY_B)
        {
            current_theme->in_shuffle = !(current_theme->in_shuffle);
        }
        else if (kDown & KEY_SELECT)
        {
            shuffle_install(themes_list, theme_count);
        }

        // Movement in the UI
        else if (kDown & KEY_DOWN) 
        {
            selected_theme++;
            if (selected_theme >= theme_count)
                selected_theme = theme_count-1;
        }
        else if (kDown & KEY_UP)
        {
            selected_theme--;
            if (selected_theme < 0)
                selected_theme = 0;
        }
        // Quick moving
        else if (kDown & KEY_LEFT) 
        {
            selected_theme = 0;
        }
        else if (kDown & KEY_RIGHT)
        {
            selected_theme = theme_count-1;
        }
    }
    
    free(themes_list);
    
    exit_screens();
    exit_services();
    
    ptmSysmInit();
    PTMSYSM_ShutdownAsync(0);
    ptmSysmExit();
    
    return 0;
}
