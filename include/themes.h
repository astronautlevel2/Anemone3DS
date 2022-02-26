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

#ifndef THEMES_H
#define THEMES_H

#include "common.h"
#include "loading.h"

#define MAX_SHUFFLE_THEMES 10

enum ThemeInstall {
    THEME_INSTALL_SHUFFLE = BIT(0),
    THEME_INSTALL_BODY = BIT(1),
    THEME_INSTALL_BGM = BIT(2),
};

typedef struct {
    u32 index;
    u8 dlc_tid_low_bits;
    u8 type;
    u16 unk;
} ThemeEntry_s;

typedef struct {
    u8 _padding1[0x13b8];
    ThemeEntry_s theme_entry;
    ThemeEntry_s shuffle_themes[MAX_SHUFFLE_THEMES];
    u8 shuffle_seedA[0xb];
    u8 shuffle;
    u8 shuffle_seedB[0xa];
} SaveData_dat_s;

typedef struct {
    u32 unk1;
    u32 unk2;
    u32 body_size;
    u32 music_size;
    u32 unk3;
    u32 unk4;
    u32 dlc_theme_content_index;
    u32 use_theme_cache;

    u8 _padding1[0x338 - 8*sizeof(u32)];

    u32 shuffle_body_sizes[MAX_SHUFFLE_THEMES];
    u32 shuffle_music_sizes[MAX_SHUFFLE_THEMES];
} ThemeManage_bin_s;

Result theme_install(Entry_s theme);
Result no_bgm_install(Entry_s theme);
Result bgm_install(Entry_s theme);

Result shuffle_install(Entry_List_s themes);

Result dump_theme(void);

void themes_check_installed(void * void_arg);

#endif
