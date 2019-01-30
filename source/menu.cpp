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

#include "menu.h"

MenuBase::MenuBase()
{

}

MenuBase::MenuBase(const fs::path& loading_path, int icon_size, u32 background_color, TextID mode_indicator_id) : background_color(background_color), path(loading_path), icon_size(icon_size), mode_indicator_id(mode_indicator_id)
{

}

bool MenuBase::in_preview()
{
    return this->preview.get() != nullptr;
}

MenuActionReturn MenuBase::exit_mode_controls()
{
    if(this->current_actions.size() <= 1)
        return RETURN_NONE;

    this->current_actions.pop();
    if(!this->in_instructions && !this->in_preview())
        this->instructions_stack.pop();
    return RETURN_NONE;
}

MenuActionReturn MenuBase::load_preview()
{
    const KeysActions exit_preview_actions_down{
        {KEY_A, std::bind(&MenuBase::exit_preview, this)},
        {KEY_B, std::bind(&MenuBase::exit_preview, this)},
        {KEY_X, std::bind(&MenuBase::exit_preview, this)},
        {KEY_Y, std::bind(&MenuBase::exit_preview, this)},
        {KEY_L, std::bind(&MenuBase::exit_preview, this)},
        {KEY_R, std::bind(&MenuBase::exit_preview, this)},
        {KEY_TOUCH, std::bind(&MenuBase::exit_preview, this)},
    };

    if(this->entries.size())
    {
        const Entry* current_entry_ptr = this->entries[this->selected_entry].get();
        if(current_entry_ptr == this->selected_entry_for_previous_preview)
        {
            this->preview = std::move(this->previous_preview);
            this->preview->resume();
            this->current_actions.push({exit_preview_actions_down, {}, -1});
        }
        else
        {
            std::unique_ptr<PreviewImage> new_preview(current_entry_ptr->load_preview());
            if(new_preview && new_preview->ready)
            {
                this->preview = std::move(new_preview);
                this->preview->pause();  // Allow the previous preview to die
                this->previous_preview = nullptr;
                this->preview->resume();
                this->selected_entry_for_previous_preview = current_entry_ptr;

                this->current_actions.push({exit_preview_actions_down, {}, -1});
            }
        }
    }

    return RETURN_NONE;
}

MenuActionReturn MenuBase::exit_preview()
{
    this->exit_mode_controls();
    this->preview->pause();
    this->previous_preview = std::move(this->preview);
    return RETURN_NONE;
}

void MenuBase::change_selected_entry(int delta)
{
    if(abs(delta) >= this->entries.size())
        return;

    this->previous_selected_entry = this->selected_entry;

    ssize_t new_selected = this->selected_entry + delta;
    if(new_selected < 0)
    {
        this->selected_entry = this->entries.size() + new_selected;
    }
    else
    {
        this->selected_entry += delta;
    }

    this->change = delta;
    this->selected_entry %= this->entries.size();
}

MenuActionReturn MenuBase::exit_instructions()
{
    this->exit_mode_controls();
    this->in_instructions = false;
    return RETURN_NONE;
}

void MenuBase::toggle_instructions_mode()
{
    if(!this->entries.size())
        return;

    if(this->in_instructions)
    {
        this->exit_instructions();
    }
    else
    {
        const KeysActions instructions_actions_down{
            {KEY_L, std::bind(&MenuBase::set_instruction_screen_to_left, this)},
            {KEY_R, std::bind(&MenuBase::set_instruction_screen_to_right, this)},
            {KEY_B, std::bind(&MenuBase::exit_instructions, this)},
            {KEY_TOUCH, std::bind(&MenuBase::instructions_handle_touch, this)},
        };

        this->current_actions.push({instructions_actions_down, {}, -1});

        this->in_instructions = true;
        this->instruction_screen_right = false;
    }
}

