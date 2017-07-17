#pragma once
#include "minizip/unzip.h"

typedef struct
{
	char *name;
	char *path;
	bool bgm;
} theme;

typedef struct
{
	theme *elem;
	theme *next;
} theme_node;

u8 regionCode;
u32 archive1;
u32 archive2;

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

Result unzip_theme(FS_DirectoryEntry*, u16*);
s8 prepareThemes();
s8 themeInstall(theme);
s8 closeThemeArchives();
