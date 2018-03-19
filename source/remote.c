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

#include "remote.h"
#include "loading.h"
#include "draw.h"
#include "fs.h"
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
                NULL,
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
                NULL,
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

static void load_remote_entry(Entry_s * entry)
{
    char * entry_json = NULL;
    char * api_url = NULL;
    asprintf(&api_url, THEMEPLAZA_ENTRY_FORMAT, entry->tp_download_id);
    u32 json_len = http_get(api_url, NULL, &entry_json);
    free(api_url);

    if(json_len)
    {
        json_error_t error;
        json_t *root = json_loadb(entry_json, json_len, 0, &error);
        if(root)
        {
            const char *key;
            json_t *value;
            json_object_foreach(root, key, value)
            {
                if(json_is_string(value))
                {
                    if(!strcmp(key, THEMEPLAZA_JSON_ENTRY_NAME))
                    {
                        utf8_to_utf16(entry->name, (u8*)json_string_value(value), 0x40);

                    }
                    else if(!strcmp(key, THEMEPLAZA_JSON_ENTRY_DESC))
                        utf8_to_utf16(entry->desc, (u8*)json_string_value(value), 0x80);
                    else if(!strcmp(key, THEMEPLAZA_JSON_ENTRY_AUTH))
                        utf8_to_utf16(entry->author, (u8*)json_string_value(value), 0x40);
                }
            }
        }
        else
            DEBUG("json error on line %d: %s\n", error.line, error.text);

        json_decref(root);
    }
    free(entry_json);
}

static void load_remote_icon(size_t textureID, json_int_t id)
{
    char * icon_data = NULL;
    char * icon_url = NULL;
    asprintf(&icon_url, THEMEPLAZA_ICON_FORMAT, id);
    u32 icon_size = http_get(icon_url, NULL, &icon_data);
    free(icon_url);

    pp2d_free_texture(textureID);
    pp2d_load_texture_png_memory(textureID, icon_data, icon_size);
    free(icon_data);
}

static void load_remote_entries(Entry_List_s * list, json_t *ids_array)
{
    list->entries_count = json_array_size(ids_array);
    list->entries = calloc(list->entries_count, sizeof(Entry_s));
    list->icons_ids = calloc(list->entries_count, sizeof(ssize_t));
    list->entries_loaded = list->entries_count;

    size_t i = 0;
    json_t * id = NULL;
    json_array_foreach(ids_array, i, id)
    {
        size_t offset = i;
        Entry_s * current_entry = &list->entries[offset];
        current_entry->tp_download_id = json_integer_value(id);
        load_remote_entry(current_entry);

        size_t textureID = list->texture_id_offset + i;
        load_remote_icon(textureID, current_entry->tp_download_id);
        list->icons_ids[offset] = textureID;
    }
}

static void load_remote_list(Entry_List_s * list, json_int_t page, EntryMode mode)
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
    asprintf(&api_url, THEMEPLAZA_PAGE_FORMAT, page, mode+1);
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
        DEBUG("%i %i\n", list->entries_per_screen_v, list->entries_per_screen_h);

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
                    load_remote_entries(list, value);
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

static char previous_preview_url[0x100] = {0};
static bool load_remote_preview(Entry_List_s list, int * preview_offset)
{
    Entry_s entry = list.entries[list.selected_entry];

    char * preview_url = NULL;
    asprintf(&preview_url, THEMEPLAZA_PREVIEW_FORMAT, entry.tp_download_id);
    if(!strncmp(previous_preview_url, preview_url, 0x100))
    {
        free(preview_url);
        return true;
    }

    draw_install(INSTALL_LOADING_REMOTE_PREVIEW);
    char * preview_png = NULL;
    u32 preview_size = http_get(preview_url, NULL, &preview_png);

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
        strncpy(previous_preview_url, preview_url, 0x100);
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
    free(preview_url);
    free(preview_png);

    return ret;
}

static void download_remote_entry(Entry_s * entry, EntryMode mode)
{
    char * download_url = NULL;
    asprintf(&download_url, THEMEPLAZA_DOWNLOAD_FORMAT, entry->tp_download_id);

    char * zip_buf = NULL;
    char * filename = NULL;
    u32 zip_size = http_get(download_url, &filename, &zip_buf);
    free(download_url);

    char path_to_file[0x107] = {0};
    strcat(path_to_file, main_paths[mode]);
    strcat(path_to_file, filename);

    char * extension = strrchr(path_to_file, '.');
    if (extension == NULL || strcmp(extension, ".zip"))
        strcat(path_to_file, ".zip");

    remake_file(path_to_file, ArchiveSD, zip_size);
    buf_to_file(zip_size, path_to_file, ArchiveSD, zip_buf);
    free(zip_buf);
}

