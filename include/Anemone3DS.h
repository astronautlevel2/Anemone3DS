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

#ifndef ANEMONE3DS_H
#define ANEMONE3DS_H

#include "common.h"
#include "menu.h"
// #include "audio.h"

class Anemone3DS
{
    public:
        Anemone3DS();
        ~Anemone3DS();

        void update();

        void scroll_thread_function();

    private:
        void init_services();
        void init_menus();
        void init_threads();

        void exit_threads();
        void exit_menus();
        void exit_services();

        void draw();

        void select_previous_menu();
        void select_next_menu();
        void select_menu(MenuType menu);
        void move_schedule_sleep();
        void set_menu();  // Actually sets the current menu
        void handle_action_return(MenuActionReturn action_result);

        bool installed_theme, running_from_hax;

        MenuBase* current_menu = nullptr;

        size_t selected_menu;
        std::array<std::unique_ptr<Menu>, MODES_AMOUNT> menus;

        // std::unique_ptr<RemoteMenu> browser_menu;

        bool sleep_scheduled = false;
        Thread scroll_thread;
        std::array<Thread, MODES_AMOUNT> install_check_thread;

        u32 old_time_limit;
};

#endif
