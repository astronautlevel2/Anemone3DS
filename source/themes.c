#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "themes.h"
#include "unicode.h"
#include "fs.h"

void parse_smdh(theme *entry, u16 *path)
{
    char *info_buffer;
    u64 size = 0;
    if (!(entry->is_zip))
    {
        u16 path_to_smdh[0x106] = {0};
        strucat(path_to_smdh, path);
        struacat(path_to_smdh, "/info.smdh");
        size = file_to_buf(fsMakePath(PATH_UTF16, path_to_smdh), ArchiveSD, &info_buffer);    
    } else {
        size = zip_file_to_buf("info.smdh", path, &info_buffer);
    }

    if (!size)
    {
        return;
    }

    memcpy(entry->name, info_buffer + 0x08, 0x80);
    memcpy(entry->desc, info_buffer + 0x88, 0x100);
    memcpy(entry->author, info_buffer + 0x188, 0x80);
    memcpy(entry->icon_data, info_buffer + 0x2040, 0x1200);
    free(info_buffer);
}

int scan_themes(theme **themes, int num_themes)
{
    Handle dir_handle;
    FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, "/Themes"));
    for (int i = 0; i < num_themes; i++)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        FSDIR_Read(dir_handle, NULL, 1, entry);
        theme *theme_info = malloc(sizeof(theme));
        u16 theme_path[0x106] = {0};
        struacat(theme_path, "/Themes/");
        strucat(theme_path, entry->name);
        if (!strcmp(entry->shortExt, "ZIP"))
        {
            theme_info->is_zip = true;
        } else {
            theme_info->is_zip = false;
        }
        parse_smdh(theme_info, theme_path);
        memcpy(theme_info->path, theme_path, 0x106 * sizeof(u16));
        theme_info->selected = true;
        themes[i] = theme_info;
        free(entry);
    }
    return 0;
}

// Install a single theme
Result single_install(theme theme_to_install)
{
    char *body;
    char *music;
    char *savedata_buf;
    char *thememanage_buf;
    u32 body_size;
    u32 music_size;
    u32 savedata_size;

    printf("Writing SaveData.dat...\n");

    savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    savedata_buf[0x141b] = 0;
    memset(&savedata_buf[0x13b8], 0, 8);
    savedata_buf[0x13bd] = 3;
    savedata_buf[0x13b8] = 0xff;
    u32 size = buf_to_file(savedata_size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    // Open body cache file. Test if theme is zipped
    printf("Writing BodyCache.bin...\n");
    if (theme_to_install.is_zip)
    {
        body_size = zip_file_to_buf("body_LZ.bin", theme_to_install.path, &body);
    } else {
        u16 path[0x106] = {0};
        memcpy(path, theme_to_install.path, 0x106 * sizeof(u16));
        struacat(path, "/body_lz.bin");
        body_size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &body);
    }

    if (body_size == 0)
    {
        free(body);
        puts("bodyrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);
    }

    size = buf_to_file(body_size, "/BodyCache.bin", ArchiveThemeExt, body); // Write body data to file
    free(body);

    if (size == 0) return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);

    printf("Writing BgmCache.bin...\n");
    if (theme_to_install.is_zip) // Same as above but this time with bgm
    {
        music_size = zip_file_to_buf("bgm.bcstm", theme_to_install.path, &music);
    } else {
        u16 path[0x106] = {0};
        memcpy(path, theme_to_install.path, 0x106 * sizeof(u16));
        struacat(path, "/bgm.bcstm");
        music_size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &music);
    }

    if (music_size == 0)
    {
        free(music);
        music = calloc(1, 3371008);
    } else if (music_size > 3371008) {
        free(music);
        puts("musicrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
    }

    size = buf_to_file(music_size, "/BgmCache.bin", ArchiveThemeExt, music);
    free(music);

    if (size == 0) return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_NOT_FOUND);

    printf("Writing ThemeManage.bin...\n");
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
    size = buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    printf("Done!\n");

    return 0;
}

