#include <3ds.h>
#include <stdlib.h>

#include "themes.h"

Result single_install(theme* theme)
{
	char *body;

	u16 path[262] = {0};
	memcpy(path, theme->path, 262);
	u16 body_path[12];
	ssize_t len = utf8_to_utf16(body_path, (u8*) "/body_lz.bin", 12);
	memcpy(&path[theme->path_len], body_path, 12);

	file_to_buf(fsMakePath(PATH_UTF16, path), body);
}