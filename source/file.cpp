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

std::pair<std::unique_ptr<char[]>, u64> file_to_buf(FS_Path path, Archive archive, u32 wanted_size)
{
    Handle file;
    Result res = 0;
    if(R_FAILED(res = FSUSER_OpenFile(&file, archives[archive], path, FS_OPEN_READ, 0)))
        return std::make_pair(nullptr, 0);

    u64 size;
    if(R_FAILED(res = FSFILE_GetSize(file, &size)))
    {
        size = 0;
    }
    char* out = nullptr;

    DEBUG("wanted, size: %lu, %llu\n", wanted_size, size);
    if((wanted_size && wanted_size == size) || (!wanted_size && size != 0))
    {
        out = new(std::nothrow) char[size];
        if(out != nullptr)
        {
            if(R_FAILED(res = FSFILE_Read(file, nullptr, 0, out, size)))
            {
                delete[] out;
                out = nullptr;
                size = 0;
            }
        }
        else
        {
            size = 0;
        }
    }

    FSFILE_Close(file);
    return std::make_pair(std::unique_ptr<char[]>(out), size);
}

bool file_to_buf(FS_Path path, Archive archive, void* buf, u32 wanted_size)
{
    Handle file;
    Result res = 0;
    if(R_FAILED(res = FSUSER_OpenFile(&file, archives[archive], path, FS_OPEN_READ, 0)))
        return false;

    u64 size;
    if(R_FAILED(res = FSFILE_GetSize(file, &size)))
    {
        FSFILE_Close(file);
        return false;
    }

    if((wanted_size && wanted_size == size) || (!wanted_size && size != 0))
    {
       FSFILE_Read(file, nullptr, 0, buf, size);
    }
    else
    {
        FSFILE_Close(file);
        return false;
    }

    FSFILE_Close(file);
    return true;
}

// If buf is nullptr, skips reading the file but does find it
static std::unique_ptr<char[]> zip_to_buf(struct archive* a, const char* filename, u64& size, u32 wanted_size, void* buf = nullptr, bool search_only = false)
{
    struct archive_entry* entry;

    bool found = false;

    while(!found && archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        found = !strcasecmp(archive_entry_pathname(entry), filename);
    }

    char* out = nullptr;
    if(found)
    {
        size = archive_entry_size(entry);
        if(!search_only)
        {
            if((wanted_size && size == wanted_size) || (!wanted_size && size != 0))
            {
                if(buf)
                {
                    archive_read_data(a, buf, size);
                }
                else
                {
                    out = new(std::nothrow) char[size];
                    archive_read_data(a, out, size);
                }
            }
            else
            {
                DEBUG("File found, but size doesn't match.\n");
                size = 0;
            }
        }
    }
    else
    {
        DEBUG("Couldn't find file in zip\n");
    }

    archive_read_free(a);

    return std::unique_ptr<char[]>(out);
}

std::pair<std::unique_ptr<char[]>, u64> zip_file_to_buf(const char* filename, const std::string& zip_path, u32 wanted_size)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_filename(a, zip_path.c_str(), 0x4000);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened: %s\n", archive_error_string(a));
        archive_read_free(a);
        return std::make_pair(nullptr, 0);;
    }

    u64 size = 0;
    std::unique_ptr<char[]> buf = zip_to_buf(a, filename, size, wanted_size);
    return std::make_pair(std::move(buf), size);
}

bool zip_file_to_buf(const char* filename, const std::string& zip_path, void* buf, u32 wanted_size)
{
    struct archive* a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_filename(a, zip_path.c_str(), 0x4000);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened: %s\n", archive_error_string(a));
        archive_read_free(a);
        return false;
    }

    u64 size = 0;
    zip_to_buf(a, filename, size, wanted_size, buf);
    return size != 0;
}

bool check_file_is_zip(void* zip_buf, size_t zip_size)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_buf, zip_size);
    archive_read_free(a);

    return r == ARCHIVE_OK;
}

bool check_file_in_zip(void* zip_buf, size_t zip_size, const char* filename)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_buf, zip_size);
    if(r != ARCHIVE_OK)
    {
        return false;
    }

    u64 size = 0;
    zip_to_buf(a, filename, size, 0, nullptr, true);
    return size != 0;
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

void remake_file(FS_Path path, Archive archive, u32 size, const void* buf)
{
    Handle handle;
    FS_Archive actual_archive =  archives[archive];
    if(R_SUCCEEDED(FSUSER_OpenFile(&handle, actual_archive, path, FS_OPEN_READ, 0)))
    {
        FSFILE_Close(handle);
        FSUSER_DeleteFile(actual_archive, path);
    }
    FSUSER_CreateFile(actual_archive, path, 0, size);
    char* empty_buf = buf ? nullptr : new(std::nothrow) char[size];
    if(empty_buf)
    {
        memset(empty_buf, 0, size);
        buf_to_file(path, archive, size, empty_buf);
        delete[] empty_buf;
    }
    else if(buf)
    {
        buf_to_file(path, archive, size, buf);
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

void delete_folder(FS_Path path, Archive archive)
{
    FSUSER_DeleteDirectoryRecursively(archives[archive], path);
}

void extract_all_badges(void* zip_buf, size_t zip_size)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);
    int r = archive_read_open_memory(a, zip_buf, zip_size);
    if(r != ARCHIVE_OK)
    {
        DEBUG("couldn't open badges zip.\n");
        return;
    }

    struct archive_entry* entry;
    std::vector<u8> buf;
    const char* filename = nullptr;
    while(archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        if(strcasecmp((filename = archive_entry_pathname(entry)), "preview.png"))
        {
            u64 size = archive_entry_size(entry);
            buf.resize(size);
            archive_read_data(a, buf.data(), size);
            char path_to_file[0x107];
            sprintf(path_to_file, "/Badges/%s", filename);
            DEBUG("reading badge to %s\n", path_to_file);
            FILE* fh = fopen(path_to_file, "wb");
            fwrite(buf.data(), 1, size, fh);
            fclose(fh);
        }
    }
}
