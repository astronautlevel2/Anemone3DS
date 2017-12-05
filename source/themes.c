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

#define BODY_CACHE_SIZE 0x150000
#define BGM_MAX_SIZE 0x337000

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

static Result install_theme_internal(Entry_List_s themes, int installmode)
{
    Result res = 0;
    char* music = NULL;
    u32 music_size = 0;
    u32 shuffle_music_sizes[MAX_SHUFFLE_THEMES] = {0};
    char* body = NULL;
    u32 body_size = 0;
    u32 shuffle_body_sizes[MAX_SHUFFLE_THEMES] = {0};

    if(installmode & THEME_INSTALL_SHUFFLE)
    {
        if(themes.shuffle_count > MAX_SHUFFLE_THEMES)
        {
            DEBUG("too many themes selected for shuffle\n");
            return MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_COMMON, RD_INVALID_SELECTION);
        }

        int shuffle_count = 0;
        Handle body_cache_handle;

        if(installmode & THEME_INSTALL_BODY)
        {
            remake_file("/BodyCache_rd.bin", ArchiveThemeExt, BODY_CACHE_SIZE * MAX_SHUFFLE_THEMES);
            FSUSER_OpenFile(&body_cache_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BodyCache_rd.bin"), FS_OPEN_WRITE, 0);
        }

        for(int i = 0; i < themes.entries_count; i++)
        {
            Entry_s * current_theme = &themes.entries[i];

            if(current_theme->in_shuffle)
            {
                if(installmode & THEME_INSTALL_BODY)
                {
                    body_size = load_data("/body_LZ.bin", *current_theme, &body);
                    if(body_size == 0)
                    {
                        free(body);
                        DEBUG("body not found\n");
                        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
                    }

                    shuffle_body_sizes[shuffle_count] = body_size;
                    FSFILE_Write(body_cache_handle, NULL, BODY_CACHE_SIZE * shuffle_count, body, body_size, FS_WRITE_FLUSH);
                    free(body);
                }

                if(installmode & THEME_INSTALL_BGM)
                {
                    char bgm_cache_path[26] = {0};
                    sprintf(bgm_cache_path, "/BgmCache_%.2i.bin", shuffle_count);
                    music_size = load_data("/bgm.bcstm", *current_theme, &music);

                    if(music_size > BGM_MAX_SIZE)
                    {
                        free(music);
                        DEBUG("bgm too big\n");
                        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
                    }

                    shuffle_music_sizes[shuffle_count] = music_size;
                    remake_file(bgm_cache_path, ArchiveThemeExt, BGM_MAX_SIZE);
                    buf_to_file(music_size, bgm_cache_path, ArchiveThemeExt, music);
                    free(music);
                }

                current_theme->in_shuffle = false;
                shuffle_count++;
            }
        }

        if(installmode & THEME_INSTALL_BGM)
        {
            for(int i = shuffle_count; i < MAX_SHUFFLE_THEMES; i++)
            {
                char bgm_cache_path[26] = {0};
                sprintf(bgm_cache_path, "/BgmCache_%.2i.bin", i);
                remake_file(bgm_cache_path, ArchiveThemeExt, BGM_MAX_SIZE);
            }
        }

        if(installmode & THEME_INSTALL_BODY)
        {
            FSFILE_Close(body_cache_handle);
        }
    }
    else
    {
        Entry_s current_theme = themes.entries[themes.selected_entry];

        if(installmode & THEME_INSTALL_BODY)
        {
            body_size = load_data("/body_LZ.bin", current_theme, &body);
            if(body_size == 0)
            {
                free(body);
                DEBUG("body not found\n");
                return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
            }

            res = buf_to_file(body_size, "/BodyCache.bin", ArchiveThemeExt, body); // Write body data to file
            free(body);

            if(R_FAILED(res)) return res;
        }

        if(installmode & THEME_INSTALL_BGM)
        {
            music_size = load_data("/bgm.bcstm", current_theme, &music);

            if(music_size > BGM_MAX_SIZE)
            {
                free(music);
                DEBUG("bgm too big\n");
                return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
            }

            res = buf_to_file(music_size, "/BgmCache.bin", ArchiveThemeExt, music);
            free(music);
            if(R_FAILED(res)) return res;
        }
    }

     //----------------------------------------
    char* thememanage_buf = NULL;
    file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, &thememanage_buf);
    ThemeManage_bin_s * theme_manage = (ThemeManage_bin_s *)thememanage_buf;

    theme_manage->unk1 = 1;
    theme_manage->unk2 = 0;

    if(installmode & THEME_INSTALL_SHUFFLE)
    {
        theme_manage->music_size = 0;
        theme_manage->body_size = 0;

        for(int i = 0; i < MAX_SHUFFLE_THEMES; i++)
        {
            theme_manage->shuffle_body_sizes[i] = shuffle_body_sizes[i];
            theme_manage->shuffle_music_sizes[i] = shuffle_music_sizes[i];
        }
    }
    else
    {
        if(installmode & THEME_INSTALL_BGM)
            theme_manage->music_size = music_size;
        if(installmode & THEME_INSTALL_BODY)
            theme_manage->body_size = body_size;
    }

    theme_manage->unk3 = 0xFF;
    theme_manage->unk4 = 1;
    theme_manage->dlc_theme_content_index = 0xFF;
    theme_manage->use_theme_cache = 0x0200;

    res = buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);
    if(R_FAILED(res)) return res;
    //----------------------------------------

    //----------------------------------------
    char* savedata_buf = NULL;
    u32 savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    SaveData_dat_s* savedata = (SaveData_dat_s*)savedata_buf;

    memset(&savedata->theme_entry, 0, sizeof(ThemeEntry_s));
    savedata->theme_entry.type = 3;
    savedata->theme_entry.index = 0xff;

    savedata->shuffle = (installmode & THEME_INSTALL_SHUFFLE);
    if(installmode & THEME_INSTALL_SHUFFLE)
    {
        memset(savedata->shuffle_themes, 0, sizeof(ThemeEntry_s)*MAX_SHUFFLE_THEMES);
        for(int i = 0; i < themes.shuffle_count; i++)
        {
            savedata->shuffle_themes[i].type = 3;
            savedata->shuffle_themes[i].index = i;
        }
    }

    res = buf_to_file(savedata_size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);
    if(R_FAILED(res)) return res;
    //----------------------------------------

    return 0;
}

Result theme_install(Entry_s theme)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.entries = &theme;
    list.selected_entry = 0;
    return install_theme_internal(list, THEME_INSTALL_BODY | THEME_INSTALL_BGM);
}

Result bgm_install(Entry_s theme)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.entries = &theme;
    list.selected_entry = 0;
    return install_theme_internal(list, THEME_INSTALL_BGM);
}

Result no_bgm_install(Entry_s theme)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.entries = &theme;
    list.selected_entry = 0;
    return install_theme_internal(list, THEME_INSTALL_BODY);
}

Result shuffle_install(Entry_List_s themes)
{
    return install_theme_internal(themes, THEME_INSTALL_SHUFFLE | THEME_INSTALL_BODY | THEME_INSTALL_BGM);
}
