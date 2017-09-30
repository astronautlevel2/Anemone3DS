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
#include "themes.h"
#include "pp2d/pp2d/pp2d.h"
#include "draw.h"

void load_splash_preview(Splash_s *splash)
{
    //free the previously loaded preview. wont do anything if there wasnt one
    pp2d_free_texture(TEXTURE_PREVIEW);
    
    char *preview_buffer = NULL;
    u64 size = 0;
    if (!(splash->is_zip))
    {
        u16 path_to_preview[0x106] = {0};
        strucat(path_to_preview, splash->path);
        struacat(path_to_preview, "/preview.png");
        size = file_to_buf(fsMakePath(PATH_UTF16, path_to_preview), ArchiveSD, &preview_buffer);    
    } else {
        size = zip_file_to_buf("preview.png", splash->path, &preview_buffer);
    }

    if (!size)
    {
        free(preview_buffer);
        return;
    }
    
    u8 * image = NULL;
    unsigned int width = 0, height = 0;
	
    if (R_SUCCEEDED(lodepng_decode32(&image, &width, &height, (u8*)preview_buffer, size))) // no error
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
        
        pp2d_load_texture_memory(TEXTURE_PREVIEW, image, (u32)width, (u32)height);
    }
    
    free(image);
    free(preview_buffer);
}


static void parse_smdh(Splash_s *splash, ssize_t textureID, u16 *splash_name)
{
    char *info_buffer = NULL;
    u64 size = 0;
    if (!(splash->is_zip))
    {
        u16 path_to_smdh[0x106] = {0};
        strucat(path_to_smdh, splash->path);
        struacat(path_to_smdh, "/info.smdh");
        
        size = file_to_buf(fsMakePath(PATH_UTF16, path_to_smdh), ArchiveSD, &info_buffer);    
    } else {
        size = zip_file_to_buf("info.smdh", splash->path, &info_buffer);
    }

    if (!size)
    {
        free(info_buffer);
        memset(splash->name, 0, 0x80);
        memset(splash->desc, 0, 0x100);
        memset(splash->author, 0, 0x80);
        memcpy(splash->name, splash_name, 0x80);
        utf8_to_utf16(splash->desc, (u8*)"No description", 0x100);
        utf8_to_utf16(splash->author, (u8*)"Unknown author", 0x80);
        splash->placeholder_color = RGBA8(rand() % 255, rand() % 255, rand() % 255, 255);
        return;
    }

    memcpy(splash->name, info_buffer + 0x08, 0x80);
    memcpy(splash->desc, info_buffer + 0x88, 0x100);
    memcpy(splash->author, info_buffer + 0x188, 0x80);
    
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
    splash->icon_id = textureID;
    
    free(image);
    free(icon_data);
    free(info_buffer);
}

Result get_splashes(Splash_s** splashes_list, int *splash_count)
{
    Result res = 0;
    Handle dir_handle;
	
    if (R_FAILED(res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, SPLASHES_PATH))))
        return res;

    if (*splashes_list != NULL)
    {
        free(*splashes_list);
        *splashes_list = NULL;
        *splash_count = 0;
    }

    u32 entries_read = 1;
    while (entries_read)
    {
        FS_DirectoryEntry entry = {0};
        if (R_FAILED(res = FSDIR_Read(dir_handle, &entries_read, 1, &entry)) || entries_read == 0) 
            break;

        if (!(entry.attributes & FS_ATTRIBUTE_DIRECTORY) && strcmp(entry.shortExt, "ZIP"))
            continue;

        *splash_count += entries_read;
        *splashes_list= realloc(*splashes_list, (*splash_count) * sizeof(Splash_s));
        if (splashes_list == NULL)
            break;

        Splash_s *current_splash = &(*splashes_list)[*splash_count-1];
        memset(current_splash, 0, sizeof(Splash_s));

        u16 splash_path[0x106]= {0};
        struacat(splash_path, SPLASHES_PATH);
        strucat(splash_path, entry.name);

        char pathchar[0x106] = {0};
        utf16_to_utf8((u8*) pathchar, splash_path, 0x106);

        memcpy(current_splash->path, splash_path, 0x106 * sizeof(u16));
        current_splash->is_zip = !strcmp(entry.shortExt, "ZIP");

        ssize_t iconID = TEXTURE_PREVIEW + theme_count + *splash_count;
        parse_smdh(current_splash, iconID, entry.name);
    }
    FSDIR_Close(dir_handle);

    return res;
}

void splash_delete() 
{
    remove("/luma/splash.bin");
    remove("/luma/splashbottom.bin");
}

void splash_install(Splash_s splash_to_install)
{
    char *screen_buf = NULL;
    u32 size = 0;
    if (splash_to_install.is_zip)
    {
        size = zip_file_to_buf("splash.bin", splash_to_install.path, &screen_buf);
        if (size)
        {
            remake_file("/luma/splash.bin", ArchiveSD, sizeof(screen_buf));
            buf_to_file(size, "/luma/splash.bin", ArchiveSD, screen_buf);
            free(screen_buf);
            screen_buf = NULL;
            size = 0;
        }

        size = zip_file_to_buf("splashbottom.bin", splash_to_install.path, &screen_buf);
        if (size)
        {
            remake_file("/luma/splashbottom.bin", ArchiveSD, sizeof(screen_buf));
            buf_to_file(size, "/luma/splashbottom.bin", ArchiveSD, screen_buf);
            free(screen_buf);
            screen_buf = NULL;
            size = 0;
        }
    } else {
        u16 path[0x106] = {0};
        memcpy(path, splash_to_install.path, 0x106 * sizeof(u16));
        struacat(path, "/splash.bin");
        size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &screen_buf);
        if (size)
        {
            remake_file("/luma/splash.bin", ArchiveSD, sizeof(screen_buf));
            buf_to_file(size, "/luma/splash.bin", ArchiveSD, screen_buf);
            free(screen_buf);
            screen_buf = NULL;
            size = 0;
        }

        memcpy(path, splash_to_install.path, 0x106 * sizeof(u16));
        struacat(path, "/splashbottom.bin");
        size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &screen_buf);
        if (size)
        {
            remake_file("/luma/splashbottom.bin", ArchiveSD, sizeof(screen_buf));
            buf_to_file(size, "/luma/splashbottom.bin", ArchiveSD, screen_buf);
            free(screen_buf);
            screen_buf = NULL;
            size = 0;
        }
    }

    char *config_buf;
    size = file_to_buf(fsMakePath(PATH_ASCII, "/luma/config.bin"), ArchiveSD, &config_buf);
    if (size)
    {
        if (config_buf[0xC] == 0)
        {
            free(config_buf);
            throw_error("WARNING: Splashes are disabled in Luma Config", WARNING);
        }
    }
}