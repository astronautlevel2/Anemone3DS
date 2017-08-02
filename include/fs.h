#ifndef FS_H
#define FS_H

#include <3ds.h>

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

Result open_archives(void);
int get_number_entries(char*);

#endif