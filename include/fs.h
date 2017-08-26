#ifndef FS_H
#define FS_H

#include <3ds.h>

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

Result open_archives(void);
Result close_archives(void);
int get_number_entries(char*);
u64 file_to_buf(FS_Path path, FS_Archive archive, char** buf);
u32 zip_file_to_buf(char *file_name, u16 *zip_path, char **buf);
u32 buf_to_file(u32 size, char *path, FS_Archive archive, char *buf);
void remake_file(char *path, FS_Archive archive, u32 size);

#endif