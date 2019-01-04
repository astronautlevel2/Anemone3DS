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

#ifndef SPLASH_MENU_H
#define SPLASH_MENU_H

#include "menu.h"

class SplashMenu : public Menu {
    public:
        SplashMenu();
        ~SplashMenu();

    private:
        MenuActionReturn change_to_action_mode();
        MenuActionReturn delete_installed_splash();
        bool install_splash_internal(FS_Path dest, const std::string& source, u32 wanted_size, ErrorType size_wrong_error, ErrorType not_found_error);
        MenuActionReturn install_splash(bool install_top, bool install_bottom);
};

#endif
