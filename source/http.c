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

static bool mime_type_is_acceptable(const char *acceptable_mime_types, const char *mime_type);

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

        if (!mime_type_is_acceptable(mime, content_buf))
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


static int url_hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// The last segment of a URL's path, percent-decoded: the name to save a download under when the server names none (no
// Content-Disposition), e.g. "MyTheme.zip" for a QR code's https://example.com/themes/MyTheme.zip?raw=1. NULL when the
// path has no last segment.
static char * url_filename(const char * url)
{
    const char * end = url + strcspn(url, "?#");
    const char * host = strstr(url, "://");
    const char * path = strchr(host != NULL ? host + 3 : url, '/');
    if (path == NULL || path >= end)
        return NULL;

    const char * start = end;
    while (start > path && start[-1] != '/')
        --start;

    char * name = malloc(end - start + 1);
    if (name == NULL)
        return NULL;

    size_t length = 0;
    for (const char * c = start; c < end; ++c)
    {
        if (*c == '%' && end - c > 2 && url_hex_digit(c[1]) >= 0 && url_hex_digit(c[2]) >= 0)
        {
            name[length++] = (char)(url_hex_digit(c[1]) * 16 + url_hex_digit(c[2]));
            c += 2;
        }
        else
            name[length++] = *c;
    }
    name[length] = '\0';

    // only keep the name itself, never a path
    const char * base = name;
    for (const char * c = name; *c; ++c)
    {
        if (*c == '/' || *c == '\\')
            base = c + 1;
    }

    char * result = *base && strcmp(base, ".") && strcmp(base, "..") ? strdup(base) : NULL;
    free(name);
    return result;
}

/*
 * call example: written = http_get("url", &filename, &buffer_to_download_to, &filesize, INSTALL_DOWNLOAD, "application/json");
 */
Result http_get(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types)
{
    Result res = http_get_with_not_found_flag(url, filename, buf, size, install_type, acceptable_mime_types, true);
    // the server named no file: use the URL's name (save_zip_to_sd falls back to "download.zip" when that's
    // missing too)
    if (R_SUCCEEDED(res) && filename != NULL && *filename == NULL)
        *filename = url_filename(url);
    return res;
}

/* 
 * curl functions modified from Universal-Updater download.cpp
 */
static size_t handle_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    curl_data *data = (curl_data *) userdata;
    const size_t bsz = size * nmemb;

    if (data->result_sz == 0 || !(data->result_buf))
    {
        data->result_sz = 0x1000;
        data->result_buf = (char *) malloc(data->result_sz);
    }

    bool need_realloc = false;
    while (data->result_written + bsz > data->result_sz)
    {
        data->result_sz <<= 1;
        need_realloc = true;
    }

    if (need_realloc)
    {
        char *new_buf = (char *)realloc(data->result_buf, data->result_sz);
        if (!new_buf) return 0;

        data->result_buf = new_buf;
    }

    memcpy(data->result_buf + data->result_written, ptr, bsz);
    data->result_written += bsz;
    return bsz;
}

static size_t curl_parse_header(char *buffer, size_t size, size_t nitems, void *userdata)
{
    curl_header *header = (curl_header *) userdata;
    for (size_t i = 0; i < size * nitems; ++i)
    {
        if (buffer[i] == '\n' || buffer[i] == '\r')
        {
            buffer[i] = '\0';
            break;
        }
    }

    if (!strncasecmp(buffer, "Content-Type: ", 14))
    {
        header->mime_type = malloc(strlen(buffer) - 13);
        strncpy(header->mime_type, buffer + 14, strlen(buffer) - 14);
        header->mime_type[strlen(buffer) - 14] = '\0';
    } else if (!strncasecmp(buffer, "Content-Disposition: ", 21))
    {
        header->filename = malloc(strlen(buffer) - 20);
        memcpy(header->filename, buffer + 21, strlen(buffer) - 21);
        header->filename[strlen(buffer) - 21] = '\0';
    } 

    return nitems * size;
}

