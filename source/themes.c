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

#include "themes.h"
#include "unicode.h"
#include "fs.h"
#include "draw.h"

#define BODY_CACHE_SIZE 0x150000
#define BGM_MAX_SIZE 0x337000

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
        if(themes.shuffle_count < 2)
        {
            DEBUG("not enough themes selected for shuffle\n");
            return MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_COMMON, RD_INVALID_SELECTION);
        }

        if(themes.shuffle_count > MAX_SHUFFLE_THEMES)
        {
            DEBUG("too many themes selected for shuffle\n");
            return MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_COMMON, RD_INVALID_SELECTION);
        }

        char * padded = NULL;

        int shuffle_count = 0;
        Handle body_cache_handle;

        if(installmode & THEME_INSTALL_BODY)
        {
            remake_file(fsMakePath(PATH_ASCII, "/BodyCache_rd.bin"), ArchiveThemeExt, BODY_CACHE_SIZE * MAX_SHUFFLE_THEMES);
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
                        throw_error("No body_LZ.bin found - is this a theme?", ERROR_LEVEL_WARNING);
                        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
                    }

                    shuffle_body_sizes[shuffle_count] = body_size;

                    padded = calloc(BODY_CACHE_SIZE, sizeof(char));
                    memcpy(padded, body, body_size);
                    free(body);

                    FSFILE_Write(body_cache_handle, NULL, BODY_CACHE_SIZE * shuffle_count, padded, BODY_CACHE_SIZE, FS_WRITE_FLUSH);

                    free(padded);
                    padded = NULL;
                }

                if(installmode & THEME_INSTALL_BGM)
                {
                    char bgm_cache_path[26] = {0};
                    sprintf(bgm_cache_path, "/BgmCache_%.2i.bin", shuffle_count);

                    if(current_theme->no_bgm_shuffle)
                    {
                        music = NULL;
                        music_size = 0;
                    }
                    else
                    {
                        music_size = load_data("/bgm.bcstm", *current_theme, &music);

                        if(music_size > BGM_MAX_SIZE)
                        {
                            free(music);
                            DEBUG("bgm too big\n");
                            return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
                        }
                    }

                    shuffle_music_sizes[shuffle_count] = music_size;
                    remake_file(fsMakePath(PATH_ASCII, bgm_cache_path), ArchiveThemeExt, BGM_MAX_SIZE);

                    Handle bgm_cache_handle;
                    FSUSER_OpenFile(&bgm_cache_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, bgm_cache_path), FS_OPEN_WRITE, 0);

                    padded = calloc(BGM_MAX_SIZE, sizeof(char));
                    if(!current_theme->no_bgm_shuffle && music_size)
                    {
                        memcpy(padded, music, music_size);
                        free(music);
                    }

                    FSFILE_Write(bgm_cache_handle, NULL, 0, padded, BGM_MAX_SIZE, FS_WRITE_FLUSH);

                    FSFILE_Close(bgm_cache_handle);
                    free(padded);
                    padded = NULL;
                }

                shuffle_count++;
            }
        }

        if(installmode & THEME_INSTALL_BGM)
        {
            char * blank = calloc(BGM_MAX_SIZE, sizeof(char));
            for(int i = shuffle_count; i < MAX_SHUFFLE_THEMES; i++)
            {
                char bgm_cache_path[26] = {0};
                sprintf(bgm_cache_path, "/BgmCache_%.2i.bin", i);
                remake_file(fsMakePath(PATH_ASCII, bgm_cache_path), ArchiveThemeExt, BGM_MAX_SIZE);
                buf_to_file(BGM_MAX_SIZE, fsMakePath(PATH_ASCII, bgm_cache_path), ArchiveThemeExt, blank);
            }
            free(blank);
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
                throw_error("No body_LZ.bin found - is this a theme?", ERROR_LEVEL_WARNING);
                return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
            }

            res = buf_to_file(body_size, fsMakePath(PATH_ASCII, "/BodyCache.bin"), ArchiveThemeExt, body); // Write body data to file
            free(body);

            if(R_FAILED(res)) return res;
        }

        if(installmode & THEME_INSTALL_BGM)
        {
            music_size = load_data("/bgm.bcstm", current_theme, &music);
            if (music_size > BGM_MAX_SIZE)
            {
                free(music);
                DEBUG("bgm too big\n");
                return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
            }

            if (music_size != 0)
            {
                remake_file(fsMakePath(PATH_ASCII, "/BgmCache.bin"), ArchiveThemeExt, BGM_MAX_SIZE);
                res = buf_to_file(music_size, fsMakePath(PATH_ASCII, "/BgmCache.bin"), ArchiveThemeExt, music);
                free(music);

                char *body_buf = NULL;
                u32 uncompressed_size = decompress_lz_file(fsMakePath(PATH_ASCII, "/BodyCache.bin"), ArchiveThemeExt, &body_buf);
                if (body_buf[5] != 1)
                {
                    installmode |= THEME_INSTALL_BODY;
                    body_buf[5] = 1;
                    body_size = compress_lz_file_fast(fsMakePath(PATH_ASCII, "/BodyCache.bin"), ArchiveThemeExt, body_buf, uncompressed_size);
                }
                    
                free(body_buf);
            }

            if(R_FAILED(res)) return res;
        } else
        {
            music = calloc(BGM_MAX_SIZE, 1);
            res = buf_to_file(BGM_MAX_SIZE, fsMakePath(PATH_ASCII, "/BgmCache.bin"), ArchiveThemeExt, music);
            free(music);
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

    res = buf_to_file(0x800, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);
    if(R_FAILED(res)) return res;
    //----------------------------------------

    //----------------------------------------
    char* savedata_buf = NULL;
    u32 savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    SaveData_dat_s* savedata = (SaveData_dat_s*)savedata_buf;

    memset(&savedata->theme_entry, 0, sizeof(ThemeEntry_s));

    savedata->shuffle = (installmode & THEME_INSTALL_SHUFFLE) ? 1 : 0;
    memset(savedata->shuffle_themes, 0, sizeof(ThemeEntry_s)*MAX_SHUFFLE_THEMES);
    if(installmode & THEME_INSTALL_SHUFFLE)
    {
        for(int i = 0; i < themes.shuffle_count; i++)
        {
            savedata->shuffle_themes[i].type = 3;
            savedata->shuffle_themes[i].index = i;
        }
        const u8 shuffle_seed[0xB] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
        memcpy(savedata->shuffle_seedA, shuffle_seed, 0xB);
        memcpy(savedata->shuffle_seedB, shuffle_seed, 0xA);
    }
    else
    {
        savedata->theme_entry.type = 3;
        savedata->theme_entry.index = 0xff;
    }

    res = buf_to_file(savedata_size, fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, savedata_buf);
    free(savedata_buf);
    if(R_FAILED(res)) return res;
    //----------------------------------------

    return 0;
}