Result shuffle_install(theme **themes_list, int num_themes)
{
    u8 count = 0;
    theme *shuffle_themes[10] = {0};
    u32 body_sizes[10] = {0};
    u32 bgm_sizes[10] = {0};
    for (int i = 0; i < num_themes; i++)
    {
        if (count > 9) return MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_COMMON, RD_INVALID_SELECTION);
        if (themes_list[i]->selected)
        {
            shuffle_themes[count++] = themes_list[i];
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

    for (int i = 0; i < 10; i++)
    {
        memset(&savedata_buf[0x13c0 + 0x8 * i], 0, 8); // clear any existing theme structure. 8 is the length of the theme structure, so 8 * i is the pos of the current one
        if (count > i) // if we are still installing themes...
        {
            savedata_buf[0x13c0 + (8 * i)] = i; // index
            savedata_buf[0x13c0 + (8 * i) + 5] = 3; // persistence (?)
        }
    }

    buf_to_file(size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    remake_file("/BodyCache_rd.bin", ArchiveThemeExt, 0x150000 * 10); // Enough space for 10 theme files
    Handle body_cache_handle;
    FSUSER_OpenFile(&body_cache_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BodyCache_rd.bin"), FS_OPEN_WRITE, 0);
    for (int i = 0; i < 10; i++)
    {
        if (count > i)
        {
            if (shuffle_themes[i]->is_zip)
            {
                char *body_buf;
                u32 body_size = zip_file_to_buf("body_LZ.bin", shuffle_themes[i]->path, &body_buf);
                body_sizes[i] = body_size;
                FSFILE_Write(body_cache_handle, NULL, 0x150000 * i, body_buf, body_size, FS_WRITE_FLUSH);
                free(body_buf);
            } else {
                u16 path[0x106] = {0};
                strucat(path, shuffle_themes[i]->path);
                struacat(path, "/body_LZ.bin");
                char *body_buf;
                u32 body_size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &body_buf);
                body_sizes[i] = body_size;
                FSFILE_Write(body_cache_handle, NULL, 0x150000 * i, body_buf, body_size, FS_WRITE_FLUSH);
                free(body_buf);
            }
        }
    }

    FSFILE_Close(body_cache_handle);

    for (int i = 0; i < 10; i++)
    {
        char bgm_cache_path[17] = {0};
        sprintf(bgm_cache_path, "/BgmCache_0%i.bin", i);
        remake_file(bgm_cache_path, ArchiveThemeExt, 3371008);
        if (count > i)
        {
            char *music_buf;
            u32 music_size;

            if (shuffle_themes[i]->is_zip)
            {
                music_size = zip_file_to_buf("bgm.bcstm", shuffle_themes[i]->path, &music_buf);
            } else {
                u16 path[0x106] = {0};
                strucat(path, shuffle_themes[i]->path);
                struacat(path, "/bgm.bcstm");
                music_size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &music_buf);
            }

            if (!music_size)
            {
                char *empty = calloc(1, 3371008);
                buf_to_file(3371008, bgm_cache_path, ArchiveThemeExt, empty);
                bgm_sizes[i] = 0;
                free(empty);
                continue;
            }
            bgm_sizes[i] = music_size;
            buf_to_file(music_size, bgm_cache_path, ArchiveThemeExt, music_buf);
            free(music_buf);
        } else {
            char *empty = calloc(1, 3371008);
            buf_to_file(3371008, bgm_cache_path, ArchiveThemeExt, empty);
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

    for (int i = 0; i < 10; i++)
    {
        bodysizeloc = (u32*) (&thememanage_buf[0x338 + (4 * i)]); // body size info for shuffle themes starts at 0x338 and is 4 bytes for each theme
        bgmsizeloc = (u32*) (&thememanage_buf[0x360 + (4 * i)]); // same thing for bgm but starting at 0x360
        *bodysizeloc = body_sizes[i]; // We don't need to check if we've already installed all the themes because all sizes initialized to 0
        *bgmsizeloc = bgm_sizes[i];
    }

    buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS);
}