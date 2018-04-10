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

#include "remote.h"
#include "loading.h"
#include "draw.h"
#include "fs.h"
#include "unicode.h"
#include "pp2d/pp2d/pp2d.h"

static Instructions_s browser_instructions[MODE_AMOUNT] = {
    {
        .info_line = NULL,
        .instructions = {
            {
                L"\uE000 Download theme",
                L"\uE001 Go back"
            },
            {
                L"\uE002 Hold for more",
                L"\uE003 Preview theme"
            },
            {
                L"\uE004 Previous page",
                L"\uE005 Next page"
            },
            {
                L"Exit",
                NULL
            }
        }
    },
    {
        .info_line = NULL,
        .instructions = {
            {
                L"\uE000 Download splash",
                L"\uE001 Go back"
            },
            {
                L"\uE002 Hold for more",
                L"\uE003 Preview splash"
            },
            {
                L"\uE004 Previous page",
                L"\uE005 Next page"
            },
            {
                L"Exit",
                NULL
            }
        }
    }
};

static Instructions_s extra_instructions = {
    .info_line = L"Release \uE002 to cancel or hold \uE006 and release \uE002 to do stuff",
    .info_line_color = COLOR_WHITE,
    .instructions = {
        {
            L"\uE079 Jump to page",
            L"\uE07A Search tags"
        },
        {
            L"\uE07B Toggle splash/theme",
            L"\uE07C Reload without cache"
        },
        {
            NULL,
            NULL
        },
        {
            L"Exit",
            NULL
        }
    }
};

static void load_remote_smdh(Entry_s * entry, size_t textureID, bool ignore_cache)
{
    bool not_cached = true;
    char * smdh_buf = NULL;
    u32 smdh_size = load_data("/info.smdh", *entry, &smdh_buf);
    Icon_s * smdh = (Icon_s *)smdh_buf;

    not_cached = !smdh_size || ignore_cache;  // if the size is 0, the file wasn't there

    if(not_cached)
    {
        free(smdh_buf);
        smdh_buf = NULL;
        char * api_url = NULL;
        asprintf(&api_url, THEMEPLAZA_SMDH_FORMAT, entry->tp_download_id);
        smdh_size = http_get(api_url, NULL, &smdh_buf);
        free(api_url);
        smdh = (Icon_s *)smdh_buf;
    }

    if(!smdh_size)
    {
        free(smdh_buf);
        utf8_to_utf16(entry->name, (u8*)"No name", 0x80);
        utf8_to_utf16(entry->desc, (u8*)"No description", 0x100);
        utf8_to_utf16(entry->author, (u8*)"Unknown author", 0x80);
        entry->placeholder_color = RGBA8(rand() % 255, rand() % 255, rand() % 255, 255);
        return;
    }

    memcpy(entry->name, smdh->name, 0x40*sizeof(u16));
    memcpy(entry->desc, smdh->desc, 0x80*sizeof(u16));
    memcpy(entry->author, smdh->author, 0x40*sizeof(u16));

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

    pp2d_free_texture(textureID);
    pp2d_load_texture_memory(textureID, (u8*)image, (u32)width, (u32)height);
    free(image);

    if(not_cached)
    {
        FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_UTF16, entry->path), FS_ATTRIBUTE_DIRECTORY);
        u16 path[0x107] = {0};
        strucat(path, entry->path);
        struacat(path, "/info.smdh");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, smdh_size);
        buf_to_file(smdh_size, fsMakePath(PATH_UTF16, path), ArchiveSD, smdh_buf);
    }
    free(smdh_buf);
}

static void load_remote_entries(Entry_List_s * list, json_t *ids_array, bool ignore_cache)
{
    list->entries_count = json_array_size(ids_array);
    free(list->entries);
    list->entries = calloc(list->entries_count, sizeof(Entry_s));
    free(list->icons_ids);
    list->icons_ids = calloc(list->entries_count, sizeof(ssize_t));
    list->entries_loaded = list->entries_count;

    size_t i = 0;
    json_t * id = NULL;
    json_array_foreach(ids_array, i, id)
    {
        size_t offset = i;
        Entry_s * current_entry = &list->entries[offset];
        current_entry->tp_download_id = json_integer_value(id);
        size_t textureID = list->texture_id_offset + i;

        char * entry_path = NULL;
        asprintf(&entry_path, CACHE_PATH_FORMAT, current_entry->tp_download_id);
        utf8_to_utf16(current_entry->path, (u8*)entry_path, 0x106);
        free(entry_path);

        load_remote_smdh(current_entry, textureID, ignore_cache);
        list->icons_ids[offset] = textureID;
    }
}

