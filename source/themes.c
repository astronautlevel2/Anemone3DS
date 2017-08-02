#include <3ds.h>
#include <stdlib.h>

#include "themes.h"

Result single_install(theme* theme)
{
    char *body;

    if (theme->is_zip)
    {
        zip_file_to_buf("body_lz.bin", theme->path, body);
    } else {
        u16 path[0x106];
        memcpy(path, theme->path, 0x106);
        struacat(path, "/body_lz.bin");
        file_to_buf(path, body);
    }
}