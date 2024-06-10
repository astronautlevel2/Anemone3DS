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

#ifndef FS_H
#define FS_H

#include "common.h"
#include "badges.h"

#define ILLEGAL_CHARS "><\"?;:/\\+,.|[=]*"

extern FS_Archive ArchiveSD;
extern FS_Archive ArchiveHomeExt;
extern FS_Archive ArchiveThemeExt;
extern FS_Archive ArchiveBadgeExt;

typedef struct {
    u32 enable : 1;
    u32 browser: 1;
    u32 stereoscopic : 1;
    u32 media_share : 1;
    u32 online : 1;
    u32 streetpass : 1;
    u32 friends : 1;
    u32 dsdownload : 1;
    u32 shopping : 1;
    u32 videos : 1;
    u32 miiverse : 1;
    u32 post : 1;
    u32 null : 19;
    u32 coppa : 1;
} Parental_Restrictions_s;

Result open_archives(void);
Result open_badge_extdata(void);
Result close_archives(void);
Result load_parental_controls(Parental_Restrictions_s *restrictions);

u32 file_to_buf(FS_Path path, FS_Archive archive, char ** buf);
u32 zip_memory_to_buf(const char * file_name, void * zip_memory, size_t zip_size, char ** buf);
u32 zip_file_to_buf(const char * file_name, const u16 * zip_path, char ** buf);
u32 decompress_lz_file(FS_Path file_name, FS_Archive archive, char ** buf);
u32 compress_lz_file_fast(FS_Path path, FS_Archive archive, char * in_buf, u32 size);

Result buf_to_file(u32 size, FS_Path path, FS_Archive archive, char * buf);
Result zero_handle_memeasy(Handle handle);
void remake_file(FS_Path path, FS_Archive archive, u32 size);
void save_zip_to_sd(char * filename, u32 size, char * buf, RemoteMode mode);
s16 for_each_file_zip(u16 *zip_path, u32 (*zip_iter_callback)(char *filebuf, u64 file_size, const char *name, void *userdata), void *userdata);

#endif