static void load_remote_list(Entry_List_s * list, json_int_t page, EntryMode mode, bool ignore_cache)
{
    if(page > list->tp_page_count)
        page = 1;
    if(page <= 0)
        page = list->tp_page_count;

    InstallType loading_screen = INSTALL_NONE;
    if(mode == MODE_THEMES)
        loading_screen = INSTALL_LOADING_REMOTE_THEMES;
    else if(mode == MODE_SPLASHES)
        loading_screen = INSTALL_LOADING_REMOTE_SPLASHES;
    draw_install(loading_screen);

    char * page_json = NULL;
    char * api_url = NULL;
    asprintf(&api_url, THEMEPLAZA_PAGE_FORMAT, page, mode+1, list->tp_search);
    u32 json_len = http_get(api_url, NULL, &page_json);
    free(api_url);

    if(json_len)
    {
        list->texture_id_offset = TEXTURE_REMOTE_ICONS;
        list->tp_current_page = page;
        list->mode = mode;
        list->entry_size = entry_size[mode];
        list->entries_per_screen_v = entries_per_screen_v[mode];
        list->entries_per_screen_h = entries_per_screen_h[mode];

        json_error_t error;
        json_t *root = json_loadb(page_json, json_len, 0, &error);
        if(root)
        {
            const char *key;
            json_t *value;
            json_object_foreach(root, key, value)
            {
                if(json_is_integer(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_COUNT))
                    list->tp_page_count = json_integer_value(value);
                else if(json_is_array(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_IDS))
                    load_remote_entries(list, value, ignore_cache);
                else if(json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_ERROR_MESSAGE) && !strcmp(json_string_value(value), THEMEPLAZA_JSON_ERROR_MESSAGE_NOT_FOUND))
                    throw_error("No results for this search.", ERROR_LEVEL_WARNING);
            }
        }
        else
            DEBUG("json error on line %d: %s\n", error.line, error.text);

        json_decref(root);
    }
    else
        throw_error("Couldn't download ThemePlaza data.\nMake sure WiFi is on.", ERROR_LEVEL_WARNING);

    free(page_json);
}

static u16 previous_path_preview[0x106] = {0};
static bool load_remote_preview(Entry_List_s list, int * preview_offset)
{
    Entry_s entry = list.entries[list.selected_entry];

    bool not_cached = true;

    if(!memcmp(&previous_path_preview, &entry.path, 0x106*sizeof(u16))) return true;

    char * preview_png = NULL;
    u32 preview_size = load_data("/preview.png", entry, &preview_png);

    not_cached = !preview_size;

    if(not_cached)
    {
        free(preview_png);
        preview_png = NULL;

        char * preview_url = NULL;
        asprintf(&preview_url, THEMEPLAZA_PREVIEW_FORMAT, entry.tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_PREVIEW);

        preview_size = http_get(preview_url, NULL, &preview_png);
        free(preview_url);
    }

    if(!preview_size)
    {
        free(preview_png);
        return false;
    }

    bool ret = false;
    u8 * image = NULL;
    unsigned int width = 0, height = 0;

    if((lodepng_decode32(&image, &width, &height, (u8*)preview_png, preview_size)) == 0) // no error
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
        pp2d_free_texture(TEXTURE_REMOTE_PREVIEW);

        pp2d_load_texture_memory(TEXTURE_REMOTE_PREVIEW, image, (u32)width, (u32)height);

        *preview_offset = (width-400)/2;
        ret = true;
    }
    else
    {
        throw_error("Corrupted/invalid preview.png", ERROR_LEVEL_WARNING);
    }

    free(image);

    if(not_cached)
    {
        u16 path[0x107] = {0};
        strucat(path, entry.path);
        struacat(path, "/preview.png");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, preview_size);
        buf_to_file(preview_size, fsMakePath(PATH_UTF16, path), ArchiveSD, preview_png);
    }
    free(preview_png);

    return ret;
}

