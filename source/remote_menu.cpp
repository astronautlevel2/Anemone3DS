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

#include "remote_menu.h"
#include "remote_entry.h"
#include "network.h"
#include "draw.h"

extern "C" {
#include <jansson.h>
}

void RemoteMenu::draw()
{
    if(this->in_preview())
    {
        this->preview->draw();
        return;
    }

    draw_basic_interface();

    size_t entries_count = this->entries.size();
    C2D_ImageTint selected_tint;
    C2D_PlainImageTint(&selected_tint, COLOR_WHITE, 0.5f);
    for(size_t i = 0; i < entries_count; i++)
    {
        size_t vertical_offset = 0;
        size_t horizontal_offset = i;
        vertical_offset = horizontal_offset/6;
        horizontal_offset %= 6;

        C2D_Image* icon= this->icons[vertical_offset][horizontal_offset]->image;
        C2D_DrawImageAt(*icon, 16 + horizontal_offset*this->icon_size, BARS_SIZE + vertical_offset*this->icon_size, 0.2f, i == this->selected_entry ? &selected_tint : nullptr);
    }

    std::string selected_entry_str = std::to_string(this->page) + "/" + std::to_string(this->page_count);
    float width, height;
    get_text_dimensions(selected_entry_str, &width, &height, 0.6f, 0.6f);
    float y = 240.0f - BARS_SIZE + (BARS_SIZE - height)/2.0f;
    draw_text(selected_entry_str, COLOR_WHITE, 316 - width, y, 0.2f, 0.6f, 0.6f);
    draw_text(TEXT_GENERAL, TEXT_THEMEPLAZA_PAGE, COLOR_WHITE, 176, y, 0.2f, 0.6f, 0.6f);

    switch_screen(GFX_TOP);
    get_text_dimensions(TEXT_GENERAL, this->mode_indicator_id, nullptr, &height, 0.6f, 0.6f);
    draw_text_centered(TEXT_GENERAL, this->mode_indicator_id, COLOR_WHITE, (BARS_SIZE - height)/2.0f - 1.0f, 0.2f, 0.6f, 0.6f);

    get_text_dimensions(TEXT_GENERAL, TEXT_INFO_INSTRUCTIONS_QUIT, nullptr, &height, 0.6f, 0.6f);
    draw_text_centered(TEXT_GENERAL, TEXT_INFO_INSTRUCTIONS_QUIT, COLOR_WHITE, 240.0f - BARS_SIZE + (BARS_SIZE - height)/2.0f - 1.0f, 0.2f, 0.6f, 0.6f);
    this->entries[this->selected_entry]->draw();
}

void RemoteMenu::calculate_new_scroll()
{

}

RemoteMenu::RemoteMenu(const std::string& loading_path, u32 background_color, TextID mode_indicator_id) : MenuBase(loading_path, 48, background_color, mode_indicator_id)
{
    this->entries.reserve(24);
    this->load_page();

    const KeysActions normal_actions_down{
        {KEY_A, std::bind(&RemoteMenu::download_entry, this)},
        {KEY_B, [](){ return RETURN_CHANGE_TO_LIST_MODE; }},
        {KEY_X, std::bind(&RemoteMenu::change_to_extra_mode, this)},
        {KEY_Y, std::bind(&MenuBase::load_preview, this)},
        {KEY_L, std::bind(&RemoteMenu::change_to_previous_page, this)},
        {KEY_R, std::bind(&RemoteMenu::change_to_next_page, this)},
        {KEY_DUP, std::bind(&RemoteMenu::select_up_entry, this)},
        {KEY_DLEFT, std::bind(&RemoteMenu::select_left_entry, this)},
        {KEY_DDOWN, std::bind(&RemoteMenu::select_down_entry, this)},
        {KEY_DRIGHT, std::bind(&RemoteMenu::select_right_entry, this)},
        {KEY_TOUCH, std::bind(&RemoteMenu::handle_touch, this)},
    };

    const KeysActions normal_actions_held{
        {KEY_CPAD_UP, std::bind(&RemoteMenu::select_up_entry_fast, this)},
        {KEY_CPAD_LEFT, std::bind(&RemoteMenu::select_left_entry_fast, this)},
        {KEY_CPAD_DOWN, std::bind(&RemoteMenu::select_down_entry_fast, this)},
        {KEY_CPAD_RIGHT, std::bind(&RemoteMenu::select_right_entry_fast, this)},
    };

    this->current_actions.push({normal_actions_down, normal_actions_held});
}

