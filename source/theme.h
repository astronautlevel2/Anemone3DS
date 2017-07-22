#pragma once

#include "minizip/unzip.h"
#include "linked_lists.h"

struct theme_data
{
	u16 title[0x40];
	u16 description[0x80];
	u16 author[0x40];
	char iconData[0x1200];
	u16 path[533];
	bool bgm;
};

typedef struct theme_data theme_data;

u8 regionCode;
u32 archive1;
u32 archive2;

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

Result unzip_file(char*, FS_DirectoryEntry*, u16*);
Result prepareThemes(node*);
Result themeInstall(theme_data);
Result closeThemeArchives();
Result parseSmdh(theme_data*, u16*);
