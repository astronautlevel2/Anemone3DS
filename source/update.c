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
#include "draw.h"
#include "fs.h"
#include "http.h"
#include "ui_strings.h"

#ifndef UPDATE_REPO
#define UPDATE_REPO "astronautlevel2/Anemone3DS"
#endif

// GitHub's latest release is the newest one that isn't a draft or marked as prerelease
#define LATEST_RELEASE_URL "https://api.github.com/repos/" UPDATE_REPO "/releases/latest"
#define UPDATE_STATE_PATH "/3ds/" APP_TITLE "/update.json"
#define UPDATE_CHECK_INTERVAL (24 * 60 * 60)
#define TAG_SIZE 0x20
#define URL_SIZE 0x200
#define SHA256_HEX_SIZE 65
#define INSTALL_CHUNK_SIZE 0x20000

extern int __stacksize__;

typedef struct {
    long major, minor, patch;
} Version_s;

typedef struct {
    char url[URL_SIZE];
    u32 size;
    char sha256[SHA256_HEX_SIZE]; // empty when GitHub didn't provide a digest
} Asset_s;

typedef struct {
    char tag[TAG_SIZE];
    Asset_s cia;
    Asset_s tdsx;
} Release_s;

static Thread update_thread = NULL;
static volatile bool update_running = false;
static volatile bool update_done = false;
static CURL * update_curl = NULL;
static Release_s latest = {0};

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

static bool ends_with(const char * text, const char * suffix)
{
    const size_t text_length = strlen(text);
    const size_t suffix_length = strlen(suffix);
    return text_length >= suffix_length && !strcasecmp(text + text_length - suffix_length, suffix);
}

static void read_assets(json_t * release, Release_s * out)
{
    size_t i = 0;
    json_t * asset = NULL;
    json_array_foreach(json_object_get(release, "assets"), i, asset)
    {
        const char * name = json_string_value(json_object_get(asset, "name"));
        const char * url = json_string_value(json_object_get(asset, "browser_download_url"));
        json_t * size = json_object_get(asset, "size");
        const char * digest = json_string_value(json_object_get(asset, "digest"));
        if (name == NULL || url == NULL || strlen(url) >= URL_SIZE || !json_is_integer(size))
            continue;

        Asset_s * target = NULL;
        if (ends_with(name, ".cia"))
            target = &out->cia;
        else if (ends_with(name, ".3dsx"))
            target = &out->tdsx;
        if (target == NULL || target->url[0] != '\0')
            continue;

        strcpy(target->url, url);
        target->size = json_integer_value(size);
        if (digest != NULL && !strncmp(digest, "sha256:", 7) && strlen(digest + 7) == SHA256_HEX_SIZE - 1)
            strcpy(target->sha256, digest + 7);
    }
}

static bool read_latest_release(const char * json, size_t size, Release_s * out)
{
    json_error_t error;
    json_t * release = json_loadb(json, size, 0, &error);
    const char * tag = json_string_value(json_object_get(release, "tag_name"));
    Version_s version;
    const bool valid = tag != NULL && strlen(tag) < TAG_SIZE && parse_version(tag, &version);
    if (valid)
    {
        memset(out, 0, sizeof(Release_s));
        strcpy(out->tag, tag);
        read_assets(release, out);
    }

    json_decref(release);
    return valid;
}

static json_t * asset_to_json(const Asset_s * asset)
{
    return json_pack("{s:s, s:I, s:s}", "url", asset->url, "size", (json_int_t)asset->size, "sha256", asset->sha256);
}

static void asset_from_json(json_t * json, Asset_s * asset)
{
    const char * url = json_string_value(json_object_get(json, "url"));
    const char * sha256 = json_string_value(json_object_get(json, "sha256"));
    json_t * size = json_object_get(json, "size");
    if (url == NULL || strlen(url) >= URL_SIZE || sha256 == NULL || strlen(sha256) >= SHA256_HEX_SIZE || !json_is_integer(size))
        return;

    strcpy(asset->url, url);
    strcpy(asset->sha256, sha256);
    asset->size = json_integer_value(size);
}

static void save_state(time_t checked, const Release_s * release)
{
    json_t * state = json_pack("{s:I, s:s, s:o, s:o}", "last_check", (json_int_t)checked, "latest", release->tag,
        "cia", asset_to_json(&release->cia), "3dsx", asset_to_json(&release->tdsx));
    char * text = json_dumps(state, JSON_COMPACT);
    json_decref(state);
    if (text == NULL)
        return;

    const u32 size = strlen(text);
    remake_file(fsMakePath(PATH_ASCII, UPDATE_STATE_PATH), ArchiveSD, size);
    buf_to_file(size, fsMakePath(PATH_ASCII, UPDATE_STATE_PATH), ArchiveSD, text);
    free(text);
}

static bool load_state(time_t * checked, Release_s * release)
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
        memset(release, 0, sizeof(Release_s));
        strcpy(release->tag, tag);
        asset_from_json(json_object_get(state, "cia"), &release->cia);
        asset_from_json(json_object_get(state, "3dsx"), &release->tdsx);
    }

    json_decref(state);
    return valid;
}

