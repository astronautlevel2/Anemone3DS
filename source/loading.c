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

#include "loading.h"
#include "fs.h"
#include "unicode.h"
#include "music.h"
#include "draw.h"
#include "conversion.h"
#include "ui_strings.h"

#include <png.h>

void copy_texture_data(C3D_Tex * texture, const u16 * src, const Entry_Icon_s * current_icon)
{
    // pointer to rgb565, offset by the number of rows and columns specified by current_icon
    // (reminder that this is z order curve storage)
    u16 * dest = ((u16 *)texture->data) + (current_icon->y * texture->width) + (current_icon->x * 8);
    for (int j = 0; j < 48; j += 8)
    {
        memcpy(dest, src, 48 * 8 * sizeof(u16));
        src += 48 * 8;
        dest += texture->width * 8;
    }
    GSPGPU_InvalidateDataCache(texture->data, texture->size);
}

void parse_smdh(Icon_s * icon, Entry_s * entry, const u16 * fallback_name)
{
    if(icon == NULL)
    {
        memcpy(entry->name, fallback_name, 0x80);
        utf8_to_utf16(entry->desc, (u8 *)"No description", 0x100);
        utf8_to_utf16(entry->author, (u8 *)"Unknown author", 0x80);
        entry->placeholder_color = C2D_Color32(rand() % 255, rand() % 255, rand() % 255, 255);
        return;
    }

    memcpy(entry->name, icon->name, 0x40 * sizeof(u16));
    memcpy(entry->desc, icon->desc, 0x80 * sizeof(u16));
    memcpy(entry->author, icon->author, 0x40 * sizeof(u16));
    entry->placeholder_color = 0;
}

static Icon_s * load_entry_icon(const Entry_s * entry)
{
    char * info_buffer = NULL;
    u32 size = load_data("/info.smdh", entry, &info_buffer);
    if(size != sizeof(Icon_s))
    {
        free(info_buffer);
        return NULL;
    }

    return (Icon_s *)info_buffer;
}

void load_icons_first(Entry_List_s * list, bool silent)
{
    if(list == NULL || list->entries == NULL) return;

    if(!silent)
        draw_install(INSTALL_LOADING_ICONS);

    int starti = 0, endi = 0;

    if(list->entries_count <= list->entries_loaded * ICONS_OFFSET_AMOUNT)
    {
        DEBUG("small load\n");
        // if the list is one that doesnt need swapping, load everything at once
        endi = list->entries_count;
    }
    else
    {
        DEBUG("extended load\n");
        // otherwise, load around to prepare for swapping
        starti = list->scroll - list->entries_loaded * ICONS_VISIBLE;
        endi = starti + list->entries_loaded * ICONS_OFFSET_AMOUNT;
    }

    for(int entry_i = starti, icon_i = 0; entry_i < endi; ++entry_i, ++icon_i)
    {
        if(!silent)
            draw_loading_bar(icon_i, endi-starti, INSTALL_LOADING_ICONS);

        int offset = entry_i;
        if(offset < 0)
            offset += list->entries_count;
        if(offset >= list->entries_count)
            offset -= list->entries_count;

        Entry_s * const current_entry = &list->entries[offset];
        Icon_s * const smdh = load_entry_icon(current_entry);
        if(smdh != NULL)
        {
            if(current_entry->placeholder_color == 0)
                parse_smdh(smdh, current_entry, current_entry->path + strlen(list->loading_path));
            copy_texture_data(&list->icons_texture, smdh->big_icon, &list->icons_info[icon_i]);
            free(smdh);
        }
    }
}

static void reverse(Entry_Icon_s a[], int sz) {
    int i, j;
    Entry_Icon_s tmp;
    for (i = 0, j = sz; i < j; i++, j--) {
        memcpy(&tmp, &a[i], sizeof(Entry_Icon_s));
        memcpy(&a[i], &a[j], sizeof(Entry_Icon_s));
        memcpy(&a[j], &tmp, sizeof(Entry_Icon_s));
    }
}

