#include <stdlib.h>
#include <3ds.h>
#include <string.h>

#include "unicode.h"

ssize_t strulen(u16 *input, ssize_t max_len)
{
    for (int i = 0; i < max_len; i++) if (input[i] == 0) return i;
    return max_len;
}

void struacat(u16 *input, char *addition)
{
    ssize_t len = strulen(input, 0x106);
    u8 *data = calloc(sizeof(u8), len * 4);
    utf16_to_utf8(data, input, len);

    memcpy(&data[len], addition, strlen(addition));
    utf8_to_utf16(input, data, len + strlen(addition));
    free(data);
}