inline Result theme_install(Entry_s theme)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.entries = &theme;
    list.selected_entry = 0;
    return install_theme_internal(list, THEME_INSTALL_BODY | THEME_INSTALL_BGM);
}

inline Result bgm_install(Entry_s theme)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.entries = &theme;
    list.selected_entry = 0;
    return install_theme_internal(list, THEME_INSTALL_BGM);
}

inline Result no_bgm_install(Entry_s theme)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.entries = &theme;
    list.selected_entry = 0;
    return install_theme_internal(list, THEME_INSTALL_BODY);
}

inline Result shuffle_install(Entry_List_s themes)
{
    return install_theme_internal(themes, THEME_INSTALL_SHUFFLE | THEME_INSTALL_BODY | THEME_INSTALL_BGM);
}

static SwkbdCallbackResult
dir_name_callback(void *data, const char ** ppMessage, const char * text, size_t textlen)
{
    (void)textlen;
    (void)data;
    if(strpbrk(text, "><\"?;:/\\+,.|[=]"))
    {
        *ppMessage = "Illegal character used.";
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

Result dump_theme(void)
{
    const int max_chars = 255;
    char * output_dir = calloc(max_chars + 1, sizeof(char));

    SwkbdState swkbd;

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, max_chars);
    swkbdSetHintText(&swkbd, "Name of output folder");

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Done", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, max_chars);
    swkbdSetFilterCallback(&swkbd, dir_name_callback, NULL);

    SwkbdButton button = swkbdInputText(&swkbd, output_dir, max_chars);

    if (button != SWKBD_BUTTON_CONFIRM)
    {
        DEBUG("<dump_theme> Something went wrong with getting swkbd\n");
        return MAKERESULT(RL_FATAL, RS_CANCELED, RM_UTIL, RD_CANCEL_REQUESTED);
    }

    u16 path[0x107] = { 0 };
    struacat(path, "/themes/");
    struacat(path, output_dir);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_UTF16, path), FS_ATTRIBUTE_DIRECTORY);

    char *thememanage_buf = NULL;
    file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, &thememanage_buf);
    ThemeManage_bin_s * theme_manage = (ThemeManage_bin_s *)thememanage_buf;
    u32 theme_size = theme_manage->body_size;
    u32 bgm_size = theme_manage->music_size;
    free(thememanage_buf);

    char *temp_buf = NULL;
    file_to_buf(fsMakePath(PATH_ASCII, "/BodyCache.bin"), ArchiveThemeExt, &temp_buf);
    u16 path_output[0x107] = { 0 };
    memcpy(path_output, path, 0x107);
    struacat(path_output, "/body_LZ.bin");
    remake_file(fsMakePath(PATH_UTF16, path_output), ArchiveSD, theme_size);
    buf_to_file(theme_size, fsMakePath(PATH_UTF16, path_output), ArchiveSD, temp_buf);
    free(temp_buf);
    temp_buf = NULL;

    file_to_buf(fsMakePath(PATH_ASCII, "/BgmCache.bin"), ArchiveThemeExt, &temp_buf);
    memcpy(path_output, path, 0x107);
    struacat(path_output, "/bgm.bcstm");
    remake_file(fsMakePath(PATH_UTF16, path_output), ArchiveSD, bgm_size);
    buf_to_file(bgm_size, fsMakePath(PATH_UTF16, path_output), ArchiveSD, temp_buf);
    free(temp_buf);
    temp_buf = NULL;

    char *smdh_file = calloc(1, 0x36c0);
    smdh_file[0] = 0x53; // SMDH magic
    smdh_file[1] = 0x4d;
    smdh_file[2] = 0x44;
    smdh_file[3] = 0x48;

    struacat((u16 *) (smdh_file + 0x8), output_dir);
    struacat((u16 *) (smdh_file + 0x88), "No description");
    struacat((u16 *) (smdh_file + 0x188), "Unknown Author");

    free(output_dir);

    u8 r = rand() % 255;
    u8 g = rand() % 255;
    u8 b = rand() % 255;

    u16 color = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);

    for (int i = 0x2040; i < 0x36c0; i += 2)
    {
        *(smdh_file + i) = (color & 0xFF00) >> 8;
        *(smdh_file + i + 1) = color & 0x00FF;
    }
    
    memcpy(path_output, path, 0x107);
    struacat(path_output, "/info.smdh");
    remake_file(fsMakePath(PATH_UTF16, path_output), ArchiveSD, 0x36c0);
    buf_to_file(0x36c0, fsMakePath(PATH_UTF16, path_output), ArchiveSD, smdh_file);

    free(smdh_file);

    return 0;
}

