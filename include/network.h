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

#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"
#include "draw.h"

#define THEMEPLAZA_BASE_URL "https://themeplaza.eu"
#define THEMEPLAZA_API_URL "/api/anemone/v1"
#define THEMEPLAZA_BASE_API_URL THEMEPLAZA_BASE_URL  THEMEPLAZA_API_URL

#define THEMEPLAZA_PAGE_FORMAT THEMEPLAZA_BASE_API_URL  "/list?page=%zd&category=%d&query=%s"
#define THEMEPLAZA_JSON_PAGE_COUNT   "pages"
#define THEMEPLAZA_JSON_PAGE_IDS     "items"

#define THEMEPLAZA_JSON_ERROR_MESSAGE             "message"
#define THEMEPLAZA_JSON_ERROR_MESSAGE_NOT_FOUND   "No items found"


#define THEMEPLAZA_DOWNLOAD_FORMAT   THEMEPLAZA_BASE_URL  "/download/%i"
#define THEMEPLAZA_PREVIEW_FORMAT    THEMEPLAZA_DOWNLOAD_FORMAT  "/preview"
#define THEMEPLAZA_BGM_FORMAT        THEMEPLAZA_DOWNLOAD_FORMAT  "/bgm"
#define THEMEPLAZA_ICON_FORMAT       THEMEPLAZA_DOWNLOAD_FORMAT  "/preview/icon"
#define THEMEPLAZA_SMDH_FORMAT       THEMEPLAZA_DOWNLOAD_FORMAT  "/smdh"

std::string get_page_url(size_t page, int sort, const std::string& search);
std::string get_download_url(const std::string& base, int entry_id);

std::pair<std::unique_ptr<u8[]>, u32> download_data(const char* url, InstallType install_type, char** filename = nullptr);
inline std::pair<std::unique_ptr<u8[]>, u32> download_data(const std::string& url, InstallType install_type, char** filename = nullptr)
{
    return download_data(url.c_str(), install_type, filename);
}

bool download_data(const char* url, InstallType install_type, void* data, u32 wanted_size, char** filename = nullptr);
inline bool download_data(const std::string& url, InstallType install_type, void* data, u32 wanted_size, char** filename = nullptr)
{
    return download_data(url.c_str(), install_type, data, wanted_size, filename);
}

#endif
