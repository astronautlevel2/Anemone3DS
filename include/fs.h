#ifndef FS_H
#define FS_H

#include <3ds.h>

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

Result open_archives(void);
int get_number_entries(char*);
u32 file_to_buf(FS_Path path, char* buf);
u32 zip_file_to_buf(char *file_name, u16 *zip_path, char *buf);
u32 buf_to_file(u32 size, char *path, FS_Archive archive, char *buf);
bool check_file_exists(char *path, FS_Archive archive);

#endif