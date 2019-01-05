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

#ifndef TEXT_ENUMS_H
#define TEXT_ENUMS_H

enum TextType {
    TEXT_GENERAL = 0,
    TEXT_INSTRUCTIONS,
    TEXT_INSTALL,
    TEXT_ERROR,

    TEXT_TYPES_AMOUNT
};

enum TextID {
    TEXT_THEME_MODE = 0,
    TEXT_SPLASH_MODE,
    TEXT_BADGE_MODE,

    // These should always follow right after
    TEXT_NO_THEME_FOUND,
    TEXT_NO_SPLASH_FOUND,
    TEXT_NO_BADGE_FOUND,

    TEXT_NOT_FOUND_PRESS_FOR_QR,
    TEXT_NOT_FOUND_OR_BROWSER,
    TEXT_NOT_FOUND_OR,
    TEXT_L,
    TEXT_R,
    TEXT_NOT_FOUND_TO_SWITCH_TO,
    TEXT_NOT_FOUND_SWITCH_TO_THEME,
    TEXT_NOT_FOUND_SWITCH_TO_SPLASH,
    TEXT_NOT_FOUND_SWITCH_TO_BADGE,
    TEXT_NOT_FOUND_START_TO_QUIT,

    TEXT_ENTRY_BY,

    TEXT_MENU_SELECTED,
    TEXT_MENU_SELECTED_SHORT,

    TEXT_THEMEPLAZA_THEME_MODE,
    TEXT_THEMEPLAZA_SPLASH_MODE,
    TEXT_THEMEPLAZA_PAGE,

    TEXT_ERROR_QUIT,
    TEXT_ERROR_CONTINUE,

    TEXT_PROMPT_ARE_YOU_SURE,
    TEXT_PROMPT_YES,
    TEXT_PROMPT_NO,

    TEXT_INFO_INSTRUCTIONS_QUIT,
    TEXT_INFO_EXIT_INSTRUCTIONS,

    TEXT_THEME_ICON,
    TEXT_SPLASH_ICON,
    TEXT_BADGE_ICON,

    // always the last
    TEXT_SPACE,
    TEXT_VERSION,

    TEXTS_AMOUNT
};

enum InstallType {
    INSTALL_LOADING_THEMES = 0,
    INSTALL_LOADING_SPLASHES,
    INSTALL_LOADING_BADGES,
    INSTALL_LOADING_ICONS,
    INSTALL_SORTING,
    INSTALL_DELETING,

    INSTALL_FINDING_MARKED_THEMES,
    INSTALL_THEME_SINGLE,
    INSTALL_THEME_SHUFFLE,
    INSTALL_THEME_BGM,
    INSTALL_THEME_NO_BGM,

    INSTALL_SPLASH,
    INSTALL_SPLASH_TOP,
    INSTALL_SPLASH_BOTTOM,
    INSTALL_SPLASH_DELETE,

    INSTALL_BADGE,

    INSTALL_DOWNLOAD,
    INSTALL_CHECKING_DOWNLOAD,
    INSTALL_ENTRY_DELETE,

    INSTALL_LOADING_REMOTE_THEMES,
    INSTALL_LOADING_REMOTE_SPLASHES,
    INSTALL_LOADING_REMOTE_PREVIEW,
    INSTALL_LOADING_REMOTE_BGM,

    INSTALLS_AMOUNT
};

enum ErrorLevel {
    ERROR_LEVEL_INFO = 0,
    ERROR_LEVEL_WARNING,
    ERROR_LEVEL_ERROR,
    ERROR_LEVEL_CRITICAL,

    ERROR_LEVEL_AMOUNT
};

enum ErrorType {
    ERROR_TYPE_NO_THEME_EXTDATA = 0,
    ERROR_TYPE_NO_BADGE_EXTDATA,
    ERROR_TYPE_NO_LUMA,

    ERROR_TYPE_NO_WIFI_QR,
    ERROR_TYPE_NO_QR_HBL,
    ERROR_TYPE_OTHER_QR_ERROR,

