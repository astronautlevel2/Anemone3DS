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

#include <malloc.h>
#include <curl/curl.h>

#include "http.h"
#include "remote.h"
#include "ui_strings.h"

#define SOC_BUFFER_SIZE 0x100000
#define MAX_DOWNLOAD_SIZE (32 * 1024 * 1024)
#define MAX_REDIRECTS 10
#define ERROR_BUFFER_SIZE 0x100
#define PROGRESS_REDRAW_MS 33

typedef struct {
    char * buf;
    size_t size;
    size_t capacity;
    size_t content_length; // 0 when the server didn't send one
    bool too_large;

    char * content_type;
    char * content_disposition;
} http_response;

typedef struct {
    InstallType install_type;
    u64 last_redraw;
} http_progress;

static u32 * soc_buffer = NULL;
static CURL * curl = NULL; // main thread only

static bool http_init(void)
{
    if (curl != NULL)
        return true;

    if (soc_buffer == NULL)
    {
        soc_buffer = memalign(0x1000, SOC_BUFFER_SIZE);
        if (soc_buffer == NULL)
            return false;

        if (R_FAILED(socInit(soc_buffer, SOC_BUFFER_SIZE)))
        {
            free(soc_buffer);
            soc_buffer = NULL;
            return false;
        }
    }

    curl = curl_easy_init();
    return curl != NULL;
}

void http_exit(void)
{
    if (curl != NULL)
    {
        curl_easy_cleanup(curl);
        curl = NULL;
    }

    if (soc_buffer != NULL)
    {
        socExit();
        free(soc_buffer);
        soc_buffer = NULL;
    }
}

static bool mime_type_is_acceptable(const char *acceptable_mime_types, const char *mime_type)
{
    if (!acceptable_mime_types || !mime_type)
        return true;

    while (*mime_type == ' ' || *mime_type == '\t')
        ++mime_type;

    size_t mime_len = strcspn(mime_type, "; \t\r\n");

    const char *cursor = acceptable_mime_types;
    while (*cursor)
    {
        while (*cursor == ' ' || *cursor == ',' || *cursor == ';')
            ++cursor;

        const char *token_start = cursor;
        while (*cursor && *cursor != ' ' && *cursor != ',' && *cursor != ';')
            ++cursor;

        size_t token_len = cursor - token_start;
        if (token_len != 0 && token_len == mime_len && !strncasecmp(token_start, mime_type, mime_len))
            return true;
    }

    return false;
}

static size_t write_callback(char * ptr, size_t size, size_t nmemb, void * userdata)
{
    http_response * response = (http_response *)userdata;
    const size_t bytes = size * nmemb;

    if (response->size + bytes + 1 > MAX_DOWNLOAD_SIZE)
    {
        response->too_large = true;
        return 0;
    }

    if (response->size + bytes + 1 > response->capacity)
    {
        // the whole file at once when its size is known, otherwise grow in steps
        size_t capacity = response->content_length + 1;
        if (capacity < response->size + bytes + 1 || capacity > MAX_DOWNLOAD_SIZE)
        {
            capacity = response->capacity ? response->capacity : 0x10000;
            while (response->size + bytes + 1 > capacity)
                capacity *= 2;
        }

        char * new_buf = realloc(response->buf, capacity);
        if (new_buf == NULL)
            return 0;

        response->buf = new_buf;
        response->capacity = capacity;
    }

    memcpy(response->buf + response->size, ptr, bytes);
    response->size += bytes;
    response->buf[response->size] = '\0';
    return bytes;
}

static void free_response_headers(http_response * response)
{
    free(response->content_type);
    free(response->content_disposition);
    response->content_type = NULL;
    response->content_disposition = NULL;
    response->content_length = 0;
}

