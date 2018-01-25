/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2017 Alex Taber ("astronautlevel"), Dawid Eckert ("daedreth")
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

#ifndef BADGES_H
#define BADGES_H

#include "common.h"
#include "loading.h"

#define BADGE_SET_LIMIT 100
#define BADGE_LIMIT 1000
#define BADGE_LAYOUT_SLOT_LIMIT 360

#define ICON_SIZE_64 (64*64)
#define ICON_SIZE_32 (32*32)

typedef struct {
    u16 icon_data[ICON_SIZE_64];
    u8 icon_alpha[ICON_SIZE_64/2];
} Badge_Icon_64_s;

typedef struct {
    u16 icon_data[ICON_SIZE_32];
    u8 icon_alpha[ICON_SIZE_32/2];
} Badge_Icon_32_s;

typedef struct {
    u16 badge_set_titles[BADGE_SET_LIMIT][16][0x45];
    u16 badge_titles[BADGE_LIMIT][16][0x45];
    u16 badge_set_icons_565_64[BADGE_SET_LIMIT][ICON_SIZE_64];
    Badge_Icon_64_s badge_icons_64[BADGE_LIMIT];
    Badge_Icon_32_s badge_icons_32[BADGE_LIMIT];
} Badge_Data_dat_s;

typedef struct {
    u32 unk;
    u32 id;
    u32 set_id;
    u16 index;
    u16 sub_id;
} Badge_Identifier_s;

typedef struct {
    Badge_Identifier_s identifier;
    u16 number_placed;
    u16 quantity;
    u32 unk;
    u64 shortcut_tid[2];
} Badge_Info_s;

typedef struct {
    u64 unk1;
    u32 unk2;
    u32 unk3;
    u32 set_id;
    u32 set_index;
} Badge_Set_Identifier_s;

typedef struct {
    Badge_Set_Identifier_s set_identifier;
    u32 unk1;
    u32 unique_badges_amount;
    u32 total_badges_amount;
    u32 start_badge_index;
    u32 unk2;
    u32 unk3;
} Badge_Set_Info_s;

typedef struct {
    Badge_Identifier_s identifier;
    u32 position;
    u32 folder_icon;
} Badge_Layout_Slot_s;

typedef struct {
    u32 zero;
    u32 badge_sets_amount;
    u32 unique_badges_amount;
    u32 placed_badges_amount;
    u32 selected_set_in_HM;
    u32 selected_badge_column_in_all_badges;
    u32 total_badges_amount;
    u32 nnid_number;
    u8 unk[0x338];
    u8 used_badge_slot[0x80];
    u8 used_badge_set_slot[0x10];
    Badge_Info_s badge_info_entries[BADGE_LIMIT];
    Badge_Set_Info_s badge_set_info_entries[BADGE_SET_LIMIT];
    Badge_Layout_Slot_s badge_layout_slot_entries[BADGE_LAYOUT_SLOT_LIMIT];
} Badge_Mng_File_dat_s;

enum BadgeInstall {
    BADGE_INSTALL_SINGLE = BIT(0),
};

Result badge_install(Entry_s badges);
Result badge_install_all(Entry_List_s list);

#endif