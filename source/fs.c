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

#include <strings.h>

#include "fs.h"
#include "unicode.h"

#include <archive.h>
#include <archive_entry.h>

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

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
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE  "/cache"), FS_ATTRIBUTE_DIRECTORY);

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

u32 file_to_buf(FS_Path path, FS_Archive archive, char** buf)
{
    Handle file;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&file, archive, path, FS_OPEN_READ, 0))) return 0;

    u64 size;
    FSFILE_GetSize(file, &size);
    if(size != 0)
    {
        *buf = calloc(1, size);
        FSFILE_Read(file, NULL, 0, *buf, size);
    }
    FSFILE_Close(file);
    return (u32)size;
}

static u32 zip_to_buf(struct archive *a, char *file_name, char ** buf)
{
    struct archive_entry *entry;

    bool found = false;
    u64 file_size = 0;

    while(!found && archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        found = !strcasecmp(archive_entry_pathname(entry), file_name);
    }

    if(found)
    {
        file_size = archive_entry_size(entry);
        *buf = calloc(file_size, sizeof(char));
        archive_read_data(a, *buf, file_size);
    }
    else
    {
        DEBUG("Couldn't find file in zip\n");
    }

    archive_read_free(a);

    return (u32)file_size;
}

u32 zip_memory_to_buf(char *file_name, void * zip_memory, size_t zip_size, char ** buf)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_memory, zip_size);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened from memory\n");
        return 0;
    }

    return zip_to_buf(a, file_name, buf);
}

u32 zip_file_to_buf(char *file_name, u16 *zip_path, char **buf)
{
    ssize_t len = strulen(zip_path, 0x106);
    char *path = calloc(sizeof(char), len*sizeof(u16));
    utf16_to_utf8((u8*)path, zip_path, len*sizeof(u16));

    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_filename(a, path, 0x4000);
    free(path);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened\n");
        return 0;
    }

    return zip_to_buf(a, file_name, buf);
}

Result buf_to_file(u32 size, FS_Path path, FS_Archive archive, char *buf)
{
    Handle handle;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&handle, archive, path, FS_OPEN_WRITE, 0))) return res;
    if (R_FAILED(res = FSFILE_Write(handle, NULL, 0, buf, size, FS_WRITE_FLUSH))) return res;
    if (R_FAILED(res = FSFILE_Close(handle))) return res;
    return 0;
}

u32 decompress_lz_file(FS_Path file_name, FS_Archive archive, char **buf)
{
    Handle handle;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&handle, archive, file_name, FS_OPEN_READ, 0))) {
        DEBUG("%lu\n", res);
        return 0;
    }
    u64 size;
    FSFILE_GetSize(handle, &size);

    char *temp_buf = NULL;

    if(size != 0)
    {
        temp_buf = calloc(1, size);
        FSFILE_Read(handle, NULL, 0, temp_buf, size);
    }
    FSFILE_Close(handle);

    if (temp_buf[0] != 0x11) {
        free(temp_buf);
        return 0;
    }

    u32 output_size = temp_buf[1] | ((temp_buf[2] << 8) & 0xFF00) | ((temp_buf[3] << 16) & 0xFF0000);
    printf("%ld\n", output_size);


    *buf = calloc(1, output_size);

    u32 pos = 4;
    u32 cur_written = 0;
    u8 counter = 0;
    u8 mask = 0;

    while (cur_written < output_size)
    {
        if (counter == 0) // read mask
        {
            mask = temp_buf[pos++];
            counter++;
            continue;
        }

        if ((mask >> (8 - counter)) & 0x01) // compressed block
        {
            int len = 0;
            int disp = 0;
            switch (temp_buf[pos] >> 4)
            {
                case 0:
                    len  = temp_buf[pos++] << 4;
                    len |= temp_buf[pos] >> 4;
                    len += 0x11;
                    break;

                case 1:
                    len  = (temp_buf[pos++] & 0x0F) << 12;
                    len |=  temp_buf[pos++] << 4;
                    len |=  temp_buf[pos] >> 4;
                    len += 0x111;
                    break;

                default:
                    len  = (temp_buf[pos] >> 4) + 1;
            }

            disp  = (temp_buf[pos++] & 0x0F) << 8;
            disp |=  temp_buf[pos++];

            for (int i = 0; i < len; ++i)
            {
                *(*buf + cur_written + i) = *(*buf + cur_written - disp - 1 + i);
            }

            cur_written += len;
        }
        else // byte literal
        {
            *(*buf + cur_written) = temp_buf[pos++];
            cur_written++;
        }

        if (++counter > 8) counter = 0;

    }


    free(temp_buf);

    return cur_written;
}

// This is an awful algorithm to "compress" LZ11 data.
// We do this instead of actual LZ11 compression because of time -
// LZ11 requires a lot of mem searching which is painfully slow on 3DS.
// This process is nearly instant but means the resulting file is actually larger
// than the input file. I don't think this is a problem as the only file we compress like this
// is the theme data installed to extdata in some rare cases, which means only 1 file at most
// is ever compressed like this. I don't think 400 KB is that big a sacrifice for probably
// half a minute or so of time save - I may change my mind on this in the future, especially
// if i figure out a dynamic programming algorithm which ends up being significantly
// faster. Otherwise, I think this is probably a fine implementation.

u32 compress_lz_file_fast(FS_Path path, FS_Archive archive, char *in_buf, u32 size)
{
    char *output_buf = calloc(1, size * 2);
    u32 output_size = 0;
    u32 mask_pos = 0;
    u32 bytes_processed = 0;
    u8 counter = 0;

    if (output_buf == NULL) return 0;

    // Set header data for the LZ11 file - 0x11 is version (LZ11), next 3 bytes are size
    output_buf[0] = 0x11;
    output_buf[3] = (size & 0xFF0000) >> 16;
    output_buf[2] = (size & 0xFF00) >> 8;
    output_buf[1] = (size & 0xFF);

    output_size += 4;

    while (bytes_processed < size)
    {
        if (counter == 0)
        {
            mask_pos = output_size++;
            output_buf[mask_pos] = 0;
        }

        output_buf[output_size++] = in_buf[bytes_processed++];

        if (++counter == 8) counter = 0;
    }

    buf_to_file(output_size, path, archive, output_buf);
    free(output_buf);

    return output_size;
}

void remake_file(FS_Path path, FS_Archive archive, u32 size)
{
    Handle handle;
    if (R_SUCCEEDED(FSUSER_OpenFile(&handle, archive, path, FS_OPEN_READ, 0)))
    {
        FSFILE_Close(handle);
        FSUSER_DeleteFile(archive, path);
    }
    FSUSER_CreateFile(archive, path, 0, size);
    char *buf = calloc(size, 1);
    buf_to_file(size, path, archive, buf);
    free(buf);
}
