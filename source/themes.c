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

#include "themes.h"
#include "unicode.h"
#include "fs.h"
#include "draw.h"

#define BGM_MAX_SIZE 3371008

void delete_theme(Entry_s theme)
{
    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_UTF16, theme.path));
    if(R_SUCCEEDED(res))
    {
        FSDIR_Close(dir_handle);
        FSUSER_DeleteDirectoryRecursively(ArchiveSD, fsMakePath(PATH_UTF16, theme.path));
    } else 
    {
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_UTF16, theme.path));
    }
}

Result bgm_install(Entry_s bgm_to_install)
{
    char *savedata_buf;
    char *thememanage_buf;
    char *music;
    u32 music_size = 0;
    u32 savedata_size;

    savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    savedata_buf[0x141b] = 0;
    memset(&savedata_buf[0x13b8], 0, 8);
    savedata_buf[0x13bd] = 3;
    savedata_buf[0x13b8] = 0xff;
    Result result = buf_to_file(savedata_size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    if(R_FAILED(result)) return result;

    music_size = load_data("/bgm.bcstm", bgm_to_install, &music);

    if(music_size == 0)
    {
        music = calloc(1, BGM_MAX_SIZE);
    } else if(music_size > BGM_MAX_SIZE) {
        free(music);
        puts("musicrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
    }

    result = buf_to_file(music_size == 0 ? BGM_MAX_SIZE : music_size, "/BgmCache.bin", ArchiveThemeExt, music);
    free(music);

    if(R_FAILED(result)) return result;

    file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, &thememanage_buf);
    thememanage_buf[0x00] = 1;
    thememanage_buf[0x01] = 0;
    thememanage_buf[0x02] = 0;
    thememanage_buf[0x03] = 0;
    thememanage_buf[0x04] = 0;
    thememanage_buf[0x05] = 0;
    thememanage_buf[0x06] = 0;
    thememanage_buf[0x07] = 0;

    u32 *music_size_location = (u32*)(&thememanage_buf[0xC]);
    *music_size_location = music_size;

    thememanage_buf[0x10] = 0xFF;
    thememanage_buf[0x14] = 0x01;
    thememanage_buf[0x18] = 0xFF;
    thememanage_buf[0x1D] = 0x02;

    memset(&thememanage_buf[0x338], 0, 4);
    memset(&thememanage_buf[0x340], 0, 4);
    memset(&thememanage_buf[0x360], 0, 4);
    memset(&thememanage_buf[0x368], 0, 4);
    result = buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    if(R_FAILED(result)) return result;

    return 0;
}

// Install a single theme
Result theme_install(Entry_s theme)
{
    char *body = NULL;
    char *music = NULL;
    char *savedata_buf = NULL;
    char *thememanage_buf = NULL;
    u32 body_size = 0;
    u32 music_size = 0;
    u32 savedata_size = 0;

    savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    DEBUGPOS("savedata: %p, %lx\n", savedata_buf, savedata_size);
    savedata_buf[0x141b] = 0;
    memset(&savedata_buf[0x13b8], 0, 8);
    savedata_buf[0x13bd] = 3;
    savedata_buf[0x13b8] = 0xff;
    Result result = buf_to_file(savedata_size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    if(R_FAILED(result)) return result;

    // Open body cache file.
    body_size = load_data("/body_LZ.bin", theme, &body);

    if(body_size == 0)
    {
        free(body);
        DEBUGPOS("bodyrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
    }

    result = buf_to_file(body_size, "/BodyCache.bin", ArchiveThemeExt, body); // Write body data to file
    free(body);

    if(R_FAILED(result)) return result;

    music_size = load_data("/bgm.bcstm", theme, &music);

    if(music_size == 0)
    {
        music = calloc(1, BGM_MAX_SIZE);
    } else if(music_size > BGM_MAX_SIZE) {
        free(music);
        DEBUGPOS("musicrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
    }

    result = buf_to_file(music_size == 0 ? BGM_MAX_SIZE : music_size, "/BgmCache.bin", ArchiveThemeExt, music);
    free(music);

    if(R_FAILED(result)) return result;

    file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, &thememanage_buf);
    thememanage_buf[0x00] = 1;
    thememanage_buf[0x01] = 0;
    thememanage_buf[0x02] = 0;
    thememanage_buf[0x03] = 0;
    thememanage_buf[0x04] = 0;
    thememanage_buf[0x05] = 0;
    thememanage_buf[0x06] = 0;
    thememanage_buf[0x07] = 0;

    u32 *body_size_location = (u32*)(&thememanage_buf[0x8]);
    u32 *music_size_location = (u32*)(&thememanage_buf[0xC]);
    *body_size_location = body_size;
    *music_size_location = music_size;

    thememanage_buf[0x10] = 0xFF;
    thememanage_buf[0x14] = 0x01;
    thememanage_buf[0x18] = 0xFF;
    thememanage_buf[0x1D] = 0x02;

    memset(&thememanage_buf[0x338], 0, 4);
    memset(&thememanage_buf[0x340], 0, 4);
    memset(&thememanage_buf[0x360], 0, 4);
    memset(&thememanage_buf[0x368], 0, 4);
    result = buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    if(R_FAILED(result)) return result;

    return 0;
}

Result shuffle_install(Entry_s* themes_list, int themes_count)
{
    u8 count = 0;
    Entry_s *shuffle_themes[10] = {0};
    u32 body_sizes[10] = {0};
    u32 bgm_sizes[10] = {0};
    for (int i = 0; i < themes_count; i++)
    {
        if (count > 10) return MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_COMMON, RD_INVALID_SELECTION);
        if (themes_list[i].in_shuffle)
        {
            shuffle_themes[count++] = &themes_list[i];
            themes_list[i].in_shuffle = false;
        }
    }
    for (int i = 0; i < count; i++)
    {
        printu(shuffle_themes[i]->name);
    }

    char *savedata_buf;
    u32 size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    if (size == 0)
    {
        return MAKERESULT(RL_USAGE, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
    }

    savedata_buf[0x141b] = 1;
    memset(&savedata_buf[0x13b8], 0, 8);
    savedata_buf[0x13bd] = 3;
    savedata_buf[0x13b8] = 0xff;

    for(int i = 0; i < 10; i++)
    {
        memset(&savedata_buf[0x13c0 + 0x8 * i], 0, 8); // clear any existing theme structure. 8 is the length of the theme structure, so 8 * i is the pos of the current one
        if(count > i) // if we are still installing themes...
        {
            savedata_buf[0x13c0 + (8 * i)] = i; // index
            savedata_buf[0x13c0 + (8 * i) + 5] = 3; // persistence (?)
        }
    }

    Result result = buf_to_file(size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    if(R_FAILED(result)) return result;

    remake_file("/BodyCache_rd.bin", ArchiveThemeExt, 0x150000 * 10); // Enough space for 10 theme files
    Handle body_cache_handle;
    FSUSER_OpenFile(&body_cache_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BodyCache_rd.bin"), FS_OPEN_WRITE, 0);
    for(int i = 0; i < count; i++)
    {
        Entry_s * current_theme = shuffle_themes[i];
        char * body_buf = NULL;
        u32 body_size = load_data("/body_LZ.bin", *current_theme, &body_buf);

        body_sizes[i] = body_size;
        FSFILE_Write(body_cache_handle, NULL, 0x150000 * i, body_buf, body_size, FS_WRITE_FLUSH);
        free(body_buf);
    }

    FSFILE_Close(body_cache_handle);

    for(int i = 0; i < 10; i++)
    {
        char bgm_cache_path[17] = {0};
        sprintf(bgm_cache_path, "/BgmCache_%.2i.bin", i);
        remake_file(bgm_cache_path, ArchiveThemeExt, BGM_MAX_SIZE);
        if(count > i)
        {
            Entry_s * current_theme = shuffle_themes[i];
            char *music_buf = NULL;
            u32 music_size = music_size = load_data("/bgm.bcstm", *current_theme, &music_buf);

            if(!music_size)
            {
                char *empty = calloc(1, BGM_MAX_SIZE);
                buf_to_file(BGM_MAX_SIZE, bgm_cache_path, ArchiveThemeExt, empty);
                bgm_sizes[i] = 0;
                free(empty);
                continue;
            }
            bgm_sizes[i] = music_size;
            buf_to_file(music_size, bgm_cache_path, ArchiveThemeExt, music_buf);
            free(music_buf);
        } else {
            char *empty = calloc(1, BGM_MAX_SIZE);
            buf_to_file(BGM_MAX_SIZE, bgm_cache_path, ArchiveThemeExt, empty);
            bgm_sizes[i] = 0;
            free(empty);
        }
    }

    char *thememanage_buf;
    file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, &thememanage_buf);
    thememanage_buf[0x00] = 1; // Unknown, normally 0x1 with size of 4
    thememanage_buf[0x01] = 0;
    thememanage_buf[0x02] = 0;
    thememanage_buf[0x03] = 0;
    thememanage_buf[0x04] = 0; // Unknown, normally 0 with size of 4
    thememanage_buf[0x05] = 0;
    thememanage_buf[0x06] = 0;
    thememanage_buf[0x07] = 0;
    thememanage_buf[0x10] = 0xFF; // Unknown
    thememanage_buf[0x14] = 0x01; // Unkown
    thememanage_buf[0x18] = 0xFF; // DLC theme index - 0xFF indicates no DLC theme
    thememanage_buf[0x1D] = 0x02; // Unknown, usually 0x200 to indicate theme-cache is being used

    u32 *bodysizeloc = (u32*) (&thememanage_buf[0x08]); // Set non-shuffle theme sizes to 0
    u32 *bgmsizeloc = (u32*) (&thememanage_buf[0x0C]);
    *bodysizeloc = (u32) 0;
    *bgmsizeloc = (u32) 0;

    for(int i = 0; i < 10; i++)
    {
        bodysizeloc = (u32*) (&thememanage_buf[0x338 + (4 * i)]); // body size info for shuffle themes starts at 0x338 and is 4 bytes for each theme
        bgmsizeloc = (u32*) (&thememanage_buf[0x360 + (4 * i)]); // same thing for bgm but starting at 0x360
        *bodysizeloc = body_sizes[i]; // We don't need to check if we've already installed all the themes because all sizes initialized to 0
        *bgmsizeloc = bgm_sizes[i];
    }

    result = buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    if(R_FAILED(result)) return result;

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS);
}
