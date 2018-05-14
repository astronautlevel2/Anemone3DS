/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2018 Contributors in CONTRIBUTORS.md
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
#include "fs.h"
#include "unicode.h"
#include "music.h"
#include "draw.h"

void delete_entry(Entry_s * entry, bool is_file)
{
    if(is_file)
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_UTF16, entry->path));
    else
        FSUSER_DeleteDirectoryRecursively(ArchiveSD, fsMakePath(PATH_UTF16, entry->path));
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
    // pp2d_free_texture(textureID);

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
    // pp2d_load_texture_memory(textureID, (u8*)image, (u32)width, (u32)height);
    free(image);
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

Result load_entries(const char * loading_path, Entry_List_s * list)
{
    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, loading_path));
    if(R_FAILED(res))
    {
        DEBUG("Failed to open folder: %s\n", loading_path);
        return res;
    }

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
        Entry_s * new_list = realloc(list->entries, list->entries_count * sizeof(Entry_s));
        if(new_list == NULL)
        {
            free(list->entries);
            list->entries = NULL;
            res = -1;
            DEBUG("break\n");
            break;
        }
        else
            list->entries = new_list;

        Entry_s * current_entry = &(list->entries[list->entries_count-1]);
        memset(current_entry, 0, sizeof(Entry_s));

        struacat(current_entry->path, loading_path);
        strucat(current_entry->path, dir_entry.name);

        current_entry->is_zip = !strcmp(dir_entry.shortExt, "ZIP");
        parse_smdh(current_entry, dir_entry.name);
    }

    FSDIR_Close(dir_handle);

    return res;
}

