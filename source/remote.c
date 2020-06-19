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
#include "fs.h"
#include "unicode.h"
#include "music.h"

static Instructions_s browser_instructions[MODE_AMOUNT] = {
    {
        .info_line = NULL,
        .instructions = {
            {
                "\uE000 Download theme",
                "\uE001 Go back"
            },
            {
                "\uE002 Hold for more",
                "\uE003 Preview theme"
            },
            {
                "\uE004 Previous page",
                "\uE005 Next page"
            },
            {
                "Exit",
                NULL
            }
        }
    },
    {
        .info_line = NULL,
        .instructions = {
            {
                "\uE000 Download splash",
                "\uE001 Go back"
            },
            {
                "\uE002 Hold for more",
                "\uE003 Preview splash"
            },
            {
                "\uE004 Previous page",
                "\uE005 Next page"
            },
            {
                "Exit",
                NULL
            }
        }
    }
};

static Instructions_s extra_instructions = {
    .info_line = "Release \uE002 to cancel or hold \uE006 and release \uE002 to do stuff",
    .instructions = {
        {
            "\uE079 Jump to page",
            "\uE07A Search tags"
        },
        {
            "\uE07B Toggle splash/theme",
            "\uE07C Reload without cache"
        },
        {
            NULL,
            NULL
        },
        {
            "Exit",
            NULL
        }
    }
};

static void free_icons(Entry_List_s * list)
{
    if(list != NULL)
    {
        if(list->icons != NULL)
        {
            for(int i = 0; i < list->entries_count; i++)
            {
                C3D_TexDelete(list->icons[i]->tex);
                free(list->icons[i]->tex);
                free(list->icons[i]);
            }
            free(list->icons);
        }
    }
}

static C2D_Image * load_remote_smdh(Entry_s * entry, bool ignore_cache)
{
    bool not_cached = true;
    char * smdh_buf = NULL;
    u32 smdh_size = load_data("/info.smdh", *entry, &smdh_buf);

    not_cached = !smdh_size || ignore_cache;  // if the size is 0, the file wasn't there

    if(not_cached)
    {
        free(smdh_buf);
        smdh_buf = NULL;
        char * api_url = NULL;
        asprintf(&api_url, THEMEPLAZA_SMDH_FORMAT, entry->tp_download_id);
        smdh_size = http_get(api_url, NULL, &smdh_buf, INSTALL_NONE, "application/octet-stream");
        free(api_url);
    }

    if(!smdh_size)
    {
        free(smdh_buf);
        smdh_buf = NULL;
    }

    Icon_s * smdh = (Icon_s *)smdh_buf;

    u16 fallback_name[0x81] = {0};
    utf8_to_utf16(fallback_name, (u8*)"No name", 0x80);

    parse_smdh(smdh, entry, fallback_name);
    C2D_Image * image = loadTextureIcon(smdh);

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

    return image;
}

static void load_remote_entries(Entry_List_s * list, json_t *ids_array, bool ignore_cache, InstallType type)
{
    free_icons(list);
    list->entries_count = json_array_size(ids_array);
    free(list->entries);
    list->entries = calloc(list->entries_count, sizeof(Entry_s));
    list->icons = calloc(list->entries_count, sizeof(C2D_Image*));
    list->entries_loaded = list->entries_count;

    size_t i = 0;
    json_t * id = NULL;
    json_array_foreach(ids_array, i, id)
    {
        draw_loading_bar(i, list->entries_count, type);
        Entry_s * current_entry = &list->entries[i];
        current_entry->tp_download_id = json_integer_value(id);

        char * entry_path = NULL;
        asprintf(&entry_path, CACHE_PATH_FORMAT, current_entry->tp_download_id);
        utf8_to_utf16(current_entry->path, (u8*)entry_path, 0x106);
        free(entry_path);

        list->icons[i] = load_remote_smdh(current_entry, ignore_cache);
    }
}