static void download_remote_entry(Entry_s * entry, EntryMode mode)
{
    char * download_url = NULL;
    asprintf(&download_url, THEMEPLAZA_DOWNLOAD_FORMAT, entry->tp_download_id);

    char * zip_buf = NULL;
    char * filename = NULL;
    draw_install(INSTALL_DOWNLOAD);
    u32 zip_size = http_get(download_url, &filename, &zip_buf);
    free(download_url);

    char path_to_file[0x107] = {0};
    sprintf(path_to_file, "%s%s", main_paths[mode], filename);
    free(filename);

    char * extension = strrchr(path_to_file, '.');
    if (extension == NULL || strcmp(extension, ".zip"))
        strcat(path_to_file, ".zip");

    DEBUG("Saving to sd: %s\n", path_to_file);
    remake_file(fsMakePath(PATH_ASCII, path_to_file), ArchiveSD, zip_size);
    buf_to_file(zip_size, fsMakePath(PATH_ASCII, path_to_file), ArchiveSD, zip_buf);
    free(zip_buf);
}

static SwkbdCallbackResult jump_menu_callback(void* page_number, const char** ppMessage, const char* text, size_t textlen)
{
    int typed_value = atoi(text);
    if(typed_value > *(json_int_t*)page_number)
    {
        *ppMessage = "The new page has to be\nsmaller or equal to the\nnumber of pages!";
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if(typed_value == 0)
    {
        *ppMessage = "The new position has to\nbe positive!";
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

static void jump_menu(Entry_List_s * list)
{
    if(list == NULL) return;

    char numbuf[64] = {0};

    SwkbdState swkbd;

    sprintf(numbuf, "%"  JSON_INTEGER_FORMAT, list->tp_page_count);
    int max_chars = strlen(numbuf);
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, max_chars);

    sprintf(numbuf, "%"  JSON_INTEGER_FORMAT, list->tp_current_page);
    swkbdSetInitialText(&swkbd, numbuf);

    sprintf(numbuf, "Which page do you want to jump to?");
    swkbdSetHintText(&swkbd, numbuf);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Jump", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, max_chars);
    swkbdSetFilterCallback(&swkbd, jump_menu_callback, &list->tp_page_count);

    memset(numbuf, 0, sizeof(numbuf));
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        json_int_t newpage = (json_int_t)atoi(numbuf);
        if(newpage != list->tp_current_page)
            load_remote_list(list, newpage, list->mode, false);
    }
}

static void search_menu(Entry_List_s * list)
{
    const int max_chars = 256;
    char * search = calloc(max_chars+1, sizeof(char));

    SwkbdState swkbd;

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, max_chars);
    swkbdSetHintText(&swkbd, "Which tags do you want to search for?");

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Search", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTBLANK, 0, max_chars);

    SwkbdButton button = swkbdInputText(&swkbd, search, max_chars);
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        free(list->tp_search);
        for(unsigned int i = 0; i < strlen(search); i++)
        {
            if(search[i] == ' ')
                search[i] = '+';
        }
        list->tp_search = search;
        load_remote_list(list, 1, list->mode, false);
    }
    else
    {
        free(search);
    }
}

static void change_selected(Entry_List_s * list, int change_value)
{
    if(abs(change_value) >= list->entries_count) return;

    int newval = list->selected_entry + change_value;

    if(abs(change_value) == 1)
    {
        if(newval < 0)
            newval += list->entries_per_screen_h;
        if(newval/list->entries_per_screen_h != list->selected_entry/list->entries_per_screen_h)
            newval += list->entries_per_screen_h*(-change_value);
    }
    else
    {
        if(newval < 0)
            newval += list->entries_per_screen_h*list->entries_per_screen_v;
        newval %= list->entries_count;
    }
    list->selected_entry = newval;
}