void load_icons_first(Entry_List_s * list, bool silent)
{
    if(list == NULL || list->entries == NULL) return;

    if(!silent)
        draw_install(INSTALL_LOADING_ICONS);

    int starti = 0, endi = 0;

    if(list->entries_count <= list->entries_loaded*ICONS_OFFSET_AMOUNT)
    {
        DEBUG("small load\n");
        // if the list is one that doesnt need swapping, load everything at once
        endi = list->entries_count;
    }
    else
    {
        DEBUG("extended load\n");
        // otherwise, load around to prepare for swapping
        starti = list->scroll - list->entries_loaded*ICONS_VISIBLE;
        endi = starti + list->entries_loaded*ICONS_OFFSET_AMOUNT;
    }

    list->icons_ids = calloc(endi-starti, sizeof(ssize_t));

    ssize_t * icons_ids = list->icons_ids;
    ssize_t id = list->texture_id_offset;

    for(int i = starti; i < endi; i++, id++)
    {
        if(!silent)
            draw_loading_bar(i - starti, endi-starti, INSTALL_LOADING_ICONS);

        int offset = i;
        if(offset < 0)
            offset += list->entries_count;
        if(offset >= list->entries_count)
            offset -= list->entries_count;

        Entry_s current_entry = list->entries[offset];
        load_smdh_icon(current_entry, id);

        icons_ids[i-starti] = id;
    }
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

void handle_scrolling(Entry_List_s * list)
{
    // Scroll the menu up or down if the selected theme is out of its bounds
    //----------------------------------------------------------------
    if(list->entries_count > list->entries_loaded)
    {
        for(int i = 0; i < list->entries_count; i++)
        {
            int change = 0;

            if(list->entries_count > list->entries_loaded*2 && list->previous_scroll < list->entries_loaded && list->selected_entry >= list->entries_count - list->entries_loaded)
            {
                list->scroll = list->entries_count - list->entries_loaded;
            }
            else if(list->entries_count > list->entries_loaded*2 && list->selected_entry < list->entries_loaded && list->previous_selected >= list->entries_count - list->entries_loaded)
            {
                list->scroll = 0;
            }
            else if(list->selected_entry == list->previous_selected+1 && list->selected_entry == list->scroll+list->entries_loaded)
            {
                change = 1;
            }
            else if(list->selected_entry == list->previous_selected-1 && list->selected_entry == list->scroll-1)
            {
                change = -1;
            }
            else if(list->selected_entry == list->previous_selected+list->entries_loaded || list->selected_entry >= list->scroll + list->entries_loaded)
            {
                change = list->entries_loaded;
            }
            else if(list->selected_entry == list->previous_selected-list->entries_loaded || list->selected_entry < list->scroll)
            {
                change = -list->entries_loaded;
            }

            list->scroll += change;

            if(list->scroll < 0)
                list->scroll = 0;
            else if(list->scroll > list->entries_count - list->entries_loaded)
                list->scroll = list->entries_count - list->entries_loaded;

            if(!change)
                list->previous_selected = list->selected_entry;
            else
                list->previous_selected += change;
        }
    }
    //----------------------------------------------------------------
}

static void load_icons(Entry_List_s * current_list)
{
    if(current_list == NULL || current_list->entries == NULL)
        return;

    handle_scrolling(current_list);

    if(current_list->entries_count <= current_list->entries_loaded*ICONS_OFFSET_AMOUNT || current_list->previous_scroll == current_list->scroll)
        return; // return if the list is one that doesnt need swapping, or if nothing changed

    #define SIGN(x) (x > 0 ? 1 : ((x < 0) ? -1 : 0))

    int delta = current_list->scroll - current_list->previous_scroll;
    if(abs(delta) >= current_list->entries_count - current_list->entries_loaded*(ICONS_OFFSET_AMOUNT-1))
        delta = -SIGN(delta) * (current_list->entries_count - abs(delta));

    int starti = current_list->scroll;
    int endi = starti + abs(delta);

    if(delta < 0)
    {
        endi -= abs(delta) + 1;
        starti += abs(delta) - 1;
    }

    int ctr = 0;
    Entry_s ** entries = calloc(abs(delta), sizeof(Entry_s *));
    ssize_t * ids = calloc(abs(delta), sizeof(ssize_t));

    #define FIRST(arr) arr[0]
    #define LAST(arr) arr[current_list->entries_loaded*ICONS_OFFSET_AMOUNT - 1]

    ssize_t * icons_ids = current_list->icons_ids;

    for(int i = starti; i != endi; i++, ctr++)
    {
        ssize_t id = 0;
        int offset = i;

        rotate(icons_ids, ICONS_OFFSET_AMOUNT*current_list->entries_loaded, -1*SIGN(delta));

        if(delta > 0)
        {
            id = LAST(icons_ids);
            offset += current_list->entries_loaded*ICONS_UNDER - delta;
        }
        else
        {
            id = FIRST(icons_ids);
            offset -= current_list->entries_loaded*ICONS_VISIBLE;
            i -= 2; //i-- twice to counter the i++, needed only for this case
        }

        if(offset < 0)
            offset += current_list->entries_count;
        if(offset >= current_list->entries_count)
            offset -= current_list->entries_count;

        entries[ctr] = &current_list->entries[offset];
        ids[ctr] = id;
    }

    #undef FIRST
    #undef LAST
    #undef SIGN

    svcSleepThread(1e6);
    for(int i = 0; i < abs(delta); i++)
    {
        Entry_s current_entry = *entries[i];
        ssize_t id = ids[i];
        load_smdh_icon(current_entry, id);
    }

    free(entries);
    free(ids);

    current_list->previous_scroll = current_list->scroll;
}

void load_icons_thread(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    Handle update_request = *(Handle *)arg->thread_arg[1];
    do
    {
        svcWaitSynchronization(update_request, U64_MAX);
        svcClearEvent(update_request);
        volatile Entry_List_s * current_list = *(volatile Entry_List_s **)arg->thread_arg[0];
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
        // pp2d_free_texture(TEXTURE_PREVIEW);

        // pp2d_load_texture_memory(TEXTURE_PREVIEW, image, (u32)width, (u32)height);

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

// Initialize the audio struct
Result load_audio(Entry_s entry, audio_s *audio) 
{
    audio->filesize = load_data("/bgm.ogg", entry, &audio->filebuf);
    if (audio->filesize == 0) {
        free(audio);
        DEBUG("File not found!\n");
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }

    audio->mix[0] = audio->mix[1] = 1.0f; // Determines volume for the 12 (?) different outputs. See http://smealum.github.io/ctrulib/channel_8h.html#a30eb26f1972cc3ec28370263796c0444
    svcCreateEvent(&audio->finished, RESET_STICKY);
    
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR); 
    ndspChnSetMix(0, audio->mix); // See mix comment above

    FILE *file = fmemopen(audio->filebuf, audio->filesize, "rb");
    DEBUG("Filesize: %lld\n", audio->filesize);
    if(file != NULL) 
    {
        int e = ov_open(file, &audio->vf, NULL, 0);
        if (e < 0) 
        {
            DEBUG("Vorbis: %d\n", e);
            free(audio->filebuf);
            free(audio);
            fclose(file);
            return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_NO_DATA);
        }

        vorbis_info *vi = ov_info(&audio->vf, -1);
        ndspChnSetRate(0, vi->rate);// Set sample rate to what's read from the ogg file
        if (vi->channels == 2) {
            DEBUG("Using stereo\n");
            ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); // 2 channels == Stereo
        } else {
            DEBUG("Invalid number of channels\n");
            free(audio->filebuf);
            free(audio);
            fclose(file);
            return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_NO_DATA);
        }

        audio->wave_buf[0].nsamples = audio->wave_buf[1].nsamples = vi->rate / 4; // 4 bytes per sample, samples = rate (bytes) / 4
        audio->wave_buf[0].status = audio->wave_buf[1].status = NDSP_WBUF_DONE; // Used in play to stop from writing to current buffer
        audio->wave_buf[0].data_vaddr = linearAlloc(BUF_TO_READ); // Most vorbis packets should only be 4 KB at most (?) Possibly dangerous assumption
        audio->wave_buf[1].data_vaddr = linearAlloc(BUF_TO_READ);
        DEBUG("Success!\n");
        return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
    } else {
        free(audio->filebuf);
        free(audio);
        DEBUG("fmemopen failed!\n");
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }
}