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
#include "draw.h"

void delete_entry(Entry_s entry)
{
    if(entry.is_zip)
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_UTF16, entry.path));
    else
        FSUSER_DeleteDirectoryRecursively(ArchiveSD, fsMakePath(PATH_UTF16, entry.path));
}

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

static void parse_smdh(Entry_s * entry, const u16 * fallback_name)
{
    char *info_buffer = NULL;
    u64 size = load_data("/info.smdh", *entry, &info_buffer);
    Icon_s * smdh = (Icon_s *)info_buffer;

    if(!size)
    {
        free(info_buffer);
        memcpy(entry->name, fallback_name, 0x80);
        utf8_to_utf16(entry->desc, (u8*)"No description", 0x100);
        utf8_to_utf16(entry->author, (u8*)"Unknown author", 0x80);
        entry->placeholder_color = RGBA8(rand() % 255, rand() % 255, rand() % 255, 255);
        return;
    }

    memcpy(entry->name, smdh->name, 0x40*sizeof(u16));
    memcpy(entry->desc, smdh->desc, 0x80*sizeof(u16));
    memcpy(entry->author, smdh->author, 0x40*sizeof(u16));
}

static void load_smdh_icon(Entry_s entry, const ssize_t textureID)
{
    pp2d_free_texture(textureID);

    char *info_buffer = NULL;
    u64 size = load_data("/info.smdh", entry, &info_buffer);
    if(!size) return;

    Icon_s * smdh = (Icon_s *)info_buffer;

    const u32 width = 48, height = 48;
    u32 *image = malloc(width*height*sizeof(u32));

    for(u32 x = 0; x < width; x++)
    {
        for(u32 y = 0; y < height; y++)
        {
            unsigned int dest_pixel = (x + y*width);
            unsigned int source_pixel = (((y >> 3) * (width >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3));

            image[dest_pixel] = RGB565_TO_ABGR8(smdh->big_icon[source_pixel]);
        }
    }

    free(info_buffer);
    pp2d_load_texture_memory(textureID, (u8*)image, (u32)width, (u32)height);
    free(image);
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

Result load_entries(const char * loading_path, Entry_List_s * list, EntryMode mode)
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

        parse_smdh(current_entry, dir_entry.name);
    }

    FSDIR_Close(dir_handle);

    sort_list(list);
    list->mode = mode;

    return res;
}

static void small_load(Entry_List_s * current_list)
{
    DEBUG("small load\n");

    ssize_t * icons_ids = (ssize_t *)current_list->icons_ids;
    ssize_t id = current_list->texture_id_offset;
    for(int i = 0; i < current_list->entries_count; i++, id++)
    {
        Entry_s current_entry = current_list->entries[i];
        load_smdh_icon(current_entry, id);
        icons_ids[i] = id;
    }
}

static void first_load(Entry_List_s * current_list)
{
    DEBUG("first load\n");

    ssize_t * above_icons_ids = current_list->icons_ids[ICONS_ABOVE];
    ssize_t * visible_icons_ids = current_list->icons_ids[ICONS_VISIBLE];
    ssize_t * under_icons_ids = current_list->icons_ids[ICONS_UNDER];

    ssize_t id = current_list->texture_id_offset;
    int starti = current_list->scroll;

    memset(visible_icons_ids, 0, ENTRIES_PER_SCREEN*sizeof(ssize_t));
    for(int i = starti; i < starti+ENTRIES_PER_SCREEN; i++, id++)
    {
        if(i >= current_list->entries_count) break;

        Entry_s current_entry = current_list->entries[i];
        load_smdh_icon(current_entry, id);
        visible_icons_ids[i-starti] = id;
    }

    memset(above_icons_ids, 0, ENTRIES_PER_SCREEN*sizeof(ssize_t));
    starti -= ENTRIES_PER_SCREEN;
    for(int i = starti; i < starti+ENTRIES_PER_SCREEN; i++, id++)
    {
        if(i >= current_list->entries_count) break;
        int used_i = i;
        if(i < 0)
            used_i = current_list->entries_count + i;

        Entry_s current_entry = current_list->entries[used_i];
        load_smdh_icon(current_entry, id);
        above_icons_ids[i - starti] = id;
    }

    memset(under_icons_ids, 0, ENTRIES_PER_SCREEN*sizeof(ssize_t));
    starti += ENTRIES_PER_SCREEN*2;
    for(int i = starti; i < starti+ENTRIES_PER_SCREEN; i++, id++)
    {
        int used_i = i;
        if(i >= current_list->entries_count)
            used_i = i - current_list->entries_count;

        Entry_s current_entry = current_list->entries[used_i];
        load_smdh_icon(current_entry, id);
        under_icons_ids[i-starti] = id;
    }
}

