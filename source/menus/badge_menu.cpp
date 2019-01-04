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

BadgeMenu::BadgeMenu() : Menu("/Badges/", 3, TEXT_BADGE_MODE, TEXT_NOT_FOUND_SWITCH_TO_SPLASH, TEXT_NOT_FOUND_SWITCH_TO_THEME, 64, COLOR_BADGE_BG, true)
{
    const KeysActions normal_actions_down{
        {KEY_A, std::bind(&BadgeMenu::change_to_action_mode, this)},
        {KEY_B, std::bind(&Menu::change_to_qr_scanner, this)},
        {KEY_X, std::bind(&Menu::change_to_extra_mode, this)},
        {KEY_Y, std::bind(&MenuBase::load_preview, this)},
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

MenuActionReturn BadgeMenu::change_to_action_mode()
{
    if(!this->entries.size())
        return RETURN_NONE;

    const KeysActions badge_actions_down{
        {KEY_B, std::bind(&MenuBase::exit_mode_controls, this)},
        {KEY_X, std::bind(&Menu::delete_selected_entry, this)},
    };

    static const Instructions badge_actions_instructions{
        INSTRUCTIONS_NONE,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTION_X_FOR_DELETING_ENTRY,
        INSTRUCTIONS_NONE,
        INSTRUCTIONS_NONE,
        INSTRUCTIONS_NONE,
        INSTRUCTIONS_NONE,
        INSTRUCTIONS_NONE,
    };

    this->current_actions.push({badge_actions_down, {}});
    this->instructions_stack.push(&badge_actions_instructions);

    return RETURN_NONE;
}
