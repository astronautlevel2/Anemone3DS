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
#include <time.h>

#include "pp2d/pp2d/pp2d.h"
#include "pp2d/pp2d/lodepng.h"

void load_theme_preview(Theme_s *theme)
{
    //free the previously loaded preview. wont do anything if there wasnt one
    pp2d_free_texture(TEXTURE_PREVIEW);
    
    char *preview_buffer = NULL;
    u64 size = 0;
    if (!(theme->is_zip))
    {
        u16 path_to_preview[0x106] = {0};
        strucat(path_to_preview, theme->path);
        struacat(path_to_preview, "/preview.png");
        size = file_to_buf(fsMakePath(PATH_UTF16, path_to_preview), ArchiveSD, &preview_buffer);    
    } else {
        size = zip_file_to_buf("preview.png", theme->path, &preview_buffer);
    }

    if (!size)
    {
        free(preview_buffer);
        return;
    }
    
    u8 * image = NULL;
    unsigned int width = 0, height = 0;
    
    int result = lodepng_decode32(&image, &width, &height, (u8*)preview_buffer, size);
    if (result == 0) // no error
    {
        for (u32 i = 0; i < width; i++)
        {
            for (u32 j = 0; j < height; j++)
            {
                u32 p = (i + j*width) * 4;

                u8 r = *(u8*)(image + p);
                u8 g = *(u8*)(image + p + 1);
                u8 b = *(u8*)(image + p + 2);
                u8 a = *(u8*)(image + p + 3);

                *(image + p) = a;
                *(image + p + 1) = b;
                *(image + p + 2) = g;
                *(image + p + 3) = r;
            }
        }
        
        theme->has_preview = true;
        pp2d_load_texture_memory(TEXTURE_PREVIEW, image, (u32)width, (u32)height);
        theme->preview_offset = (width-400)/2;
    }
    
    free(image);
    free(preview_buffer);
}

static void parse_smdh(Theme_s *theme, ssize_t textureID, u16 *dir_name)
{
    char *info_buffer = NULL;
    u64 size = 0;
    if (!(theme->is_zip))
    {
        u16 path_to_smdh[0x106] = {0};
        strucat(path_to_smdh, theme->path);
        struacat(path_to_smdh, "/info.smdh");
        
        size = file_to_buf(fsMakePath(PATH_UTF16, path_to_smdh), ArchiveSD, &info_buffer);    
    } else {
        size = zip_file_to_buf("info.smdh", theme->path, &info_buffer);
    }

    if (!size)
    {
        free(info_buffer);
        memset(theme->name, 0, 0x80);
        memset(theme->desc, 0, 0x100);
        memset(theme->author, 0, 0x80);
        memcpy(theme->name, dir_name, 0x80);
        utf8_to_utf16(theme->desc, (u8*)"No description", 0x100);
        utf8_to_utf16(theme->author, (u8*)"Unknown author", 0x80);
        theme->placeholder_color = RGBA8(rand() % 255, rand() % 255, rand() % 255, 255);
        return;
    }

    memcpy(theme->name, info_buffer + 0x08, 0x80);
    memcpy(theme->desc, info_buffer + 0x88, 0x100);
    memcpy(theme->author, info_buffer + 0x188, 0x80);
    
    u16 *icon_data = malloc(0x1200);
    memcpy(icon_data, info_buffer + 0x24C0, 0x1200);
    
    const u32 width = 48, height = 48;
    u32 *image = malloc(width*height*sizeof(u32));

    for (u32 x = 0; x < width; x++)
    {
        for (u32 y = 0; y < height; y++)
        {
            unsigned int dest_pixel = (x + y*width);
            unsigned int source_pixel = (((y >> 3) * (width >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3));
            
            u8 r = ((icon_data[source_pixel] >> 11) & 0b11111) << 3;
            u8 g = ((icon_data[source_pixel] >> 5) & 0b111111) << 2;
            u8 b = (icon_data[source_pixel] & 0b11111) << 3;
            u8 a = 0xFF;
            
            image[dest_pixel] = (r << 24) | (g << 16) | (b << 8) | a;
        }
    }
    
    pp2d_load_texture_memory(textureID, (u8*)image, (u32)width, (u32)height);
    theme->icon_id = textureID;
    
    free(image);
    free(icon_data);
    free(info_buffer);
}

Result get_themes(Theme_s **themes_list, int *theme_count)
{
    shuffle_theme_count = 0;
    Result res = 0;
    Handle dir_handle;
    res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, THEMES_PATH));
    if (R_FAILED(res))
        return res;
    
    if (*themes_list != NULL) //used for QR reading and also for theme deletion
    {
        free(*themes_list);
        *themes_list = NULL;
        *theme_count = 0;
    }

    u32 entries_read = 1;

    while (entries_read)
    {
        FS_DirectoryEntry entry = {0};
        res = FSDIR_Read(dir_handle, &entries_read, 1, &entry);
        if (R_FAILED(res) || entries_read == 0)
            break;
        
        if (!(entry.attributes & FS_ATTRIBUTE_DIRECTORY) && strcmp(entry.shortExt, "ZIP"))
            continue;
        
        *theme_count += entries_read;
        *themes_list = realloc(*themes_list, (*theme_count) * sizeof(Theme_s));
        if (*themes_list == NULL)
            break;
        
        Theme_s* current_theme = &(*themes_list)[*theme_count-1];
        memset(current_theme, 0, sizeof(Theme_s));
        
        u16 theme_path[0x106] = {0};
        struacat(theme_path, THEMES_PATH);
        strucat(theme_path, entry.name);
        
        char pathchar[0x106] = {0};
        utf16_to_utf8((u8*)pathchar, theme_path, 0x106);
        
        memcpy(current_theme->path, theme_path, 0x106 * sizeof(u16));
        current_theme->is_zip = !strcmp(entry.shortExt, "ZIP");
        
        ssize_t iconID = TEXTURE_PREVIEW + *theme_count;
        parse_smdh(current_theme, iconID, entry.name);
    }
    
    FSDIR_Close(dir_handle);
    
    return res;
}

