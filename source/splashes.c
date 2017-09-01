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

Result get_splashes(Splash_s** splashes_list, int *splash_count)
{
    Result res = 0;
    Handle dir_handle;
    res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, SPLASHES_PATH));
    if (R_FAILED(res))
        return res;

    u32 entries_read = 1;
    while (entries_read)
    {
        FS_DirectoryEntry entry = {0};
        res = FSDIR_Read(dir_handle, &entries_read, 1, &entry);
        if (R_FAILED(res) || entries_read == 0) 
            break;

        if (!(entry.attributes & FS_ATTRIBUTE_DIRECTORY))
            continue;

        *splash_count += entries_read;
        *splashes_list= realloc(*splashes_list, (*splash_count) * sizeof(Splash_s));
        if (splashes_list == NULL)
            break;

        Splash_s *current_splash = &(*splashes_list)[*splash_count-1];
        memset(current_splash, 0, sizeof(Splash_s));

        u16 splash_path[0x106] = {0};
        struacat(splash_path, SPLASHES_PATH);
        strucat(splash_path, entry.name);
        u16 top_path[0x106] = {0};
        memcpy(top_path, splash_path, 0x106);
        struacat(top_path, "/splash.bin");
        u16 bottom_path[0x106] = {0};
        memcpy(bottom_path, splash_path, 0x106);
        struacat(bottom_path, "/splashbottom.bin");

        char *temp_buf = NULL;
        u32 bytes = file_to_buf(fsMakePath(PATH_UTF16, top_path), ArchiveSD, &temp_buf);
        if (!bytes)
        {
            memset(top_path, 0, 0x106);
        }
        free(temp_buf);
        temp_buf = NULL;
        bytes = file_to_buf(fsMakePath(PATH_UTF16, bottom_path), ArchiveSD, &temp_buf);
        if (!bytes)
        {
            memset(bottom_path, 0, 0x106);
        }
        free(temp_buf);
        memcpy(current_splash->name, entry.name, 0x106);
        memcpy(current_splash->top_path, top_path, 0x106);
        memcpy(current_splash->bottom_path, bottom_path, 0x106);
    }
    FSDIR_Close(dir_handle);

    return res;
}

void splash_install(Splash_s splash_to_install)
{
    char *screen_buf;
    u32 size = file_to_buf(fsMakePath(PATH_UTF16, splash_to_install.top_path), ArchiveSD, &screen_buf);
    if (size)
    {
        buf_to_file(size, "/luma/splash.bin", ArchiveSD, screen_buf);
        free(screen_buf);
    }

    size = file_to_buf(fsMakePath(PATH_UTF16, splash_to_install.bottom_path), ArchiveSD, &screen_buf);
    if (size)
    {
        buf_to_file(size, "/luma/splashbottom.bin", ArchiveSD, screen_buf);
        free(screen_buf);
    }
}