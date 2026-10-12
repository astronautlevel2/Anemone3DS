/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
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
#include "ui_strings.h"

void splash_delete(void)
{
    remove("/luma/splash.bin");
    remove("/luma/splashbottom.bin");
}

void splash_install(const Entry_s * splash, SplashInstallType install_type)
{
    char *screen_buf = NULL;
    bool installed_any = false;

    if(install_type != SPLASH_INSTALL_BOTTOM)
    {
        u32 size = load_data("/splash.bin", splash, &screen_buf);
        if(size != 0)
        {
            installed_any = true;
            remake_file(fsMakePath(PATH_ASCII, "/luma/splash.bin"), ArchiveSD, size);
            buf_to_file(size, fsMakePath(PATH_ASCII, "/luma/splash.bin"), ArchiveSD, screen_buf);
        }
        free(screen_buf);
        screen_buf = NULL;
    }

    if(install_type != SPLASH_INSTALL_TOP)
    {
        u32 bottom_size = load_data("/splashbottom.bin", splash, &screen_buf);
        if(bottom_size != 0)
        {
            installed_any = true;
            remake_file(fsMakePath(PATH_ASCII, "/luma/splashbottom.bin"), ArchiveSD, bottom_size);
            buf_to_file(bottom_size, fsMakePath(PATH_ASCII, "/luma/splashbottom.bin"), ArchiveSD, screen_buf);
        }
        free(screen_buf);
        screen_buf = NULL;
    }

    if(!installed_any)
    {
        throw_error(language.splashes.no_splash_found, ERROR_LEVEL_WARNING);
    }
    else
    {
        char *config_buf = NULL;
        u32 size = file_to_buf(fsMakePath(PATH_ASCII, "/luma/config.bin"), ArchiveSD, &config_buf);
        if(size && config_buf[0xC] == 0)
        {
            throw_error(language.splashes.splash_disabled, ERROR_LEVEL_WARNING);
        }
        free(config_buf);
    }
}

void splash_check_installed(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    Entry_List_s * list = (Entry_List_s *)arg->thread_arg;
    if(list == NULL || list->entries == NULL) return;

    #ifndef CITRA_MODE
    // this also runs again after a top or bottom only install, so start from a clean state
    for(int i = 0; i < list->entries_count; i++)
        list->entries[i].installed = false;

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

    const bool has_top = top_size != 0;
    const bool has_bottom = bottom_size != 0;

    #define HASH_SIZE_BYTES 256/8
    u8 top_hash[HASH_SIZE_BYTES] = {0};
    FSUSER_UpdateSha256Context(top_buf, top_size, top_hash);
    free(top_buf);
    top_buf = NULL;
    u8 bottom_hash[HASH_SIZE_BYTES] = {0};
    FSUSER_UpdateSha256Context(bottom_buf, bottom_size, bottom_hash);
    free(bottom_buf);
    bottom_buf = NULL;

    // with top or bottom only installs, each screen can come from a different splash
    int top_match = -1;
    int bottom_match = -1;

    for(int i = 0; i < list->entries_count && arg->run_thread; i++)
    {
        Entry_s * splash = &list->entries[i];
        top_size = load_data("/splash.bin", splash, &top_buf);
        bottom_size = load_data("/splashbottom.bin", splash, &bottom_buf);

        if(!top_size && !bottom_size)
        {
            free(top_buf);
            free(bottom_buf);
            top_buf = NULL;
            bottom_buf = NULL;
            continue;
        }

        const bool splash_has_top = top_size != 0;
        const bool splash_has_bottom = bottom_size != 0;

        u8 splash_top_hash[HASH_SIZE_BYTES] = {0};
        FSUSER_UpdateSha256Context(top_buf, top_size, splash_top_hash);
        free(top_buf);
        top_buf = NULL;
        u8 splash_bottom_hash[HASH_SIZE_BYTES] = {0};
        FSUSER_UpdateSha256Context(bottom_buf, bottom_size, splash_bottom_hash);
        free(bottom_buf);
        bottom_buf = NULL;

        const bool same_top = !memcmp(splash_top_hash, top_hash, HASH_SIZE_BYTES);
        const bool same_bottom = !memcmp(splash_bottom_hash, bottom_hash, HASH_SIZE_BYTES);

        if(same_top && same_bottom)
        {
            splash->installed = true;
            return;
        }

        if(same_top && has_top && splash_has_top && top_match < 0)
            top_match = i;
        if(same_bottom && has_bottom && splash_has_bottom && bottom_match < 0)
            bottom_match = i;
    }
    #undef HASH_SIZE_BYTES

    if(!arg->run_thread)
        return;

    if(top_match >= 0)
        list->entries[top_match].installed = true;
    if(bottom_match >= 0)
        list->entries[bottom_match].installed = true;
    #endif
}
