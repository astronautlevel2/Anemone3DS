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
    if(size != 0)
    {
        remake_file(fsMakePath(PATH_ASCII, "/luma/splash.bin"), ArchiveSD, size);
        buf_to_file(size, fsMakePath(PATH_ASCII, "/luma/splash.bin"), ArchiveSD, screen_buf);
    }

    u32 bottom_size = load_data("/splashbottom.bin", splash, &screen_buf);
    if(bottom_size != 0)
    {
        remake_file(fsMakePath(PATH_ASCII, "/luma/splashbottom.bin"), ArchiveSD, bottom_size);
        buf_to_file(bottom_size, fsMakePath(PATH_ASCII, "/luma/splashbottom.bin"), ArchiveSD, screen_buf);
    }

    if(size == 0 && bottom_size == 0)
    {
        throw_error("No splash.bin or splashbottom.bin found.\nIs this a splash?", ERROR_LEVEL_WARNING);
    }
    else
    {
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
}

void splash_check_installed(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    Entry_List_s * list = (Entry_List_s *)arg->thread_arg;
    if(list == NULL || list->entries == NULL) return;

    #ifndef CITRA_MODE
    char * top_buf = NULL;
    u32 top_size = file_to_buf(fsMakePath(PATH_ASCII, "/luma/splash.bin"), ArchiveSD, &top_buf);
    char * bottom_buf = NULL;
    u32 bottom_size = file_to_buf(fsMakePath(PATH_ASCII, "/luma/splashbottom.bin"), ArchiveSD, &bottom_buf);

    if(!top_size && !bottom_size)
    {
        free(top_buf);
        free(bottom_buf);
        return;
    }

    #define HASH_SIZE_BYTES 256/8
    u8 top_hash[HASH_SIZE_BYTES] = {0};
    FSUSER_UpdateSha256Context(top_buf, top_size, top_hash);
    free(top_buf);
    top_buf = NULL;
    u8 bottom_hash[HASH_SIZE_BYTES] = {0};
    FSUSER_UpdateSha256Context(bottom_buf, bottom_size, bottom_hash);
    free(bottom_buf);
    bottom_buf = NULL;

    for(int i = 0; i < list->entries_count && arg->run_thread; i++)
    {
        Entry_s * splash = &list->entries[i];
        top_size = load_data("/splash.bin", *splash, &top_buf);
        bottom_size = load_data("/splashbottom.bin", *splash, &bottom_buf);

        if(!top_size && !bottom_size)
        {
            continue;
        }

        u8 splash_top_hash[HASH_SIZE_BYTES] = {0};
        FSUSER_UpdateSha256Context(top_buf, top_size, splash_top_hash);
        free(top_buf);
        top_buf = NULL;
        u8 splash_bottom_hash[HASH_SIZE_BYTES] = {0};
        FSUSER_UpdateSha256Context(bottom_buf, bottom_size, splash_bottom_hash);
        free(bottom_buf);
        bottom_buf = NULL;

        if(!memcmp(splash_bottom_hash, bottom_hash, HASH_SIZE_BYTES) && !memcmp(splash_top_hash, top_hash, HASH_SIZE_BYTES))
        {
            splash->installed = true;
            break;
        }
    }
    #endif
}