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

#include "network.h"
#include "draw.h"

std::string get_download_url(const std::string& base, int entry_id)
{
    char* url = nullptr;
    asprintf(&url, base.c_str(), entry_id);
    std::unique_ptr<char, decltype(free)*> formatted_url(url, free);
    return std::string(formatted_url.get());
}

static std::pair<std::unique_ptr<u8[]>, u32> download_data_internal(const char* url, InstallType install_type, char** filename, void* buf = nullptr)
{
    Result ret;
    httpcContext context;
    char* new_url = nullptr;
    u32 status_code;
    u32 content_size = 0;
    u32 read_size;
    u32 size = 0;

    // Follow redirections
    do {
        ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
        if(R_FAILED(ret))
        {
            free(new_url);
            DEBUG("httpcOpenContext %.8lx\n", ret);
            return std::make_pair(nullptr, 0);
        }
        ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
        ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
        ret = httpcAddRequestHeaderField(&context, "User-Agent", USER_AGENT);
        ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

        ret = httpcBeginRequest(&context);
        if(R_FAILED(ret))
        {
            httpcCloseContext(&context);
            free(new_url);
            DEBUG("httpcBeginRequest %.8lx\n", ret);
            return std::make_pair(nullptr, 0);
        }

        ret = httpcGetResponseStatusCode(&context, &status_code);
        if(R_FAILED(ret))
        {
            httpcCloseContext(&context);
            free(new_url);
            DEBUG("httpcGetResponseStatusCode\n");
            return std::make_pair(nullptr, 0);
        }

        if((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308))
        {
            if(!new_url)
                new_url = (char*)malloc(0x1000);
            memset(new_url, 0, 0x1000);
            ret = httpcGetResponseHeader(&context, "Location", new_url, 0x1000);
            httpcCloseContext(&context);
            url = new_url;
        }
    } while((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308));

    if(status_code != 200)
    {
        httpcCloseContext(&context);
        free(new_url);
        DEBUG("status_code, %lu\n", status_code);
        return std::make_pair(nullptr, 0);
    }

    ret = httpcGetDownloadSizeState(&context, NULL, &content_size);
    if(R_FAILED(ret))
    {
        httpcCloseContext(&context);
        free(new_url);
        DEBUG("httpcGetDownloadSizeState\n");
        return std::make_pair(nullptr, 0);
    }

    while(filename)
    {
        char* content_disposition = new char[1024];
        memset(content_disposition, 0, 1024);
        ret = httpcGetResponseHeader(&context, "Content-Disposition", content_disposition, 1024);
        if(R_FAILED(ret))
        {
            delete[] content_disposition;
            free(new_url);
            DEBUG("httpcGetResponseHeader, aborting getting filename\n");
            break;
        }
        DEBUG("content disposition:\n'%s'\n", content_disposition);

        // char * tok = strtok(content_disposition, "\"");
        // tok = strtok(NULL, "\"");

        // if(!(tok))
        // {
            // free(content_disposition);
            // free(new_url);
            // free(*buf);
            // throw_error("Target is not valid!", ERROR_LEVEL_WARNING);
            // DEBUG("filename\n");
            // return 0;
        // }

        // char *illegal_characters = "\"?;:/\\+";
        // for (size_t i = 0; i < strlen(tok); i++)
        // {
            // for (size_t n = 0; n < strlen(illegal_characters); n++)
            // {
                // if ((tok)[i] == illegal_characters[n])
                // {
                    // (tok)[i] = '-';
                // }
            // }
        // }

        // *filename = new char[1024];
        // strcpy(*filename, tok);
        delete[] content_disposition;
        
        break;
    }

    u8* out = buf ? static_cast<u8*>(buf) : new(std::nothrow) u8[content_size ? content_size : 0x1000];
    if(out == nullptr)
    {
        DEBUG("Error allocating buf for first chunk, aborting...\n");
        httpcCloseContext(&context);
        free(new_url);
        return std::make_pair(nullptr, 0);
    }

    do {
        ret = httpcDownloadData(&context, out + size, 0x1000, &read_size);
        size += read_size;

        if(install_type != INSTALLS_AMOUNT && content_size)
            draw_loading_bar(size, content_size, install_type);

        if(ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING && !buf && !content_size)
        {
            u8* new_buf = new(std::nothrow) u8[size + 0x1000];
            if(new_buf == nullptr)
            {
                DEBUG("Error reallocating out for next chunk, aborting...\n");
                httpcCloseContext(&context);
                free(new_url);
                delete[] out;
                return std::make_pair(nullptr, 0);
            }
            out = new_buf;
        }
    } while(ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

    httpcCloseContext(&context);
    free(new_url);

    DEBUG("downloaded size: %lu\n", size);
    return std::make_pair(std::unique_ptr<u8[]>(buf ? nullptr : out), size);
}

std::pair<std::unique_ptr<u8[]>, u32> download_data(const char* url, InstallType install_type, char** filename)
{
    return download_data_internal(url, install_type, filename);
}

bool download_data(const char* url, InstallType install_type, void* data, u32 wanted_size, char** filename)
{
    const auto& [buf, size] = download_data_internal(url, install_type, filename, data);
    return size == wanted_size;
}