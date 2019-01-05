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

#include "menus/badge_menu.h"
#include "file.h"

#include <png.h>

static constexpr u32 BADGE_SET_LIMIT = 100;
static constexpr u32 BADGE_LIMIT = 1000;
static constexpr u32 BADGE_LAYOUT_SLOT_LIMIT = 360;
static constexpr u32 BADGE_LANGUAGES_COUNT = 16;
static constexpr int BADGE_MAX_SPLIT_WITH = 12;
static constexpr u32 ICON_SIZE_64 = (64*64);
static constexpr u32 ICON_SIZE_32 = (32*32);
static FS_Path badge_manage_path = fsMakePath(PATH_ASCII, "/BadgeMngFile.dat");
static FS_Path badge_data_path = fsMakePath(PATH_ASCII, "/BadgeData.dat");

struct Pixel_s {
    u8 r, g, b ,a;
};

struct Badge_Icon_64_s {
    u16 icon_data[ICON_SIZE_64];
    u8 icon_alpha[ICON_SIZE_64/2];
};

struct Badge_Icon_32_s {
    u16 icon_data[ICON_SIZE_32];
    u8 icon_alpha[ICON_SIZE_32/2];
};

struct Badge_Data_dat_s {
    u16 badge_set_titles[BADGE_SET_LIMIT][BADGE_LANGUAGES_COUNT][0x45];
    u16 badge_titles[BADGE_LIMIT][BADGE_LANGUAGES_COUNT][0x45];
    u16 badge_set_icons_565_64[BADGE_SET_LIMIT][ICON_SIZE_64];
    Badge_Icon_64_s badge_icons_64[BADGE_LIMIT];
    Badge_Icon_32_s badge_icons_32[BADGE_LIMIT];
};

struct Badge_Identifier_s {
    u32 unk;
    u32 id;
    u32 set_id;
    u16 index;
    u16 sub_id;
};

struct Badge_Info_s {
    Badge_Identifier_s identifier;
    u16 number_placed;
    u16 quantity;
    u32 unk;
    u64 shortcut_tid[2];
};

struct Badge_Set_Identifier_s {
    u64 unk1;
    u32 unk2;
    u32 unk3;
    u32 set_id;
    u32 set_index;
};

struct Badge_Set_Info_s {
    Badge_Set_Identifier_s set_identifier;
    u32 unk1;
    u32 unique_badges_amount;
    u32 total_badges_amount;
    u32 start_badge_index;
    u32 unk2;
    u32 unk3;
};

struct Badge_Layout_Slot_s {
    Badge_Identifier_s identifier;
    u32 position;
    u32 folder_icon;
};

struct Badge_Mng_File_dat_s {
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
};

BadgeMenu::BadgeMenu() : Menu("/Badges/", 3, TEXT_BADGE_MODE, TEXT_NOT_FOUND_SWITCH_TO_SPLASH, TEXT_NOT_FOUND_SWITCH_TO_THEME, 64, COLOR_BADGE_BG, true)
{
    const KeysActions normal_actions_down{
        {KEY_A, std::bind(&BadgeMenu::change_to_action_mode, this)},
        {KEY_B, std::bind(&Menu::change_to_qr_scanner, this)},
        {KEY_X, std::bind(&Menu::change_to_extra_mode, this)},
        {KEY_Y, std::bind(&Menu::change_to_browser_mode, this)},
        {KEY_L, std::bind(&Menu::change_to_previous_mode, this)},
        {KEY_R, std::bind(&Menu::change_to_next_mode, this)},
        {KEY_DUP, std::bind(&Menu::select_previous_entry, this)},
        {KEY_DLEFT, std::bind(&Menu::select_previous_page, this)},
        {KEY_DDOWN, std::bind(&Menu::select_next_entry, this)},
        {KEY_DRIGHT, std::bind(&Menu::select_next_page, this)},
        {KEY_TOUCH, std::bind(&Menu::handle_touch, this)},
    };

    const KeysActions normal_actions_held{
        {KEY_CPAD_UP, std::bind(&Menu::select_previous_entry_fast, this)},
        {KEY_CPAD_LEFT, std::bind(&Menu::select_previous_page_fast, this)},
        {KEY_CPAD_DOWN, std::bind(&Menu::select_next_entry_fast, this)},
        {KEY_CPAD_RIGHT, std::bind(&Menu::select_next_page_fast, this)},
    };

    this->current_actions.push({normal_actions_down, normal_actions_held});
}

BadgeMenu::~BadgeMenu()
{

}

