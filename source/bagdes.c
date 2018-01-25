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
#include "pp2d/pp2d/lodepng.h"

static void rgba8_to_tiled_buffers(u8* image, unsigned int width, unsigned int height, u16* rgb_buf_64x64, u8* alpha_buf_64x64, u16* rgb_buf_32x32, u8* alpha_buf_32x32) {
    u8 r, g, b, a;
    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            r = image[y*width*4 + x*4 + 0] >> 3;
            g = image[y*width*4 + x*4 + 1] >> 2;
            b = image[y*width*4 + x*4 + 2] >> 3;
            a = image[y*width*4 + x*4 + 3] >> 4;

            unsigned int rgb565_index = 8*64*((y/8)%8) | 64*((x/8)%8) | 32*((y/4)%2) | 16*((x/4)%2) | 8*((y/2)%2) | 4*((x/2)%2) | 2*(y%2) | (x%2);

            //only applicable when more than one badge being created from image
            rgb565_index |= 64*64*(height/64)*(x/64) + 64*64*(y/64);

            if(rgb_buf_64x64) rgb_buf_64x64[rgb565_index] = (r << 11) | (g << 5) | b;
            if(alpha_buf_64x64) alpha_buf_64x64[rgb565_index / 2] |= a << (4*(x%2));

            if(x % 2 == 0 && y % 2 == 0 && (rgb_buf_32x32 || alpha_buf_32x32))
            {
                r = (image[y*width*4 + x*4 + 0] + image[(y+1)*width*4 + x*4 + 0] + image[y*width*4 + (x+1)*4 + 0] + image[(y+1)*width*4 + (x+1)*4 + 0]) >> 5;
                g = (image[y*width*4 + x*4 + 1] + image[(y+1)*width*4 + x*4 + 1] + image[y*width*4 + (x+1)*4 + 1] + image[(y+1)*width*4 + (x+1)*4 + 1]) >> 4;
                b = (image[y*width*4 + x*4 + 2] + image[(y+1)*width*4 + x*4 + 2] + image[y*width*4 + (x+1)*4 + 2] + image[(y+1)*width*4 + (x+1)*4 + 2]) >> 5;
                a = (image[y*width*4 + x*4 + 3] + image[(y+1)*width*4 + x*4 + 3] + image[y*width*4 + (x+1)*4 + 3] + image[(y+1)*width*4 + (x+1)*4 + 3]) >> 6;

                unsigned int halfx = x/2;
                unsigned int halfy = y/2;

                rgb565_index = 4*64*((halfy/8)%4) | 64*((halfx/8)%4) | 32*((halfy/4)%2) | 16*((halfx/4)%2) | 8*((halfy/2)%2) | 4*((halfx/2)%2) | 2*(halfy%2) | (halfx%2);

                rgb565_index |= 32*32*(height/64)*(x/64) + 32*32*(y/64);

                rgb_buf_32x32[rgb565_index] = (r << 11) | (g << 5) | b;
                alpha_buf_32x32[rgb565_index / 2] |= a << (4*(halfx%2));
            }
        }
    }
}