void RemoteMenu::load_page()
{
    InstallType loading_screen = INSTALLS_AMOUNT;
    if(this->mode_indicator_id == TEXT_THEMEPLAZA_THEME_MODE)
        loading_screen = INSTALL_LOADING_REMOTE_THEMES;
    else if(this->mode_indicator_id == TEXT_THEMEPLAZA_SPLASH_MODE)
        loading_screen = INSTALL_LOADING_REMOTE_SPLASHES;
    else
        svcBreak(USERBREAK_PANIC);

    draw_install(loading_screen);

    const auto& [json_buf, json_len] = download_data(get_page_url(this->page, this->sort, this->search), INSTALLS_AMOUNT);

    if(json_len)
    {
        json_error_t error;
        json_t* root = json_loadb(reinterpret_cast<const char*>(json_buf.get()), json_len, 0, &error);
        if(root)
        {
            const char* key;
            json_t* value;
            json_object_foreach(root, key, value)
            {
                if(json_is_integer(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_COUNT))
                {
                    this->page_count = json_integer_value(value);
                    DEBUG("page count: %zd\n", this->page_count);
                }
                else if(json_is_array(value) && !strcmp(key, THEMEPLAZA_JSON_PAGE_IDS))
                {
                    this->entries.clear();
                    size_t i = 0;
                    json_t* id = nullptr;
                    size_t ids_count = json_array_size(value);
                    DEBUG("page entries count: %zd\n", ids_count);
                    json_array_foreach(value, i, id)
                    {
                        draw_loading_bar(i, ids_count, loading_screen);
                        this->entries.push_back(std::make_unique<RemoteEntry>(static_cast<int>(json_integer_value(id))));
                        this->icons[i/6][i%6] = std::unique_ptr<EntryIcon>(this->entries.back()->load_icon());
                        DEBUG("entry %zd title, author: %s %s\n", i, this->entries.back()->title.c_str(), this->entries.back()->author.c_str());
                    }
                    draw_install(loading_screen);
                    this->ready = true;
                    this->selected_entry = 0;
                }
                else if(json_is_string(value) && !strcmp(key, THEMEPLAZA_JSON_ERROR_MESSAGE) && !strcmp(json_string_value(value), THEMEPLAZA_JSON_ERROR_MESSAGE_NOT_FOUND))
                {
                    draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_THEMEPLAZA_NO_RESULT);
                }
            }
        }
        else
            DEBUG("json error on line %d: %s\n", error.line, error.text);

        json_decref(root);
    }
    else
        draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_THEMEPLAZA_COULDNT_LOAD);
}

void RemoteMenu::select_up_entry_internal()
{
    if(this->entries.size() > 6)
    {
        if(this->selected_entry < 6)
            this->selected_entry = this->entries.size() - (6 - this->selected_entry) - 1;
        else
            this->selected_entry -= 6;
    }
}

void RemoteMenu::select_left_entry_internal()
{
    if(this->entries.size() > 1)
    {
        if(this->selected_entry % 6 == 0)
            this->selected_entry += 6 - 1;
        else
            --this->selected_entry;
    }
}

void RemoteMenu::select_down_entry_internal()
{
    if(this->entries.size() > 6)
    {
        if(this->selected_entry >= this->entries.size() - 6)
            this->selected_entry = 6 - (this->entries.size() - this->selected_entry);
        else
            this->selected_entry += 6;
    }
}

void RemoteMenu::select_right_entry_internal()
{
    if(this->entries.size() > 1)
    {
        if(this->selected_entry % 6 == 5)
            this->selected_entry -= 5;
        else
            ++this->selected_entry;
    }
}


MenuActionReturn RemoteMenu::select_up_entry()
{
    this->select_up_entry_internal();
    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::select_left_entry()
{
    this->select_left_entry_internal();
    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::select_down_entry()
{
    this->select_down_entry_internal();
    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::select_right_entry()
{
    this->select_right_entry_internal();
    return RETURN_NONE;
}


MenuActionReturn RemoteMenu::select_up_entry_fast()
{
    this->select_up_entry_internal();
    return RETURN_MOVE_SLEEP;
}

MenuActionReturn RemoteMenu::select_left_entry_fast()
{
    this->select_left_entry_internal();
    return RETURN_MOVE_SLEEP;
}

MenuActionReturn RemoteMenu::select_down_entry_fast()
{
    this->select_down_entry_internal();
    return RETURN_MOVE_SLEEP;
}

MenuActionReturn RemoteMenu::select_right_entry_fast()
{
    this->select_right_entry_internal();
    return RETURN_MOVE_SLEEP;
}


MenuActionReturn RemoteMenu::change_to_previous_page()
{
    if(this->page_count > 1)
    {
        if(this->page == 1)
            this->page = this->page_count;
        else
            --this->page;
        this->load_page();
    }
    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::change_to_next_page()
{
    if(this->page_count > 1)
    {
        if(this->page == this->page_count)
            this->page = 0;
        ++this->page;
        this->load_page();
    }
    return RETURN_NONE;
}


MenuActionReturn RemoteMenu::change_to_extra_mode()
{
    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::handle_touch()
{
    return RETURN_NONE;
}


MenuActionReturn RemoteMenu::download_entry()
{
    return RETURN_DOWNLOADED_FROM_TP;
}
