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
#include <time.h>
#include <jansson.h>

#include "update.h"
#include "config.h"
#include "fs.h"
#include "http.h"

#ifndef UPDATE_REPO
#define UPDATE_REPO "astronautlevel2/Anemone3DS"
#endif

// GitHub's latest release is the newest one that isn't a draft or marked as prerelease
#define LATEST_RELEASE_URL "https://api.github.com/repos/" UPDATE_REPO "/releases/latest"
#define UPDATE_STATE_PATH "/3ds/" APP_TITLE "/update.json"
#define UPDATE_CHECK_INTERVAL (24 * 60 * 60)
#define TAG_SIZE 0x20

extern int __stacksize__;

typedef struct {
    long major, minor, patch;
} Version_s;

static Thread update_thread = NULL;
static volatile bool update_running = false;
static volatile bool update_done = false;
static CURL * update_curl = NULL;
static char latest_tag[TAG_SIZE] = {0};

// "v1.2.3" or "1.2.3", possibly followed by a suffix ("v1.2.3-abcdef" for dev builds)
static bool parse_version(const char * text, Version_s * version)
{
    if (text == NULL)
        return false;
    if (*text == 'v' || *text == 'V')
        ++text;

    long * parts[3] = { &version->major, &version->minor, &version->patch };
    for (int i = 0; i < 3; ++i)
    {
        if (!isdigit((unsigned char)*text))
            return false;

        char * end = NULL;
        *parts[i] = strtol(text, &end, 10);
        text = end;

        if (i < 2)
        {
            if (*text != '.')
                return false;
            ++text;
        }
    }

    return true;
}

static int compare_versions(const Version_s * a, const Version_s * b)
{
    if (a->major != b->major) return a->major < b->major ? -1 : 1;
    if (a->minor != b->minor) return a->minor < b->minor ? -1 : 1;
    if (a->patch != b->patch) return a->patch < b->patch ? -1 : 1;
    return 0;
}

static bool read_latest_release(const char * json, size_t size, char * tag_out)
{
    json_error_t error;
    json_t * release = json_loadb(json, size, 0, &error);
    const char * tag = json_string_value(json_object_get(release, "tag_name"));
    Version_s version;
    const bool valid = tag != NULL && strlen(tag) < TAG_SIZE && parse_version(tag, &version);
    if (valid)
        strcpy(tag_out, tag);

    json_decref(release);
    return valid;
}

static void save_state(time_t checked, const char * tag)
{
    json_t * state = json_pack("{s:I, s:s}", "last_check", (json_int_t)checked, "latest", tag);
    char * text = json_dumps(state, JSON_COMPACT);
    json_decref(state);
    if (text == NULL)
        return;

    const u32 size = strlen(text);
    remake_file(fsMakePath(PATH_ASCII, UPDATE_STATE_PATH), ArchiveSD, size);
    buf_to_file(size, fsMakePath(PATH_ASCII, UPDATE_STATE_PATH), ArchiveSD, text);
    free(text);
}

static bool load_state(time_t * checked, char * tag_out)
{
    char * buf = NULL;
    const u32 size = file_to_buf(fsMakePath(PATH_ASCII, UPDATE_STATE_PATH), ArchiveSD, &buf);
    if (size == 0)
    {
        free(buf);
        return false;
    }

    json_error_t error;
    json_t * state = json_loadb(buf, size, 0, &error);
    free(buf);

    json_t * last_check = json_object_get(state, "last_check");
    const char * tag = json_string_value(json_object_get(state, "latest"));
    const bool valid = json_is_integer(last_check) && tag != NULL && strlen(tag) < TAG_SIZE;
    if (valid)
    {
        *checked = (time_t)json_integer_value(last_check);
        strcpy(tag_out, tag);
    }

    json_decref(state);
    return valid;
}

static void update_check_thread(void * arg)
{
    (void)arg;

    char * buf = NULL;
    u32 size = 0;
    char tag[TAG_SIZE] = {0};
    if (R_SUCCEEDED(http_get_background(update_curl, LATEST_RELEASE_URL, &buf, &size, "application/vnd.github+json, application/json", &update_running)))
    {
        if (!read_latest_release(buf, size, tag))
            tag[0] = '\0';
        save_state(time(NULL), tag);
        strcpy(latest_tag, tag);
    }
    free(buf);

    // publish the result before the main thread sees update_done
    __sync_synchronize();
    update_done = true;
}

void update_check_start(void)
{
    update_done = true; // unless a check actually starts below

    Version_s current;
    if (config.disable_update_check || !parse_version(VERSION, &current))
        return;
    // v0.0.0 is the version of builds without git information
    if (current.major == 0 && current.minor == 0 && current.patch == 0)
        return;

    time_t checked = 0;
    char tag[TAG_SIZE] = {0};
    const time_t now = time(NULL);
    if (load_state(&checked, tag) && checked <= now && now - checked < UPDATE_CHECK_INTERVAL)
    {
        strcpy(latest_tag, tag);
        return;
    }

    u32 wifi_status = 0;
    if (R_FAILED(ACU_GetWifiStatus(&wifi_status)) || wifi_status == 0)
        return;

    update_curl = http_background_handle();
    if (update_curl == NULL)
        return;

    update_done = false;
    update_running = true;
    update_thread = threadCreate(update_check_thread, NULL, __stacksize__, 0x3F, -2, false);
    if (update_thread == NULL)
    {
        update_running = false;
        update_done = true;
    }
}

void update_check_stop(void)
{
    // also aborts the request if it is still running
    update_running = false;
    if (update_thread != NULL)
    {
        threadJoin(update_thread, U64_MAX);
        threadFree(update_thread);
        update_thread = NULL;
    }

    http_background_handle_free(update_curl);
    update_curl = NULL;
}

bool update_check_done(void)
{
    return update_done;
}

const char * update_check_newer_tag(void)
{
    Version_s current, latest;
    if (!update_done || !parse_version(VERSION, &current) || !parse_version(latest_tag, &latest))
        return NULL;

    return compare_versions(&latest, &current) > 0 ? latest_tag : NULL;
}
