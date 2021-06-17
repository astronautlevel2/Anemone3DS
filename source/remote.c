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

#include <ctype.h>

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
    if (list != NULL)
    {
        if (list->icons != NULL)
        {
            for (int i = 0; i < list->entries_count; i++)
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

    if (not_cached)
    {
        free(smdh_buf);
        smdh_buf = NULL;
        char * api_url = NULL;
        asprintf(&api_url, THEMEPLAZA_SMDH_FORMAT, entry->tp_download_id);
        Result res = http_get(api_url, NULL, &smdh_buf, &smdh_size, INSTALL_NONE, "application/octet-stream");
        if (R_FAILED(res))
        {
            free(smdh_buf);
            return false;
        }
        free(api_url);
    }

    if (!smdh_size)
    {
        free(smdh_buf);
        smdh_buf = NULL;
    }

    Icon_s * smdh = (Icon_s *)smdh_buf;

    u16 fallback_name[0x81] = { 0 };
    utf8_to_utf16(fallback_name, (u8 *)"No name", 0x80);

    parse_smdh(smdh, entry, fallback_name);
    C2D_Image * image = loadTextureIcon(smdh);

    if (not_cached)
    {
        FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_UTF16, entry->path), FS_ATTRIBUTE_DIRECTORY);
        u16 path[0x107] = { 0 };
        strucat(path, entry->path);
        struacat(path, "/info.smdh");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, smdh_size);
        buf_to_file(smdh_size, fsMakePath(PATH_UTF16, path), ArchiveSD, smdh_buf);
    }
    free(smdh_buf);

    return image;
}

static void load_remote_entries(Entry_List_s * list, json_t * ids_array, bool ignore_cache, InstallType type)
{
    free_icons(list);
    list->entries_count = json_array_size(ids_array);
    free(list->entries);
    list->entries = calloc(list->entries_count, sizeof(Entry_s));
    list->icons = calloc(list->entries_count, sizeof(C2D_Image * ));
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
        utf8_to_utf16(current_entry->path, (u8 *)entry_path, 0x106);
        free(entry_path);

        list->icons[i] = load_remote_smdh(current_entry, ignore_cache);
    }
}

static void load_remote_list(Entry_List_s * list, json_int_t page, EntryMode mode, bool ignore_cache)
{
    if (page > list->tp_page_count)
        page = 1;
    if (page <= 0)
        page = list->tp_page_count;

    list->selected_entry = 0;

    InstallType loading_screen = INSTALL_NONE;
    if (mode == MODE_THEMES)
        loading_screen = INSTALL_LOADING_REMOTE_THEMES;
    else if (mode == MODE_SPLASHES)
        loading_screen = INSTALL_LOADING_REMOTE_SPLASHES;
    draw_install(loading_screen);

    char * page_json = NULL;
    char * api_url = NULL;
    asprintf(&api_url, THEMEPLAZA_PAGE_FORMAT, page, mode + 1, list->tp_search);
    u32 json_len;
    Result res = http_get(api_url, NULL, &page_json, &json_len, INSTALL_NONE, "application/json");
    free(api_url);
    if (R_FAILED(res))
    {
        free(page_json);
        return;
    }

    if (json_len)
    {
        list->tp_current_page = page;
        list->mode = mode;
        list->entry_size = entry_size[mode];
        list->entries_per_screen_v = entries_per_screen_v[mode];
        list->entries_per_screen_h = entries_per_screen_h[mode];

        json_error_t error;
        json_t * root = json_loadb(page_json, json_len, 0, &error);
        if (root)
        {
            const char * key;
            json_t * value;
            json_object_foreach(root, key, value)
            {
                if (json_is_integer(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_COUNT))
                    list->tp_page_count = json_integer_value(value);
                else if (json_is_array(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_IDS))
                    load_remote_entries(list, value, ignore_cache, loading_screen);
                else if (json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_ERROR_MESSAGE)
                    && !strcmp(json_string_value(value), THEMEPLAZA_JSON_ERROR_MESSAGE_NOT_FOUND))
                    throw_error("No results for this search.", ERROR_LEVEL_WARNING);
            }
        }
        else
            DEBUG("json error on line %d: %s\n", error.line, error.text);

        json_decref(root);
    }
    else
        throw_error("Couldn't download Theme Plaza data.\nMake sure WiFi is on.", ERROR_LEVEL_WARNING);

    free(page_json);
}

