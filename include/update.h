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

#ifndef UPDATE_H
#define UPDATE_H

#include "common.h"

// Looks for a newer stable release of Anemone3DS on GitHub in the background
// (the result is cached for a day, so most launches don't need the network)
void update_check_start(void);
void update_check_stop(void);

// true once the check finished (or was skipped); tag of the newer stable release, or NULL
bool update_check_done(void);
const char * update_check_newer_tag(void);

// app_path is the path of the running 3dsx (argv[0]), unused for the CIA build
bool update_can_install(const char * app_path);
// downloads the update for the running build (CIA or 3dsx), checks it and installs it;
// shows its own errors. The app has to be restarted afterwards.
bool update_install(const char * app_path);

#endif
