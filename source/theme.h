#pragma once
#include "minizip/unzip.h"

u8 regionCode;
u32 archive1;
u32 archive2;

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

Result unzip_file(unzFile zip_handle, char *theme_path);
Result unzip_theme(char*);
s8 prepareThemes();
s8 themeInstall(const char* path, bool music);
s8 closeThemeArchives();