static u16 previous_path_preview[0x106] = { 0 };

static bool load_remote_preview(Entry_s * entry, C2D_Image * preview_image, int * preview_offset)
{
    bool not_cached = true;

    if (!memcmp(&previous_path_preview, entry->path, 0x106 * sizeof(u16))) return true;

    char * preview_png = NULL;
    u32 preview_size = load_data("/preview.png", *entry, &preview_png);

    not_cached = !preview_size;

    if (not_cached)
    {
        free(preview_png);
        preview_png = NULL;

        char * preview_url = NULL;
        asprintf(&preview_url, THEMEPLAZA_PREVIEW_FORMAT, entry->tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_PREVIEW);
        Result res = http_get(preview_url, NULL, &preview_png, &preview_size, INSTALL_LOADING_REMOTE_PREVIEW, "image/png");
        free(preview_url);
        if (R_FAILED(res))
            return false;
    }

    if (!preview_size)
    {
        free(preview_png);
        return false;
    }

    bool ret = load_preview_from_buffer(preview_png, preview_size, preview_image, preview_offset);

    if (ret && not_cached) // only save the preview if it loaded correctly - isn't corrupted
    {
        u16 path[0x107] = { 0 };
        strucat(path, entry->path);
        struacat(path, "/preview.png");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, preview_size);
        buf_to_file(preview_size, fsMakePath(PATH_UTF16, path), ArchiveSD, preview_png);
    }

    free(preview_png);

    return ret;
}

static u16 previous_path_bgm[0x106] = { 0 };

