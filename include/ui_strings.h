/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
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

#ifndef UISTRINGS_H
#define UISTRINGS_H

#include "colors.h"
#include "draw.h"

#define MAIN_STRINGS 14
#define REMOTE_STRINGS 29
#define SPLASHES_STRINGS 2
#define THEMES_STRINGS 6

typedef struct {
    const char *quit;
    const char *thread_error;
    const char *zip_not_theme_splash;
    const char *file_not_zip;
    const char *download_failed;
} Camera_Strings_s;

typedef struct {
    const char *theme_mode;
    const char *splash_mode;
    const char *no_themes;
    const char *no_splashes;
    const char *qr_download;
    const char *switch_splashes;
    const char *switch_themes;
    const char *quit;
    const char *by;
    const char *selected;
    const char *sel;
    const char *tp_theme_mode;
    const char *tp_splash_mode;
    const char *search;
    const char *page;
    const char *err_quit;
    const char *warn_continue;
    const char *yes_no;
    const char *load_themes;
    const char *load_splash;
    const char *load_icons;
    const char *install_splash;
    const char *delete_splash;
    const char *install_theme;
    const char *install_shuffle;
    const char *install_bgm;
    const char *install_no_bgm;
    const char *downloading;
    const char *checking_dl;
    const char *delete_sd;
    const char *download_themes;
    const char *download_splashes;
    const char *download_preview;
    const char *download_bgm;
    const char *dump_single;
    const char *dump_all_official;
    float start_pos;
} Draw_Strings_s;

typedef struct {
    const char *illegal_input;
    const char *new_or_overwrite;
    const char *cancel;
    const char *overwrite;
    const char *rename;
    const char *swkbd_fail;
    const char *sd_full;
    const char *fs_error;
} FS_Strings_s;

typedef struct {
    const char *no_preview;
} Loading_Strings_s;

typedef struct {
    const char *position_too_big;
    const char *position_zero;
    const char *jump_q;
    const char *cancel;
    const char *jump;
    const char *no_theme_extdata;
    const char *loading_qr;
    const char *no_wifi;
    const char *qr_homebrew;
    const char *camera_broke;
    const char *too_many_themes;
    const char *not_enough_themes;
    const char *uninstall_confirm;
    const char *delete_confirm;
} Main_Strings_s;

typedef struct {
    Instructions_s normal_instructions[MODE_AMOUNT];
    Instructions_s install_instructions;
    Instructions_s extra_instructions[3];
    Camera_Strings_s camera;
    Draw_Strings_s draw;
    FS_Strings_s fs;
    Loading_Strings_s loading;
    Main_Strings_s main;
    char *remote[REMOTE_STRINGS];
    char *splashes[SPLASHES_STRINGS];
    char *themes[THEMES_STRINGS];
} Language_s;

typedef enum {
    LANGUAGE_EN,

    LANGUAGE_AMOUNT,
} Language_Name;

extern Language_s languages[LANGUAGE_AMOUNT];
extern Language_s language;

#endif