    ERROR_TYPE_DOWNLOADED_NOT_ZIP,
    ERROR_TYPE_DOWNLOAD_FAILED,

    ERROR_TYPE_INVALID_PREVIEW_PNG,
    ERROR_TYPE_NO_PREVIEW,

    ERROR_TYPE_SHUFFLE_NOT_ENOUGH,
    ERROR_TYPE_SHUFFLE_TOO_MANY,

    ERROR_TYPE_THEMEPLAZA_COULDNT_LOAD,
    ERROR_TYPE_THEMEPLAZA_NO_RESULT,
    ERROR_TYPE_THEMEPLAZA_BADGES_DISABLED,

    ERROR_TYPE_TARGET_NOT_VALID,

    ERROR_TYPE_SPLASH_LUMA_DISABLED,
    ERROR_TYPE_SPLASH_TOP_SIZE_WRONG,
    ERROR_TYPE_SPLASH_BOTTOM_SIZE_WRONG,
    ERROR_TYPE_SPLASH_TOP_NOT_FOUND,
    ERROR_TYPE_SPLASH_BOTTOM_NOT_FOUND,

    ERROR_TYPE_THEME_NO_BODY,
    ERROR_TYPE_THEME_NO_BGM,
    ERROR_TYPE_THEME_BGM_TOO_BIG,

    ERRORS_AMOUNT
};

enum InstructionType {
    // Used almost everywhere except normal mode
    INSTRUCTION_B_FOR_GOING_BACK = 0,

    // General normal mode
    INSTRUCTION_A_FOR_ACTION_MODE,
    INSTRUCTION_B_FOR_QR_SCANNER,
    INSTRUCTION_X_FOR_EXTRA_MODE,
    INSTRUCTION_Y_FOR_ENTERING_BROWSER,
    INSTRUCTION_UP_TO_MOVE_UP,
    INSTRUCTION_LEFT_TO_MOVE_PAGE_UP,
    INSTRUCTION_DOWN_TO_MOVE_DOWN,
    INSTRUCTION_RIGHT_TO_MOVE_PAGE_DOWN,

    // General extra mode
    INSTRUCTION_A_FOR_JUMPING,
    INSTRUCTION_UP_FOR_SORTING_AUTHOR,
    INSTRUCTION_LEFT_FOR_SORTING_FILENAME,
    INSTRUCTION_RIGHT_FOR_SORTING_NAME,

    // General action mode
    INSTRUCTION_X_FOR_DELETING_ENTRY,
    INSTRUCTION_Y_FOR_PREVIEW,

    // ThemePlaza normal mode
    INSTRUCTION_A_FOR_DOWNLOADING,
    // X for extra mode
    // Y for preview
    // Up to move up
    INSTRUCTION_LEFT_TO_MOVE_LEFT,
    // Down to move down
    INSTRUCTION_RIGHT_TO_MOVE_RIGHT,

    // Theme action mode
    INSTRUCTION_THEME_A_FOR_MARKING,
    INSTRUCTION_THEME_UP_FOR_NORMAL,
    INSTRUCTION_THEME_LEFT_FOR_BGM_ONLY,
    INSTRUCTION_THEME_DOWN_FOR_SHUFFLE,
    INSTRUCTION_THEME_RIGHT_FOR_NO_BGM,

    // Splash action mode
    INSTRUCTIONS_SPLASH_A_TO_DELETE_INSTALLED,
    INSTRUCTION_SPLASH_UP_FOR_INSTALLING,
    INSTRUCTION_SPLASH_LEFT_FOR_INSTALLING_TOP,
    INSTRUCTION_SPLASH_RIGHT_FOR_INSTALLING_BOTTOM,

    INSTRUCTIONS_NONE
};

enum KeyboardText {
    KEYBOARD_GENERAL_TOO_HIGH,
    KEYBOARD_GENERAL_NON_ZERO,

    KEYBOARD_THEMEPLAZA_TOO_HIGH,
    KEYBOARD_THEMEPLAZA_NON_ZERO,

    KEYBOARD_TEXTS_AMOUNT
};

extern std::vector<char*> keyboard_shown_text;

#endif