bool themeplaza_browser(EntryMode mode)
{
    bool downloaded = false;

    bool preview_mode = false;
    int preview_offset = 0;

    Entry_List_s list = {0};
    Entry_List_s * current_list = &list;
    current_list->tp_search = strdup("");
    load_remote_list(current_list, 1, mode, false);

    bool extra_mode = false;

    while(aptMainLoop())
    {
        if(current_list->entries == NULL)
            break;

        if(preview_mode)
            draw_preview(TEXTURE_REMOTE_PREVIEW, preview_offset);
        else
        {
            Instructions_s instructions = browser_instructions[mode];
            if(extra_mode)
                instructions = extra_instructions;
            draw_grid_interface(current_list, instructions);
        }
        pp2d_end_draw();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();



        if(kDown & KEY_START)
        {
            exit:
            quit = true;
            downloaded = false;
            break;
        }

        if(extra_mode)
        {
            if(kUp & KEY_X)
                extra_mode = false;
            if(!extra_mode)
            {
                if((kDown | kHeld) & KEY_DLEFT)
                {
                    change_mode:
                    mode++;
                    mode %= MODE_AMOUNT;

                    free(current_list->tp_search);
                    current_list->tp_search = strdup("");

                    load_remote_list(current_list, 1, mode, false);
                }
                else if((kDown | kHeld) & KEY_DUP)
                {
                    jump_menu(current_list);
                }
                else if((kDown | kHeld) & KEY_DRIGHT)
                {
                    load_remote_list(current_list, current_list->tp_current_page, mode, true);
                }
                else if((kDown | kHeld) & KEY_DDOWN)
                {
                    search_menu(current_list);
                }
            }
            continue;
        }

        int selected_entry = current_list->selected_entry;
        Entry_s * current_entry = &current_list->entries[selected_entry];

        if(kDown & KEY_Y)
        {
            toggle_preview:
            if(!preview_mode)
                preview_mode = load_remote_preview(*current_list, &preview_offset);
            else
                preview_mode = false;
        }
        else if(kDown & KEY_B)
        {
            if(preview_mode)
                preview_mode = false;
            else
                break;
        }

        if(preview_mode)
            goto touch;

        if(kDown & KEY_A)
        {
            download_remote_entry(current_entry, mode);
            downloaded = true;
        }
        else if(kDown & KEY_X)
        {
            extra_mode = true;
        }
        else if(kDown & KEY_L)
        {
            load_remote_list(current_list, current_list->tp_current_page-1, mode, false);
        }
        else if(kDown & KEY_R)
        {
            load_remote_list(current_list, current_list->tp_current_page+1, mode, false);
        }

        // Movement in the UI
        else if(kDown & KEY_UP)
        {
            change_selected(current_list, -current_list->entries_per_screen_h);
        }
        else if(kDown & KEY_DOWN)
        {
            change_selected(current_list, current_list->entries_per_screen_h);
        }
        // Quick moving
        else if(kDown & KEY_LEFT)
        {
            change_selected(current_list, -1);
        }
        else if(kDown & KEY_RIGHT)
        {
            change_selected(current_list, 1);
        }

        // Fast scroll using circle pad
        else if(kHeld & KEY_CPAD_UP)
        {
            change_selected(current_list, -1);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_DOWN)
        {
            change_selected(current_list, 1);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_LEFT)
        {
            change_selected(current_list, -current_list->entries_per_screen_v);
            svcSleepThread(FASTSCROLL_WAIT);
        }
        else if(kHeld & KEY_CPAD_RIGHT)
        {
            change_selected(current_list, current_list->entries_per_screen_v);
            svcSleepThread(FASTSCROLL_WAIT);
        }

        touch:
        if((kDown | kHeld) & KEY_TOUCH)
        {
            touchPosition touch = {0};
            hidTouchRead(&touch);

            u16 x = touch.px;
            u16 y = touch.py;

            #define BETWEEN(min, x, max) (min < x && x < max)

            int border = 16;
            if(kDown & KEY_TOUCH)
            {
                if(preview_mode)
                {
                    preview_mode = false;
                    continue;
                }
                else if(y < 24)
                {
                    if(BETWEEN(0, x, 80))
                    {
                        search_menu(current_list);
                    }
                    else if(BETWEEN(320-96, x, 320-72))
                    {
                        break;
                    }
                    else if(BETWEEN(320-72, x, 320-48))
                    {
                        goto exit;
                    }
                    else if(BETWEEN(320-48, x, 320-24))
                    {
                        goto toggle_preview;
                    }
                    else if(BETWEEN(320-24, x, 320))
                    {
                        goto change_mode;
                    }
                }
                else if(BETWEEN(240-24, y, 240) && BETWEEN(176, x, 320))
                {
                    jump_menu(current_list);
                }
                else
                {
                    if(BETWEEN(0, x, border))
                    {
                        load_remote_list(current_list, current_list->tp_current_page-1, mode, false);
                    }
                    else if(BETWEEN(320-border, x, 320))
                    {
                        load_remote_list(current_list, current_list->tp_current_page+1, mode, false);
                    }
                }
            }
            else
            {
                if(preview_mode)
                {
                    preview_mode = false;
                    continue;
                }
                else if(BETWEEN(24, y, 240-24))
                {
                    if(BETWEEN(border, x, 320-border))
                    {
                        x -= border;
                        x /= current_list->entry_size;
                        y -= 24;
                        y /= current_list->entry_size;
                        int new_selected = y*current_list->entries_per_screen_h + x;
                        if(new_selected < current_list->entries_count)
                            current_list->selected_entry = new_selected;
                    }
                }
            }
        }
    }

    free(current_list->entries);
    free(current_list->icons_ids);
    free(current_list->tp_search);

    return downloaded;
}