static void load_remote_list(Entry_List_s * list, json_int_t page, EntryMode mode, bool ignore_cache)
{
    if(page > list->tp_page_count)
        page = 1;
    if(page <= 0)
        page = list->tp_page_count;

    list->selected_entry = 0;

    InstallType loading_screen = INSTALL_NONE;
    if(mode == MODE_THEMES)
        loading_screen = INSTALL_LOADING_REMOTE_THEMES;
    else if(mode == MODE_SPLASHES)
        loading_screen = INSTALL_LOADING_REMOTE_SPLASHES;
    draw_install(loading_screen);

    char * page_json = NULL;
    char * api_url = NULL;
    asprintf(&api_url, THEMEPLAZA_PAGE_FORMAT, page, mode+1, list->tp_search);
    u32 json_len = http_get(api_url, NULL, &page_json, INSTALL_NONE, "application/json");
    free(api_url);

    if(json_len)
    {
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
                    load_remote_entries(list, value, ignore_cache, loading_screen);
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
static bool load_remote_preview(Entry_s * entry, C2D_Image* preview_image, int * preview_offset)
{
    bool not_cached = true;

    if(!memcmp(&previous_path_preview, entry->path, 0x106*sizeof(u16))) return true;

    char * preview_png = NULL;
    u32 preview_size = load_data("/preview.png", *entry, &preview_png);

    not_cached = !preview_size;

    if(not_cached)
    {
        free(preview_png);
        preview_png = NULL;

        char * preview_url = NULL;
        asprintf(&preview_url, THEMEPLAZA_PREVIEW_FORMAT, entry->tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_PREVIEW);

        preview_size = http_get(preview_url, NULL, &preview_png, INSTALL_LOADING_REMOTE_PREVIEW, "image/png");
        free(preview_url);
    }

    if(!preview_size)
    {
        free(preview_png);
        return false;
    }

    bool ret = load_preview_from_buffer(preview_png, preview_size, preview_image, preview_offset);

    if(ret && not_cached) // only save the preview if it loaded correctly - isn't corrupted
    {
        u16 path[0x107] = {0};
        strucat(path, entry->path);
        struacat(path, "/preview.png");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, preview_size);
        buf_to_file(preview_size, fsMakePath(PATH_UTF16, path), ArchiveSD, preview_png);
    }

    free(preview_png);

    return ret;
}

static u16 previous_path_bgm[0x106] = {0};
static void load_remote_bgm(Entry_s * entry)
{
    if(!memcmp(&previous_path_bgm, entry->path, 0x106*sizeof(u16))) return;

    char * bgm_ogg = NULL;
    u32 bgm_size = load_data("/bgm.ogg", *entry, &bgm_ogg);

    if(!bgm_size)
    {
        free(bgm_ogg);
        bgm_ogg = NULL;

        char * bgm_url = NULL;
        asprintf(&bgm_url, THEMEPLAZA_BGM_FORMAT, entry->tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_BGM);

        bgm_size = http_get(bgm_url, NULL, &bgm_ogg, INSTALL_LOADING_REMOTE_BGM, "application/ogg, audio/ogg");
        free(bgm_url);

        u16 path[0x107] = {0};
        strucat(path, entry->path);
        struacat(path, "/bgm.ogg");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, bgm_size);
        buf_to_file(bgm_size, fsMakePath(PATH_UTF16, path), ArchiveSD, bgm_ogg);

        memcpy(&previous_path_bgm, entry->path, 0x106*sizeof(u16));
    }

    free(bgm_ogg);
}