void MenuBase::draw_instructions()
{
    switch_screen(GFX_BOTTOM);

    const Instructions& current_instructions = *this->instructions_stack.top();
    static constexpr float instructions_scale = 0.8f;

    float l_width, r_width, l_height, r_height;
    get_text_dimensions(TEXT_GENERAL, TEXT_L, &l_width, &l_height, 0.6f, 0.6f);
    get_text_dimensions(TEXT_GENERAL, TEXT_R, &r_width, &r_height, 0.6f, 0.6f);

    float y = BARS_SIZE;
    static const float step = (240.0f - BARS_SIZE*2.0f)/4.0f;
    float height;
    int start = this->instruction_screen_right ? 0 : 4;
    C2D_DrawRectSolid((this->instruction_screen_right ? 320.0f/2.0f : 0.0f) + 2.0f, 2.0f, 0.25f, 320.0f/2.0f - 2.0f*2.0f, BARS_SIZE - 2.0f*2.0f, COLOR_CURSOR);
    draw_text(TEXT_GENERAL, TEXT_L, this->instruction_screen_right ? COLOR_CURSOR : COLOR_BLACK, (320.0f/2.0f*0.5f) - l_width/2.0f, (BARS_SIZE - l_height)/2.0f, 0.5f, 0.6f, 0.6f);
    draw_text(TEXT_GENERAL, TEXT_R, this->instruction_screen_right ? COLOR_BLACK : COLOR_CURSOR, (320.0f/2.0f*1.5f) - r_width/2.0f, (BARS_SIZE - r_height)/2.0f, 0.5f, 0.6f, 0.6f);
    for(int i = start; i < start + 4; ++i, y += step)
    {
        InstructionType current_instruction_id = current_instructions[i];
        if(current_instruction_id == INSTRUCTIONS_NONE)
            continue;

        get_text_dimensions(TEXT_INSTRUCTIONS, current_instruction_id, nullptr, &height, instructions_scale, instructions_scale);
        draw_text_centered(TEXT_INSTRUCTIONS, current_instruction_id, COLOR_WHITE, y + (step - height)/2.0f, 0.5f, instructions_scale, instructions_scale);
    }

    C2D_DrawRectSolid(2.0f, 240.0f - BARS_SIZE + 2.0f, 0.25f, 320.0f - 2.0f*2.0f, BARS_SIZE - 2.0f*2.0f, COLOR_CURSOR);
    get_text_dimensions(TEXT_GENERAL, TEXT_INFO_EXIT_INSTRUCTIONS, nullptr, &height, 0.6f, 0.6f);
    draw_text_centered(TEXT_GENERAL, TEXT_INFO_EXIT_INSTRUCTIONS, COLOR_BLACK, 240.0f - BARS_SIZE + (BARS_SIZE - height)/2.0f, 0.5f, 0.6f, 0.6f);
}

MenuActionReturn MenuBase::set_instruction_screen_to_left()
{
    this->instruction_screen_right = false;
    return RETURN_NONE;
}

MenuActionReturn MenuBase::set_instruction_screen_to_right()
{
    this->instruction_screen_right = true;
    return RETURN_NONE;
}

MenuActionReturn MenuBase::instructions_handle_touch()
{
    touchPosition touch;
    hidTouchRead(&touch);
    u16 bars_size = static_cast<u16>(BARS_SIZE);
    if(touch.py < bars_size)
    {
        if(touch.px < 320/2)
        {
            this->set_instruction_screen_to_left();
        }
        else
        {
            this->set_instruction_screen_to_right();
        }
    }
    else if(touch.py >= 240 - bars_size)
    {
        this->exit_instructions();
    }
    return RETURN_NONE;
}

