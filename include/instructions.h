/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2017 Alex Taber ("astronautlevel"), Dawid Eckert ("daedreth")
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

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "draw.h"
#include "colors.h"

Instructions_s normal_instructions[MODE_AMOUNT] = {
    {
        .info_line = NULL,
        .instructions = {
            {
                L"\uE000 Hold to install",
                L"\uE001 Queue shuffle theme"
            },
            {
                L"\uE002 Hold for more",
                L"\uE003 Preview theme"
            },
            {
                L"\uE004 Switch to splashes",
                L"\uE005 Scan QR code"
            },
            {
                L"Exit",
                L"Delete from SD"
            }
        }
    },
    {
        .info_line = NULL,
        .instructions = {
            {
                L"\uE000 Install splash",
                L"\uE001 Delete installed splash"
            },
            {
                L"\uE002 Hold for more",
                L"\uE003 Preview splash"
            },
            {
                L"\uE004 Switch to themes",
                L"\uE005 Scan QR code"
            },
            {
                L"Exit",
                L"Delete from SD"
            }
        }
    }
};

Instructions_s install_instructions = {
    .info_line = L"Release \uE000 to cancel or hold \uE006 and release \uE000 to install",
    .info_line_color = COLOR_WHITE,
    .instructions = {
        {
            L"\uE079 Normal install",
            L"\uE07A Shuffle install"
        },
        {
            L"\uE07B BGM-only install",
            L"\uE07C No-BGM install"
        },
        {
            NULL,
            NULL
        },
        {
            L"Exit",
            NULL
        }
    }
};

Instructions_s extra_instructions = {
    .info_line = L"Release \uE002 to cancel or hold \uE006 and release \uE002 to do stuff",
    .info_line_color = COLOR_WHITE,
    .instructions = {
        {
            L"\uE079 Jump in the list",
            L"\uE07A Reload broken icons"
        },
        {
            L"\uE07B Browse ThemePlaza",
            NULL
        },
        {
            NULL,
            NULL
        },
        {
            L"Exit",
            NULL
        }
    }
};

#endif