static void download_remote_entry(Entry_s * entry, EntryMode mode)
{
    char * download_url = NULL;
    asprintf(&download_url, THEMEPLAZA_DOWNLOAD_FORMAT, entry->tp_download_id);

    char * zip_buf = NULL;
    char * filename = NULL;
    draw_install(INSTALL_DOWNLOAD);
    u32 zip_size = http_get(download_url, &filename, &zip_buf, INSTALL_DOWNLOAD, "application/zip");
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
    (void)textlen;
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

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cance", false);
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
    audio_s * audio = NULL;

    Entry_List_s list = {0};
    Entry_List_s * current_list = &list;
    current_list->tp_search = strdup("");
    load_remote_list(current_list, 1, mode, false);
    C2D_Image preview = {0};

    bool extra_mode = false;

    while(aptMainLoop())
    {
        if(current_list->entries == NULL)
            break;

        if(preview_mode)
        {
            draw_preview(preview, preview_offset);
        }
        else
        {
            Instructions_s instructions = browser_instructions[mode];
            if(extra_mode)
                instructions = extra_instructions;
            draw_grid_interface(current_list, instructions);
        }
        end_frame();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        if(kDown & KEY_START)
        {
            exit:
            quit = true;
            downloaded = false;
            if(audio)
            {
                audio->stop = true;
                svcWaitSynchronization(audio->finished, U64_MAX);
                audio = NULL;
            }
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
            {
                preview_mode = load_remote_preview(current_entry, &preview, &preview_offset);
                if(mode == MODE_THEMES && dspfirm)
                {
                    load_remote_bgm(current_entry);
                    audio = calloc(1, sizeof(audio_s));
                    if(R_FAILED(load_audio(*current_entry, audio))) audio = NULL;
                    if(audio != NULL) play_audio(audio);
                }
            }
            else
            {
                preview_mode = false;
                if(mode == MODE_THEMES && audio != NULL)
                {
                    audio->stop = true;
                    svcWaitSynchronization(audio->finished, U64_MAX);
                    audio = NULL;
                }
            }
        }
        else if(kDown & KEY_B)
        {
            if(preview_mode)
            {
                preview_mode = false;
                if(mode == MODE_THEMES && audio != NULL)
                {
                    audio->stop = true;
                    svcWaitSynchronization(audio->finished, U64_MAX);
                    audio = NULL;
                }
            }
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
                    if(mode == MODE_THEMES && audio)
                    {
                        audio->stop = true;
                        svcWaitSynchronization(audio->finished, U64_MAX);
                        audio = NULL;
                    }
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

    free_preview(preview);

    free_icons(current_list);
    free(current_list->entries);
    free(current_list->tp_search);

    return downloaded;
}

typedef struct header
{
    char *filename; // allocated in parse_header; if NULL, this is user-provided
    u32 file_size; // if == (u64) -1, fall back to chunked read
} header;

typedef enum ParseResult {
    SUCCESS = 0,
    REDIRECT,
    HTTPC_ERROR,
    HTTP_STATUS_UNHANDLED,
    HTTP_UNACCEPTABLE,
    SERVER_IS_MISBEHAVING,
    NO_FILENAME, // provisional
} ParseResult;

// the good paths for this function return SUCCESS or REDIRECT;
// all other paths are failures
static ParseResult parse_header(struct header *out, httpcContext *context, char **redirect_url, bool get_filename, const char *mime)
{
    char* content_buf = calloc(1024, sizeof(char));
    // status code
    u32 status_code;

    if(httpcGetResponseStatusCode(context, &status_code))
    {
        DEBUG("httpcGetResponseStatusCode\n");
        return HTTPC_ERROR;
    }

    DEBUG("HTTP %lu\n", status_code);
    switch(status_code)
    {
        // TODO: special cases for: 401, 403, 404, 500, 503
        // 406 is special: it should be handled in a similar way to a mismatched MIME type
        case 406:
            DEBUG("HTTP 406 Unacceptable; Accept: %s\n", mime);
            return HTTP_UNACCEPTABLE;
        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            if(*redirect_url == NULL)
                *redirect_url = malloc(0x1000);
            return REDIRECT;
        case 200:
            break;
        default:
            return HTTP_STATUS_UNHANDLED;
    }

    // Content-Type

    if (mime)
    {
        httpcGetResponseHeader(context, "Content-Type", content_buf, 1024);
        if (!strstr(mime, content_buf))
        {
            return SERVER_IS_MISBEHAVING;
        }
    }


    // Content-Length

    if(httpcGetDownloadSizeState(context, NULL, &out->file_size))
    {
        DEBUG("httpcGetDownloadSizeState\n");
        return HTTPC_ERROR;
    }

    // Content-Disposition

    if (get_filename)
    {
        if (httpcGetResponseHeader(context, "Content-Disposition", content_buf, 1024))
        {
            free(content_buf);
            DEBUG("httpcGetResponseHeader\n");
            return HTTPC_ERROR;
        }

        // content_buf: Content-Disposition: attachment; ... filename=<filename>;? ...

        char *filename = strstr(content_buf, "filename="); // filename=<filename>;? ...
        if(!filename)
            return NO_FILENAME;

        filename = strpbrk(filename, "=") + 1; // <filename>;?
        char *end = strpbrk(filename, ";");
        if(end)
            *end = '\0'; // <filename>

        if(filename[0] == '"')
        // safe to assume the filename is quoted
        // TODO: what if it isn't?
        {
            filename[strlen(filename) - 1] = '\0';
            filename++;
        }

        char *illegal_char;
        while((illegal_char = strpbrk(filename, "\"?;:/\\+")))
            *illegal_char = '-';
    }
    free(content_buf);

    return SUCCESS;
}

/*
 * call example: written = http_get("url", &filename, &buffer_to_download_to, INSTALL_DOWNLOAD, "application/json");
 */
u32 http_get(const char *url, char ** filename, char ** buf, InstallType install_type, const char *acceptable_mime_types)
{
    Result ret;
    httpcContext context;
    u32 content_size = 0;
    u32 read_size = 0;
    u32 size = 0;
    char *last_buf;
    char *redirect_url = NULL;

    struct header _header = {};

    DEBUG("Original URL: %s\n", url);

redirect: // goto here if we need to redirect
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (ret != 0)
    {
        httpcCloseContext(&context);
        DEBUG("httpcOpenContext %.8lx\n", ret);
        return 0;
    }

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    httpcAddRequestHeaderField(&context, "User-Agent", USER_AGENT);
    httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
    if (acceptable_mime_types)
        httpcAddRequestHeaderField(&context, "Accept", acceptable_mime_types);

    ret = httpcBeginRequest(&context);
    if (ret != 0)
    {
        httpcCloseContext(&context);
        DEBUG("httpcBeginRequest %.8lx\n", ret);
        return 0;
    }

    ParseResult parse = parse_header(&_header, &context, &redirect_url, (bool)filename, acceptable_mime_types);
    switch(parse)
    {
    case SUCCESS:
        free(redirect_url);
        break;
    case REDIRECT:
        url = redirect_url;
        if(redirect_url == NULL)
            redirect_url = malloc(0x1000);
        httpcGetResponseHeader(&context, "Location", redirect_url, 0x1000);
        httpcCloseContext(&context);
        DEBUG("HTTP Redirect: %s\n", redirect_url);
        goto redirect;
    case HTTP_STATUS_UNHANDLED:
    case HTTP_UNACCEPTABLE:
    case SERVER_IS_MISBEHAVING:
    case NO_FILENAME:
    case HTTPC_ERROR: // no special handling here - the service fell over and we can't really do anything about that
    default:
        httpcCloseContext(&context);
        free(_header.filename);
        free(redirect_url);
        return 0;
    }

    *buf = malloc(0x1000);

    if(filename)
        *filename = _header.filename;

    do {
        ret = httpcDownloadData(&context, (*(u8**)buf) + size, 0x1000, &read_size);
        size += read_size;

        if(content_size && install_type != INSTALL_NONE)
            draw_loading_bar(size, content_size, install_type);

        if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
        {
            last_buf = *buf;
            *buf = realloc(*buf, size + 0x1000);
            if (*buf == NULL)
            {
                httpcCloseContext(&context);
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
        free(last_buf);
        DEBUG("realloc\n");
        return 0;
    }

    httpcCloseContext(&context);

    DEBUG("size: %lu\n", size);
    return size;
}
