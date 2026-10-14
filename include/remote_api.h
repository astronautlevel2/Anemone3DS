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

#ifndef REMOTE_API_H
#define REMOTE_API_H

#include "common.h"
#include "entries_list.h"
#include "draw.h"

// The remote API (v2) every provider implements: <base URL>/themes|splashes|badges?page=<page>&q=<search>
char * remote_api_page_url(RemoteProvider provider, RemoteMode mode, json_int_t page, const char * escaped_search);

// Replaces the list's entries with the page in root. Returns false and leaves the list untouched if the page has no entries.
bool remote_api_load_page(Entry_List_s * list, json_t * root);

// Icons are PNGs downloaded in the background while the list is shown
Result remote_icons_init(void);
void remote_icons_exit(void);
void remote_icons_start(Entry_List_s * list);
void remote_icons_stop(void);
// hold while drawing the list, the icon thread writes into its texture
void remote_icons_lock(void);
void remote_icons_unlock(void);

#endif
