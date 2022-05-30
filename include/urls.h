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

static char from_hex(char c)
{
    return isdigit(c) ? c - '0' : tolower(c) - 'a' + 10;
}

static char to_hex(char code)
{
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}

// ensure caller frees
char * url_escape(char * url)
{
    char * ptr = url;
    char * buf = malloc(3 * strlen(url) + 1);
    char * ptr_buf = buf;

    while (*ptr)
    {
        if (isalnum((int)*ptr) || strchr("-_.~", *ptr))
            *ptr_buf++ = *ptr;
        else if (*ptr == ' ')
            *ptr_buf++ = '+';
        else
        {
            *ptr_buf++ = '%';
            *ptr_buf++ = to_hex(*ptr >> 4);
            *ptr_buf++ = to_hex(*ptr & 15);
        }
        ptr++;
    }
    *ptr_buf = '\0';
    return buf;
}

// ensure caller frees
char * url_unescape(char * url)
{
    char * ptr = url;
    char * buf = malloc(strlen(url) + 1);
    char * ptr_buf = buf;

    while (*ptr)
    {
        if (*ptr == '%')
        {
            if (ptr[1] && ptr[2])
            {
                *ptr_buf++ = from_hex(ptr[1]) << 4 | from_hex(ptr[2]);
                ptr += 2;
            }
        }
        else if (*ptr == '+')
            *ptr_buf++ = ' ';
        else
            *ptr_buf++ = *ptr;
        ptr++;
    }
    *ptr_buf = '\0';
    return buf;
}
