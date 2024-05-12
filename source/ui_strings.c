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
#include "fs.h"

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
        .quit = "Press \uE005 To Quit",
        .thread_error = "Capture cam thread creation failed\nPlease report this to the developers",
        .zip_not_theme_splash = "Zip downloaded is neither\na splash nor a theme",
        .file_not_zip = "File downloaded isn't a zip.",
        .download_failed = "Download failed.",
    },
    .draw = 
    {
        .theme_mode = "Theme mode",
        .splash_mode = "Splash mode",
        .no_themes = "No theme found",
        .no_splashes = "No splash found",
        .qr_download = "Press \uE005 to download from QR",
        .switch_splashes = "Or \uE004 to switch to splashes",
        .switch_themes = "Or \uE004 to switch to themes",
        .quit = "Or        to quit",
        .start_pos = 162, // Adjust x pos of start glyph to line up with quit string
        .by = "By ",
        .selected = "Selected:",
        .sel = "Sel.:",
        .tp_theme_mode = "ThemePlaza Theme mode",
        .tp_splash_mode = "ThemePlaza Splash mode",
        .search = "Search...",
        .page = "Page:",
        .err_quit = "Press \uE000 to quit.",
        .warn_continue = "Press \uE000 to continue.",
        .yes_no = "\uE000 Yes   \uE001 No",
        .load_themes = "Loading themes, please wait...",
        .load_splash = "Loading splashes, please wait...",
        .load_icons = "Loading icons, please wait...",
        .install_splash = "Installing a splash...",
        .delete_splash = "Deleting installed splash...",
        .install_theme = "Installing a single theme...",
        .install_shuffle = "Installing shuffle themes...",
        .install_bgm = "Installing BGM-only theme...",
        .install_no_bgm = "Installing theme without BGM...",
        .downloading = "Downloading...",
        .checking_dl = "Checking downloaded file...",
        .delete_sd = "Deleting from SD...",
        .download_themes = "Downloading theme list, please wait...",
        .download_splashes = "Downloading splash list, please wait...",
        .download_preview = "Downloading preview, please wait...",
        .download_bgm = "Downloading BGM, please wait...",
        .dump_single = "Dumping theme, please wait...",
        .dump_all_official = "Dumping official themes, please wait...",
    },
    .fs =
    {
        .illegal_input = "Input must not contain:\n" ILLEGAL_CHARS,
        .new_or_overwrite = "Choose a new filename or tap Overwrite",
        .cancel = "Cancel",
        .overwrite = "Overwrite",
        .rename = "Rename",
        .swkbd_fail = "???\nTry a USB keyboard", // Should never be used
        .sd_full = "SD card is full.\nDelete some themes to make space.",
        .fs_error = "Error:\nGet a new SD card.",
    },
    .loading =
    {
        .no_preview = "No preview found.",
    },
    .main =
    {
        .position_too_big = "The new position has to be\nsmaller or equal to the\nnumber of entries!",
        .position_zero = "The new position has to\nbe positive!",
        .jump_q = "Where do you want to jump to?\nMay cause icons to reload.",
        .cancel = "Cancel",
        .jump = "Jump",
        .no_theme_extdata = "Theme extdata does not exist!\nSet a default theme from the home menu.",
        .loading_qr = "Loading QR Scanner...",
        .no_wifi = "Please connect to Wi-Fi before scanning QR codes",
        .qr_homebrew = "QR scanning doesnt work from the Homebrew\nLauncher, use the ThemePlaza browser instead.",
        .camera_broke = "Your camera seems to have a problem,\nunable to scan QR codes.",
        .too_many_themes = "You have too many themes selected.",
        .not_enough_themes = "You don't have enough themes selected.",
        .uninstall_confirm = "Are you sure you would like to delete\nthe installed splash?",
        .delete_confirm = "Are you sure you would like to delete this?",
    },
    .remote =
    {
        .no_results = "No results for this search.",
        .check_wifi = "Couldn't download Theme Plaza data.\nMake sure WiFi is on.",
        .new_page_big = "The new page has to be\nsmaller or equal to the\nnumber of pages!",
        .new_page_zero = "The new position has to\nbe positive!",
        .jump_page = "Which page do you want to jump to?",
        .cancel = "Cancel",
        .jump = "Jump",
        .tags = "Which tags do you want to search for?",
        .search = "Search",
        .parental_fail = "Parental Control validation failed!\nBrowser Access restricted.",
        .zip_not_found = "ZIP not found at this URL\nIf you believe this is an error, please\ncontact the site administrator",
        .generic_httpc_error = "Error in HTTPC sysmodule - 0x%08lx.\nIf you are seeing this, please contact an\nAnemone developer on the Theme Plaza Discord.",
        .http303_tp = "HTTP 303 See Other (Theme Plaza)\nHas this theme been approved?",
        .http303 = "HTTP 303 See Other\nDownload the resource directly\nor contact the site administrator.",
        .http404 = "HTTP 404 Not Found\nHas this theme been approved?",
        .http_err_url = "HTTP %s\nCheck that the URL is correct.",
        .http_errcode_generic = "HTTP %s\nContact the site administrator.",
        .http401 = "401 Unauthorized",
        .http403 = "403 Forbidden",
        .http407 = "407 Proxy Authentication Required",
        .http414 = "HTTP 414 URI Too Long\nThe QR code points to a really long URL.\nDownload the file directly.",
        .http418 = "HTTP 418 I'm a teapot\nContact the site administrator.",
        .http426 = "HTTP 426 Upgrade Required\nThe 3DS cannot connect to this server.\nContact the site administrator.",
        .http451 = "HTTP 451 Unavailable for Legal Reasons\nSome entity is preventing access\nto the host server for legal reasons.",
        .http500 = "HTTP 500 Internal Server Error\nContact the site administrator.",
        .http502 = "HTTP 502 Bad Gateway\nContact the site administrator.",
        .http503 = "HTTP 503 Service Unavailable\nContact the site administrator.",
        .http504 = "HTTP 504 Gateway Timeout\nContact the site administrator.",
        .http_unexpected = "HTTP %u\nIf you believe this is unexpected, please\ncontact the site administrator.",
    },
    .remote_instructions =
    {
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download theme",
                    "\uE001 Go back"
                },
                {
                    "\uE002 Hold for more",
                    "\uE003 Preview theme"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        },
        {
            .info_line = NULL,
            .instructions = {
                {
                    "\uE000 Download splash",
                    "\uE001 Go back"
                },
                {
                    "\uE002 Hold for more",
                    "\uE003 Preview splash"
                },
                {
                    "\uE004 Previous page",
                    "\uE005 Next page"
                },
                {
                    "Exit",
                    NULL
                }
            }
        }
    },
    .remote_extra_instructions =
    {
        .info_line = "Release \uE002 to cancel or hold \uE006 and release \uE002 to do stuff",
        .instructions = {
            {
                "\uE079 Jump to page",
                "\uE07A Search tags"
            },
            {
                "\uE07B Toggle splash/theme",
                "\uE07C Reload without cache"
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

Language_s init_strings(CFG_Language lang)
{
    switch (lang)
    {
        case CFG_LANGUAGE_JP:
        case CFG_LANGUAGE_FR:
        case CFG_LANGUAGE_DE:
        case CFG_LANGUAGE_IT:
        case CFG_LANGUAGE_ES:
        case CFG_LANGUAGE_ZH:
        case CFG_LANGUAGE_KO:
        case CFG_LANGUAGE_NL:
        case CFG_LANGUAGE_PT:
        case CFG_LANGUAGE_RU:
        case CFG_LANGUAGE_TW:
        case CFG_LANGUAGE_EN:
            return language_english;
    }
}
