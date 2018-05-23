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

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "draw.h"
#include "colors.h"

Instructions_s normal_instructions[MODE_AMOUNT] = {
    {
        .info_line = NULL,
        .instructions = {
            {
                "\uE000 Hold to install",
                "\uE001 Queue shuffle theme"
            },
            {
                "\uE002 Hold for more",
                "\uE003 Preview theme"
            },
            {
                "\uE004 Switch to splashes",
                "\uE005 Scan QR code"
            },
            {
                "Exit",
                "Delete from SD"
            }
        }
    },
    {
        .info_line = NULL,
        .instructions = {
            {
                "\uE000 Install splash",
                "\uE001 Delete installed splash"
            },
            {
                "\uE002 Hold for more",
                "\uE003 Preview splash"
            },
            {
                "\uE004 Switch to themes",
                "\uE005 Scan QR code"
            },
            {
                "Exit",
                "Delete from SD"
            }
        }
    }
};

Instructions_s install_instructions = {
    .info_line = "Release \uE000 to cancel or hold \uE006 and release \uE000 to install",
    .instructions = {
        {
            "\uE079 Normal install",
            "\uE07A Shuffle install"
        },
        {
            "\uE07B BGM-only install",
            "\uE07C No-BGM install"
        },
        {
            NULL,
            NULL
        },
        {
            "Exit",
            NULL
        }
    }
};

Instructions_s extra_instructions[3] = {
    {
        .info_line = "Release \uE002 to cancel or hold \uE006 and release \uE002 to sort",
        .instructions = {
            {
                "\uE079 Sort by name",
                "\uE07A Sort by author"
            },
            {
                "\uE07B Sort by filename",
                NULL
            },
            {
                NULL,
                NULL
            },
            {
                "Exit",
                NULL
            }
        }
    },
    {
        .info_line = "Release \uE002 to cancel or hold \uE006 and release \uE002 to do stuff",
        .instructions = {
            {
                "\uE079 Jump in the list",
                "\uE07A Reload broken icons"
            },
            {
                "\uE07B Browse ThemePlaza",
                NULL,
            },
            {
                "\uE004 Sorting menu",
                NULL
            },
            {
                "Exit",
                NULL
            }
        }
    },
};

#endif