void BadgeMenu::draw()
{
    Menu::draw();
    if(this->entries.size() && !this->in_preview())
    {
        switch_screen(GFX_BOTTOM);
        static constexpr float multi_indicator_x = 320.0f - 4.0f - 24.0f;
        C2D_ImageTint indicator_tint;
        u32 text_color = COLOR_WHITE;
        C2D_PlainImageTint(&indicator_tint, text_color, 1.0f);

        std::string shuffle_count_str = std::to_string(this->marked_count);
        draw_image(sprites_multi_idx, multi_indicator_x + 1.0f, 0.0f, 0.2f, &indicator_tint);
        float width, height;
        get_text_dimensions(shuffle_count_str, &width, &height, 0.6f, 0.6f);
        draw_text(shuffle_count_str, text_color, multi_indicator_x - width - 1.0f, (BARS_SIZE - 4.0f - height)/2.0f, 0.2f, 0.6f, 0.6f);
    }
}

MenuActionReturn BadgeMenu::change_to_action_mode()
{
    if(!this->entries.size())
        return RETURN_NONE;

    const KeysActions badge_actions_down{
        {KEY_A, std::bind(&BadgeMenu::mark_entry, this)},
        {KEY_B, std::bind(&MenuBase::exit_mode_controls, this)},
        {KEY_X, std::bind(&Menu::delete_selected_entry, this)},
        {KEY_Y, std::bind(&MenuBase::load_preview, this)},
        {KEY_DUP, std::bind(&BadgeMenu::install_badges, this, false)},
        {KEY_DDOWN, std::bind(&BadgeMenu::install_badges, this, true)},
    };

    static const Instructions badge_actions_instructions{
        INSTRUCTION_BADGE_A_FOR_MARKING,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTION_X_FOR_DELETING_ENTRY,
        INSTRUCTION_Y_FOR_PREVIEW,
        INSTRUCTION_BADGE_UP_FOR_SINGLE,
        INSTRUCTIONS_NONE,
        INSTRUCTION_BADGE_DOWN_FOR_MULTI,
        INSTRUCTIONS_NONE,
    };

    this->current_actions.push({badge_actions_down, {}});
    this->instructions_stack.push(&badge_actions_instructions);

    return RETURN_NONE;
}

MenuActionReturn BadgeMenu::mark_entry()
{
    auto& current_entry = this->entries[this->selected_entry];
    if(current_entry->state == Entry::STATE_NONE)
    {
        current_entry->state = Entry::STATE_MULTI;
        ++this->marked_count;
    }
    else if(current_entry->state == Entry::STATE_MULTI)
    {
        current_entry->state = Entry::STATE_NONE;
        --this->marked_count;
    }
    return RETURN_NONE;
}

static png_bytep* load_image_to_rows(const fs::path& path, int* width, int* height)
{
    FILE* fh = fopen(path.c_str(), "rb");

    u8 sig[8];
    fread(sig, 1, 8, fh);
    if(png_sig_cmp(sig, 0, 8))
    {
        fclose(fh);
        return nullptr;
    }

    fseek(fh, 0, SEEK_SET);

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fh);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    if(*width < 64 || *height < 64 || *width % 64 != 0 || *height % 64 != 0 || *width > BADGE_MAX_SPLIT_WITH*64  || *height > 6*64)
    {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fh);
        return nullptr;
    }

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth  = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    png_size_t row_bytes = png_get_rowbytes(png,info);
    png_bytep* row_pointers = new(std::nothrow) png_bytep[*height];
    for(int y = 0; y < *height; y++)
        row_pointers[y] = new(std::nothrow) png_byte[row_bytes];

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fh);

    return row_pointers;
}