Menu::Menu(const fs::path& loading_path, size_t icons_per_screen, TextID mode_indicator_id, TextID previous_mode_indicator_id, TextID next_mode_indicator_id, int icon_size, u32 background_color, bool badge_menu):
MenuBase(loading_path, icon_size, background_color, mode_indicator_id),
icons_per_screen(icons_per_screen),
icons(icons_per_screen*3),
previous_mode_indicator_id(previous_mode_indicator_id),
next_mode_indicator_id(next_mode_indicator_id)
{
    draw_install(static_cast<InstallType>(mode_indicator_id));
    for(const fs::directory_entry& p : fs::directory_iterator(loading_path))
    {
        if(badge_menu)
        {
            if(p.is_regular_file() && p.path().extension() == ".png")
            {
                this->entries.push_back(std::move(std::make_unique<Entry>(p.path(), true)));
            }
        }
        else
        {
            if(p.is_regular_file() && p.path().extension() == ".zip")
            {
                this->entries.push_back(std::move(std::make_unique<Entry>(p.path(), true)));
            }
            else if(p.is_directory())
            {
                this->entries.push_back(std::move(std::make_unique<Entry>(p.path(), false)));
            }
        }
    }

    DEBUG("got %zd entries.\n", this->entries.size());
    this->entries.shrink_to_fit();
    this->sort(SORT_NAME);

    static const Instructions normal_actions_instructions{
        INSTRUCTION_A_FOR_ACTION_MODE,
        INSTRUCTION_B_FOR_QR_SCANNER,
        INSTRUCTION_X_FOR_EXTRA_MODE,
        INSTRUCTION_Y_FOR_ENTERING_BROWSER,
        INSTRUCTION_UP_TO_MOVE_UP,
        INSTRUCTION_LEFT_TO_MOVE_PAGE_UP,
        INSTRUCTION_DOWN_TO_MOVE_DOWN,
        INSTRUCTION_RIGHT_TO_MOVE_PAGE_DOWN,
    };
    this->instructions_stack.push(&normal_actions_instructions);
}

void Menu::load_icons()
{
    if(this->entries.size() <= this->icons.size())
    {
        size_t i = 0;
        for(const auto& entry : this->entries)
        {
            draw_loading_bar(i, this->entries.size(), INSTALL_LOADING_ICONS);
            this->icons[i++] = std::move(std::unique_ptr<EntryIcon>(entry->load_icon()));
        }
    }
    else
    {
        size_t entries_count = this->entries.size();
        for(size_t i = 0; i < this->icons_per_screen; i++)
        {
            draw_loading_bar(i, this->icons_per_screen, INSTALL_LOADING_ICONS);
            int above_pos = this->scroll + i - this->icons_per_screen;
            if(above_pos < 0)
                above_pos = entries_count + above_pos;
            int visible_pos = this->scroll + i;
            int under_pos = this->scroll + i + this->icons_per_screen;
            under_pos %= entries_count;
            this->icons[i] = std::move(std::unique_ptr<EntryIcon>(this->entries[above_pos]->load_icon()));
            this->icons[i + this->icons_per_screen] = std::move(std::unique_ptr<EntryIcon>(this->entries[visible_pos]->load_icon()));
            this->icons[i + this->icons_per_screen*2] = std::move(std::unique_ptr<EntryIcon>(this->entries[under_pos]->load_icon()));
        }
    }
}

