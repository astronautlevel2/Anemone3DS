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

#ifndef REMOTE_MENU_H
#define REMOTE_MENU_H

#include "common.h"
#include "menu.h"
#include "icons.h"

enum RemoteSortType {
    SORT_NEWEST,
    SORT_DOWNLOAD_COUNT,
    SORT_LIKE_COUNT,
};

// Screw badges, get them from QRs
class RemoteMenu : public MenuBase {
    public:
        void draw();
        void calculate_new_scroll();

        bool ready = false;

    protected:
        RemoteMenu(const std::string& loading_path, u32 background_color, TextID mode_indicator_id);
        void load_page();
        std::array<std::array<std::unique_ptr<EntryIcon>, 6>, 4> icons;
        size_t page = 1;
        size_t page_count;
        std::string search = "";
        RemoteSortType sort = SORT_NEWEST;

        void select_up_entry_internal();
        void select_left_entry_internal();
        void select_down_entry_internal();
        void select_right_entry_internal();

        MenuActionReturn select_up_entry();
        MenuActionReturn select_left_entry();
        MenuActionReturn select_down_entry();
        MenuActionReturn select_right_entry();

        MenuActionReturn select_up_entry_fast();
        MenuActionReturn select_left_entry_fast();
        MenuActionReturn select_down_entry_fast();
        MenuActionReturn select_right_entry_fast();

        MenuActionReturn change_to_previous_page();
        MenuActionReturn change_to_next_page();

        MenuActionReturn change_to_extra_mode();
        MenuActionReturn handle_touch();

        MenuActionReturn open_search();
        MenuActionReturn open_page_jump();

        MenuActionReturn change_sort(RemoteSortType new_sort);

        MenuActionReturn download_entry();
};

#endif