void themes_check_installed(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    Entry_List_s * list = (Entry_List_s *)arg->thread_arg;
    if(list == NULL || list->entries == NULL) return;

    #ifndef CITRA_MODE
    char* savedata_buf = NULL;
    u32 savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    if(!savedata_size) return;
    SaveData_dat_s* savedata = (SaveData_dat_s*)savedata_buf;
    bool shuffle = savedata->shuffle;
    free(savedata_buf);

    #define HASH_SIZE_BYTES 256/8
    u8 body_hash[MAX_SHUFFLE_THEMES][HASH_SIZE_BYTES];
    memset(body_hash, 0, MAX_SHUFFLE_THEMES*HASH_SIZE_BYTES);

    char* thememanage_buf = NULL;
    u32 theme_manage_size = file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, &thememanage_buf);
    if(!theme_manage_size) return;
    ThemeManage_bin_s * theme_manage = (ThemeManage_bin_s *)thememanage_buf;

    u32 single_body_size = theme_manage->body_size;
    u32 shuffle_body_sizes[MAX_SHUFFLE_THEMES] = {0};
    memcpy(shuffle_body_sizes, theme_manage->shuffle_body_sizes, sizeof(u32)*MAX_SHUFFLE_THEMES);
    free(thememanage_buf);

    if(shuffle)
    {
        char * body_buf = NULL;
        u32 body_cache_size = file_to_buf(fsMakePath(PATH_ASCII, "/BodyCache_rd.bin"), ArchiveThemeExt, &body_buf);
        if(!body_cache_size) return;

        for(int i = 0; i < MAX_SHUFFLE_THEMES; i++)
        {
            FSUSER_UpdateSha256Context(body_buf + BODY_CACHE_SIZE*i, shuffle_body_sizes[i], body_hash[i]);
        }

        free(body_buf);
    }
    else
    {
        char * body_buf = NULL;
        u32 body_size = file_to_buf(fsMakePath(PATH_ASCII, "/BodyCache.bin"), ArchiveThemeExt, &body_buf);
        if(!body_size) return;

        u8 * hash = body_hash[0];
        FSUSER_UpdateSha256Context(body_buf, single_body_size, hash);
        free(body_buf);
    }

    int total_installed = 0;
    for(int i = 0; i < list->entries_count && total_installed < MAX_SHUFFLE_THEMES && arg->run_thread; i++)
    {
        Entry_s * theme = &list->entries[i];
        char * theme_body = NULL;
        u32 theme_body_size = load_data("/body_LZ.bin", *theme, &theme_body);
        if(!theme_body_size) return;

        u8 theme_body_hash[HASH_SIZE_BYTES];
        FSUSER_UpdateSha256Context(theme_body, theme_body_size, theme_body_hash);
        free(theme_body);

        for(int j = 0; j < MAX_SHUFFLE_THEMES; j++)
        {
            if(!memcmp(body_hash[j], theme_body_hash, HASH_SIZE_BYTES))
            {
                theme->installed = true;
                total_installed++;
                if(!shuffle) break; //only need to check the first if the installed theme inst shuffle
            }
        }
    }
    #endif
}