void load_icons_first(Entry_List_s * current_list)
{
    if(current_list->entries_count <= ENTRIES_PER_SCREEN*ICONS_OFFSET_AMOUNT)
        small_load(current_list); // if the list is one that doesnt need swapping, load everything at once
    else
        first_load(current_list);
}

static void reverse(ssize_t a[], int sz) {
    int i, j;
    for (i = 0, j = sz; i < j; i++, j--) {
        ssize_t tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

static void rotate(ssize_t array[], int size, int amt) {
    if (amt < 0)
        amt = size + amt;
    reverse(array, size-amt-1);
    reverse(array+size-amt, amt-1);
    reverse(array, size-1);
}

static void load_icons(Entry_List_s * current_list)
{
    if(current_list == NULL || current_list->entries == NULL) return;

    // Scroll the menu up or down if the selected theme is out of its bounds
    //----------------------------------------------------------------
    if(current_list->entries_count > ENTRIES_PER_SCREEN)
    {
        if(current_list->previous_scroll < ENTRIES_PER_SCREEN && current_list->selected_entry >= current_list->entries_count - ENTRIES_PER_SCREEN)
        {
            current_list->scroll = current_list->entries_count - ENTRIES_PER_SCREEN;
        }
        else if(current_list->selected_entry <= ENTRIES_PER_SCREEN && current_list->previous_selected >= current_list->entries_count - ENTRIES_PER_SCREEN)
        {
            current_list->scroll = 0;
        }
        else if(current_list->selected_entry == current_list->previous_selected+1 && current_list->selected_entry == current_list->scroll+ENTRIES_PER_SCREEN)
        {
            current_list->scroll++;
        }
        else if(current_list->selected_entry == current_list->previous_selected-1 && current_list->selected_entry == current_list->scroll-1)
        {
            current_list->scroll--;
        }
        else if(current_list->selected_entry == current_list->previous_selected+ENTRIES_PER_SCREEN || current_list->selected_entry >= current_list->scroll + ENTRIES_PER_SCREEN)
        {
            current_list->scroll += ENTRIES_PER_SCREEN;
        }
        else if(current_list->selected_entry == current_list->previous_selected-ENTRIES_PER_SCREEN || current_list->selected_entry < current_list->scroll)
        {
            current_list->scroll -= ENTRIES_PER_SCREEN;
        }

        if(current_list->scroll < 0)
            current_list->scroll = 0;
        if(current_list->scroll > current_list->entries_count - ENTRIES_PER_SCREEN)
            current_list->scroll = current_list->entries_count - ENTRIES_PER_SCREEN;
    }
    //----------------------------------------------------------------

    if(current_list->entries_count <= ENTRIES_PER_SCREEN*ICONS_OFFSET_AMOUNT || current_list->previous_scroll == current_list->scroll)
        goto end; // return if the list is one that doesnt need swapping, or if nothing changed

    int delta = current_list->scroll - current_list->previous_scroll;
    if(delta >= current_list->entries_count - ENTRIES_PER_SCREEN*2 && delta <= current_list->entries_count - ENTRIES_PER_SCREEN)
        delta = -ENTRIES_PER_SCREEN;
    else if(-delta >= current_list->entries_count - ENTRIES_PER_SCREEN*2 && -delta <= current_list->entries_count - ENTRIES_PER_SCREEN)
        delta = ENTRIES_PER_SCREEN;

    #define FIRST(arr) arr[0]
    #define LAST(arr) arr[ENTRIES_PER_SCREEN*ICONS_OFFSET_AMOUNT - 1]

    int starti = current_list->scroll;
    int endi = starti + abs(delta);

    if(delta == -ENTRIES_PER_SCREEN)
    {
        endi -= abs(delta) + 1;
        starti += abs(delta) - 1;
    }

    int ctr = 0;
    Entry_s ** entries = calloc(abs(delta), sizeof(Entry_s *));
    ssize_t * ids = calloc(abs(delta), sizeof(ssize_t));

    for(int i = starti; i != endi; i++, ctr++)
    {
        ssize_t id = 0;
        if(delta > 0)
        {
            rotate((ssize_t *)current_list->icons_ids, 3*ENTRIES_PER_SCREEN, -1);
            id = LAST(((ssize_t *)current_list->icons_ids));
        }
        else
        {
            rotate((ssize_t *)current_list->icons_ids, 3*ENTRIES_PER_SCREEN, 1);
            id = FIRST(((ssize_t *)current_list->icons_ids));
        }

        int offset = i;

        if(delta == 1)
        {
            offset += ENTRIES_PER_SCREEN*2 - 1;
        }
        else if(delta == -1)
        {
            offset -= ENTRIES_PER_SCREEN;
        }
        else if(delta == ENTRIES_PER_SCREEN)
        {
            offset += ENTRIES_PER_SCREEN;
        }
        else if(delta == -ENTRIES_PER_SCREEN)
        {
            offset = offset - ENTRIES_PER_SCREEN;
            i -= 2; //i-- twice to counter the i++, needed only for this case
        }

        if(offset < 0)
            offset = current_list->entries_count + offset;
        if(offset >= current_list->entries_count)
            offset = offset - current_list->entries_count;

        entries[ctr] = &current_list->entries[offset];
        ids[ctr] = id;
    }

    svcSleepThread(1e6);
    for(int i = 0; i < abs(delta); i++)
        load_smdh_icon(*entries[i], ids[i]);

    #undef FIRST
    #undef LAST

    end:
    current_list->previous_scroll = current_list->scroll;
    current_list->previous_selected = current_list->selected_entry;
}

void load_icons_thread(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    do
    {
        svcWaitSynchronization(*arg->update_request, U64_MAX);
        svcClearEvent(*arg->update_request);
        volatile Entry_List_s * current_list = *(volatile Entry_List_s **)arg->thread_argument;
        load_icons((Entry_List_s *)current_list);
    }
    while(arg->run_thread);
}

static u16 previous_path_preview[0x106] = {0};
bool load_preview(Entry_List_s list, int * preview_offset)
{
    if(list.entries == NULL) return false;

    Entry_s entry = list.entries[list.selected_entry];

    if(!memcmp(&previous_path_preview, &entry.path, 0x106*sizeof(u16))) return true;

    char *preview_buffer = NULL;
    u64 size = load_data("/preview.png", entry, &preview_buffer);

    if(!size)
    {
        free(preview_buffer);
        throw_error("No preview found.", ERROR_LEVEL_WARNING);
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

        // mark the new preview as loaded for optimisation
        memcpy(&previous_path_preview, &entry.path, 0x106*sizeof(u16));
        // free the previously loaded preview. wont do anything if there wasnt one
        pp2d_free_texture(TEXTURE_PREVIEW);

        pp2d_load_texture_memory(TEXTURE_PREVIEW, image, (u32)width, (u32)height);

        *preview_offset = (width-400)/2;
        ret = true;
    }
    else
    {
        throw_error("Corrupted/invalid preview.png", ERROR_LEVEL_WARNING);
    }

    free(image);
    free(preview_buffer);

    return ret;
}