static void update_check_thread(void * arg)
{
    (void)arg;

    char * buf = NULL;
    u32 size = 0;
    Release_s release = {0};
    if (R_SUCCEEDED(http_get_background(update_curl, LATEST_RELEASE_URL, &buf, &size, "application/vnd.github+json, application/json", &update_running)))
    {
        if (!read_latest_release(buf, size, &release))
            memset(&release, 0, sizeof(release));
        save_state(time(NULL), &release);
        latest = release;
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
    Release_s cached = {0};
    const time_t now = time(NULL);
    if (load_state(&checked, &cached) && checked <= now && now - checked < UPDATE_CHECK_INTERVAL)
    {
        latest = cached;
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
    Version_s current, latest_version;
    if (!update_done || !parse_version(VERSION, &current) || !parse_version(latest.tag, &latest_version))
        return NULL;

    return compare_versions(&latest_version, &current) > 0 ? latest.tag : NULL;
}

// the 3dsx build replaces the file it was started from, so it needs a usable path to it
static bool valid_3dsx_path(const char * app_path)
{
    return app_path != NULL && !strncmp(app_path, "sdmc:/", 6) && ends_with(app_path, ".3dsx")
        && strlen(app_path) < 0x100;
}

bool update_can_install(const char * app_path)
{
    if (update_check_newer_tag() == NULL)
        return false;

    if (envIsHomebrew())
        return latest.tdsx.url[0] != '\0' && valid_3dsx_path(app_path);
    return latest.cia.url[0] != '\0';
}

static bool sha256_matches(const Asset_s * asset, const char * buf, u32 size)
{
    if (asset->sha256[0] == '\0')
        return true;

    u8 hash[32] = {0};
    if (R_FAILED(FSUSER_UpdateSha256Context(buf, size, hash)))
        return false;

    char hex[SHA256_HEX_SIZE] = {0};
    for (int i = 0; i < 32; ++i)
        sprintf(hex + i * 2, "%02x", hash[i]);
    return !strcasecmp(hex, asset->sha256);
}

static Result install_cia(const char * buf, u32 size)
{
    Result res = amInit();
    if (R_FAILED(res))
        return res;

    Handle cia = 0;
    res = AM_StartCiaInstall(MEDIATYPE_SD, &cia);
    if (R_SUCCEEDED(res))
    {
        for (u32 offset = 0; offset < size && R_SUCCEEDED(res); offset += INSTALL_CHUNK_SIZE)
        {
            draw_loading_bar(offset, size, INSTALL_UPDATE);
            const u32 chunk = min(INSTALL_CHUNK_SIZE, size - offset);
            u32 written = 0;
            res = FSFILE_Write(cia, &written, offset, buf + offset, chunk, 0);
            if (R_SUCCEEDED(res) && written != chunk)
                res = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_NO_DATA);
        }

        // nothing is replaced unless the whole CIA was written
        if (R_SUCCEEDED(res))
            res = AM_FinishCiaInstall(cia);
        else
            AM_CancelCIAInstall(cia);
    }

    amExit();
    return res;
}

static Result install_3dsx(const char * buf, u32 size, const char * app_path)
{
    const Result failed = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_NO_DATA);
    char new_path[0x110], old_path[0x110];
    snprintf(new_path, sizeof(new_path), "%s.new", app_path);
    snprintf(old_path, sizeof(old_path), "%s.old", app_path);

    // write the new file next to the old one first, so an interrupted update leaves the old one working
    FILE * file = fopen(new_path, "wb");
    if (file == NULL)
        return failed;
    const size_t written = fwrite(buf, 1, size, file);
    const bool closed = fclose(file) == 0;
    if (written != size || !closed)
    {
        remove(new_path);
        return failed;
    }

    remove(old_path);
    if (rename(app_path, old_path) != 0)
    {
        remove(new_path);
        return failed;
    }
    if (rename(new_path, app_path) != 0)
    {
        rename(old_path, app_path);
        return failed;
    }

    remove(old_path);
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
}

bool update_install(const char * app_path)
{
    if (!update_can_install(app_path))
        return false;

    const bool is_3dsx = envIsHomebrew();
    const Asset_s * asset = is_3dsx ? &latest.tdsx : &latest.cia;

    char * buf = NULL;
    u32 size = 0;
    draw_install(INSTALL_DOWNLOAD);
    // http_get shows its own errors, and nothing when the user cancelled
    if (R_FAILED(http_get(asset->url, NULL, &buf, &size, INSTALL_DOWNLOAD, NULL)))
    {
        free(buf);
        return false;
    }

    Result res;
    if ((asset->size != 0 && size != asset->size) || !sha256_matches(asset, buf, size))
        res = MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_SIZE);
    else
    {
        draw_install(INSTALL_UPDATE);
        res = is_3dsx ? install_3dsx(buf, size, app_path) : install_cia(buf, size);
    }
    free(buf);

    if (R_FAILED(res))
    {
        char error[0x100] = {0};
        snprintf(error, sizeof(error), language.draw.update_failed, res);
        throw_error(error, ERROR_LEVEL_WARNING);
        return false;
    }

    return true;
}