static Result badge_install_internal(Entry_List_s list, int install_mode)
{
    char* badgemanage_buf = NULL;
    if(file_to_buf(fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), ArchiveBadgeExt, &badgemanage_buf) == 0)
    {
        return -1;
    }
    Badge_Mng_File_dat_s * badge_manage = (Badge_Mng_File_dat_s *)badgemanage_buf;

    char* badgedata_buf = NULL;
    if(file_to_buf(fsMakePath(PATH_ASCII, "/BadgeData.dat"), ArchiveBadgeExt, &badgedata_buf) == 0)
    {
        free(badgemanage_buf);
        return -1;
    }
    Badge_Data_dat_s * badge_data = (Badge_Data_dat_s *)badgedata_buf;

    const u32 homebrew_set_id = 0x0000BEEF;
    const u16 badge_quantity = 0xFFFF;
    u32 start_index = badge_manage->unique_badges_amount;
    u32 total_installed_badges = 0;

    for(int i = (install_mode & BADGE_INSTALL_SINGLE ? list.selected_entry : 0); i < list.entries_count; i++)
    {
        Entry_s current_entry = list.entries[i];

        char * image_buf = NULL;
        u32 size = load_data("", current_entry, &image_buf);
        u32 * image = NULL;
        unsigned int width = 0, height = 0;
        if(lodepng_decode32((unsigned char**)&image, &width, &height, (unsigned char*)image_buf, size) == 0)
        {
            if (width < 64 || height < 64 || width % 64 != 0 || width % 64 != 0 || width > 12*384  || height > 6*384)
            {
                throw_error("PNG file doesnt have the right size.", ERROR_LEVEL_WARNING);
            }
            else
            {
                unsigned int badges_in_image = (height/64)*(width/64);

                u16 ** badge_icons_565_64 = calloc(badges_in_image, sizeof(u16*));
                u8 ** badge_icons_A4_64 = calloc(badges_in_image, sizeof(u8*));
                u16 ** badge_icons_565_32 = calloc(badges_in_image, sizeof(u16*));
                u8 ** badge_icons_A4_32 = calloc(badges_in_image, sizeof(u8*));

                u16 * icon_data_64 = calloc(badges_in_image*ICON_SIZE_64, sizeof(u16));
                u8 * icon_alpha_64 = calloc((badges_in_image*ICON_SIZE_64)/2, sizeof(u8));
                u16 * icon_data_32 = calloc(badges_in_image*ICON_SIZE_32, sizeof(u16*));
                u8 * icon_alpha_32 = calloc((badges_in_image*ICON_SIZE_32)/2, sizeof(u8));

                for(unsigned int j = 0; j < badges_in_image; j++)
                {
                    badge_icons_565_64[j]  = icon_data_64 + j*ICON_SIZE_64*sizeof(u16);
                    badge_icons_A4_64[j] = icon_alpha_64 + j*ICON_SIZE_64*sizeof(u8)/2;
                    badge_icons_565_32[j] = icon_data_32 + j*ICON_SIZE_32*sizeof(u16);
                    badge_icons_A4_32[j] = icon_alpha_32 + j*ICON_SIZE_32*sizeof(u8)/2;
                }

                rgba8_to_tiled_buffers((u8*)image, width, height,
                                       icon_data_64,
                                       icon_alpha_64,
                                       icon_data_32,
                                       icon_alpha_32);
                free(image);
                image = NULL;

                for(unsigned int j = 0; j < badges_in_image; j++)
                {
                    u32 current_index = start_index+total_installed_badges;
                    for(int k = 0; k < 16; k++)
                        memcpy(badge_data->badge_titles[current_index][k], current_entry.name, 0x40); //entry name is only 0x41, but badge name can go up to 0x45

                    memcpy(badge_data->badge_icons_64[current_index].icon_data, badge_icons_565_64[j], ICON_SIZE_64*sizeof(u16));
                    memcpy(badge_data->badge_icons_64[current_index].icon_alpha, badge_icons_A4_64[j], ICON_SIZE_64/2);

                    memcpy(badge_data->badge_icons_32[current_index].icon_data, badge_icons_565_32[j], ICON_SIZE_32*sizeof(u16));
                    memcpy(badge_data->badge_icons_32[current_index].icon_alpha, badge_icons_A4_32[j], ICON_SIZE_32/2);
                    u32 shortcut_lowid = 0;

                    Badge_Info_s * current_slot = &badge_manage->badge_info_entries[current_index];
                    Badge_Identifier_s * current_identifier = &current_slot->identifier;

                    current_identifier->id = current_index+1;
                    current_identifier->set_id = homebrew_set_id;
                    current_identifier->index = current_index;

                    current_slot->number_placed = 0;
                    current_slot->quantity = badge_quantity;
                    if(shortcut_lowid)
                    {
                        current_slot->shortcut_tid[0] = ((u64)0x00040010 << 32) | shortcut_lowid;
                        current_slot->shortcut_tid[1] = ((u64)0x00040010 << 32) | shortcut_lowid;
                    }

                    badge_manage->used_badge_slot[current_index/8] |= 1 << (current_index % 8);
                    badge_manage->total_badges_amount += badge_quantity;
                    total_installed_badges++;
                }

                free(badge_icons_565_64);
                free(badge_icons_A4_64);
                free(badge_icons_565_32);
                free(badge_icons_A4_32);

                free(icon_data_64);
                free(icon_alpha_64);
                free(icon_data_32);
                free(icon_alpha_32);
            }
        }
        else
        {
            throw_error("Failed to open png file.", ERROR_LEVEL_WARNING);
        }

        free(image_buf);
        free(image);

        if(install_mode & BADGE_INSTALL_SINGLE)
            break;
    }

    badge_manage->unique_badges_amount += total_installed_badges;

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

        u16 title[0x46] = {0};
        utf8_to_utf16(title, (u8*)"Homebrew Badges", 0x45);
        for(int j = 0; j < 16; j++)
            memcpy(badge_data->badge_set_titles[badge_manage->badge_sets_amount][j], title, 0x45);

        u8 * image = NULL;
        unsigned int width = 0, height = 0;
        if(lodepng_decode32_file(&image, &width, &height, "romfs:/badge_set_icon.png") == 0)
            rgba8_to_tiled_buffers(image, 64, 64, badge_data->badge_set_icons_565_64[badge_manage->badge_sets_amount], NULL, NULL, NULL);
        free(image);

        badge_manage->used_badge_set_slot[badge_manage->badge_sets_amount/8] |= 1 << (badge_manage->badge_sets_amount % 8);
        badge_manage->badge_sets_amount++;
    }

    buf_to_file(sizeof(Badge_Mng_File_dat_s), "/BadgeMngFile.dat", ArchiveBadgeExt, badgemanage_buf);
    buf_to_file(sizeof(Badge_Data_dat_s), "/BadgeData.dat", ArchiveBadgeExt, badgedata_buf);

    free(badgemanage_buf);
    free(badgedata_buf);
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