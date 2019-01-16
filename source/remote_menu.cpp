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

    float height;
    get_text_dimensions(TEXT_GENERAL, TEXT_THEMEPLAZA_SEARCH, nullptr, &height, 0.6f, 0.6f);
    draw_text(TEXT_GENERAL, TEXT_THEMEPLAZA_SEARCH, COLOR_WHITE, 4, (BARS_SIZE - height)/2.0f, 0.2f, 0.6f, 0.6f);

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
    float width;
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

    static const Instructions normal_actions_instructions{
        INSTRUCTION_A_FOR_DOWNLOADING,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTION_X_FOR_EXTRA_MODE,
        INSTRUCTION_Y_FOR_PREVIEW,
        INSTRUCTION_UP_TO_MOVE_UP,
        INSTRUCTION_LEFT_TO_MOVE_LEFT,
        INSTRUCTION_DOWN_TO_MOVE_DOWN,
        INSTRUCTION_RIGHT_TO_MOVE_RIGHT,
    };

    this->instructions_stack.push(&normal_actions_instructions);
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
        {
            if(this->entries.size() % 6)
            {
                if(this->selected_entry >= this->entries.size() % 6)
                    this->selected_entry = this->entries.size() - 6 + this->selected_entry - (this->entries.size() % 6);
                else
                    this->selected_entry = this->entries.size() - ((this->entries.size() % 6) - this->selected_entry);
            }
            else
                this->selected_entry = this->entries.size() - (6 - this->selected_entry);
        }
        else
            this->selected_entry -= 6;
    }
}

void RemoteMenu::select_left_entry_internal()
{
    if(this->entries.size() > 1)
    {
        // If you're on the last row and it's not full, add the row width if you're at the start
        if(this->selected_entry == this->entries.size() - (this->entries.size() % 6))
            this->selected_entry += (this->entries.size() % 6 ? (this->entries.size() % 6) : 6) - 1;
        // otherwise, if you're not on the last row, it has to be a full row, so just add a full row's width if you're at the stat
        else if(this->selected_entry % 6 == 0)
            this->selected_entry += 6 - 1;
        // finally, if you're not at the start of any row, just move left
        else
            --this->selected_entry;
    }
}

void RemoteMenu::select_down_entry_internal()
{
    if(this->entries.size() > 6)
    {
        if(this->selected_entry >= this->entries.size() - 6)
            this->selected_entry %= 6;
        else
            this->selected_entry += 6;
    }
}

