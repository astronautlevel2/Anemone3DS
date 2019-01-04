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

static constexpr size_t SHUFFLE_MAX = 10;

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
            entries_to_install.reserve(this->shuffle_count);
            for(auto& entry : this->entries)
            {
                if(entry->state == Entry::STATE_SHUFFLE || entry->state == Entry::STATE_SHUFFLE_NO_BGM)
                {
                    entries_to_install.push_back(entry.get());
                }
            }
        }
    }
    else
        entries_to_install.push_back(this->entries[this->selected_entry].get());

    return RETURN_INSTALLED_THEME;
}