void Menu::scroll_icons(const Handle* scroll_ready_to_draw_event)
{
    if(this->entries.size() <= this->icons.size())
    {
        this->scroll = this->new_scroll;
        this->previous_selected_entry = this->selected_entry;

        if(scroll_ready_to_draw_event)
            svcSignalEvent(*scroll_ready_to_draw_event);

        return;
    }

    #define SIGN(x) (x > 0 ? 1 : ((x < 0) ? -1 : 0))
    int delta = this->scroll - this->new_scroll;
    if(abs(delta) > this->icons_per_screen)
        delta = -SIGN(delta) * (this->entries.size() - abs(delta));

    this->scroll = this->new_scroll;
    this->previous_selected_entry = this->selected_entry;

    if(delta > 0)  // scroll went up -> rotate right to make icons from above visible
    {
        auto begin = this->icons.rbegin();
        auto step = this->icons.rbegin() + delta;
        auto end = this->icons.rend();

        std::rotate(begin, step, end);
    }
    else  // scroll went down -> rotate left to make icons from under visible
    {
        auto begin = this->icons.begin();
        auto step = this->icons.begin() + (-delta);
        auto end = this->icons.end();

        std::rotate(begin, step, end);
    }

    if(scroll_ready_to_draw_event && abs(delta) <= this->icons_per_screen)
    {
        svcSignalEvent(*scroll_ready_to_draw_event);
        // even though it shouldn't happen anyway, ensure the event can't be signaled a second time in the loop
        scroll_ready_to_draw_event = nullptr;
    }

    const int step = -SIGN(delta);
    const int icon_pos = delta < 0 ? static_cast<int>(this->icons.size()) : -1;
    int entry_pos;
    if(delta < 0)
    {
        entry_pos = this->scroll + this->icons_per_screen*2 + delta;
        if(static_cast<size_t>(entry_pos) >= this->entries.size())
        {
            entry_pos %= this->entries.size();
        }
    }
    else
    {
        entry_pos = this->scroll - this->icons_per_screen + delta - 1;
        if(entry_pos < 0)
        {
            entry_pos += this->entries.size();
        }
    }

    // DEBUG("delta: %d, entry_pos: %d, icon_pos: %d\n", delta, entry_pos, icon_pos);
    for(int i = delta; i != 0; i += step)
    {
        // In certain conditions (scrolling a page near the ends with scroll not being minimal or maximum), you can end up with more than 1 page to reload
        // So only allow it to draw once you've reached the end of the visible page and enter the invisible one, to make it seem faster
        if(scroll_ready_to_draw_event && abs(delta - i) == this->icons_per_screen)
        {
            svcSignalEvent(*scroll_ready_to_draw_event);
            // even though the condition should only be true once, ensure the event can't be signaled a second time in the loop
            scroll_ready_to_draw_event = nullptr;
        }

        this->icons[icon_pos + i].reset(this->entries[entry_pos]->load_icon());

        entry_pos += step;
        if(entry_pos == -1)
        {
            entry_pos = this->entries.size() - 1;
        }
        else if(static_cast<size_t>(entry_pos) == this->entries.size())
        {
            entry_pos = 0;
        }
    }

    #undef SIGN
}

bool Menu::needs_thread()
{
    return this->entries.size() > this->icons.size();
}