void RemoteMenu::select_right_entry_internal()
{
    if(this->entries.size() > 1)
    {
        // If you're on the last row and it's not full, subtract the row width if you're at the end
        if((this->entries.size() < 6 && this->selected_entry == this->entries.size() - 1) || (this->selected_entry >= this->entries.size() - (this->entries.size() % 6) && this->selected_entry % 6 == 6 - (this->entries.size() % 6) - 1))
            this->selected_entry -= 6 - (this->entries.size() % 6) - 1;
        // otherwise, if you're not on the last row, it has to be a full row, so just subtract a full row's width if you're at the end
        else if(this->selected_entry < this->entries.size() - (this->entries.size() % 6) && this->selected_entry % 6 == 6 - 1)
            this->selected_entry -= 6 - 1;
        // finally, if you're not at the end of any row, just move right
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
    const KeysActions extra_actions_down{
        {KEY_A, std::bind(&RemoteMenu::open_search, this)},
        {KEY_B, std::bind(&MenuBase::exit_mode_controls, this)},
        {KEY_X, std::bind(&RemoteMenu::open_page_jump, this)},
        {KEY_DUP, std::bind(&RemoteMenu::change_sort, this, SORT_DOWNLOAD_COUNT)},
        {KEY_DLEFT, std::bind(&RemoteMenu::change_sort, this, SORT_NEWEST)},
        {KEY_DRIGHT, std::bind(&RemoteMenu::change_sort, this, SORT_LIKE_COUNT)},
    };

    const KeysActions extra_actions_held{};

    this->current_actions.push({extra_actions_down, extra_actions_held});

    static const Instructions extra_actions_instructions{
        INSTRUCTION_A_FOR_SEARCHING,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTION_X_FOR_JUMP_PAGE,
        INSTRUCTIONS_NONE,
        INSTRUCTION_UP_TO_SORT_DOWNLOAD_COUNT,
        INSTRUCTION_LEFT_TO_SORT_NEWEST,
        INSTRUCTIONS_NONE,
        INSTRUCTION_RIGHT_TO_SORT_LIKE_COUNT,
    };

    this->instructions_stack.push(&extra_actions_instructions);

    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::handle_touch()
{
    touchPosition touch;
    hidTouchRead(&touch);
    u16 bars_size = static_cast<u16>(BARS_SIZE);
    if(touch.py < bars_size)
    {
        static const float width = get_text_width(TEXT_GENERAL, TEXT_THEMEPLAZA_SEARCH, 0.6f);
        if(touch.px >= 4 && touch.px < 4 + static_cast<u16>(width))
            this->open_search();
    }
    else if(touch.py >= 240 - bars_size)
    {
        if(touch.px > 176)
            this->open_page_jump();
    }
    else
    {
        if(touch.px >= 16 && touch.px < 320 - 16)
        {
            for(int y = 0; y < 4; y++)
            {
                for(int x = 0; x < 6; x++)
                {
                    u16 actual_y = bars_size + y*this->icon_size;
                    u16 actual_x = 16 + x*this->icon_size;
                    if(touch.py >= actual_y && touch.py < actual_y + this->icon_size && touch.px >= actual_x && touch.px < actual_x + this->icon_size)
                    {
                        size_t new_selected = y*6 + x;
                        if(new_selected < this->entries.size())
                            this->selected_entry = new_selected;
                        return RETURN_NONE;
                    }
                }
            }
        }
    }

    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::open_search()
{
    static constexpr int max_chars = 256;
    char search_value[max_chars + 1] = {0};

    SwkbdState swkbd;

    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, max_chars);
    swkbdSetHintText(&swkbd, keyboard_shown_text[KEYBOARD_HINT_ENTER_SEARCH]);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, keyboard_shown_text[KEYBOARD_BUTTON_CANCEL], false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, keyboard_shown_text[KEYBOARD_BUTTON_SEARCH], true);
    swkbdSetValidation(&swkbd, SWKBD_NOTBLANK, 0, max_chars);

    SwkbdButton button = swkbdInputText(&swkbd, search_value, max_chars);
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        std::string search(search_value, max_chars);
        if(search != this->search)
        {
            std::string previous_search = this->search;
            size_t previous_page = this->page;

            this->search = search;
            this->page = 1;
            this->ready = false;
            this->load_page();
            
            if(!this->ready)
            {
                this->search = previous_search;
                this->page = previous_page;
                this->ready = true;
            }
        }
    }

    this->exit_mode_controls();
    return RETURN_NONE;
}

static SwkbdCallbackResult jump_page_menu_callback(void* page_count, const char** ppMessage, const char* text, size_t textlen)
{
    size_t typed_value = strtoul(text, nullptr, 10);
    if(typed_value > *static_cast<size_t*>(page_count))
    {
        *ppMessage = keyboard_shown_text[KEYBOARD_THEMEPLAZA_TOO_HIGH];
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if(typed_value == 0)
    {
        *ppMessage = keyboard_shown_text[KEYBOARD_THEMEPLAZA_NON_ZERO];
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

MenuActionReturn RemoteMenu::open_page_jump()
{
    SwkbdState swkbd;

    size_t page_count = this->page_count;
    const std::string page_count_str = std::to_string(page_count);
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, page_count_str.length());

    const std::string current_page_str = std::to_string(this->page);
    char* current_page_char = strdup(current_page_str.c_str());
    swkbdSetInitialText(&swkbd, current_page_char);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, keyboard_shown_text[KEYBOARD_BUTTON_CANCEL], false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, keyboard_shown_text[KEYBOARD_BUTTON_JUMP], true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, page_count_str.length());

    swkbdSetFilterCallback(&swkbd, jump_page_menu_callback, &page_count);

    char numbuf[8] = {0};
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        size_t new_page = strtoul(numbuf, nullptr, 10);
        if(this->page != new_page)
        {
            this->page = new_page;
            this->load_page();
        }
    }
    free(current_page_char);

    this->exit_mode_controls();
    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::change_sort(RemoteSortType new_sort)
{
    if(this->sort != new_sort)
    {
        this->sort = new_sort;
        this->page = 1;
        this->load_page();
    }

    return RETURN_NONE;
}

MenuActionReturn RemoteMenu::download_entry()
{
    return RETURN_DOWNLOADED_FROM_TP;
}