static u32 split_badge(const fs::path& path, Badge_Data_dat_s* badgedata, u32 start_index)
{
    int width, height;
    png_bytep* row_pointers = load_image_to_rows(path, &width, &height);
    if(row_pointers == nullptr)
        return 0;

    const int endx = width/64;
    Pixel_s *top_left_pixel, *top_right_pixel, *bottom_left_pixel, *bottom_right_pixel;

    const u32 max_badges = endx * height/64;
    u32 index = start_index;
    for(int y = 0; y < height; y += 64)
    {
        if(max_badges > 1)
            draw_loading_bar(y/64*endx, max_badges, INSTALL_BADGES_SPLITTING);

        for(u32 j = 0; j < 32; ++j)
        {
            png_bytep top_row = row_pointers[y + j*2];
            png_bytep bottom_row = row_pointers[y + j*2 + 1];
            for(u32 i = 0; i < 32; ++i)
            {
                u32 dst = ((((j >> 3) * (32 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3)));
                for(int x = 0; x < BADGE_MAX_SPLIT_WITH; ++x)
                {
                    if(x >= endx)
                        break;

                    u32 actual_x =  x*64 + i*2;
                    top_left_pixel = reinterpret_cast<Pixel_s*>(&top_row[(actual_x)*sizeof(u32)]);
                    top_right_pixel = reinterpret_cast<Pixel_s*>(&top_row[(actual_x + 1)*sizeof(u32)]);
                    bottom_left_pixel = reinterpret_cast<Pixel_s*>(&bottom_row[(actual_x)*sizeof(u32)]);
                    bottom_right_pixel = reinterpret_cast<Pixel_s*>(&bottom_row[(actual_x + 1)*sizeof(u32)]);

                    Pixel_s px = {0};
                    px.r = (top_left_pixel->r + top_right_pixel->r + bottom_left_pixel->r + bottom_right_pixel->r)/4;
                    px.g = (top_left_pixel->g + top_right_pixel->g + bottom_left_pixel->g + bottom_right_pixel->g)/4;
                    px.b = (top_left_pixel->b + top_right_pixel->b + bottom_left_pixel->b + bottom_right_pixel->b)/4;
                    px.a = (top_left_pixel->a + top_right_pixel->a + bottom_left_pixel->a + bottom_right_pixel->a)/4;
                    Badge_Icon_32_s* icon = &badgedata->badge_icons_32[index + x];

                    u16 pixel_565 = RGB8_to_565(px.r, px.g, px.b);
                    icon->icon_data[dst] = pixel_565;
                    u8 alpha_4 = (px.a * 0x0f)/0xff;
                    icon->icon_alpha[dst] |= (alpha_4 & 0x0f) << ((i % 2) * 4);
                }
            }
        }

        for(u32 j = 0; j < 64; ++j)
        {
            png_bytep row = row_pointers[y + j];
            for(u32 i = 0; i < 64; ++i)
            {
                u32 dst = ((((j >> 3) * (64 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3)));
                for(int x = 0; x < BADGE_MAX_SPLIT_WITH; ++x)
                {
                    if(x >= endx)
                        break;

                    Pixel_s* px = reinterpret_cast<Pixel_s*>(&row[(i + x*64)*sizeof(u32)]);

                    Badge_Icon_64_s* icon = &badgedata->badge_icons_64[index + x];

                    u16 pixel_565 = RGB8_to_565(px->r, px->g, px->b);
                    icon->icon_data[dst] = pixel_565;
                    u8 alpha_4 = (px->a * 0x0f)/0xff;
                    icon->icon_alpha[dst] |= (alpha_4 & 0x0f) << ((i % 2) * 4);
                }
            }
            delete[] row;
        }

        index += endx;
    }

    delete[] row_pointers;
    return max_badges;
}

static void load_homebrew_set_icon(u16* icon_out)
{
    int width, height;
    png_bytep* row_pointers = load_image_to_rows("romfs:/badge_set_icon.png", &width, &height);
    for(int j = 0; j < 64; j++)
    {
        png_bytep row = row_pointers[j];
        for(int i = 0; i < 64; i++)
        {
            Pixel_s* px = reinterpret_cast<Pixel_s*>(&(row[i * sizeof(u32)]));
            u32 dst = ((((j >> 3) * (64 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3)));

            icon_out[dst] = RGB8_to_565(px->r, px->g, px->b);
        }
        delete[] row;
    }
    delete[] row_pointers;
}

MenuActionReturn BadgeMenu::install_badges(bool multi)
{
    this->exit_mode_controls();
    std::vector<Entry*> entries_to_install;
    if(multi)
    {
        draw_loading_bar(0, this->marked_count, INSTALL_FINDING_MARKED_BADGES);
        entries_to_install.reserve(this->marked_count);
        for(auto& entry : this->entries)
        {
            if(entry->state == Entry::STATE_MULTI)
            {
                entries_to_install.push_back(entry.get());
                draw_loading_bar(entries_to_install.size(), this->marked_count, INSTALL_FINDING_MARKED_BADGES);
            }
        }
        this->marked_count = 0;
    }
    else
    {
        draw_install(INSTALL_BADGE);
        entries_to_install.push_back(this->entries[this->selected_entry].get());
    }

    char* badgemanage_buf = nullptr;
    if(file_to_buf(badge_manage_path, BADGE_EXTDATA, &badgemanage_buf) == 0)
    {
        draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_BADGE_EXTDATA_IN_USE);
        return RETURN_NONE;
    }

    char* badgedata_buf = nullptr;
    if(file_to_buf(badge_data_path, BADGE_EXTDATA, &badgedata_buf) == 0)
    {
        delete[] badgemanage_buf;
        draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_BADGE_EXTDATA_IN_USE);
        return RETURN_NONE;
    }

    Badge_Mng_File_dat_s* badgemanage = reinterpret_cast<Badge_Mng_File_dat_s*>(badgemanage_buf);
    Badge_Data_dat_s* badgedata = reinterpret_cast<Badge_Data_dat_s*>(badgedata_buf);

    static constexpr u32 homebrew_set_id = 0x0000BEEF;
    static constexpr u16 badge_quantity = 0xFFFF;
    u32 start_index = badgemanage->unique_badges_amount;

    u32 total_installed_badges = 0;
    u32 i = 0;
    u64 tid_high = 0x00040010ULL << 32; 
    for(Entry* marked_entry : entries_to_install)
    {
        if(multi)
            draw_loading_bar(i, entries_to_install.size(), INSTALL_BADGES_MULTI);

        u32 badges_split = split_badge(marked_entry->path, badgedata, start_index + total_installed_badges);

        u16 name[0x41] = {0};
        utf8_to_utf16(name, reinterpret_cast<const u8*>(marked_entry->title.c_str()), 0x40);
        u32 shortcut_lowid = 0;

        for(u32 j = 0; j < badges_split; ++j)
        {
            u32 current_index = start_index + total_installed_badges;

            for(u32 k = 0; k < BADGE_LANGUAGES_COUNT; k++)
                memcpy(badgedata->badge_titles[current_index][k], name, 0x40); //entry name is only 0x41, but badge name can go up to 0x45


            Badge_Info_s* current_slot = &badgemanage->badge_info_entries[current_index];
            current_slot->number_placed = 0;
            current_slot->quantity = badge_quantity;
            current_slot->shortcut_tid[0] = tid_high | shortcut_lowid;
            current_slot->shortcut_tid[1] = tid_high | shortcut_lowid;

            Badge_Identifier_s* current_identifier = &current_slot->identifier;
            current_identifier->id = current_index + 1;
            current_identifier->set_id = homebrew_set_id;
            current_identifier->index = current_index;

            badgemanage->used_badge_slot[current_index/8] |= 1 << (current_index % 8);
            badgemanage->total_badges_amount += badge_quantity;


            ++total_installed_badges;
        }

        ++i;
        if(multi)
            marked_entry->state = Entry::STATE_NONE;
    }

    badgemanage->unique_badges_amount += total_installed_badges;
    for(; i < badgemanage->badge_sets_amount; i++)
    {
        Badge_Set_Info_s* current_set = &badgemanage->badge_set_info_entries[i];
        if(current_set->set_identifier.set_id == homebrew_set_id) //if there already is a homebrew set, add to it
        {
            current_set->unique_badges_amount += total_installed_badges;
            current_set->total_badges_amount += total_installed_badges * badge_quantity;
            break;
        }
    }

    if(i == badgemanage->badge_sets_amount) //if the homebrew set wasnt found, create it
    {
        draw_install(INSTALL_BADGES_CREATING_SET);
        Badge_Set_Info_s* current_set = &badgemanage->badge_set_info_entries[i];
        Badge_Set_Identifier_s* current_identifier = &current_set->set_identifier;

        current_identifier->unk3 = 0x2710;
        current_identifier->set_id = homebrew_set_id;
        current_identifier->set_index = badgemanage->badge_sets_amount;

        current_set->unique_badges_amount = total_installed_badges;
        current_set->total_badges_amount = total_installed_badges * badge_quantity;
        current_set->start_badge_index = start_index;

        u16 title[0x46] = {0};
        utf8_to_utf16(title, reinterpret_cast<const u8*>("Homebrew Badges"), 0x45);
        for(u32 j = 0; j < BADGE_LANGUAGES_COUNT; j++)
            memcpy(badgedata->badge_set_titles[badgemanage->badge_sets_amount][j], title, 0x45);

        load_homebrew_set_icon(badgedata->badge_set_icons_565_64[badgemanage->badge_sets_amount]);

        badgemanage->used_badge_set_slot[badgemanage->badge_sets_amount/8] |= 1 << (badgemanage->badge_sets_amount % 8);
        ++badgemanage->badge_sets_amount;
    }

    buf_to_file(badge_manage_path, BADGE_EXTDATA, sizeof(Badge_Mng_File_dat_s), badgemanage_buf);
    buf_to_file(badge_data_path, BADGE_EXTDATA, sizeof(Badge_Data_dat_s), badgedata_buf);

    delete[] badgemanage_buf;
    delete[] badgedata_buf;
    return RETURN_NONE;
}