void Menu::draw()
{
    if(this->in_preview())
    {
        this->preview->draw();
        return;
    }

    draw_basic_interface();

    float height;
    size_t entries_count = this->entries.size();
    if(entries_count)
    {
        float y = BARS_SIZE;
        for(size_t i = 0; i < this->icons_per_screen; i++, y += this->icon_size)
        {
            size_t actual_i = i + this->scroll;
            if(actual_i >= entries_count)
                break;

            C2D_ImageTint indicator_tint;
            u32 text_color = COLOR_WHITE;
            if(actual_i == this->selected_entry)
            {
                C2D_PlainImageTint(&indicator_tint, (text_color = COLOR_BLACK), 1.0f);
                C2D_DrawRectSolid(0.0f, y, 0.1f, 320.0f, this->icon_size, COLOR_CURSOR);
            }
            else
            {
                C2D_PlainImageTint(&indicator_tint, text_color, 1.0f);
            }

            const auto& current_entry = this->entries[actual_i];
            if(!current_entry->color)
            {
                C2D_Image * image = NULL;
                if(entries_count <= this->icons.size())
                    image = this->icons[actual_i]->image;
                else
                    image = this->icons[i + this->icons_per_screen]->image;
                C2D_DrawImageAt(*image, 0.0f, y, 0.2f);
            }
            else
            {
                C2D_DrawRectSolid(0.0f, y, 0.2f, this->icon_size, this->icon_size, current_entry->color);
            }

            get_text_dimensions(current_entry->title, nullptr, &height, 0.55f, 0.55f);
            draw_text(current_entry->title, text_color, this->icon_size + 6, y + (this->icon_size - height)/2.0f, 0.2f, 0.55f, 0.55f);

            if(current_entry->state == Entry::STATE_SHUFFLE)
            {
                draw_image(sprites_shuffle_idx, 320.0f - 24.0f - 4.0f, y, 0.3f, &indicator_tint);
            }
            else if(current_entry->state == Entry::STATE_SHUFFLE_NO_BGM)
            {
                draw_image(sprites_shuffle_no_bgm_idx, 320.0f - 24.0f - 4.0f, y, 0.3f, &indicator_tint);
            }
            else if(current_entry->state == Entry::STATE_MULTI)
            {
                draw_image(sprites_multi_idx, 320.0f - 24.0f - 4.0f, y, 0.3f, &indicator_tint);
            }
        }

        // Draw entries list
        std::string selected_entry_str = std::to_string(this->selected_entry + 1) + "/" + std::to_string(entries_count);
        float x = 316;
        float width;
        get_text_dimensions(selected_entry_str, &width, &height, 0.6f, 0.6f);
        x -= width;
        y = 240.0f - BARS_SIZE + (BARS_SIZE - height)/2.0f;
        draw_text(selected_entry_str, COLOR_WHITE, x, y, 0.2f, 0.6f, 0.6f);
        draw_text(TEXT_GENERAL, this->entries.size() > 999 ? TEXT_MENU_SELECTED_SHORT : TEXT_MENU_SELECTED, COLOR_WHITE, 176, y, 0.2f, 0.6f, 0.6f);
    }

    switch_screen(GFX_TOP);
    get_text_dimensions(TEXT_GENERAL, this->mode_indicator_id, nullptr, &height, 0.6f, 0.6f);
    draw_text_centered(TEXT_GENERAL, this->mode_indicator_id, COLOR_WHITE, (BARS_SIZE - height)/2.0f - 1.0f, 0.2f, 0.6f, 0.6f);

    if(entries_count)
    {
        get_text_dimensions(TEXT_GENERAL, TEXT_INFO_INSTRUCTIONS_QUIT, nullptr, &height, 0.6f, 0.6f);
        draw_text_centered(TEXT_GENERAL, TEXT_INFO_INSTRUCTIONS_QUIT, COLOR_WHITE, 240.0f - BARS_SIZE + (BARS_SIZE - height)/2.0f - 1.0f, 0.2f, 0.6f, 0.6f);
        this->entries[this->selected_entry]->draw();
    }
    else
    {
        static constexpr float not_found_scale = 0.7f;
        static constexpr float y_step = 2.0f + 30.0f*not_found_scale;
        static constexpr float steps_amount = 6;
        static constexpr float y_start = (240.0f - steps_amount*y_step)/2.0f;

        static const float space_width = get_text_width(TEXT_GENERAL, TEXT_SPACE, not_found_scale);
        static const float or_width = get_text_width(TEXT_GENERAL, TEXT_NOT_FOUND_OR, not_found_scale);
        static const float l_width = get_text_width(TEXT_GENERAL, TEXT_L, not_found_scale);
        static const float r_width = get_text_width(TEXT_GENERAL, TEXT_R, not_found_scale);
        static const float to_switch_to_width = get_text_width(TEXT_GENERAL, TEXT_NOT_FOUND_TO_SWITCH_TO, not_found_scale);

        float y = y_start;
        draw_text_centered(TEXT_GENERAL, static_cast<TextID>(this->mode_indicator_id + MODES_AMOUNT), COLOR_WARNING, y, 0.1f, not_found_scale, not_found_scale);

        y += y_step;
        draw_text_centered(TEXT_GENERAL, TEXT_NOT_FOUND_PRESS_FOR_QR, COLOR_WARNING, y, 0.1f, not_found_scale, not_found_scale);
        y += y_step;
        draw_text_centered(TEXT_GENERAL, TEXT_NOT_FOUND_OR_BROWSER, COLOR_WARNING, y, 0.1f, not_found_scale, not_found_scale);

        float total_width, x;

        y += y_step;
        total_width = 0.0f,
        total_width += or_width;
        total_width += space_width;
        total_width += l_width;
        total_width += space_width;
        total_width += to_switch_to_width;
        total_width += space_width;
        total_width += get_text_width(TEXT_GENERAL, this->previous_mode_indicator_id, not_found_scale);
        x = (400.0f - total_width)/2.0f;

        draw_text(TEXT_GENERAL, TEXT_NOT_FOUND_OR, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);
        x += or_width + space_width;
        draw_text(TEXT_GENERAL, TEXT_L, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);
        x += l_width + space_width;
        draw_text(TEXT_GENERAL, TEXT_NOT_FOUND_TO_SWITCH_TO, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);
        x += to_switch_to_width + space_width;
        draw_text(TEXT_GENERAL, this->previous_mode_indicator_id, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);

        y += y_step;
        total_width = 0.0f,
        total_width += or_width;
        total_width += space_width;
        total_width += r_width;
        total_width += space_width;
        total_width += to_switch_to_width;
        total_width += space_width;
        total_width += get_text_width(TEXT_GENERAL, this->next_mode_indicator_id, not_found_scale);
        x = (400.0f - total_width)/2.0f;

        draw_text(TEXT_GENERAL, TEXT_NOT_FOUND_OR, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);
        x += or_width + space_width;
        draw_text(TEXT_GENERAL, TEXT_R, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);
        x += r_width + space_width;
        draw_text(TEXT_GENERAL, TEXT_NOT_FOUND_TO_SWITCH_TO, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);
        x += to_switch_to_width + space_width;
        draw_text(TEXT_GENERAL, this->next_mode_indicator_id, COLOR_WARNING, x, y, 0.1f, not_found_scale, not_found_scale);

        y += y_step;
        draw_text_centered(TEXT_GENERAL, TEXT_NOT_FOUND_START_TO_QUIT, COLOR_WARNING, y, 0.1f, not_found_scale, not_found_scale);
    }
}

