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

#ifndef FILE_H
#define FILE_H

#include "common.h"

enum Archive {
    SD_CARD = 0,
    CTRNAND,
    HOME_EXTDATA,
    THEME_EXTDATA,
    BADGE_EXTDATA,

    ACHIVE_AMOUNT
};

extern Result theme_result, badge_result;
extern bool have_luma_folder;
Result open_archives();
Result close_archives();

Result file_open(FS_Path path, Archive archive, Handle* handle, int mode);
// For both of these functions, if allocation fails, buf is unmodified
// and if buf points to non-nullptr, it will try to read into there directly, no matter the size
// If file couldn't be opened, buf is unmodified
std::pair<std::unique_ptr<char[]>, u64> file_to_buf(FS_Path path, Archive archive, u32 wanted_size = 0);
bool file_to_buf(FS_Path path, Archive archive, void* buf, u32 wanted_size = 0);
// If zip couldn't be opened, buf is unmodified
std::pair<std::unique_ptr<char[]>, u64> zip_file_to_buf(const char* filename, const std::string& zip_path, u32 wanted_size = 0);
bool zip_file_to_buf(const char* filename, const std::string& zip_path, void* buf, u32 wanted_size = 0);

bool check_file_is_zip(void* zip_buf, size_t zip_size);
bool check_file_in_zip(void* zip_buf, size_t zip_size, const char* filename);

Result buf_to_file(FS_Path path, Archive archive, u32 size, const void* buf);
// If you pass a buffer, it will write it directly instead of wasting time clearing the file first and you having to call buf_to_file after
void remake_file(FS_Path path, Archive archive, u32 size, const void* buf = nullptr);
void delete_file(FS_Path path, Archive archive);
void delete_folder(FS_Path path, Archive archive);

#endif