static bool mime_type_is_acceptable(const char *acceptable_mime_types, const char *mime_type)
{
    if (!acceptable_mime_types || !mime_type)
        return true;

    while (*mime_type == ' ' || *mime_type == '\t')
        ++mime_type;

    char mime_base[128] = {0};
    size_t mime_len = strcspn(mime_type, "; \t\r\n");
    if (mime_len >= sizeof(mime_base))
        mime_len = sizeof(mime_base) - 1;
    memcpy(mime_base, mime_type, mime_len);
    mime_base[mime_len] = '\0';

    const char *cursor = acceptable_mime_types;
    while (*cursor)
    {
        while (*cursor == ' ' || *cursor == ',' || *cursor == ';')
            ++cursor;

        const char *token_start = cursor;
        while (*cursor && *cursor != ' ' && *cursor != ',' && *cursor != ';')
            ++cursor;

        size_t token_len = cursor - token_start;
        if (token_len == 0)
            continue;

        if (token_len == mime_len)
        {
            bool match = true;
            for (size_t i = 0; i < mime_len; ++i)
            {
                if (tolower((unsigned char)token_start[i]) != tolower((unsigned char)mime_base[i]))
                {
                    match = false;
                    break;
                }
            }

            if (match)
                return true;
        }
    }

    return false;
}

static int64_t curl_http_get(const char * url, char ** out_filename, char ** buf, u32 * size, const char * acceptable_mime_types)
{
    DEBUG("attempting curl_http_get\n");
    curl_data data = {0};
    curl_header header = {0};
    void *socubuf = memalign(0x1000, 0x100000);
    if (!socubuf)
    {
        return -1;
    }

    Result ret = socInit((u32 *) socubuf, 0x100000);
    if (R_FAILED(ret))
    {
        free(socubuf);
        return ret;
    }

    CURL *handle;
    handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, 102400L);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, handle_data);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(handle, CURLOPT_STDERR, stderr);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, curl_parse_header);

    struct curl_slist *list = NULL;
    char mime_string[512] = {0};
    sprintf(mime_string, "Accept:%s", acceptable_mime_types);
    list = curl_slist_append(list, mime_string);

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);

    CURLcode cres = curl_easy_perform(handle);
    curl_easy_cleanup(handle);
    char *newbuf = (char *) realloc(data.result_buf, data.result_written + 1);
    data.result_buf = newbuf;
    data.result_buf[data.result_written] = 0;
    if (cres != CURLE_OK)
    {
        socExit();
        free(data.result_buf);
        free(socubuf);
        if (header.mime_type) free(header.mime_type);
        if (header.filename) free(header.filename);
        return -1;
    }
    
    DEBUG("Mime Type: %s\n", header.mime_type);
    DEBUG("Acceptable Mime Types: %s\n", acceptable_mime_types);
    if (header.mime_type)
    {
        if (!mime_type_is_acceptable(acceptable_mime_types, header.mime_type))
        {
            socExit();
            free(data.result_buf);
            free(socubuf);
            if (header.mime_type) free(header.mime_type);
            if (header.filename) free(header.filename);
            return -2;
        }
    } 

    DEBUG("Content-Disposition: %s\n", header.filename);
    if (out_filename)
    {
        if (header.filename)
        {
            char *filename = strstr(header.filename, "filename=");
            if (filename)
            {
                filename = strpbrk(filename, "=") + 1;
                char *end = strpbrk(filename, ";");
                if (end)
                    *end = '\0';

                if (filename[0] == '"')
                {
                    filename[strlen(filename) - 1] = '\0';
                    filename++;
                }

                *out_filename = strdup(filename);
            } else {
                *out_filename = NULL;
            }
        } else {
            *out_filename = NULL;
        }
    }

    *buf = data.result_buf;
    *size = data.result_written;

    socExit();
    if (header.mime_type) free(header.mime_type);
    if (header.filename) free(header.filename);
    free(socubuf);

    return 0;
}

