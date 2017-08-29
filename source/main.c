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

#include "fs.h"
#include "themes.h"
#include "unicode.h"

int init_services(void)
{
    gfxInitDefault();
    cfguInit();
    srvInit();  
    hidInit();
    fsInit();   
    ptmSysmInit();
    open_archives();
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

int main(void)
{
    init_services();
    consoleInit(GFX_TOP, NULL);

    int theme_count = get_number_entries("/Themes");
    printf("Theme count: %i\n", theme_count);
    theme **themes_list = calloc(theme_count, sizeof(theme));
    scan_themes(themes_list, theme_count);
    
    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A)
        {
            for (int i = 0; i < theme_count; i++)
            {
                printu(themes_list[i]->name);
                printu(themes_list[i]->path);
            }
        } 
        if (kDown & KEY_B)
        {
            shuffle_install(themes_list, theme_count);
            close_archives();
            PTMSYSM_ShutdownAsync(0);
            ptmSysmExit();
        }
        if (kDown & KEY_START)
        {
            close_archives();   
            PTMSYSM_ShutdownAsync(0);
            ptmSysmExit();
        }
    }

    de_init_services();
    return 0;
}
