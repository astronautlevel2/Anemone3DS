#pragma once
#include "minizip/unzip.h"

typedef struct
{
	u16 title[0x40];
	u16 description[0x80];
	u16 author[0x40];
	char iconData[0x1200];
	u16 *path;
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
