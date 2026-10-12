/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
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

#ifndef HTTP_H
#define HTTP_H

#include "common.h"
#include "draw.h"

typedef struct {
    char *result_buf;
    size_t result_written;
    size_t result_sz;
} curl_data;

typedef struct {
    char *filename;
    char *mime_type;
} curl_header;

Result http_get(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types);

// special case used only by load_remote_bgm
// TODO: replace this travesty with a proper handler
Result http_get_with_not_found_flag(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types, bool not_found_is_error);

#endif