void Menu::calculate_new_scroll()
{
    if(this->previous_selected_entry != this->selected_entry && this->entries.size() > this->icons_per_screen)
    {
        const size_t max_scroll = this->entries.size() - this->icons_per_screen;
        this->new_scroll = this->scroll;

        if(this->entries.size() >= this->icons.size())
        {
            if(this->previous_selected_entry < this->icons_per_screen && this->selected_entry >= max_scroll)
            {
                this->new_scroll = max_scroll;
            }
            else if(this->previous_selected_entry >= max_scroll && this->selected_entry < this->icons_per_screen)
            {
                this->new_scroll = 0;
            }
            else if(this->selected_entry >= this->scroll + this->icons_per_screen || this->selected_entry < this->scroll)
            {
                this->new_scroll = this->scroll + this->change;
            }
        }
        else  // No page scrolling allowed, easier
        {
            if(this->selected_entry < this->scroll && this->previous_selected_entry >= 1 && this->selected_entry == this->previous_selected_entry - 1)
            {
                this->new_scroll = this->scroll - 1;
            }
            else if(this->selected_entry == this->scroll + this->icons_per_screen && this->selected_entry == this->previous_selected_entry + 1)
            {
                this->new_scroll = this->scroll + 1;
            }
            else if(this->selected_entry == this->entries.size() - 1 && this->previous_selected_entry == 0)
            {
                this->new_scroll = max_scroll;
            }
            else if(this->selected_entry == 0 && this->previous_selected_entry == this->entries.size() - 1)
            {
                this->new_scroll = 0;
            }
        }


        if(this->new_scroll > max_scroll)
            this->new_scroll = max_scroll;

        this->should_scroll = this->new_scroll != this->scroll;
    }
}

MenuActionReturn Menu::sort(SortType sort_type)
{
    if(!this->entries.size())
        return RETURN_NONE;

    if(this->sort_type == sort_type)
        return RETURN_NONE;

    this->sort_type = sort_type;
    DEBUG("Sorting menu with sort mode %d\n", sort_type);
    std::array<std::function<bool(const std::unique_ptr<Entry>&, const std::unique_ptr<Entry>&)>, SORTS_AMOUNT> sorting_functions{
        [](const std::unique_ptr<Entry>& a, const std::unique_ptr<Entry>& b) { return a->title < b->title; },
        [](const std::unique_ptr<Entry>& a, const std::unique_ptr<Entry>& b) { return a->author < b->author; },
        [](const std::unique_ptr<Entry>& a, const std::unique_ptr<Entry>& b) { return a->path < b->path; },
    };
    draw_install(INSTALL_SORTING);
    std::sort(this->entries.begin(), this->entries.end(), sorting_functions[sort_type]);
    this->previous_selected_entry = this->selected_entry = this->scroll = 0;
    this->load_icons();

    this->exit_mode_controls();
    return RETURN_NONE;
}