void del_theme(u16 *path)
{
    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_UTF16, path));
    if (R_SUCCEEDED(res))
    {
        FSDIR_Close(dir_handle);
        FSUSER_DeleteDirectoryRecursively(ArchiveSD, fsMakePath(PATH_UTF16, path));
    } else 
    {
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_UTF16, path));
    }
}

Result bgm_install(Theme_s bgm_to_install)
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

    if (!R_SUCCEEDED(result)) return result;

    if (bgm_to_install.is_zip) // Same as above but this time with bgm
    {
        music_size = zip_file_to_buf("bgm.bcstm", bgm_to_install.path, &music);
    } else {
        u16 path[0x106] = {0};
        memcpy(path, bgm_to_install.path, 0x106 * sizeof(u16));
        struacat(path, "/bgm.bcstm");
        music_size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &music);
    }

    if (music_size == 0)
    {
        music = calloc(1, 3371008);
    } else if (music_size > 3371008) {
        free(music);
        puts("musicrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
    }

    result = buf_to_file(music_size == 0 ? 3371008 : music_size, "/BgmCache.bin", ArchiveThemeExt, music);
    free(music);

    if (!R_SUCCEEDED(result)) return result;

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

    if (!R_SUCCEEDED(result)) return result;

    return 0;
}

// Install a single theme
Result single_install(Theme_s theme_to_install)
{
    char *body;
    char *music;
    char *savedata_buf;
    char *thememanage_buf;
    u32 body_size;
    u32 music_size;
    u32 savedata_size;

    savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, &savedata_buf);
    savedata_buf[0x141b] = 0;
    memset(&savedata_buf[0x13b8], 0, 8);
    savedata_buf[0x13bd] = 3;
    savedata_buf[0x13b8] = 0xff;
    Result result = buf_to_file(savedata_size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    if (!R_SUCCEEDED(result)) return result;

    // Open body cache file. Test if theme is zipped
    if (theme_to_install.is_zip)
    {
        body_size = zip_file_to_buf("body_LZ.bin", theme_to_install.path, &body);
    }
    else
    {
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

    result = buf_to_file(body_size, "/BodyCache.bin", ArchiveThemeExt, body); // Write body data to file
    free(body);

    if (!R_SUCCEEDED(result)) return result;

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
        music = calloc(1, 3371008);
    } else if (music_size > 3371008) {
        free(music);
        puts("musicrip");
        return MAKERESULT(RL_PERMANENT, RS_CANCELED, RM_APPLICATION, RD_TOO_LARGE);
    }

    result = buf_to_file(music_size == 0 ? 3371008 : music_size, "/BgmCache.bin", ArchiveThemeExt, music);
    free(music);

    if (!R_SUCCEEDED(result)) return result;

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

    if (!R_SUCCEEDED(result)) return result;

    return 0;
}

Result shuffle_install(Theme_s *themes_list, int theme_count)
{
    u8 count = 0;
    Theme_s *shuffle_themes[10] = {0};
    u32 body_sizes[10] = {0};
    u32 bgm_sizes[10] = {0};
    for (int i = 0; i < theme_count; i++)
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

    for (int i = 0; i < 10; i++)
    {
        memset(&savedata_buf[0x13c0 + 0x8 * i], 0, 8); // clear any existing theme structure. 8 is the length of the theme structure, so 8 * i is the pos of the current one
        if (count > i) // if we are still installing themes...
        {
            savedata_buf[0x13c0 + (8 * i)] = i; // index
            savedata_buf[0x13c0 + (8 * i) + 5] = 3; // persistence (?)
        }
    }

    Result result = buf_to_file(size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    if (!R_SUCCEEDED(result)) return result;

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
        sprintf(bgm_cache_path, "/BgmCache_%.2i.bin", i);
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

    result = buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    if (!R_SUCCEEDED(result)) return result;

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS);
}
