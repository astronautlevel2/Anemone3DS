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

#include "i18n.h"

void i18n_set(u8 language)
{
    char filename[19] = "romfs:/lang/"; // 19 = length of "romfs:/lang/cc.txt" + NULL
    switch(language)
    {
        case CFG_LANGUAGE_EN:
        default:
            strcat(filename, "en.txt");
    }

    FILE* file = fopen(filename, "r");
    if(file == NULL)
    {
        DEBUG(filename);
        DEBUG(" missing\n")
        return;
    }

    char text_buf[128];

    for(int i = 0; i < TEXT_AMOUNT; i++)
    {
        memset(text_buf, 0, sizeof(text_buf));
        if(i == TEXT_VERSION)
            continue;
        fgets(text_buf, sizeof(text_buf), file);
        text_buf[strlen(text_buf) - 1] = '\0'; // set \n to null char
        C2D_TextParse(&text[i], staticBuf, text_buf);
        C2D_TextOptimize(&text[i]);
    }
}

//void i18n_instructions