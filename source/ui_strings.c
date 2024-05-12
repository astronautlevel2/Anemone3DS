/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2024 Contributors in CONTRIBUTORS.md
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

#include "ui_strings.h"

const Language_s language_english = {
    .normal_instructions = 
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Install Theme(s)",
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
    },

    .install_instructions =
    {
        .info_line = "\uE001 Cancel theme install",
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
    },

    .extra_instructions = 
    {
        {
            .info_line = "\uE001 Leave sorting menu",
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
            .info_line = "\uE001 Leave extra menu",
            .instructions = {
                {
                    "\uE079 Jump in the list",
                    "\uE07A Reload broken icons"
                },
                {
                    "\uE07B Browse ThemePlaza",
                    NULL
                },
                {
                    "\uE004 Sorting menu",
                    "\uE005 Dumping menu"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = "\uE001 Leave dump menu",
            .instructions = {
                {
                    "\uE079 Dump Current Theme",
                    "\uE07A Dump All Themes"
                },
                {
                    NULL,
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
        }
    },
    .camera = 
    {
        "Press \uE005 To Quit",
        "Capture cam thread creation failed\nPlease report this to the developers",
        "Zip downloaded is neither\na splash nor a theme",
        "File downloaded isn't a zip.",
        "Download failed.",
    },
    .draw = 
    {
        "Theme mode",
        "Splash mode",
        "No theme found",
        "No splash found",
        "Press \uE005 to download from QR",
        "Or \uE004 to switch to splashes",
        "Or \uE004 to switch to themes",
        "Or        to quit",
        "By ",
        "Selected:",
        "Sel.:",
        "ThemePlaza Theme mode",
        "ThemePlaza Splash mode",
        "Search...",
        "Page:",
        "Press \uE000 to quit.",
        "Press \uE000 to continue.",
        "\uE000 Yes   \uE001 No",
        "Loading themes, please wait...",
        "Loading splashes, please wait...",
        "Loading icons, please wait...",
        "Installing a splash...",
        "Deleting installed splash...",
        "Installing a single theme...",
        "Installing shuffle themes...",
        "Installing BGM-only theme...",
        "Installing theme without BGM...",
        "Downloading...",
        "Checking downloaded file...",
        "Deleting from SD...",
        "Downloading theme list, please wait...",
        "Downloading splash list, please wait...",
        "Downloading preview, please wait...",
        "Downloading BGM, please wait...",
        "Dumping theme, please wait...",
        "Dumping official themes, please wait...",
    },
    .fs =
    {
        "Input must not contain:\n",
        "Choose a new filename or tap Overwrite",
        "Cancel",
        "Overwrite",
        "Rename",
        "???\nTry a USB keyboard", // Should never be used
        "SD card is full.\nDelete some themes to make space.",
        "Error:\nGet a new SD card.",
    },
    .loading =
    {
        "No preview found.",
    },
    .main =
    {
        "The new position has to be\nsmaller or equal to the\nnumber of entries!",
        "The new position has to\nbe positive!",
        "Where do you want to jump to?\nMay cause icons to reload.",
        "Cancel",
        "Jump",
        "Theme extdata does not exist!\nSet a default theme from the home menu.",
        "Loading QR Scanner...",
        "Please connect to Wi-Fi before scanning QR codes",
        "QR scanning doesnt work from the Homebrew\nLauncher, use the ThemePlaza browser instead.",
        "Your camera seems to have a problem,\nunable to scan QR codes.",
        "You have too many themes selected.",
        "You don't have enough themes selected.",
        "Are you sure you would like to delete\nthe installed splash?",
        "Are you sure you would like to delete this?",
    },
    .remote =
    {
        "No results for this search.",
        "Couldn't download Theme Plaza data.\nMake sure WiFi is on.",
        "The new page has to be\nsmaller or equal to the\nnumber of pages!",
        "The new position has to\nbe positive!",
        "Which page do you want to jump to?",
        "Cancel",
        "Jump",
        "Which tags do you want to search for?",
        "Search",
        "Parental Control validation failed!\nBrowser Access restricted.",
        "ZIP not found at this URL\nIf you believe this is an error, please\ncontact the site administrator",
        "Error in HTTPC sysmodule - 0x%08lx.\nIf you are seeing this, please contact an\nAnemone developer on the Theme Plaza Discord.",
        "HTTP 303 See Other (Theme Plaza)\nHas this theme been approved?",
        "HTTP 303 See Other\nDownload the resource directly\nor contact the site administrator.",
        "HTTP 404 Not Found\nHas this theme been approved?",
        "HTTP %s\nCheck that the URL is correct.",
        "HTTP %s\nContact the site administrator.",
        "401 Unauthorized",
        "403 Forbidden",
        "407 Proxy Authentication Required",
        "HTTP 414 URI Too Long\nThe QR code points to a really long URL.\nDownload the file directly.",
        "HTTP 418 I'm a teapot\nContact the site administrator.",
        "HTTP 426 Upgrade Required\nThe 3DS cannot connect to this server.\nContact the site administrator.",
        "HTTP 451 Unavailable for Legal Reasons\nSome entity is preventing access\nto the host server for legal reasons.",
        "HTTP 500 Internal Server Error\nContact the site administrator.",
        "HTTP 502 Bad Gateway\nContact the site administrator.",
        "HTTP 503 Service Unavailable\nContact the site administrator.",
        "HTTP 504 Gateway Timeout\nContact the site administrator.",
        "HTTP %u\nIf you believe this is unexpected, please\ncontact the site administrator.",
    },
    .splashes =
    {
        "No splash.bin or splashbottom.bin found.\nIs this a splash?",
        "WARNING: Splashes are disabled in Luma Config",
    },
    .themes =
    {
        "No body_LZ.bin found - is this a theme?",
        "One or more installed themes use mono audio.\nMono audio causes a number of issues.\nCheck the wiki for more information.",
        "Illegal character used.",
        "Name of output folder",
        "Cancel",
        "Done"
    }
};

Language_s languages[LANGUAGE_AMOUNT] = 
{
    language_english,
};

