/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2017 Alex Taber ("astronautlevel"), Dawid Eckert ("daedreth")
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include <strings.h>
 
#include "fs.h"
#include "unicode.h"
 
#include "minizip/unzip.h"
 
int filename_compare(__attribute__((unused)) unzFile file, const char *current_filename, const char *filename)
{
    return strcasecmp(current_filename, filename);
}
 
Result open_archives(void)
{
    romfsInit();
    u8 regionCode;
    u32 archive1;
    u32 archive2;
 
    Result res = 0;
 
    FS_Path home;
    FS_Path theme;
 
    CFGU_SecureInfoGetRegion(&regionCode);
    switch(regionCode)
    {
        case 0:
            archive1 = 0x000002cc;
            archive2 = 0x00000082;
            break;
        case 1:
            archive1 = 0x000002cd;
            archive2 = 0x0000008f;
            break;
        case 2:
            archive1 = 0x000002ce;
            archive2 = 0x00000098;
            break;
        default:
            archive1 = 0x00;
            archive2 = 0x00;
    }
 
    if(R_FAILED(res = FSUSER_OpenArchive(&ArchiveSD, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return res;

    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/Themes"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/Splashes"), FS_ATTRIBUTE_DIRECTORY);
 
    u32 homeMenuPath[3] = {MEDIATYPE_SD, archive2, 0};
    home.type = PATH_BINARY;
    home.size = 0xC;
    home.data = homeMenuPath;
    if(R_FAILED(res = FSUSER_OpenArchive(&ArchiveHomeExt, ARCHIVE_EXTDATA, home))) return res;
 
    u32 themePath[3] = {MEDIATYPE_SD, archive1, 0};
    theme.type = PATH_BINARY;
    theme.size = 0xC;
    theme.data = themePath;
    if(R_FAILED(res = FSUSER_OpenArchive(&ArchiveThemeExt, ARCHIVE_EXTDATA, theme))) return res;
 
    Handle test_handle;
    if(R_FAILED(res = FSUSER_OpenFile(&test_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ, 0))) return res;
    FSFILE_Close(test_handle);
 
    return 0;
}

Result close_archives(void)
{
    Result res;
 
    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveSD))) return res;
    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveHomeExt))) return res;
    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveThemeExt))) return res;

    return 0;
}
 
u64 file_to_buf(FS_Path path, FS_Archive archive, char** buf)
{
    Handle file;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&file, archive, path, FS_OPEN_READ, 0))) return 0;
 
    u64 size;
    FSFILE_GetSize(file, &size);
    *buf = calloc(1, size);
    FSFILE_Read(file, NULL, 0, *buf, size);
    FSFILE_Close(file);
    return size;
}

u32 zip_file_to_buf(char *file_name, u16 *zip_path, char **buf)
{
    ssize_t len = strulen(zip_path, 0x106);
 
    u8 *path = calloc(sizeof(u8), len * 4);
    utf16_to_utf8(path, zip_path, len * 4);
 
    unzFile zip_handle = unzOpen((char*)path);
 
    if (zip_handle == NULL) return 0;
    u32 file_size = 0;
 
    int status = unzLocateFile(zip_handle, file_name, filename_compare);
    if (status == UNZ_OK)
    {
        unz_file_info *file_info = malloc(sizeof(unz_file_info));
        unzGetCurrentFileInfo(zip_handle, file_info, NULL, 0, NULL, 0, NULL, 0);
        file_size = file_info->uncompressed_size;
        *buf = calloc(1, file_size);
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

Result buf_to_file(u32 size, char *path, FS_Archive archive, char *buf)
{
    Handle handle;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_ASCII, path), FS_OPEN_WRITE, 0))) return res;
    if (R_FAILED(res = FSFILE_SetSize(handle, 0))) return res;
    if (R_FAILED(res = FSFILE_Write(handle, NULL, 0, buf, size, FS_WRITE_FLUSH))) return res;
    if (R_FAILED(res = FSFILE_Close(handle))) return res;
    return 0;
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