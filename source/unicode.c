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

#include "unicode.h"

void replace_chars(u16 *input, char *remove, u16 with)
{
    for (u16 *cursor = input; *cursor != '\0'; cursor++)
    {
        if (strchr(remove, (char) (*cursor & 0xFF)))
        {
            *cursor = with;
        }
    }
}

size_t strulen(const u16 * input, ssize_t max_len)
{
    for (int i = 0; i < max_len; i++) if (input[i] == 0) return i;
    return max_len;
}

void struacat(u16 * input, const char * addition)
{
    const ssize_t len = strulen(input, 0x106);
    const u16 stop_at = strlen(addition);
    for (u16 i = 0; i < stop_at; i++) 
    {
        input[i + len] = addition[i];
    }
    input[stop_at + len] = 0;
}

void printu(u16 * input)
{
    ssize_t in_len = strulen(input, 0x106);
    ssize_t buf_len = in_len + 1; // Plus 1 for proper null termination
    wchar_t * buf = calloc(buf_len, sizeof(wchar_t));
    utf16_to_utf32((u32 *)buf, input, buf_len);
    char cbuf[0x106];
    sprintf(cbuf, "%ls\n", buf);
    DEBUG(cbuf);
    free(buf);
}

size_t strucat(u16 * destination, const u16 * source)
{
    size_t dest_len = strulen(destination, 0x106);

    size_t source_len = strulen(source, 0x106);

    memcpy(&destination[dest_len], source, source_len * sizeof(u16));
    destination[min(dest_len + source_len, 0x106 - 1)] = 0;
    return source_len;
}