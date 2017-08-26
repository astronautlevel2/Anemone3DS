#include <stdlib.h>
#include <3ds.h>
#include <string.h>
#include <stdio.h>

#include "unicode.h"

ssize_t strulen(u16 *input, ssize_t max_len)
{
    for (int i = 0; i < max_len; i++) if (input[i] == 0) return i;
    return max_len;
}

void struacat(u16 *input, const char *addition)
{
    ssize_t len = strulen(input, 0x106);
    for (u16 i = len; i < strlen(addition) + len; i++) 
    {
        input[i] = addition[i - len];
    }
    input[strlen(addition) + len] = 0;
}

void printu(u16 *input)
{
    ssize_t in_len = strulen(input, 0x106);
    ssize_t buf_len = in_len + 1; // Plus 1 for proper null termination
    wchar_t *buf = calloc(buf_len, sizeof(wchar_t));
    for (u16 i = 0; i < buf_len; i++) buf[i] = input[i];
    printf("%ls\n", buf);
    free(buf);
}

u16 *strucat(u16 *destination, const u16 *source)
{
    ssize_t dest_len = strulen(destination, 0x106);

    ssize_t source_len = strulen(source, 0x106);

    memcpy(&destination[dest_len], source, source_len * sizeof(u16));
    return destination;
}