static void load_remote_bgm(Entry_s * entry)
{
    if (!memcmp(&previous_path_bgm, entry->path, 0x106 * sizeof(u16))) return;

    char * bgm_ogg = NULL;
    u32 bgm_size = load_data("/bgm.ogg", *entry, &bgm_ogg);

    if (!bgm_size)
    {
        free(bgm_ogg);
        bgm_ogg = NULL;

        char * bgm_url = NULL;
        asprintf(&bgm_url, THEMEPLAZA_BGM_FORMAT, entry->tp_download_id);

        draw_install(INSTALL_LOADING_REMOTE_BGM);

        Result res = http_get(bgm_url, NULL, &bgm_ogg, &bgm_size, INSTALL_LOADING_REMOTE_BGM, "application/ogg, audio/ogg");
        free(bgm_url);
        if (R_FAILED(res))
            return;

        u16 path[0x107] = { 0 };
        strucat(path, entry->path);
        struacat(path, "/bgm.ogg");
        remake_file(fsMakePath(PATH_UTF16, path), ArchiveSD, bgm_size);
        buf_to_file(bgm_size, fsMakePath(PATH_UTF16, path), ArchiveSD, bgm_ogg);

        memcpy(&previous_path_bgm, entry->path, 0x106 * sizeof(u16));
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
    u32 zip_size;
    if(R_FAILED(http_get(download_url, &filename, &zip_buf, &zip_size, INSTALL_DOWNLOAD, "application/zip")))
    {
        free(download_url);
        free(filename);
        return;
    }
    free(download_url);

    save_zip_to_sd(filename, zip_size, zip_buf, mode);
    free(filename);
    free(zip_buf);
}

static SwkbdCallbackResult
jump_menu_callback(void * page_number, const char ** ppMessage, const char * text, size_t textlen)
{
    (void)textlen;
    int typed_value = atoi(text);
    if (typed_value > *(json_int_t *)page_number)
    {
        *ppMessage = "The new page has to be\nsmaller or equal to the\nnumber of pages!";
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if (typed_value == 0)
    {
        *ppMessage = "The new position has to\nbe positive!";
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

static void jump_menu(Entry_List_s * list)
{
    if (list == NULL) return;

    char numbuf[64] = { 0 };

    SwkbdState swkbd;

    sprintf(numbuf, "%"
    JSON_INTEGER_FORMAT, list->tp_page_count);
    int max_chars = strlen(numbuf);
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, max_chars);

    sprintf(numbuf, "%"
    JSON_INTEGER_FORMAT, list->tp_current_page);
    swkbdSetInitialText(&swkbd, numbuf);

    sprintf(numbuf, "Which page do you want to jump to?");
    swkbdSetHintText(&swkbd, numbuf);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Jump", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, max_chars);
    swkbdSetFilterCallback(&swkbd, jump_menu_callback, &list->tp_page_count);

    memset(numbuf, 0, sizeof(numbuf));
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if (button == SWKBD_BUTTON_CONFIRM)
    {
        json_int_t newpage = (json_int_t)atoi(numbuf);
        if (newpage != list->tp_current_page)
            load_remote_list(list, newpage, list->mode, false);
    }
}

static void search_menu(Entry_List_s * list)
{
    const int max_chars = 256;
    char * search = calloc(max_chars + 1, sizeof(char));

    SwkbdState swkbd;

    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, max_chars);
    swkbdSetHintText(&swkbd, "Which tags do you want to search for?");

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Search", true);
    swkbdSetValidation(&swkbd, SWKBD_NOTBLANK, 0, max_chars);

    SwkbdButton button = swkbdInputText(&swkbd, search, max_chars);
    if (button == SWKBD_BUTTON_CONFIRM)
    {
        free(list->tp_search);
        for (unsigned int i = 0; i < strlen(search); i++)
        {
            if (search[i] == ' ')
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
    if (abs(change_value) >= list->entries_count) return;

    int newval = list->selected_entry + change_value;

    if (abs(change_value) == 1)
    {
        if (newval < 0)
            newval += list->entries_per_screen_h;
        if (newval / list->entries_per_screen_h != list->selected_entry / list->entries_per_screen_h)
            newval += list->entries_per_screen_h * (-change_value);
        newval %= list->entries_count;
    }
    else
    {
        if (newval < 0)
            newval += list->entries_per_screen_h * list->entries_per_screen_v;
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

    Entry_List_s list = { 0 };
    Entry_List_s * current_list = &list;
    current_list->tp_search = strdup("");
    load_remote_list(current_list, 1, mode, false);
    C2D_Image preview = { 0 };

    bool extra_mode = false;

    while (aptMainLoop())
    {
        if (current_list->entries == NULL)
            break;

        if (preview_mode)
        {
            draw_preview(preview, preview_offset);
        }
        else
        {
            Instructions_s instructions = browser_instructions[mode];
            if (extra_mode)
                instructions = extra_instructions;
            draw_grid_interface(current_list, instructions);
        }
        end_frame();

        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        u32 kUp = hidKeysUp();

        if (kDown & KEY_START)
        {
        exit:
            quit = true;
            downloaded = false;
            if (audio)
            {
                audio->stop = true;
                svcWaitSynchronization(audio->finished, U64_MAX);
                audio = NULL;
            }
            break;
        }

        if (extra_mode)
        {
            if (kUp & KEY_X)
                extra_mode = false;
            if (!extra_mode)
            {
                if ((kDown | kHeld) & KEY_DLEFT)
                {
                change_mode:
                    mode++;
                    mode %= MODE_AMOUNT;

                    free(current_list->tp_search);
                    current_list->tp_search = strdup("");

                    load_remote_list(current_list, 1, mode, false);
                }
                else if ((kDown | kHeld) & KEY_DUP)
                {
                    jump_menu(current_list);
                }
                else if ((kDown | kHeld) & KEY_DRIGHT)
                {
                    load_remote_list(current_list, current_list->tp_current_page, mode, true);
                }
                else if ((kDown | kHeld) & KEY_DDOWN)
                {
                    search_menu(current_list);
                }
            }
            continue;
        }

        int selected_entry = current_list->selected_entry;
        Entry_s * current_entry = &current_list->entries[selected_entry];

        if (kDown & KEY_Y)
        {
        toggle_preview:
            if (!preview_mode)
            {
                preview_mode = load_remote_preview(current_entry, &preview, &preview_offset);
                if (mode == MODE_THEMES && dspfirm)
                {
                    load_remote_bgm(current_entry);
                    audio = calloc(1, sizeof(audio_s));
                    if (R_FAILED(load_audio(*current_entry, audio))) audio = NULL;
                    if (audio != NULL) play_audio(audio);
                }
            }
            else
            {
                preview_mode = false;
                if (mode == MODE_THEMES && audio != NULL)
                {
                    audio->stop = true;
                    svcWaitSynchronization(audio->finished, U64_MAX);
                    audio = NULL;
                }
            }
        }
        else if (kDown & KEY_B)
        {
            if (preview_mode)
            {
                preview_mode = false;
                if (mode == MODE_THEMES && audio != NULL)
                {
                    audio->stop = true;
                    svcWaitSynchronization(audio->finished, U64_MAX);
                    audio = NULL;
                }
            }
            else
                break;
        }

        if (preview_mode)
            goto touch;

        if (kDown & KEY_A)
        {
            download_remote_entry(current_entry, mode);
            downloaded = true;
        }
        else if (kDown & KEY_X)
        {
            extra_mode = true;
        }
        else if (kDown & KEY_L)
        {
            load_remote_list(current_list, current_list->tp_current_page - 1, mode, false);
        }
        else if (kDown & KEY_R)
        {
            load_remote_list(current_list, current_list->tp_current_page + 1, mode, false);
        }

            // Movement in the UI
        else if (kDown & KEY_UP)
        {
            change_selected(current_list, -current_list->entries_per_screen_h);
        }
        else if (kDown & KEY_DOWN)
        {
            change_selected(current_list, current_list->entries_per_screen_h);
        }
            // Quick moving
        else if (kDown & KEY_LEFT)
        {
            change_selected(current_list, -1);
        }
        else if (kDown & KEY_RIGHT)
        {
            change_selected(current_list, 1);
        }

    touch:
        if ((kDown | kHeld) & KEY_TOUCH)
        {
            touchPosition touch = { 0 };
            hidTouchRead(&touch);

            u16 x = touch.px;
            u16 y = touch.py;

#define BETWEEN(min, x, max) (min < x && x < max)

            int border = 16;
            if (kDown & KEY_TOUCH)
            {
                if (preview_mode)
                {
                    preview_mode = false;
                    if (mode == MODE_THEMES && audio)
                    {
                        audio->stop = true;
                        svcWaitSynchronization(audio->finished, U64_MAX);
                        audio = NULL;
                    }
                    continue;
                }
                else if (y < 24)
                {
                    if (BETWEEN(0, x, 80))
                    {
                        search_menu(current_list);
                    }
                    else if (BETWEEN(320 - 96, x, 320 - 72))
                    {
                        break;
                    }
                    else if (BETWEEN(320 - 72, x, 320 - 48))
                    {
                        goto exit;
                    }
                    else if (BETWEEN(320 - 48, x, 320 - 24))
                    {
                        goto toggle_preview;
                    }
                    else if (BETWEEN(320 - 24, x, 320))
                    {
                        goto change_mode;
                    }
                }
                else if (BETWEEN(240 - 24, y, 240) && BETWEEN(176, x, 320))
                {
                    jump_menu(current_list);
                }
                else
                {
                    if (BETWEEN(0, x, border))
                    {
                        load_remote_list(current_list, current_list->tp_current_page - 1, mode, false);
                    }
                    else if (BETWEEN(320 - border, x, 320))
                    {
                        load_remote_list(current_list, current_list->tp_current_page + 1, mode, false);
                    }
                }
            }
            else
            {
                if (preview_mode)
                {
                    preview_mode = false;
                    continue;
                }
                else if (BETWEEN(24, y, 240 - 24))
                {
                    if (BETWEEN(border, x, 320 - border))
                    {
                        x -= border;
                        x /= current_list->entry_size;
                        y -= 24;
                        y /= current_list->entry_size;
                        int new_selected = y * current_list->entries_per_screen_h + x;
                        if (new_selected < current_list->entries_count)
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
    char ** filename; // pointer to location for filename; if NULL, no filename is parsed
    u32 file_size; // if == 0, fall back to chunked read
    Result result_code;
} header;

typedef enum ParseResult
{
    SUCCESS, // 200/203 (203 indicates a successful request with a transformation applied by a proxy)
    REDIRECT, // 301/302/307/308
    HTTPC_ERROR,
    SERVER_IS_MISBEHAVING,
    SEE_OTHER = 303, // Theme Plaza returns these
    HTTP_UNAUTHORIZED = 401,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_UNACCEPTABLE = 406, // like 204, usually doesn't happen
    HTTP_PROXY_UNAUTHORIZED = 407,
    HTTP_GONE = 410,
    HTTP_URI_TOO_LONG = 414,
    HTTP_IM_A_TEAPOT = 418, // Note that a combined coffee/tea pot that is temporarily out of coffee should instead return 503.
    HTTP_UPGRADE_REQUIRED = 426, // the 3DS doesn't support HTTP/2, so we can't upgrade - inform and return
    HTTP_LEGAL_REASONS = 451,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVAILABLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
} ParseResult;

// unescapes URL-encoded string non-destructively
static char * unescape(const char * url)
{
    char * unescaped = malloc(strlen(url) + 1);
    // source shamelessly ripped from https://stackoverflow.com/a/14530993/4073959
    char a;
    char b;
    while (*url)
    {
        if ((*url == '%') &&
            ((a = url[1]) && (b = url[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *unescaped++ = 16 * a + b;
            url += 3;
        }
        else if (*url == '+')
        {
            *unescaped++ = ' ';
            url++;
        }
        else
        {
            *unescaped++ = *url++;
        }
    }
    *unescaped++ = '\0';
    return unescaped;
}

// the good paths for this function return SUCCESS, ABORTED, or REDIRECT;
// all other paths are failures
static ParseResult parse_header(struct header * out, httpcContext * context, const char * mime)
{
    // status code
    u32 status_code;

    out->result_code = httpcGetResponseStatusCode(context, &status_code);
    if (R_FAILED(out->result_code))
    {
        DEBUG("httpcGetResponseStatusCode\n");
        return HTTPC_ERROR;
    }

    DEBUG("HTTP %lu\n", status_code);
    switch (status_code)
    {
    case 301:
    case 302:
    case 307:
    case 308:
        return REDIRECT;
    case 200:
    case 203:
        break;
    default:
        return (ParseResult)status_code;
    }

    char content_buf[1024] = {0};

    // Content-Type

    if (mime)
    {
        out->result_code = httpcGetResponseHeader(context, "Content-Type", content_buf, 1024);
        if (R_FAILED(out->result_code))
        {
            return HTTPC_ERROR;
        }

        if (!strstr(mime, content_buf))
        {
            return SERVER_IS_MISBEHAVING;
        }
    }

    // Content-Length

    out->result_code = httpcGetDownloadSizeState(context, NULL, &out->file_size);
    if (R_FAILED(out->result_code))
    {
        DEBUG("httpcGetDownloadSizeState\n");
        return HTTPC_ERROR; // no need to free, program dies anyway
    }

    // Content-Disposition

    if (out->filename)
    {
        bool present = 1;
        out->result_code = httpcGetResponseHeader(context, "Content-Disposition", content_buf, 1024);
        if (R_FAILED(out->result_code))
        {
            if (out->result_code == (long)0xD8A0A028L)
                present = 0;
            else
            {
                DEBUG("httpcGetResponseHeader\n");
                return HTTPC_ERROR;
            }
        }

        // content_buf: Content-Disposition: attachment; ... filename=<filename>;? ...

        if (present)
        {
            char * filename = strstr(content_buf, "filename="); // filename=<filename>;? ...
            // in the extreme fringe case that filename is missing:
            if (filename != NULL)
            {
                filename = strpbrk(filename, "=") + 1; // <filename>;?
                char * end = strpbrk(filename, ";");
                if (end)
                    *end = '\0'; // <filename>

                // safe to assume the filename is quoted
                // (if it isn't, then we already have a null-terminated string <filename>)
                if (filename[0] == '"')
                {
                    filename[strlen(filename) - 1] = '\0';
                    filename++;
                }

                *out->filename = malloc(strlen(filename) + 1);
                strcpy(*out->filename, filename);
            }
            else
            {
                *out->filename = NULL;
            }
        }
        else
        {
            *out->filename = NULL;
        }
    }
    return SUCCESS;
}

#define ZIP_NOT_AVAILABLE "ZIP not found at this URL\nIf you believe this is an error, please\ncontact the site administrator"

/*
 * call example: written = http_get("url", &filename, &buffer_to_download_to, &filesize, INSTALL_DOWNLOAD, "application/json");
 */
Result http_get(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types)
{
    Result ret;
    httpcContext context;
    char redirect_url[0x824] = {0};
    char new_url[0x824] = {0};

    struct header _header = { .filename = filename };

    DEBUG("Original URL: %s\n", url);

redirect: // goto here if we need to redirect
    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(ret))
    {
        httpcCloseContext(&context);
        DEBUG("httpcOpenContext %.8lx\n", ret);
        return ret;
    }

    httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
    httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
    httpcAddRequestHeaderField(&context, "User-Agent", USER_AGENT);
    httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
    if (acceptable_mime_types)
        httpcAddRequestHeaderField(&context, "Accept", acceptable_mime_types);

    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret))
    {
        httpcCloseContext(&context);
        DEBUG("httpcBeginRequest %.8lx\n", ret);
        return ret;
    }

#define ERROR_BUFFER_SIZE 0x80
    char err_buf[ERROR_BUFFER_SIZE];
    ParseResult parse = parse_header(&_header, &context, acceptable_mime_types);
    switch (parse)
    {
    case SUCCESS:
        break;
    case HTTPC_ERROR:
        DEBUG("httpc error %lx\n", _header.result_code);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "Error in HTTPC sysmodule - 0x%08lx.\nIf you are seeing this, please contact an\nAnemone developer on the Theme Plaza Discord.", _header.result_code);
        throw_error(err_buf, ERROR_LEVEL_ERROR);
        quit = true;
        httpcCloseContext(&context);
        return _header.result_code;
    case SEE_OTHER:
        if (strstr(url, THEMEPLAZA_BASE_URL))
        {
            snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 303 See Other (Theme Plaza)\nHas this theme been approved?");
            goto error;
        }
        else
        {
            snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 303 See Other\nDownload the resource directly\nor contact the site administrator.");
            goto error;
        }
    case REDIRECT:
        httpcGetResponseHeader(&context, "Location", redirect_url, 0x824);
        httpcCloseContext(&context);
        if (*redirect_url == '/') // if relative URL
        {
            strcpy(new_url, url);
            char * last_slash = strchr(strchr(strchr(new_url, '/') + 1, '/') + 1, '/');
            if (last_slash) *last_slash = '\0'; // prevents a NULL deref in case the original domain was not /-delimited
            strncat(new_url, redirect_url, 0x824 - strlen(new_url));
            url = new_url;
        }
        else
        {
            url = redirect_url;
        }
        DEBUG("HTTP Redirect: %s %s\n", new_url, *redirect_url == '/' ? "relative" : "absolute");
        goto redirect;
    case SERVER_IS_MISBEHAVING:
        DEBUG("Server is misbehaving (provided resource with incorrect MIME)\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, ZIP_NOT_AVAILABLE);
        goto error;
    case HTTP_NOT_FOUND:
    case HTTP_GONE: ;
        const char * http_error = parse == HTTP_NOT_FOUND ? "404 Not Found" : "410 Gone";
        DEBUG("HTTP %s; URL: %s\n", http_error, url);
        if (strstr(url, THEMEPLAZA_BASE_URL) && parse == HTTP_NOT_FOUND)
            snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 404 Not Found\nHas this theme been approved?");
        else
            snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP %s\nCheck that the URL is correct.", http_error);
        goto error;
    case HTTP_UNACCEPTABLE:
        DEBUG("HTTP 406 Unacceptable; Accept: %s\n", acceptable_mime_types);
        snprintf(err_buf, ERROR_BUFFER_SIZE, ZIP_NOT_AVAILABLE);
        goto error;
    case HTTP_UNAUTHORIZED:
    case HTTP_FORBIDDEN:
    case HTTP_PROXY_UNAUTHORIZED:
        DEBUG("HTTP %u: device not authenticated\n", parse);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP %s\nContact the site administrator.", parse == HTTP_UNAUTHORIZED
            ? "401 Unauthorized"
            : parse == HTTP_FORBIDDEN
            ? "403 Forbidden"
            : "407 Proxy Authentication Required");
        goto error;
    case HTTP_URI_TOO_LONG:
        DEBUG("HTTP 414; URL is too long, maybe too many redirects?\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 414 URI Too Long\nThe QR code points to a really long URL.\nDownload the file directly.");
        goto error;
    case HTTP_IM_A_TEAPOT:
        DEBUG("HTTP 418 I'm a teapot\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 418 I'm a teapot\nContact the site administrator.");
        goto error;
    case HTTP_UPGRADE_REQUIRED:
        DEBUG("HTTP 426; HTTP/2 required\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 426 Upgrade Required\nThe 3DS cannot connect to this server.\nContact the site administrator.");
        goto error;
    case HTTP_LEGAL_REASONS:
        DEBUG("HTTP 451; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 451 Unavailable for Legal Reasons\nSome entity is preventing access\nto the host server for legal reasons.");
        goto error;
    case HTTP_INTERNAL_SERVER_ERROR:
        DEBUG("HTTP 500; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 500 Internal Server Error\nContact the site administrator.");
        goto error;
    case HTTP_BAD_GATEWAY:
        DEBUG("HTTP 502; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 502 Bad Gateway\nContact the site administrator.");
        goto error;
    case HTTP_SERVICE_UNAVAILABLE:
        DEBUG("HTTP 503; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 503 Service Unavailable\nContact the site administrator.");
        goto error;
    case HTTP_GATEWAY_TIMEOUT:
        DEBUG("HTTP 504; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP 504 Gateway Timeout\nContact the site administrator.");
        goto error;
    default:
        DEBUG("HTTP %u; URL: %s\n", parse, url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, "HTTP %u\nIf you believe this is unexpected, please\ncontact the site administrator.", parse);
        goto error;
    }

    goto no_error;
error:
    throw_error(err_buf, ERROR_LEVEL_WARNING);
    Result res = httpcCloseContext(&context);
    if (R_FAILED(res)) return res;
    return MAKERESULT(RL_TEMPORARY, RS_CANCELED, RM_APPLICATION, RD_NO_DATA);
no_error:;
    u32 chunk_size;
    if (_header.file_size)
        // the only reason we chunk this at all is for the download bar;
        // in terms of efficiency, allocating the full size
        // would avoid 3 reallocs whenever the server isn't lying
        chunk_size = _header.file_size / 4;
    else
        chunk_size = 0x80000;

    *buf = NULL;
    char * new_buf;
    *size = 0;
    u32 read_size = 0;

    do
    {
        new_buf = realloc(*buf, *size + chunk_size);
        if (new_buf == NULL)
        {
            httpcCloseContext(&context);
            free(*buf);
            DEBUG("realloc failed in http_get - file possibly too large?\n");
            return MAKERESULT(RL_FATAL, RS_INTERNAL, RM_KERNEL, RD_OUT_OF_MEMORY);
        }
        *buf = new_buf;

        // download exactly chunk_size bytes and toss them into buf.
        // size contains the current offset into buf.
        ret = httpcDownloadData(&context, (u8*)(*buf) + *size, chunk_size, &read_size);
        /* FIXME: I have no idea why this doesn't work, but it causes problems. Look into it later
        if (R_FAILED(ret))
        {
            httpcCloseContext(&context);
            free(*buf);
            DEBUG("download failed in http_get\n");
            return ret;
        }
        */
        *size += read_size;

        if (_header.file_size && install_type != INSTALL_NONE)
            draw_loading_bar(*size, _header.file_size, install_type);
    } while (ret == (Result)HTTPC_RESULTCODE_DOWNLOADPENDING);
    httpcCloseContext(&context);

    // shrink to size
    new_buf = realloc(*buf, *size);
    if (new_buf == NULL)
    {
        httpcCloseContext(&context);
        free(*buf);
        DEBUG("shrinking realloc failed\n"); // 何？
        return MAKERESULT(RL_FATAL, RS_INTERNAL, RM_KERNEL, RD_OUT_OF_MEMORY);
    }
    *buf = new_buf;

    DEBUG("size: %lu\n", *size);
    if (filename) {
        if (*filename == NULL)
        {
            // Content-Disposition extraction failed somehow
            *filename = basename(unescape(url));
        }
        DEBUG("filename: %s\n", *filename);
    }
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
 }
