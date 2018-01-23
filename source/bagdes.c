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
#include "badges.h"
#include "unicode.h"
#include "fs.h"
#include "draw.h"

static Result badge_install_internal(Entry_List_s list, int install_mode)
{
    char* badgemanage_buf = NULL;
    file_to_buf(fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), ArchiveBadgeExt, &badgemanage_buf);
    Badge_Mng_File_dat_s * badge_manage = (Badge_Mng_File_dat_s *)badgemanage_buf;

    const u32 homebrew_set_id = 0x0000BEEF;
    const u16 badge_quantity = 0xFFFF;
    u32 start_index = badge_manage->unique_badges_amount;
    u32 total_installed_badges = 0;

    for(int i = (install_mode & BADGE_INSTALL_SINGLE ? list.selected_entry : 0); i < list.entries_count; i++)
    {
        Entry_s current_entry = list.entries[i];
        u32 shortcut_lowid = 0;

        Badge_Info_s * current_slot = &badge_manage->badge_info_entries[(start_index+total_installed_badges)];
        Badge_Identifier_s * current_identifier = &current_slot->identifier;

        current_identifier->id = (start_index+total_installed_badges)+1;
        current_identifier->set_id = homebrew_set_id;
        current_identifier->index = (start_index+total_installed_badges);

        current_slot->number_placed = 0;
        current_slot->quantity = badge_quantity;
        if(shortcut_lowid)
        {
            current_slot->shortcut_tid[0] = ((u64)0x00040010 << 32) | shortcut_lowid;
            current_slot->shortcut_tid[1] = ((u64)0x00040010 << 32) | shortcut_lowid;
        }

        /*
        TODO: title and actual image data
        */

        badge_manage->used_badge_slot[(start_index+total_installed_badges)/8] |= 1 << ((start_index+total_installed_badges) % 8);
        badge_manage->total_badges_amount += badge_quantity;
        total_installed_badges++;

        if(install_mode & BADGE_INSTALL_SINGLE)
            break;
    }

    u32 i = 0;
    for(; i < badge_manage->badge_sets_amount; i++)
    {
        Badge_Set_Info_s * current_set = &badge_manage->badge_set_info_entries[i];
        if(current_set->set_identifier.set_id == homebrew_set_id) //if there already is a homebrew set, add to it
        {
            current_set->unique_badges_amount += total_installed_badges;
            current_set->total_badges_amount += total_installed_badges*badge_quantity;
            break;
        }
        else
            continue;
    }

    if(i == badge_manage->badge_sets_amount) //if the homebrew set wasnt found, create it
    {
        Badge_Set_Info_s * current_set = &badge_manage->badge_set_info_entries[i];
        Badge_Set_Identifier_s * current_identifier = &current_set->set_identifier;

        current_identifier->unk3 = 0x2710;
        current_identifier->set_id = homebrew_set_id;
        current_identifier->set_index = badge_manage->badge_sets_amount;

        current_set->unique_badges_amount = total_installed_badges;
        current_set->total_badges_amount = total_installed_badges*badge_quantity;
        current_set->start_badge_index = start_index;

        /*
        TODO: set title and icon data (romfs:/badge_set_icon.png)
        */

        badge_manage->used_badge_set_slot[badge_manage->badge_sets_amount/8] |= 1 << (badge_manage->badge_sets_amount % 8);
        badge_manage->badge_sets_amount++;
    }

    free(badgemanage_buf);
    return 0;
}

Result badge_install(Entry_s badges)
{
    Entry_List_s list = {0};
    list.entries_count = 1;
    list.selected_entry = 0;
    list.entries = &badges;
    return badge_install_internal(list, BADGE_INSTALL_SINGLE);
}

Result badge_install_all(Entry_List_s list)
{
    return badge_install_internal(list, 0);
}