MenuActionReturn Menu::select_previous_entry()
{
    this->change_selected_entry(-1);
    return RETURN_NONE;
}

MenuActionReturn Menu::select_previous_page()
{
    if(this->entries.size() >= this->icons.size())
        this->change_selected_entry(-this->icons_per_screen);
    return RETURN_NONE;
}

MenuActionReturn Menu::select_next_entry()
{
    this->change_selected_entry(1);
    return RETURN_NONE;
}

MenuActionReturn Menu::select_next_page()
{
    if(this->entries.size() >= this->icons.size())
        this->change_selected_entry(this->icons_per_screen);
    return RETURN_NONE;
}


MenuActionReturn Menu::select_previous_entry_fast()
{
    this->change_selected_entry(-1);
    return RETURN_MOVE_SLEEP;
}

MenuActionReturn Menu::select_previous_page_fast()
{
    if(this->entries.size() < this->icons.size())
        return RETURN_NONE;

    this->change_selected_entry(-this->icons_per_screen);
    return RETURN_MOVE_SLEEP;
}

MenuActionReturn Menu::select_next_entry_fast()
{
    this->change_selected_entry(1);
    return RETURN_MOVE_SLEEP;
}

MenuActionReturn Menu::select_next_page_fast()
{
    if(this->entries.size() < this->icons.size())
        return RETURN_NONE;

    this->change_selected_entry(this->icons_per_screen);
    return RETURN_MOVE_SLEEP;
}


MenuActionReturn Menu::change_to_previous_mode()
{
    return RETURN_CHANGE_TO_PREVIOUS_MODE;
}

MenuActionReturn Menu::change_to_next_mode()
{
    return RETURN_CHANGE_TO_NEXT_MODE;
}

MenuActionReturn Menu::change_to_extra_mode()
{
    if(!this->entries.size())
        return RETURN_NONE;

    const KeysActions extra_actions_down{
        {KEY_A, std::bind(&Menu::jump_in_selection, this)},
        {KEY_B, std::bind(&MenuBase::exit_mode_controls, this)},
        {KEY_DUP, std::bind(&Menu::sort, this, SORT_AUTHOR)},
        {KEY_DLEFT, std::bind(&Menu::sort, this, SORT_FILENAME)},
        {KEY_DRIGHT, std::bind(&Menu::sort, this, SORT_NAME)},
    };

    static const Instructions extra_actions_instructions{
        INSTRUCTION_A_FOR_JUMPING,
        INSTRUCTION_B_FOR_GOING_BACK,
        INSTRUCTIONS_NONE,
        INSTRUCTIONS_NONE,
        INSTRUCTION_UP_FOR_SORTING_AUTHOR,
        INSTRUCTION_LEFT_FOR_SORTING_FILENAME,
        INSTRUCTIONS_NONE,
        INSTRUCTION_RIGHT_FOR_SORTING_NAME,
    };

    this->current_actions.push({extra_actions_down, {}, sprites_extra_mode_idx});
    this->instructions_stack.push(&extra_actions_instructions);

    return RETURN_NONE;
}

MenuActionReturn Menu::change_to_qr_scanner()
{
    return RETURN_CHANGE_TO_QR_MODE;
}

static SwkbdCallbackResult jump_entry_menu_callback(void* entries_count, const char** ppMessage, const char* text, size_t textlen)
{
    size_t typed_value = strtoul(text, nullptr, 10);
    if(typed_value > *reinterpret_cast<size_t*>(entries_count))
    {
        *ppMessage = keyboard_shown_text[KEYBOARD_GENERAL_TOO_HIGH];
        return SWKBD_CALLBACK_CONTINUE;
    }
    else if(typed_value == 0)
    {
        *ppMessage = keyboard_shown_text[KEYBOARD_GENERAL_NON_ZERO];
        return SWKBD_CALLBACK_CONTINUE;
    }
    return SWKBD_CALLBACK_OK;
}

