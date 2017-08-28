#include <3ds.h>
#include <string.h>
#include <stdlib.h>

#include "fs.h"
#include "unicode.h"
#include "minizip/unzip.h"

Result open_archives(void)
{

    u8 regionCode;
    u32 archive1;
    u32 archive2;

    Result retValue;

    FS_Path home;
    FS_Path theme;

    CFGU_SecureInfoGetRegion(&regionCode);
    switch(regionCode)
    {
        case 1:
            archive1 = 0x000002cd;
            archive2 = 0x0000008f;
            break;
        case 2:
            archive1 = 0x000002ce;
            archive2 = 0x00000098;
            break;
        case 3:
            archive1 = 0x000002cc;
            archive2 = 0x00000082;
            break;
        default:
            archive1 = 0x00;
            archive2 = 0x00;
    }

    retValue = FSUSER_OpenArchive(&ArchiveSD, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    if(R_FAILED(retValue)) return retValue;

    u32 homeMenuPath[3] = {MEDIATYPE_SD, archive2, 0};
    home.type = PATH_BINARY;
    home.size = 0xC;
    home.data = homeMenuPath;
    retValue = FSUSER_OpenArchive(&ArchiveHomeExt, ARCHIVE_EXTDATA, home);  
    if(R_FAILED(retValue)) return retValue;

    u32 themePath[3] = {MEDIATYPE_SD, archive1, 0};
    theme.type = PATH_BINARY;
    theme.size = 0xC;
    theme.data = themePath;
    retValue = FSUSER_OpenArchive(&ArchiveThemeExt, ARCHIVE_EXTDATA, theme);    
    if(R_FAILED(retValue)) return retValue;
    return 0;
}

Result close_archives(void)
{
    Result retValue;

    retValue = FSUSER_CloseArchive(ArchiveSD);
    if(R_FAILED(retValue)) return retValue;
    retValue = FSUSER_CloseArchive(ArchiveHomeExt);
    if(R_FAILED(retValue)) return retValue;
    retValue = FSUSER_CloseArchive(ArchiveThemeExt);
    if(R_FAILED(retValue)) return retValue;
    
    return 0;
}

int get_number_entries(char *path)
{
    int count = 0;
    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, path));
    if (R_FAILED(res)) return -1;

    bool done = false;
    while (!done)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        u32 entries_read = 0;
        FSDIR_Read(dir_handle, &entries_read, 1, entry);
        free(entry);
        if (entries_read) count++;
        else if (!entries_read) break;
    }
    FSDIR_Close(dir_handle);
    return count;
}

u64 file_to_buf(FS_Path path, FS_Archive archive, char** buf)
{
    Handle file;
    Result res = FSUSER_OpenFile(&file, archive, path, FS_OPEN_READ, 0);
    if (R_FAILED(res)) return 0;

    u64 size;
    FSFILE_GetSize(file, &size);
    *buf = malloc(size);
    FSFILE_Read(file, NULL, 0, *buf, size);
    FSFILE_Close(file);
    return size;
}

u32 zip_file_to_buf(char *file_name, u16 *zip_path, char **buf)
{
    fflush(stdout);
    ssize_t len = strulen(zip_path, 0x106);

    u8 *path = calloc(sizeof(u8), len * 4);
    utf16_to_utf8(path, zip_path, len);

    unzFile zip_handle = unzOpen(path); // Can unzOpen really handle utf8?

    if (zip_handle == NULL) return 0;
    u32 file_size = 0;

    int status = unzLocateFile(zip_handle, file_name, 0);
    if (status == UNZ_OK)
    {
        unz_file_info *file_info = malloc(sizeof(unz_file_info));
        unzGetCurrentFileInfo(zip_handle, file_info, NULL, 0, NULL, 0, NULL, 0);
        file_size = file_info->uncompressed_size;
        *buf = malloc(file_size);
        unzOpenCurrentFile(zip_handle);
        unzReadCurrentFile(zip_handle, *buf, file_size);
        unzCloseCurrentFile(zip_handle);
        unzClose(zip_handle);

        free(path);
        free(file_info);
        return file_size;
    } else {
        free(path);
        puts("fileziprip");
        return 0;
    }
}

u32 buf_to_file(u32 size, char *path, FS_Archive archive, char *buf)
{
    Handle handle;
    u32 bytes = 0;
    Result res = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_ASCII, path), FS_OPEN_WRITE, 0);
    if (R_FAILED(res)) return res;
    res = FSFILE_Write(handle, &bytes, 0, buf, size, FS_WRITE_FLUSH);
    if (R_FAILED(res)) return res;
    res = FSFILE_Close(handle);
    if (R_FAILED(res)) return res;
    return bytes;
}

void remake_file(char *path, FS_Archive archive, u32 size)
{
    Handle handle;
    if (R_SUCCEEDED(FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0)))
    {
        FSFILE_Close(handle);
        FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, path));
    }
    FSUSER_CreateFile(archive, fsMakePath(PATH_ASCII, path), 0, size);
}