bool themeplaza_browser(EntryMode mode)
{
    bool downloaded = false;

    bool preview_mode = false;
    int preview_offset = 0;

    Entry_List_s list = {0};
    Entry_List_s * current_list = &list;
    load_remote_list(current_list, 1, mode);

    if(current_list->entries == NULL)
        return false;

    while(aptMainLoop())
    {
        if(preview_mode)
            draw_preview(TEXTURE_REMOTE_PREVIEW, preview_offset);
        else
            draw_grid_interface(current_list, browser_instructions[mode]);
        pp2d_end_draw();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        if(kDown & KEY_START)
        {
            quit = true;
            downloaded = false;
            break;
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

        if(preview_mode)
            goto touch;

        if(kDown & KEY_A)
        {
            download_remote_entry(current_entry, mode);
            downloaded = true;
        }
        else if(kDown & KEY_B)
        {
            break;
        }
        else if(kDown & KEY_L)
        {
            load_remote_list(current_list, current_list->tp_current_page-1, mode);
        }
        else if(kDown & KEY_R)
        {
            load_remote_list(current_list, current_list->tp_current_page+1, mode);
        }

        touch:
        if((kDown | kHeld) & KEY_TOUCH)
        {
            touchPosition touch = {0};
            hidTouchRead(&touch);

            u16 x = touch.px;
            u16 y = touch.py;

            #define BETWEEN(min, x, max) (min < x && x < max)

            if(kDown & KEY_TOUCH)
            {
                if(preview_mode)
                {
                    preview_mode = false;
                    continue
                }
                else if(y < 24)
                {
                    if(BETWEEN(320-96, x, 320-72))
                    {
                        break;
                    }
                    else if(BETWEEN(320-72, x, 320-48))
                    {
                        quit = true;
                        downloaded = false;
                        break;
                    }
                    else if(BETWEEN(320-48, x, 320-24))
                    {
                        goto toggle_preview;
                    }
                    // else if(BETWEEN(320-24, x, 320))
                    // {
                        // goto switch_mode;
                    // }
                }
                else if(y >= 216)
                {
                    if(current_list->entries != NULL && BETWEEN(arrowStartX, x, arrowEndX) && current_list->scroll < current_list->entries_count - current_list->entries_per_screen_v)
                    {
                        change_selected(current_list, current_list->entries_per_screen_v);
                    }
                    else if(current_list->entries != NULL && BETWEEN(176, x, 320))
                    {
                        jump_menu(current_list);
                    }
                }
            }
            else
            {
                if(current_list->entries != NULL && BETWEEN(24, y, 216))
                {
                    for(int i = 0; i < current_list->entries_loaded; i++)
                    {
                        u16 miny = 24 + current_list->entry_size*i;
                        u16 maxy = miny + current_list->entry_size;
                        if(BETWEEN(miny, y, maxy) && current_list->scroll + i < current_list->entries_count)
                        {
                            current_list->selected_entry = current_list->scroll + i;
                            break;
                        }
                    }
                }
            }
        }
    }

    free(current_list->entries);
    free(current_list->icons_ids);

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
        DEBUG("status_code\n");
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
        char *content_disposition = malloc(1024);
        ret = httpcGetResponseHeader(&context, "Content-Disposition", content_disposition, 1024);
        if (ret != 0)
        {
            free(content_disposition);
            free(new_url);
            free(*buf);
            DEBUG("httpcGetResponseHeader\n");
            return 0;
        }

        *filename = strtok(content_disposition, "\"");
        *filename = strtok(NULL, "\"");

        if(!(*filename))
        {
            free(content_disposition);
            free(new_url);
            free(*buf);
            throw_error("Target is not valid!", ERROR_LEVEL_WARNING);
            DEBUG("filename\n");
            return 0;
        }

        char *illegal_characters = "\"?;:/\\+";
        for (size_t i = 0; i < strlen(*filename); i++)
        {
            for (size_t n = 0; n < strlen(illegal_characters); n++)
            {
                if ((*filename)[i] == illegal_characters[n])
                {
                    (*filename)[i] = '-';
                }
            }
        }
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

    return size;
}
