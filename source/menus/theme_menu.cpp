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

#include "menus/theme_menu.h"
#include "file.h"

static constexpr size_t SHUFFLE_MAX = 10;
static constexpr u32 BODY_CACHE_SIZE = 0x150000;
static constexpr u32 BGM_MAX_SIZE = 0x337000;
static FS_Path body_rd_path = fsMakePath(PATH_ASCII, "/BodyCache_rd.bin");
static FS_Path body_path = fsMakePath(PATH_ASCII, "/BodyCache.bin");

ThemeMenu::ThemeMenu() : Menu("/Themes/", 4, TEXT_THEME_MODE, TEXT_NOT_FOUND_SWITCH_TO_BADGE, TEXT_NOT_FOUND_SWITCH_TO_SPLASH, 48, COLOR_THEME_BG)
{
    const KeysActions normal_actions_down{
        {KEY_A, std::bind(&ThemeMenu::change_to_action_mode, this)},
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

ThemeMenu::~ThemeMenu()
{

}

void ThemeMenu::draw()
{
    Menu::draw();
    if(this->entries.size() && !this->in_preview())
    {
        switch_screen(GFX_BOTTOM);
        static constexpr float shuffle_indicator_x = 320.0f - 4.0f - 24.0f;
        C2D_ImageTint indicator_tint;
        u32 text_color;
        if(this->shuffle_count < 2 || this->shuffle_count > 10)
            C2D_PlainImageTint(&indicator_tint, (text_color = COLOR_SHUFFLE_BAD), 1.0f);
        else
            C2D_PlainImageTint(&indicator_tint, (text_color = COLOR_SHUFFLE_OK), 1.0f);

        std::string shuffle_count_str = std::to_string(this->shuffle_count) + "/" + std::to_string(SHUFFLE_MAX);
        draw_image(sprites_shuffle_idx, shuffle_indicator_x + 1.0f, 0.0f, 0.2f, &indicator_tint);
        float width, height;
        get_text_dimensions(shuffle_count_str, &width, &height, 0.6f, 0.6f);
        draw_text(shuffle_count_str, text_color, shuffle_indicator_x - width - 1.0f, (BARS_SIZE - 4.0f - height)/2.0f, 0.2f, 0.6f, 0.6f);
    }
}

MenuActionReturn ThemeMenu::change_to_action_mode()
{
    if(!this->entries.size())
        return RETURN_NONE;

    const KeysActions theme_actions_down{
        {KEY_A, std::bind(&ThemeMenu::mark_entry, this)},
        {KEY_B, std::bind(&MenuBase::exit_mode_controls, this)},
        {KEY_X, std::bind(&Menu::delete_selected_entry, this)},
        {KEY_Y, std::bind(&MenuBase::load_preview, this)},
        {KEY_DUP, std::bind(&ThemeMenu::install_themes, this, true, true, false)},
        {KEY_DLEFT, std::bind(&ThemeMenu::install_themes, this, false, true, false)},
        {KEY_DDOWN, std::bind(&ThemeMenu::install_themes, this, true, true, true)},
        {KEY_DRIGHT, std::bind(&ThemeMenu::install_themes, this, true, false, false)},
    };

    static const Instructions theme_actions_instructions{
        INSTRUCTION_THEME_A_FOR_MARKING,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTION_X_FOR_DELETING_ENTRY,
        INSTRUCTION_Y_FOR_PREVIEW,
        INSTRUCTION_THEME_UP_FOR_NORMAL,
        INSTRUCTION_THEME_LEFT_FOR_BGM_ONLY,
        INSTRUCTION_THEME_DOWN_FOR_SHUFFLE,
        INSTRUCTION_THEME_RIGHT_FOR_NO_BGM,
    };

    this->current_actions.push({theme_actions_down, {}});
    this->instructions_stack.push(&theme_actions_instructions);

    return RETURN_NONE;
}

MenuActionReturn ThemeMenu::mark_entry()
{
    auto& current_entry = this->entries[this->selected_entry];
    if(current_entry->state == Entry::STATE_NONE)
    {
        current_entry->state = Entry::STATE_SHUFFLE;
        ++this->shuffle_count;
    }
    else if(current_entry->state == Entry::STATE_SHUFFLE)
    {
        current_entry->state = Entry::STATE_SHUFFLE_NO_BGM;
    }
    else if(current_entry->state == Entry::STATE_SHUFFLE_NO_BGM)
    {
        current_entry->state = Entry::STATE_NONE;
        --this->shuffle_count;
    }
    return RETURN_NONE;
}

MenuActionReturn ThemeMenu::install_themes(bool install_body, bool install_bgm, bool shuffle)
{
    this->exit_mode_controls();
    std::vector<Entry*> entries_to_install;
    if(shuffle)
    {
        if(this->shuffle_count < 2)
        {
            draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_SHUFFLE_NOT_ENOUGH);
            return RETURN_NONE;
        }
        else if(this->shuffle_count > 10)
        {
            draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_SHUFFLE_TOO_MANY);
            return RETURN_NONE;
        }
        else
        {
            draw_loading_bar(0, this->shuffle_count, INSTALL_FINDING_MARKED_THEMES);
            entries_to_install.reserve(this->shuffle_count);
            for(auto& entry : this->entries)
            {
                if(entry->state == Entry::STATE_SHUFFLE || entry->state == Entry::STATE_SHUFFLE_NO_BGM)
                {
                    entries_to_install.push_back(entry.get());
                    draw_loading_bar(entries_to_install.size(), this->shuffle_count, INSTALL_FINDING_MARKED_THEMES);
                }
            }
            this->shuffle_count = 0;
        }
    }
    else
        entries_to_install.push_back(this->entries[this->selected_entry].get());

    if(!shuffle)
    {
        if(install_body && install_bgm)
            draw_install(INSTALL_THEME_SINGLE);
        else if(install_body)
            draw_install(INSTALL_THEME_NO_BGM);
        else if(install_bgm)
            draw_install(INSTALL_THEME_BGM);
        else
            svcBreak(USERBREAK_PANIC);
    }


    size_t i = 0;
    Handle shuffle_install_file;
    char* padded_body = new(std::nothrow) char[BODY_CACHE_SIZE];
    char* padded_bgm = new(std::nothrow) char[BGM_MAX_SIZE];
    u32 body_sizes[SHUFFLE_MAX] = {0};
    u32 music_sizes[SHUFFLE_MAX] = {0};
    char to_append[3] = "_0";

    if(shuffle)
    {
        remake_file(body_rd_path, THEME_EXTDATA, BODY_CACHE_SIZE * SHUFFLE_MAX);
        file_open(body_rd_path, THEME_EXTDATA, &shuffle_install_file, FS_OPEN_WRITE);
    }

    for(Entry* marked_entry : entries_to_install)
    {
        if(install_body)
            memset(padded_body, 0, BODY_CACHE_SIZE);
        if(install_bgm)
            memset(padded_bgm, 0, BGM_MAX_SIZE);

        char bgm_path_str[9 + 3 + 4 + 1] = "/BgmCache";
        if(shuffle)
            draw_loading_bar(i, entries_to_install.size(), INSTALL_THEME_SHUFFLE);

        if(install_body)
        {
            body_sizes[i] = marked_entry->get_file("body_LZ.bin", &padded_body);
            if(body_sizes[i] == 0)
            {
                draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_THEME_NO_BODY);
                delete[] padded_body;
                delete[] padded_bgm;
                if(shuffle)
                {
                    FSFILE_Close(shuffle_install_file);
                }
                return RETURN_NONE;
            }
        }

        if(shuffle)
        {
            FSFILE_Write(shuffle_install_file, nullptr, BODY_CACHE_SIZE * i, padded_body, BODY_CACHE_SIZE, FS_WRITE_FLUSH);
            to_append[2] = '0' + i;
            strncat(bgm_path_str, to_append, 3);
        }
        else if(install_body)
        {
            buf_to_file(body_path, THEME_EXTDATA, BODY_CACHE_SIZE, padded_body); // Write body data to file
        }

        strcat(bgm_path_str, ".bin");
        FS_Path bgm_path = fsMakePath(PATH_ASCII, bgm_path_str);
        remake_file(bgm_path, THEME_EXTDATA, BGM_MAX_SIZE);
        if(install_bgm && !(shuffle && marked_entry->state == Entry::STATE_SHUFFLE_NO_BGM))
        {
            music_sizes[i] = marked_entry->get_file("bgm.bcstm", &padded_bgm);
            if(music_sizes[i] > BGM_MAX_SIZE)
            {
                draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_THEME_BGM_TOO_BIG);
                delete[] padded_body;
                delete[] padded_bgm;
                if(shuffle)
                {
                    FSFILE_Close(shuffle_install_file);
                }
                return RETURN_NONE;
            }
            else if(music_sizes[i] == 0)
            {
                draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_THEME_NO_BGM);
            }

            buf_to_file(bgm_path, THEME_EXTDATA, BGM_MAX_SIZE, padded_bgm);
        }

        ++i;
        if(shuffle)
            marked_entry->state = Entry::STATE_NONE;
    }

    delete[] padded_body;
    delete[] padded_bgm;

    if(shuffle)
    {
        FSFILE_Close(shuffle_install_file);
    }

    //------------------------------------------
    {
        struct ThemeManage_bin_s {
            u32 unk1;
            u32 unk2;
            u32 body_size;
            u32 music_size;
            u32 unk3;
            u32 unk4;
            u32 dlc_theme_content_index;
            u32 use_theme_cache;

            u8 _padding1[0x338 - 8*sizeof(u32)];

            u32 shuffle_body_sizes[SHUFFLE_MAX];
            u32 shuffle_music_sizes[SHUFFLE_MAX];
        };

        char* thememanage_buf = nullptr;
        FS_Path thememanage_path = fsMakePath(PATH_ASCII, "/ThemeManage.bin");
        file_to_buf(thememanage_path, THEME_EXTDATA, &thememanage_buf);
        ThemeManage_bin_s * theme_manage = reinterpret_cast<ThemeManage_bin_s*>(thememanage_buf);

        theme_manage->unk1 = 1;
        theme_manage->unk2 = 0;

        if(shuffle)
        {
            theme_manage->music_size = 0;
            theme_manage->body_size = 0;

            for(size_t i = 0; i < SHUFFLE_MAX; i++)
            {
                theme_manage->shuffle_body_sizes[i] = body_sizes[i];
                theme_manage->shuffle_music_sizes[i] = music_sizes[i];
            }
        }
        else
        {
            if(install_bgm)
                theme_manage->music_size = music_sizes[0];
            if(install_body)
                theme_manage->body_size = body_sizes[0];
        }

        theme_manage->unk3 = 0xFF;
        theme_manage->unk4 = 1;
        theme_manage->dlc_theme_content_index = 0xFF;
        theme_manage->use_theme_cache = 0x0200;

        buf_to_file(thememanage_path, THEME_EXTDATA, 0x800, thememanage_buf);
        delete[] thememanage_buf;
    }
    //------------------------------------------

    //------------------------------------------
    {
        struct ThemeEntry_s {
            u32 index;
            u8 dlc_tid_low_bits;
            u8 type;
            u16 unk;
        };

        struct SaveData_dat_s {
            u8 _padding1[0x13b8];
            ThemeEntry_s theme_entry;
            ThemeEntry_s shuffle_themes[SHUFFLE_MAX];
            u8 _padding2[0xb];
            u8 shuffle;
        };

        char* savedata_buf = nullptr;
        FS_Path savedata_path = fsMakePath(PATH_ASCII, "/SaveData.dat");
        u32 savedata_size = file_to_buf(savedata_path, HOME_EXTDATA, &savedata_buf);
        SaveData_dat_s* savedata = reinterpret_cast<SaveData_dat_s*>(savedata_buf);

        memset(&savedata->theme_entry, 0, sizeof(ThemeEntry_s));
        savedata->theme_entry.type = 3;
        savedata->theme_entry.index = 0xff;

        if((savedata->shuffle = shuffle))
        {
            memset(savedata->shuffle_themes, 0, sizeof(ThemeEntry_s)*SHUFFLE_MAX);
            for(size_t i = 0; i < entries_to_install.size(); i++)
            {
                savedata->shuffle_themes[i].type = 3;
                savedata->shuffle_themes[i].index = i;
            }
        }

        buf_to_file(savedata_path, HOME_EXTDATA, savedata_size, savedata_buf);
        delete[] savedata_buf;
    }
    //------------------------------------------

    return RETURN_INSTALLED_THEME;
}
