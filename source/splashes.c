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
#include "splashes.h"
#include "unicode.h"
#include "fs.h"
#include "draw.h"

void splash_delete(void) 
{
    remove("/luma/splash.bin");
    remove("/luma/splashbottom.bin");
}

void splash_install(Entry_s splash)
{
    char *screen_buf = NULL;

    u32 size = load_data("/splash.bin", splash, &screen_buf);
    remake_file("/luma/splash.bin", ArchiveSD, size);
    buf_to_file(size, "/luma/splash.bin", ArchiveSD, screen_buf);

    size = load_data("/splashbottom.bin", splash, &screen_buf);
    remake_file("/luma/splashbottom.bin", ArchiveSD, size);
    buf_to_file(size, "/luma/splashbottom.bin", ArchiveSD, screen_buf);

    char *config_buf;
    size = file_to_buf(fsMakePath(PATH_ASCII, "/luma/config.bin"), ArchiveSD, &config_buf);
    if(size)
    {
        if(config_buf[0xC] == 0)
        {
            free(config_buf);
            throw_error("WARNING: Splashes are disabled in Luma Config", ERROR_LEVEL_WARNING);
        }
    }
}