MenuActionReturn Menu::jump_in_selection()
{
    if(!this->entries.size())
        return RETURN_NONE;

    SwkbdState swkbd;

    size_t entries_count = this->entries.size();
    const std::string entries_count_str = std::to_string(entries_count);
    swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 2, entries_count_str.length());

    const std::string selected_entry_str = std::to_string(this->selected_entry + 1);
    char* selected_entry_char = strdup(selected_entry_str.c_str());
    swkbdSetInitialText(&swkbd, selected_entry_char);

    swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, keyboard_shown_text[KEYBOARD_BUTTON_CANCEL], false);
    swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, keyboard_shown_text[KEYBOARD_BUTTON_JUMP], true);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, entries_count_str.length());

    swkbdSetFilterCallback(&swkbd, jump_entry_menu_callback, &entries_count);

    char numbuf[8] = {0};
    SwkbdButton button = swkbdInputText(&swkbd, numbuf, sizeof(numbuf));
    if(button == SWKBD_BUTTON_CONFIRM)
    {
        this->selected_entry = strtoul(numbuf, nullptr, 10) - 1;
        if(this->selected_entry < this->scroll || this->selected_entry >= this->scroll + this->icons_per_screen)
        {
            this->scroll = this->selected_entry;
            const size_t max_scroll = this->entries.size() - this->icons_per_screen;
            if(this->scroll > max_scroll)
                this->scroll = max_scroll;

            if(this->entries.size() > this->icons.size())
                this->load_icons();
        }
    }
    free(selected_entry_char);

    this->exit_mode_controls();
    return RETURN_NONE;
}

MenuActionReturn Menu::handle_touch()
{
    touchPosition touch;
    hidTouchRead(&touch);
    u16 bars_size = static_cast<u16>(BARS_SIZE);
    if(touch.py < bars_size)
    {
        u16 x = 8;
        if(touch.px >= x && touch.px < x + bars_size)
            return RETURN_CHANGE_TO_PREVIOUS_MODE;

        for(MenuType i = MODE_THEMES; i < MODES_AMOUNT; i = static_cast<MenuType>(i + 1))
        {
            x += bars_size;
            if(touch.px >= x && touch.px < x + bars_size)
                return static_cast<MenuActionReturn>(RETURN_CHANGE_TO_THEME_MODE + i);
        }

        x += bars_size;
        if(touch.px >= x && touch.px < x + bars_size)
            return RETURN_CHANGE_TO_NEXT_MODE;
    }
    else if(touch.py >= 240 - bars_size)
    {
        if(touch.px >= 176)
        {
            this->jump_in_selection();
        }
    }
    else
    {
        if(!this->entries.size())
            return RETURN_NONE;

        u16 y = touch.py - bars_size;
        this->previous_selected_entry = this->selected_entry = this->scroll + (y / this->icon_size);
    }
    return RETURN_NONE;
}

MenuActionReturn Menu::delete_selected_entry()
{
    if(!this->entries.size())
        return RETURN_NONE;

    draw_install(INSTALL_DELETING);
    if(this->scroll > 0 && this->entries.size() > this->icons_per_screen && this->scroll == this->entries.size() - this->icons_per_screen)
        --this->scroll;

    this->entries[this->selected_entry]->delete_entry();
    this->entries.erase(this->entries.begin() + this->selected_entry);

    if(this->entries.size())
    {
        if(this->selected_entry == this->entries.size())
            this->selected_entry = this->previous_selected_entry = this->selected_entry - 1;

        this->load_icons();
    }

    this->exit_mode_controls();
    return RETURN_NONE;
}

MenuActionReturn Menu::change_to_browser_mode()
{
    return RETURN_CHANGE_TO_BROWSER_MODE;
}