Result http_get_with_not_found_flag(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types, bool not_found_is_error)
{
    const char *zip_not_available = language.remote.zip_not_found;
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
    Result res;
    ParseResult parse = parse_header(&_header, &context, acceptable_mime_types);
    switch (parse)
    {
    case SUCCESS:
        break;
    case HTTPC_ERROR:
        DEBUG("httpc error %lx\n", _header.result_code);
        switch (_header.result_code)
        {
        case 0xd8a0a028: // bad zip file
        case 0xd8a0a03c: // SSL failure
            // try curl?
            res = curl_http_get(url, filename, buf, size, acceptable_mime_types);
            if (R_SUCCEEDED(res))
            {
                return res;
            }

            if(_header.result_code == 0xd8a0a028)
            {
                snprintf(err_buf, ERROR_BUFFER_SIZE, zip_not_available);
            }
            else
            {
                snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_ssl_error, _header.result_code);
            }
            quit = false;
            break;
        case 0xd8a0a049:
            // Timeout (bad wifi/proxy)
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_timeout);
            quit = false;
            break;
        case 0xd8a0a046:
            // poor reception/no wifi/custom dns set
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_no_network);
            quit = false;
            break;
        default:
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.generic_httpc_error, _header.result_code);
            quit = true;
            break;
        }
        throw_error(err_buf, ERROR_LEVEL_ERROR);
        httpcCloseContext(&context);
        return _header.result_code;
    case SEE_OTHER:
        if (strstr(url, THEMEPLAZA_BASE_URL))
        {
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http303_tp);
            goto error;
        }
        else
        {
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http303);
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
        snprintf(err_buf, ERROR_BUFFER_SIZE, zip_not_available);
        goto error;
    case HTTP_NOT_FOUND:
        if (!not_found_is_error)
            goto not_found_non_error;
        [[fallthrough]];
    case HTTP_GONE:
        const char * http_error = parse == HTTP_NOT_FOUND ? "404 Not Found" : "410 Gone";
        DEBUG("HTTP %s; URL: %s\n", http_error, url);
        if (strstr(url, THEMEPLAZA_BASE_URL) && parse == HTTP_NOT_FOUND)
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http404);
        else
            snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_err_url, http_error);
        goto error;
    case HTTP_UNACCEPTABLE:
        DEBUG("HTTP 406 Unacceptable; Accept: %s\n", acceptable_mime_types);
        snprintf(err_buf, ERROR_BUFFER_SIZE, zip_not_available);
        goto error;
    case HTTP_UNAUTHORIZED:
    case HTTP_FORBIDDEN:
    case HTTP_PROXY_UNAUTHORIZED:
        DEBUG("HTTP %u: device not authenticated\n", parse);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_errcode_generic, parse == HTTP_UNAUTHORIZED
            ? language.remote.http401
            : parse == HTTP_FORBIDDEN
            ? language.remote.http403
            : language.remote.http407);
        goto error;
    case HTTP_URI_TOO_LONG:
        DEBUG("HTTP 414; URL is too long, maybe too many redirects?\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http414);
        goto error;
    case HTTP_IM_A_TEAPOT:
        DEBUG("HTTP 418 I'm a teapot\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http418);
        goto error;
    case HTTP_UPGRADE_REQUIRED:
        DEBUG("HTTP 426; HTTP/2 required\n");
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http426);
        goto error;
    case HTTP_LEGAL_REASONS:
        DEBUG("HTTP 451; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http451);
        goto error;
    case HTTP_INTERNAL_SERVER_ERROR:
        DEBUG("HTTP 500; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http500);
        goto error;
    case HTTP_BAD_GATEWAY:
        DEBUG("HTTP 502; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http502);
        goto error;
    case HTTP_SERVICE_UNAVAILABLE:
        DEBUG("HTTP 503; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http503);
        goto error;
    case HTTP_GATEWAY_TIMEOUT:
        DEBUG("HTTP 504; URL: %s\n", url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http504);
        goto error;
    default:
        DEBUG("HTTP %u; URL: %s\n", parse, url);
        snprintf(err_buf, ERROR_BUFFER_SIZE, language.remote.http_unexpected, parse);
        goto error;
    }

    goto no_error;
error:
    throw_error(err_buf, ERROR_LEVEL_WARNING);
    res = httpcCloseContext(&context);
    if (R_FAILED(res)) return res;
    return MAKERESULT(RL_TEMPORARY, RS_CANCELED, RM_APPLICATION, RD_NO_DATA);
not_found_non_error:
    res = httpcCloseContext(&context);
    if (R_FAILED(res)) return res;
    return MAKERESULT(RL_SUCCESS, RS_NOTFOUND, RM_FILE_SERVER, RD_NO_DATA);
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
        ret = httpcDownloadData(&context, (u8 *)(*buf) + *size, chunk_size, &read_size);
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
    if (filename) { DEBUG("filename: %s\n", *filename); }
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
 }
