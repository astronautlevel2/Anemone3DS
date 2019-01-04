/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2018 Contributors in CONTRIBUTORS.md
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

#include "file.h"
// #include "unicode.h"

extern "C" {
#include <archive.h>
#include <archive_entry.h>
#include <strings.h>
}

Result theme_result = 0;
Result badge_result = 0;
bool have_luma_folder = fs::exists("/luma/");

static std::array<FS_Archive, ACHIVE_AMOUNT> archives;

Result open_archives()
{
    std::fill(archives.begin(), archives.end(), 0);

    romfsInit();
    u8 regionCode;
    u32 archive1;
    u32 archive2;
    u32 archiveBadge = 0x000014d1;

    Result res = 0;

    FS_Path home;
    FS_Path theme;
    FS_Path badge;

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

    if(R_FAILED(res = FSUSER_OpenArchive(&archives[SD_CARD], ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""))))
        return res;

    FSUSER_CreateDirectory(archives[SD_CARD], fsMakePath(PATH_ASCII, "/Themes"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(archives[SD_CARD], fsMakePath(PATH_ASCII, "/Splashes"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(archives[SD_CARD], fsMakePath(PATH_ASCII, "/Badges"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(archives[SD_CARD], fsMakePath(PATH_ASCII, "/3ds"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(archives[SD_CARD], fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(archives[SD_CARD], fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE  "/cache"), FS_ATTRIBUTE_DIRECTORY);

    if(have_luma && !have_luma_folder)
    {
        if(R_FAILED(res = FSUSER_OpenArchive(&archives[CTRNAND], ARCHIVE_NAND_CTR_FS, fsMakePath(PATH_EMPTY, ""))))
            return res;
    }

    u32 homeMenuPath[3] = {MEDIATYPE_SD, archive2, 0};
    home.type = PATH_BINARY;
    home.size = 0xC;
    home.data = homeMenuPath;
    if(R_FAILED(res = FSUSER_OpenArchive(&archives[HOME_EXTDATA], ARCHIVE_EXTDATA, home)))
        return res;


    u32 themePath[3] = {MEDIATYPE_SD, archive1, 0};
    theme.type = PATH_BINARY;
    theme.size = 0xC;
    theme.data = themePath;
    if(R_FAILED(theme_result = res = FSUSER_OpenArchive(&archives[THEME_EXTDATA], ARCHIVE_EXTDATA, theme)))
        return res;

    u32 badgePath[3] = {MEDIATYPE_SD, archiveBadge, 0};
    badge.type = PATH_BINARY;
    badge.size = 0xC;
    badge.data = badgePath;
    if(R_FAILED(badge_result = res = FSUSER_OpenArchive(&archives[BADGE_EXTDATA], ARCHIVE_EXTDATA, badge)))
        return res;

    return 0;
}

Result close_archives()
{
    for(FS_Archive archive : archives)
    {
        if(archive)
            FSUSER_CloseArchive(archive);
    }
    return 0;
}

Result file_open(FS_Path path, Archive archive, Handle* handle, int mode)
{
    return FSUSER_OpenFile(handle, archives[archive], path, mode, 0);
}

u32 file_to_buf(FS_Path path, Archive archive, char** buf)
{
    Handle file;
    Result res = 0;
    if(R_FAILED(res = FSUSER_OpenFile(&file, archives[archive], path, FS_OPEN_READ, 0)))
        return 0;

    u64 size;
    if(R_FAILED(res = FSFILE_GetSize(file, &size)))
    {
        size = 0;
    }

    if(size != 0)
    {
        char* actual_buf = *buf ? *buf : new(std::nothrow) char[size];
        if(actual_buf != nullptr)
        {
            if(R_SUCCEEDED(res = FSFILE_Read(file, nullptr, 0, actual_buf, size)))
            {
                *buf = actual_buf;
            }
            else
            {
                if(!*buf)
                    delete[] actual_buf;
                size = 0;
            }
        }
        else
        {
            size = 0;
        }
    }

    FSFILE_Close(file);
    return (u32)size;
}

// If buf is nullptr, skips reading the file but does find it
static u32 zip_to_buf(struct archive* a, const char* filename, char** buf)
{
    struct archive_entry* entry;

    bool found = false;
    u64 size = 0;

    while(!found && archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        found = !strcasecmp(archive_entry_pathname(entry), filename);
    }

    if(found)
    {
        size = archive_entry_size(entry);
        if(buf)
        {
            char* actual_buf = *buf ? *buf : new(std::nothrow) char[size];
            if(actual_buf != nullptr)
            {
                archive_read_data(a, actual_buf, size);
                *buf = actual_buf;
            }
            else
            {
                DEBUG("File found, but allocation failed.\n");
                size = 0;
            }
        }
    }
    else
    {
        DEBUG("Couldn't find file in zip\n");
    }

    archive_read_free(a);

    return (u32)size;
}

u32 zip_file_to_buf(const char* filename, const std::string& zip_path, char** buf)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_filename(a, zip_path.c_str(), 0x4000);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened: %s\n", archive_error_string(a));
        archive_read_free(a);
        return 0;
    }

    return zip_to_buf(a, filename, buf);
}

bool check_file_is_zip(void* zip_buf, size_t zip_size, char** buf)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_buf, zip_size);
    archive_read_free(a);

    return r == ARCHIVE_OK;
}

bool check_file_in_zip(const char* filename, void* zip_buf, size_t zip_size)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_buf, zip_size);
    if(r != ARCHIVE_OK)
    {
        return false;
    }

    return zip_to_buf(a, filename, nullptr);
}


Result buf_to_file(FS_Path path, Archive archive, u32 size, const void* buf)
{
    Handle handle;
    Result res = 0;
    if(R_FAILED(res = FSUSER_OpenFile(&handle, archives[archive], path, FS_OPEN_WRITE, 0)))
        return res;
    if(R_FAILED(res = FSFILE_Write(handle, nullptr, 0, buf, size, FS_WRITE_FLUSH)))
        return res;
    if(R_FAILED(res = FSFILE_Close(handle)))
        return res;
    return 0;
}

void remake_file(FS_Path path, Archive archive, u32 size)
{
    Handle handle;
    FS_Archive actual_archive =  archives[archive];
    if(R_SUCCEEDED(FSUSER_OpenFile(&handle, actual_archive, path, FS_OPEN_READ, 0)))
    {
        FSFILE_Close(handle);
        FSUSER_DeleteFile(actual_archive, path);
    }
    FSUSER_CreateFile(actual_archive, path, 0, size);
    char* buf = new(std::nothrow) char[size];
    if(buf)
    {
        memset(buf, 0, size);
        buf_to_file(path, archive, size, buf);
        delete[] buf;
    }
    else
    {
        DEBUG("problems in memory for remake_file!\n");
    }
}

void delete_file(FS_Path path, Archive archive)
{
    FSUSER_DeleteFile(archives[archive], path);
}
