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

#ifndef REMOTE_H
#define REMOTE_H

#include "common.h"
#include "draw.h"
#include <ctype.h>

#define THEMEPLAZA_BASE_URL "https://themeplaza.eu"
#define THEMEPLAZA_API_URL "/api/anemone/v1"
#define THEMEPLAZA_BASE_API_URL THEMEPLAZA_BASE_URL  THEMEPLAZA_API_URL

#define THEMEPLAZA_PAGE_FORMAT THEMEPLAZA_BASE_API_URL  "/list?page=%"  JSON_INTEGER_FORMAT  "&category=%i&query=%s"
#define THEMEPLAZA_JSON_PAGE_COUNT   "pages"
#define THEMEPLAZA_JSON_PAGE_IDS     "items"

#define THEMEPLAZA_JSON_ERROR_MESSAGE             "message"
#define THEMEPLAZA_JSON_ERROR_MESSAGE_NOT_FOUND   "No items found"


#define THEMEPLAZA_DOWNLOAD_FORMAT   THEMEPLAZA_BASE_URL  "/download/%"  JSON_INTEGER_FORMAT
#define THEMEPLAZA_PREVIEW_FORMAT    THEMEPLAZA_DOWNLOAD_FORMAT  "/preview"
#define THEMEPLAZA_BGM_FORMAT        THEMEPLAZA_DOWNLOAD_FORMAT  "/bgm"
#define THEMEPLAZA_ICON_FORMAT       THEMEPLAZA_DOWNLOAD_FORMAT  "/preview/icon"
#define THEMEPLAZA_SMDH_FORMAT       THEMEPLAZA_DOWNLOAD_FORMAT  "/smdh"

#define CACHE_PATH_FORMAT            "/3ds/"  APP_TITLE  "/cache/%"  JSON_INTEGER_FORMAT

bool themeplaza_browser(EntryMode mode);
u32 http_get(const char *url, char ** filename, char ** buf, InstallType install_type);

#endif