static void rotate(Entry_Icon_s array[], int size, int amt) {
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

            if(list->entries_count > list->entries_loaded * 2 && list->previous_scroll < list->entries_loaded && list->selected_entry >= list->entries_count - list->entries_loaded)
            {
                list->scroll = list->entries_count - list->entries_loaded;
            }
            else if(list->entries_count > list->entries_loaded * 2 && list->selected_entry < list->entries_loaded && list->previous_selected >= list->entries_count - list->entries_loaded)
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

static bool load_icons(Entry_List_s * current_list, Handle mutex)
{
    if(current_list == NULL || current_list->entries == NULL)
        return false;

    handle_scrolling(current_list);

    if(current_list->entries_count <= current_list->entries_loaded * ICONS_OFFSET_AMOUNT || current_list->previous_scroll == current_list->scroll)
        return false; // return if the list is one that doesnt need swapping, or if nothing changed

    #define SIGN(x) (x > 0 ? 1 : ((x < 0) ? -1 : 0))

    int delta = current_list->scroll - current_list->previous_scroll;
    if(abs(delta) >= current_list->entries_count - current_list->entries_loaded * (ICONS_OFFSET_AMOUNT-1))
        delta = -SIGN(delta) * (current_list->entries_count - abs(delta));

    int starti = current_list->scroll;
    int endi = starti + abs(delta);

    if(delta < 0)
    {
        endi -= abs(delta) + 1;
        starti += abs(delta) - 1;
    }

    int ctr = 0;
    Entry_s ** const entries = calloc(abs(delta), sizeof(Entry_s *));
    int * const indexes = calloc(abs(delta), sizeof(int));
    bool released = false;

    Entry_Icon_s * const icons = current_list->icons_info;

    for(int i = starti; i != endi; i++, ctr++)
    {
        int index = 0;
        int offset = i;

        rotate(icons, ICONS_OFFSET_AMOUNT * current_list->entries_loaded, -SIGN(delta));

        if(delta > 0)
        {
            index = current_list->entries_loaded * ICONS_OFFSET_AMOUNT - delta + i - starti;
            offset += current_list->entries_loaded * ICONS_UNDER - delta;
        }
        else
        {
            index = 0 - delta - 1 + i - starti;
            offset -= current_list->entries_loaded * ICONS_VISIBLE;
            i -= 2; //i-- twice to counter the i++, needed only for this case
        }

        if(offset < 0)
            offset += current_list->entries_count;
        if(offset >= current_list->entries_count)
            offset -= current_list->entries_count;

        entries[ctr] = &current_list->entries[offset];
        indexes[ctr] = index;
    }

    #undef SIGN

    if(abs(delta) <= current_list->entries_loaded)
    {
        svcReleaseMutex(mutex);
        released = true;
    }

    svcSleepThread(1e7);
    starti = 0;
    endi = abs(delta);
    for(int i = starti; i < endi; i++)
    {
        Entry_s * const current_entry = entries[i];
        const int index = indexes[i];
        const Entry_Icon_s * const current_icon = &icons[index];

        Icon_s * const smdh = load_entry_icon(current_entry);
        if(smdh != NULL)
        {
            if(current_entry->placeholder_color == 0)
                parse_smdh(smdh, current_entry, current_entry->path + strlen(current_list->loading_path));
            copy_texture_data(&current_list->icons_texture, smdh->big_icon, current_icon);
            free(smdh);
        }

        if(!released && i > endi/2)
        {
            svcReleaseMutex(mutex);
            released = true;
        }
    }

    free(entries);
    free(indexes);

    current_list->previous_scroll = current_list->scroll;

    return released;
}

void load_icons_thread(void * void_arg)
{
    Thread_Arg_s * arg = (Thread_Arg_s *)void_arg;
    Handle mutex = *(Handle *)arg->thread_arg[1];
    do {
        svcWaitSynchronization(mutex, U64_MAX);
        Entry_List_s * const current_list = *(Entry_List_s ** volatile)arg->thread_arg[0];
        const bool released = load_icons(current_list, mutex);
        if(!released)
            svcReleaseMutex(mutex);
    } while(arg->run_thread);
}

bool load_preview_from_buffer(char * row_pointers, u32 size, C2D_Image * preview_image, int * preview_offset)
{
    int height = SCREEN_HEIGHT * 2;
    int width = (uint32_t)((size / 4) / height);

    free_preview(*preview_image);

    C3D_Tex * tex = malloc(sizeof(C3D_Tex));
    preview_image->tex = tex;

    Tex3DS_SubTexture * subt3x = malloc(sizeof(Tex3DS_SubTexture));
    subt3x->width = width;
    subt3x->height = height;
    subt3x->left = 0.0f;
    subt3x->top = 1.0f;
    subt3x->right = width/512.0f;
    subt3x->bottom = 1.0-(height/512.0f);
    preview_image->subtex = subt3x;

    C3D_TexInit(preview_image->tex, 512, 512, GPU_RGBA8);

    memset(preview_image->tex->data, 0, preview_image->tex->size);

    for(int j = 0; j < height; j++) {
        png_bytep row = (png_bytep)(row_pointers + (width * 4 * j));
        for(int i = 0; i < width; i++) {
            png_bytep px = &(row[i * 4]);
            u32 dst = ((((j >> 3) * (512 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy(preview_image->tex->data + dst, px, sizeof(u32));
        }
    }

    *preview_offset = (width - TOP_SCREEN_WIDTH) / 2;

    return true;
}

static u16 previous_path_preview[0x106] = {0};
bool load_preview(const Entry_List_s * list, C2D_Image * preview_image, int * preview_offset)
{
    if(list->entries == NULL) return false;

    const Entry_s * entry = &list->entries[list->selected_entry];

    if(!memcmp(&previous_path_preview, &entry->path, 0x106 * sizeof(u16))) return true;

    char * preview_buffer = NULL;
    u32 size = load_data("/preview.png", entry, &preview_buffer);

    if(size)
    {
        if (!(size = png_to_abgr(&preview_buffer, size)))
        {
            return false;
        }
    }
    else
    {
        free(preview_buffer);

        const int top_size =  TOP_SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_COLOR_DEPTH;
        const int out_size = top_size * 2;

        char * rgba_buffer = malloc(out_size);
        memset(rgba_buffer, 0, out_size);
        bool found_splash = false;

        // try to assembly a preview from the splash screens
        size = load_data("/splash.bin", entry, &preview_buffer);
        if (size)
        {
            found_splash = true;
            bin_to_abgr(&preview_buffer, size);

            memcpy(rgba_buffer, preview_buffer, top_size);
            free(preview_buffer);
        }

        size = load_data("/splashbottom.bin", entry, &preview_buffer);
        if (size)
        {
            found_splash = true;
            bin_to_abgr(&preview_buffer, size);

            const int buffer_width = TOP_SCREEN_WIDTH * SCREEN_COLOR_DEPTH;
            const int bottom_buffer_width = BOTTOM_SCREEN_WIDTH * SCREEN_COLOR_DEPTH;
            const int bottom_centered_offset = (TOP_SCREEN_WIDTH - BOTTOM_SCREEN_WIDTH) / 2;

            // Store the bottom splash screen under the top splash and centered
            for (int i = 0; i < SCREEN_HEIGHT; ++i)
                memcpy(
                    rgba_buffer + top_size + (buffer_width * i) + (bottom_centered_offset * SCREEN_COLOR_DEPTH),
                    preview_buffer + (bottom_buffer_width * i),
                    bottom_buffer_width
                );
    
            free(preview_buffer);
        }

        if (!found_splash)
        {
            free(rgba_buffer);
            throw_error(language.loading.no_preview, ERROR_LEVEL_WARNING);
            return false;
        }

        size = out_size;
        preview_buffer = rgba_buffer;
    }

    bool ret = load_preview_from_buffer(preview_buffer, size, preview_image, preview_offset);
    free(preview_buffer);

    if(ret)
    {
        // mark the new preview as loaded for optimisation
        memcpy(&previous_path_preview, &entry->path, 0x106 * sizeof(u16));
    }

    return ret;
}

void free_preview(C2D_Image preview)
{
    if(preview.tex)
        C3D_TexDelete(preview.tex);
    free(preview.tex);
    free((Tex3DS_SubTexture *)preview.subtex);
}

// Initialize the audio struct
Result load_audio(const Entry_s * entry, audio_s * audio) 
{
    audio->filesize = load_data("/bgm.ogg", entry, &audio->filebuf);
    if (audio->filesize == 0) {
        free(audio);
        DEBUG("<load_audio> File not found!\n");
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }

    audio->mix[0] = audio->mix[1] = 1.0f; // Determines volume for the 12 (?) different outputs. See http://smealum.github.io/ctrulib/channel_8h.html#a30eb26f1972cc3ec28370263796c0444

    ndspChnSetInterp(0, NDSP_INTERP_LINEAR); 
    ndspChnSetMix(0, audio->mix); // See mix comment above

    FILE * file = fmemopen(audio->filebuf, audio->filesize, "rb");
    DEBUG("<load_audio> Filesize: %ld\n", audio->filesize);
    if(file != NULL) 
    {
        int e = ov_open(file, &audio->vf, NULL, 0);
        if (e < 0) 
        {
            DEBUG("<load_audio> Vorbis: %d\n", e);
            free(audio->filebuf);
            free(audio);
            fclose(file);
            return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_NO_DATA);
        }

        vorbis_info * vi = ov_info(&audio->vf, -1);
        ndspChnSetRate(0, vi->rate);// Set sample rate to what's read from the ogg file
        if (vi->channels == 2) {
            DEBUG("<load_audio> Using stereo\n");
            ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); // 2 channels == Stereo
        } else {
            DEBUG("<load_audio> Invalid number of channels\n");
            free(audio->filebuf);
            free(audio);
            fclose(file);
            return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_APPLICATION, RD_NO_DATA);
        }

        audio->wave_buf[0].nsamples = audio->wave_buf[1].nsamples = vi->rate / 4; // 4 bytes per sample, samples = rate (bytes) / 4
        audio->wave_buf[0].status = audio->wave_buf[1].status = NDSP_WBUF_DONE; // Used in play to stop from writing to current buffer
        audio->wave_buf[0].data_vaddr = linearAlloc(BUF_TO_READ); // Most vorbis packets should only be 4 KB at most (?) Possibly dangerous assumption
        audio->wave_buf[1].data_vaddr = linearAlloc(BUF_TO_READ);
        DEBUG("<load_audio> Success!\n");
        return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
    } else {
        free(audio->filebuf);
        free(audio);
        DEBUG("<load_audio> fmemopen failed!\n");
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_APPLICATION, RD_NOT_FOUND);
    }
}