u32 http_get(const char *url, char ** filename, char ** buf)
{
    Result ret;
    httpcContext context;
    char *new_url = NULL;
    u32 status_code;
    u32 content_size = 0;
    u32 read_size = 0;
    u32 size = 0;
    char *last_buf;

    do {
        ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
        if (ret != 0)
        {
            httpcCloseContext(&context);
            if (new_url != NULL) free(new_url);
            DEBUG("httpcOpenContext %.8lx\n", ret);
            return 0;
        }
        ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
        ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        ret = httpcAddRequestHeaderField(&context, "User-Agent", USER_AGENT);
        ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

        ret = httpcBeginRequest(&context);
        if (ret != 0)
        {
            httpcCloseContext(&context);
            if (new_url != NULL) free(new_url);
            DEBUG("httpcBeginRequest %.8lx\n", ret);
            return 0;
        }

        ret = httpcGetResponseStatusCode(&context, &status_code);
        if(ret!=0){
            httpcCloseContext(&context);
            if(new_url!=NULL) free(new_url);
            DEBUG("httpcGetResponseStatusCode\n");
            return 0;
        }

        if ((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308))
        {
            if (new_url == NULL) new_url = malloc(0x1000);
            ret = httpcGetResponseHeader(&context, "Location", new_url, 0x1000);
            url = new_url;
            httpcCloseContext(&context);
        }
    } while ((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308));

    if (status_code != 200)
    {
        httpcCloseContext(&context);
        if (new_url != NULL) free(new_url);
        DEBUG("status_code, %lu\n", status_code);
        return 0;
    }

    ret = httpcGetDownloadSizeState(&context, NULL, &content_size);
    if (ret != 0)
    {
        httpcCloseContext(&context);
        if (new_url != NULL) free(new_url);
        DEBUG("httpcGetDownloadSizeState\n");
        return 0;
    }

    *buf = malloc(0x1000);
    if (*buf == NULL)
    {
        httpcCloseContext(&context);
        free(new_url);
        DEBUG("malloc\n");
        return 0;
    }

    if(filename)
    {
        char *content_disposition = calloc(1024, sizeof(char));
        ret = httpcGetResponseHeader(&context, "Content-Disposition", content_disposition, 1024);
        if (ret != 0)
        {
            free(content_disposition);
            free(new_url);
            free(*buf);
            DEBUG("httpcGetResponseHeader\n");
            return 0;
        }

        char * tok = strtok(content_disposition, "\"");
        tok = strtok(NULL, "\"");

        if(!(tok))
        {
            free(content_disposition);
            free(new_url);
            free(*buf);
            throw_error("Target is not valid!", ERROR_LEVEL_WARNING);
            DEBUG("filename\n");
            return 0;
        }

        char *illegal_characters = "\"?;:/\\+";
        for (size_t i = 0; i < strlen(tok); i++)
        {
            for (size_t n = 0; n < strlen(illegal_characters); n++)
            {
                if ((tok)[i] == illegal_characters[n])
                {
                    (tok)[i] = '-';
                }
            }
        }

        *filename = calloc(1024, sizeof(char));
        strcpy(*filename, tok);
        free(content_disposition);
    }

    do {
        ret = httpcDownloadData(&context, (*(u8**)buf) + size, 0x1000, &read_size);
        size += read_size;

        if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
        {
            last_buf = *buf;
            *buf = realloc(*buf, size + 0x1000);
            if (*buf == NULL)
            {
                httpcCloseContext(&context);
                free(new_url);
                free(last_buf);
                DEBUG("NULL\n");
                return 0;
            }
        }
    } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

    last_buf = *buf;
    *buf = realloc(*buf, size);
    if (*buf == NULL)
    {
        httpcCloseContext(&context);
        free(new_url);
        free(last_buf);
        DEBUG("realloc\n");
        return 0;
    }

    httpcCloseContext(&context);
    free(new_url);

    DEBUG("size: %lu\n", size);
    return size;
}