static size_t header_callback(char * buffer, size_t size, size_t nitems, void * userdata)
{
    http_response * response = (http_response *)userdata;
    const size_t length = size * nitems;

    // a new status line starts the headers of another response (e.g. after a 100 Continue)
    if (length >= 5 && !strncmp(buffer, "HTTP/", 5))
    {
        free_response_headers(response);
        return length;
    }

    const char * colon = memchr(buffer, ':', length);
    if (colon == NULL)
        return length;

    const size_t name_length = colon - buffer;
    const char * value = colon + 1;
    size_t value_length = length - name_length - 1;

    while (value_length && (*value == ' ' || *value == '\t'))
    {
        ++value;
        --value_length;
    }
    while (value_length && (value[value_length - 1] == '\r' || value[value_length - 1] == '\n' || value[value_length - 1] == ' '))
        --value_length;

    char ** field = NULL;
    if (name_length == strlen("Content-Type") && !strncasecmp(buffer, "Content-Type", name_length))
        field = &response->content_type;
    else if (name_length == strlen("Content-Disposition") && !strncasecmp(buffer, "Content-Disposition", name_length))
        field = &response->content_disposition;

    if (field != NULL)
    {
        free(*field);
        *field = strndup(value, value_length);
    }
    else if (name_length == strlen("Content-Length") && !strncasecmp(buffer, "Content-Length", name_length))
        response->content_length = strtoul(value, NULL, 10);

    return length;
}

static int progress_callback(void * clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    (void)ultotal;
    (void)ulnow;

    http_progress * progress = (http_progress *)clientp;
    if (progress->install_type == INSTALL_NONE || dltotal <= 0)
        return 0;

    const u64 now = svcGetSystemTick() / CPU_TICKS_PER_MSEC;
    if (now - progress->last_redraw >= PROGRESS_REDRAW_MS || dlnow == dltotal)
    {
        progress->last_redraw = now;
        draw_loading_bar((u32)dlnow, (u32)dltotal, progress->install_type);
    }

    return 0;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Content-Disposition: attachment; filename="name.zip"; filename*=UTF-8''name.zip
static char * parse_disposition_filename(const char * disposition)
{
    if (disposition == NULL)
        return NULL;

    char * filename = NULL;
    const char * value = strcasestr(disposition, "filename*=");
    if (value != NULL)
    {
        value += strlen("filename*=");
        const char * encoded = strstr(value, "''");
        if (encoded != NULL)
            value = encoded + 2;

        const size_t length = strcspn(value, ";");
        filename = malloc(length + 1);
        if (filename == NULL)
            return NULL;

        size_t out = 0;
        for (size_t i = 0; i < length; ++i)
        {
            if (value[i] == '%' && i + 2 < length && hex_value(value[i + 1]) >= 0 && hex_value(value[i + 2]) >= 0)
            {
                filename[out++] = (char)(hex_value(value[i + 1]) * 16 + hex_value(value[i + 2]));
                i += 2;
            }
            else
                filename[out++] = value[i];
        }
        filename[out] = '\0';
    }
    else if ((value = strcasestr(disposition, "filename=")) != NULL)
    {
        value += strlen("filename=");
        size_t length;
        if (*value == '"')
        {
            ++value;
            length = strcspn(value, "\"");
        }
        else
            length = strcspn(value, ";");

        filename = strndup(value, length);
    }

    if (filename == NULL)
        return NULL;

    // only keep the name itself, never a path
    char * name = filename;
    for (char * c = filename; *c; ++c)
    {
        if (*c == '/' || *c == '\\')
            name = c + 1;
    }
    while (*name == ' ')
        ++name;

    char * result = *name ? strdup(name) : NULL;
    free(filename);
    return result;
}

static bool is_themeplaza_url(const char * url)
{
    return strstr(url, THEMEPLAZA_BASE_URL) != NULL;
}

static void show_curl_error(CURLcode code)
{
    char err_buf[ERROR_BUFFER_SIZE];
    switch (code)
    {
    case CURLE_OPERATION_TIMEDOUT:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http_timeout);
        break;
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http_no_network);
        break;
    case CURLE_SSL_CONNECT_ERROR:
    case CURLE_PEER_FAILED_VERIFICATION:
    case CURLE_SSL_CERTPROBLEM:
    case CURLE_SSL_CIPHER:
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_ssl_error, (u32)code);
        break;
    default:
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.network_error, curl_easy_strerror(code));
        break;
    }
    throw_error(err_buf, ERROR_LEVEL_WARNING);
}

