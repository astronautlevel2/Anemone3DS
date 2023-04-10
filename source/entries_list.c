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

#include "entries_list.h"
#include "loading.h"
#include "fs.h"
#include "unicode.h"

void delete_entry(Entry_s * entry, bool is_file)
{
    if(is_file)
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_UTF16, entry->path));
    else
        FSUSER_DeleteDirectoryRecursively(ArchiveSD, fsMakePath(PATH_UTF16, entry->path));
}

u32 load_data(const char * filename, const Entry_s * entry, char ** buf)
{
    if(entry->is_zip)
    {
        return zip_file_to_buf(filename+1, entry->path, buf); //the first character will always be '/' because of the other case
    }
    else
    {
        u16 path[0x106] = {0};
        strucat(path, entry->path);
        struacat(path, filename);

        return file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, buf);
    }
}

typedef int (*sort_comparator)(const void *, const void *);
static int compare_entries_by_name(const void * a, const void * b)
{
    Entry_s *entry_a = (Entry_s *)a;
    Entry_s *entry_b = (Entry_s *)b;

    return memcmp(entry_a->name, entry_b->name, 0x40*sizeof(u16));
}
static int compare_entries_by_author(const void * a, const void * b)
{
    Entry_s *entry_a = (Entry_s *)a;
    Entry_s *entry_b = (Entry_s *)b;

    return memcmp(entry_a->author, entry_b->author, 0x40*sizeof(u16));
}
static int compare_entries_by_filename(const void * a, const void * b)
{
    Entry_s *entry_a = (Entry_s *)a;
    Entry_s *entry_b = (Entry_s *)b;

    return memcmp(entry_a->path, entry_b->path, 0x106*sizeof(u16));
}

static void sort_list(Entry_List_s * list, sort_comparator compare_entries)
{
    if(list->entries != NULL && list->entries != NULL)
        qsort(list->entries, list->entries_count, sizeof(Entry_s), compare_entries); //alphabet sort
}

void sort_by_name(Entry_List_s * list)
{
    sort_list(list, compare_entries_by_name);
    list->current_sort = SORT_NAME;
}
void sort_by_author(Entry_List_s * list)
{
    sort_list(list, compare_entries_by_author);
    list->current_sort = SORT_AUTHOR;
}
void sort_by_filename(Entry_List_s * list)
{
    sort_list(list, compare_entries_by_filename);
    list->current_sort = SORT_PATH;
}

#define LOADING_DIR_ENTRIES_COUNT 16
static FS_DirectoryEntry loading_dir_entries[LOADING_DIR_ENTRIES_COUNT];
static u16 loading_entry_path[0x106];
Result load_entries(const char * loading_path, Entry_List_s * list)
{
    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, loading_path));
    if(R_FAILED(res))
    {
        DEBUG("Failed to open folder: %s\n", loading_path);
        return res;
    }

    list_init_capacity(list, LOADING_DIR_ENTRIES_COUNT);

    u32 entries_read = LOADING_DIR_ENTRIES_COUNT;
    while(entries_read == LOADING_DIR_ENTRIES_COUNT)
    {
        res = FSDIR_Read(dir_handle, &entries_read, LOADING_DIR_ENTRIES_COUNT, loading_dir_entries);
        if(R_FAILED(res))
            break;

        for(u32 i = 0; i < entries_read; ++i)
        {
            const FS_DirectoryEntry* dir_entry = &loading_dir_entries[i];
            const bool is_zip = !strcmp(dir_entry->shortExt, "ZIP");
            if(!(dir_entry->attributes & FS_ATTRIBUTE_DIRECTORY) && !is_zip)
                continue;

            loading_entry_path[0] = 0;
            struacat(loading_entry_path, loading_path);
            strucat(loading_entry_path, dir_entry->name);
            char * buf = NULL;
            u32 buflen = 0;

            if (is_zip)
            {
                buflen = zip_file_to_buf("info.smdh", loading_entry_path, &buf);
            }
            else
            {
                const ssize_t len = strulen(loading_entry_path, 0x106);
                struacat(loading_entry_path, "/info.smdh");
                buflen = file_to_buf(fsMakePath(PATH_UTF16, loading_entry_path), ArchiveSD, &buf);
                memset(&loading_entry_path[len], 0, (0x106 - len) * sizeof(u16));
            }

            const ssize_t new_entry_index = list_add_entry(list);
            if(new_entry_index < 0)
            {
                // out of memory: still allow use of currently loaded entries.
                // Many things might die, depending on the heap layout after
                free(buf);
                entries_read = 0;
                break;
            }

            Entry_s * const current_entry = &list->entries[new_entry_index];
            memset(current_entry, 0, sizeof(Entry_s));
            parse_smdh(buflen == sizeof(Icon_s) ? (Icon_s *)buf : NULL, current_entry, dir_entry->name);
            free(buf);

            memcpy(current_entry->path, loading_entry_path, 0x106 * sizeof(u16));
            current_entry->is_zip = is_zip;
        }
    }

    FSDIR_Close(dir_handle);

    return res;
}

void list_init_capacity(Entry_List_s * list, const int init_capacity)
{
    list->entries = malloc(init_capacity * sizeof(Entry_s));
    list->entries_capacity = init_capacity;
}

#define LIST_CAPACITY_THRESHOLD 512
ssize_t list_add_entry(Entry_List_s * list)
{
    if(list->entries_count == list->entries_capacity)
    {
        int next_capacity = list->entries_capacity;
        // expand by doubling until we hit LIST_CAPACITY_THRESHOLD
        // then simply increment by that, to have less extra space leftover
        if(next_capacity < LIST_CAPACITY_THRESHOLD)
        {
            next_capacity *= 2;
        }
        else
        {
            next_capacity += LIST_CAPACITY_THRESHOLD;
        }

        Entry_s * const new_list = realloc(list->entries, next_capacity * sizeof(Entry_s));
        if(new_list == NULL)
        {
            return -1;
        }

        list->entries = new_list;
        list->entries_capacity = next_capacity;
    }

    return list->entries_count++;
}
