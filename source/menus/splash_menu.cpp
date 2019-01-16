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

#include "menus/splash_menu.h"
#include "file.h"

static constexpr u32 TOP_SCREEN_SPLASH_SIZE = 3 * 240 * 400;
static constexpr u32 BOTTOM_SCREEN_SPLASH_SIZE = 3 * 240 * 320;

static const FS_Path top_splash_path = fsMakePath(PATH_ASCII, "/luma/splash.bin");
static const FS_Path bottom_splash_path = fsMakePath(PATH_ASCII, "/luma/splashbottom.bin");
static const FS_Path luma_config_path = fsMakePath(PATH_ASCII, "/luma/config.bin");

SplashMenu::SplashMenu() : Menu("/Splashes", 4, TEXT_SPLASH_MODE, TEXT_NOT_FOUND_SWITCH_TO_THEME, TEXT_NOT_FOUND_SWITCH_TO_BADGE, 48, COLOR_SPLASH_BG)
{
    const KeysActions normal_actions_down{
        {KEY_A, std::bind(&SplashMenu::change_to_action_mode, this)},
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

SplashMenu::~SplashMenu()
{

}

MenuActionReturn SplashMenu::change_to_action_mode()
{
    if(!this->entries.size())
        return RETURN_NONE;

    const KeysActions splash_actions_down{
        {KEY_A, std::bind(&SplashMenu::delete_installed_splash, this)},
        {KEY_B, std::bind(&MenuBase::exit_mode_controls, this)},
        {KEY_X, std::bind(&Menu::delete_selected_entry, this)},
        {KEY_Y, std::bind(&MenuBase::load_preview, this)},
        {KEY_DUP, std::bind(&SplashMenu::install_splash, this, true, true)},
        {KEY_DLEFT, std::bind(&SplashMenu::install_splash, this, true, false)},
        {KEY_DRIGHT, std::bind(&SplashMenu::install_splash, this, false, true)},
    };

    static const Instructions splash_actions_instructions{
        INSTRUCTION_SPLASH_A_TO_DELETE_INSTALLED,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTION_X_FOR_DELETING_ENTRY,
        INSTRUCTION_Y_FOR_PREVIEW,
        INSTRUCTION_SPLASH_UP_FOR_INSTALLING,
        INSTRUCTION_SPLASH_LEFT_FOR_INSTALLING_TOP,
        INSTRUCTIONS_NONE,
        INSTRUCTION_SPLASH_RIGHT_FOR_INSTALLING_BOTTOM,
    };

    this->current_actions.push({splash_actions_down, {}});
    this->instructions_stack.push(&splash_actions_instructions);

    return RETURN_NONE;
}

MenuActionReturn SplashMenu::delete_installed_splash()
{
    delete_file(top_splash_path, have_luma_folder ? SD_CARD : CTRNAND);
    delete_file(bottom_splash_path, have_luma_folder ? SD_CARD : CTRNAND);
    return RETURN_NONE;
}

bool SplashMenu::install_splash_internal(FS_Path dest, const std::string& source, u32 wanted_size, ErrorType size_wrong_error, ErrorType not_found_error)
{
    const auto& [splash_buf, splash_size] = this->entries[this->selected_entry]->get_file(source);
    if(splash_buf)
    {
        if(splash_size == wanted_size)
        {
            remake_file(dest, have_luma_folder ? SD_CARD : CTRNAND, wanted_size, splash_buf.get());
        }
        else
        {
            draw_error(ERROR_LEVEL_ERROR, size_wrong_error);
        }
        return splash_size == wanted_size;
    }
    else
    {
        draw_error(ERROR_LEVEL_ERROR, not_found_error);
        return false;
    }
}

MenuActionReturn SplashMenu::install_splash(bool install_top, bool install_bottom)
{
    bool installed_top = true;
    if(install_top)
        installed_top = this->install_splash_internal(top_splash_path, "splash.bin", TOP_SCREEN_SPLASH_SIZE, ERROR_TYPE_SPLASH_TOP_SIZE_WRONG, ERROR_TYPE_SPLASH_TOP_NOT_FOUND);

    bool installed_bottom = true;
    if(install_bottom)
        installed_bottom = this->install_splash_internal(bottom_splash_path, "splashbottom.bin", BOTTOM_SCREEN_SPLASH_SIZE, ERROR_TYPE_SPLASH_BOTTOM_SIZE_WRONG, ERROR_TYPE_SPLASH_BOTTOM_NOT_FOUND);

    if(installed_top && installed_bottom)
    {
        const auto& [config_buf, config_size] = file_to_buf(luma_config_path, have_luma_folder ? SD_CARD : CTRNAND);
        if(config_size)
        {
            if(config_buf[0xC] == 0)
                draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_SPLASH_LUMA_DISABLED);
        }
    }

    return RETURN_NONE;
}
