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

/*
 * Downloads url into a newly allocated *buf (NUL terminated, *size excludes the terminator).
 * filename, if not NULL, receives the Content-Disposition filename (or NULL).
 * acceptable_mime_types is sent as the Accept header and checked against the Content-Type.
 * Failures show an error to the user and return a failed Result. Holding B cancels the
 * download without an error message: the Result then has the RD_CANCEL_REQUESTED description.
 *
 * call example: http_get("url", &filename, &buffer_to_download_to, &filesize, INSTALL_DOWNLOAD, "application/json");
 */
Result http_get(const char * url, char ** filename, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types);

/*
 * Same as http_get, but a 404 is not an error: it returns
 * MAKERESULT(RL_SUCCESS, RS_NOTFOUND, RM_FILE_SERVER, RD_NO_DATA) with *size == 0.
 */
Result http_get_optional(const char * url, char ** buf, u32 * size, InstallType install_type, const char * acceptable_mime_types);

void http_exit(void);

#endif
