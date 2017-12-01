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

#include "loading.h"
#include "pp2d/pp2d/pp2d.h"
#include "fs.h"
#include "unicode.h"

u32 load_data(char * filename, Entry_s entry, char ** buf)
{
    if(entry.is_zip)
    {
        return zip_file_to_buf(filename+1, entry.path, buf); //the first character will always be '/' because of the other case
    }
    else
    {
        u16 path[0x106] = {0};
        strucat(path, entry.path);
        struacat(path, filename);

        return file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, buf);
    }
}

static void parse_smdh(Entry_s * entry, const ssize_t textureID, const u16 * fallback_name)
{
    pp2d_free_texture(textureID);

    char *info_buffer = NULL;
    u64 size = load_data("/info.smdh", *entry, &info_buffer);

    if(!size)
    {
        free(info_buffer);
        memcpy(entry->name, fallback_name, 0x80);
        utf8_to_utf16(entry->desc, (u8*)"No description", 0x100);
        utf8_to_utf16(entry->author, (u8*)"Unknown author", 0x80);
        entry->placeholder_color = RGBA8(rand() % 255, rand() % 255, rand() % 255, 255);
        return;
    }

    memcpy(entry->name, info_buffer + 0x08, 0x80);
    memcpy(entry->desc, info_buffer + 0x88, 0x100);
    memcpy(entry->author, info_buffer + 0x188, 0x80);

    u16 *icon_data = malloc(0x1200);
    memcpy(icon_data, info_buffer + 0x24C0, 0x1200);
    free(info_buffer);

    const u32 width = 48, height = 48;
    u32 *image = malloc(width*height*sizeof(u32));

    for(u32 x = 0; x < width; x++)
    {
        for(u32 y = 0; y < height; y++)
        {
            unsigned int dest_pixel = (x + y*width);
            unsigned int source_pixel = (((y >> 3) * (width >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3));

            image[dest_pixel] = RGB565_TO_ABGR8(icon_data[source_pixel]);
        }
    }

    free(icon_data);
    pp2d_load_texture_memory(textureID, (u8*)image, (u32)width, (u32)height);
    free(image);

    entry->icon_id = textureID;
}

static int compare_entries(const void * a, const void * b)
{
    Entry_s *entry_a = (Entry_s *)a;
    Entry_s *entry_b = (Entry_s *)b;

    return memcmp(entry_a->name, entry_b->name, 0x40*sizeof(u16));
}

static void sort_list(Entry_List_s * list)
{
    qsort(list->entries, list->entries_count, sizeof(Entry_s), compare_entries); //alphabet sort
}

Result load_entries(const char * loading_path, Entry_List_s * list)
{
    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, loading_path));
    if(R_FAILED(res))
        return res;

    u32 entries_read = 1;

    while(entries_read)
    {
        FS_DirectoryEntry dir_entry = {0};
        res = FSDIR_Read(dir_handle, &entries_read, 1, &dir_entry);
        if(R_FAILED(res) || entries_read == 0)
            break;

        if(!(dir_entry.attributes & FS_ATTRIBUTE_DIRECTORY) && strcmp(dir_entry.shortExt, "ZIP"))
            continue;

        list->entries_count++;
        list->entries = realloc(list->entries, list->entries_count * sizeof(Entry_s));
        if(list->entries == NULL)
            break;

        Entry_s * current_entry = &(list->entries[list->entries_count-1]);
        memset(current_entry, 0, sizeof(Entry_s));

        struacat(current_entry->path, loading_path);
        strucat(current_entry->path, dir_entry.name);

        current_entry->is_zip = !strcmp(dir_entry.shortExt, "ZIP");

        ssize_t iconID = TEXTURE_ICON + list->entries_count;
        parse_smdh(current_entry, iconID, dir_entry.name);
    }

    FSDIR_Close(dir_handle);

    sort_list(list);

    return res;
}

static u16 previous_path[0x106] = {0};

bool load_preview(Entry_List_s list, int * preview_offset)
{
    if(list.entries == NULL) return false;

    Entry_s entry = list.entries[list.selected_entry];

    if(!memcmp(&previous_path, &entry.path, 0x106*sizeof(u16))) return true;
    else memcpy(&previous_path, &entry.path, 0x106*sizeof(u16));

    // free the previously loaded preview. wont do anything if there wasnt one
    pp2d_free_texture(TEXTURE_PREVIEW);

    char *preview_buffer = NULL;
    u64 size = load_data("/preview.png", entry, &preview_buffer);

    if(!size)
    {
        free(preview_buffer);
        return false;
    }

    bool ret = false;
    u8 * image = NULL;
    unsigned int width = 0, height = 0;

    if((lodepng_decode32(&image, &width, &height, (u8*)preview_buffer, size)) == 0) // no error 
    {
        for(u32 i = 0; i < width; i++)
        {
            for(u32 j = 0; j < height; j++)
            {
                u32* pixel = (u32*)(image + (i + j*width) * 4);
                *pixel = __builtin_bswap32(*pixel); //swap from RGBA to ABGR, needed for pp2d
            }
        }

        pp2d_load_texture_memory(TEXTURE_PREVIEW, image, (u32)width, (u32)height);
        *preview_offset = (width-400)/2;
        ret = true;
    }

    free(image);
    free(preview_buffer);

    return ret;
}