// returns false and shows an error for anything that isn't a usable response
static bool check_status(long status, const char * url, bool not_found_is_error)
{
    char err_buf[ERROR_BUFFER_SIZE];
    switch (status)
    {
    case 200:
    case 203: // a successful request with a transformation applied by a proxy
        return true;
    case 303: // Theme Plaza returns these
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", is_themeplaza_url(url) ? language.remote.http303_tp : language.remote.http303);
        break;
    case 401:
    case 403:
    case 407:
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_errcode_generic, status == 401
            ? language.remote.http401
            : status == 403
            ? language.remote.http403
            : language.remote.http407);
        break;
    case 404:
        if (!not_found_is_error)
            return false;
        if (is_themeplaza_url(url))
            snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http404);
        else
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_err_url, "404 Not Found");
        break;
    case 406:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.zip_not_found);
        break;
    case 410:
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_err_url, "410 Gone");
        break;
    case 414:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http414);
        break;
    case 418:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http418);
        break;
    case 426:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http426);
        break;
    case 451:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http451);
        break;
    case 500:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http500);
        break;
    case 502:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http502);
        break;
    case 503:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http503);
        break;
    case 504:
        snprintf(err_buf, ERROR_BUFFER_SIZE, "%s", language.remote.http504);
        break;
    default:
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_unexpected, (unsigned int)status);
        break;
    }

    DEBUG("HTTP %ld; URL: %s\n", status, url);
    throw_error(err_buf, ERROR_LEVEL_WARNING);
    return false;
}

static Result http_request(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types, bool not_found_is_error)
{
    const Result shown_error = MAKERESULT(RL_TEMPORARY, RS_CANCELED, RM_APPLICATION, RD_NO_DATA);

    *buf = NULL;
    *size = 0;
    if (filename != NULL)
        *filename = NULL;

    u32 wifi_status = 0;
    if (R_SUCCEEDED(ACU_GetWifiStatus(&wifi_status)) && wifi_status == 0)
    {
        throw_error(language.remote.http_no_network, ERROR_LEVEL_WARNING);
        return shown_error;
    }

    if (!http_init())
    {
        show_curl_error(CURLE_FAILED_INIT);
        return shown_error;
    }

    char accept_header[0x100] = {0};
    struct curl_slist * headers = NULL;
    if (acceptable_mime_types != NULL)
    {
        snprintf(accept_header, sizeof(accept_header), "Accept: %s", acceptable_mime_types);
        headers = curl_slist_append(headers, accept_header);
    }

    http_response response = {0};
    http_progress progress = { .install_type = install_type };
    char * current_url = strdup(url);
    long status = 0;
    CURLcode code = CURLE_OK;

    DEBUG("HTTP GET %s\n", url);

    // redirects are followed here rather than by curl, so a 303 can be reported instead of followed
    for (int redirects = 0; current_url != NULL; ++redirects)
    {
        curl_easy_reset(curl);
        curl_easy_setopt(curl, CURLOPT_URL, current_url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        // no CA bundle is shipped (bundled certificates expire); httpc never verified either
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);

        response.size = 0;
        free_response_headers(&response);

        code = curl_easy_perform(curl);
        if (code != CURLE_OK)
            break;

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        if (status != 301 && status != 302 && status != 307 && status != 308)
            break;

        char * location = NULL;
        curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);
        free(current_url);
        current_url = NULL;
        if (location == NULL || redirects >= MAX_REDIRECTS)
        {
            code = CURLE_TOO_MANY_REDIRECTS;
            break;
        }

        DEBUG("HTTP redirect to %s\n", location);
        current_url = strdup(location);
    }

    free(current_url);
    curl_slist_free_all(headers);

    Result ret = shown_error;
    if (code != CURLE_OK)
    {
        DEBUG("curl error %d: %s\n", code, curl_easy_strerror(code));
        show_curl_error(code);
    }
    else if (!check_status(status, url, not_found_is_error))
    {
        if (status == 404 && !not_found_is_error)
            ret = MAKERESULT(RL_SUCCESS, RS_NOTFOUND, RM_FILE_SERVER, RD_NO_DATA);
    }
    else if (!mime_type_is_acceptable(acceptable_mime_types, response.content_type))
    {
        DEBUG("Server sent %s, expected %s\n", response.content_type, acceptable_mime_types);
        throw_error(language.remote.zip_not_found, ERROR_LEVEL_WARNING);
    }
    else
    {
        if (filename != NULL)
            *filename = parse_disposition_filename(response.content_disposition);

        if (response.buf == NULL)
            response.buf = calloc(1, 1);

        *buf = response.buf;
        *size = response.size;
        response.buf = NULL;
        ret = MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
        DEBUG("size: %lu\n", *size);
    }

    free(response.buf);
    free_response_headers(&response);
    return ret;
}

Result http_get(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types)
{
    return http_request(url, filename, buf, size, install_type, acceptable_mime_types, true);
}

Result http_get_optional(const char * url, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types)
{
    return http_request(url, NULL, buf, size, install_type, acceptable_mime_types, false);
}
