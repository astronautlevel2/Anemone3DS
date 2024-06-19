/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2020 Contributors in CONTRIBUTORS.md
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
#include "draw.h"
#include "unicode.h"
#include "ui_strings.h"
#include "remote.h"

#include <archive.h>
#include <archive_entry.h>

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;
FS_Archive ArchiveBadgeExt;

Result createExtSaveData(u32 extdataID)
{
    u8 null_smdh[0x36C0] = {0};
    Handle *handle = fsGetSessionHandle();

    u32 *cmdbuf = getThreadCommandBuffer();

    u32 directory_limit = 1000;
    u32 file_limit = 1000;

    cmdbuf[0] = 0x08300182;
    cmdbuf[1] = MEDIATYPE_SD;
    cmdbuf[2] = extdataID;
    cmdbuf[3] = 0;
    cmdbuf[4] = 0x36C0;
    cmdbuf[5] = directory_limit;
    cmdbuf[6] = file_limit;
    cmdbuf[7] = (0x36C0 << 4) | 0xA;
    cmdbuf[8] = (u32)&null_smdh;

    Result ret = 0;
    if ((ret = svcSendSyncRequest(*handle)))
        return ret;

    return cmdbuf[1];
}

Result init_sd(void)
{
    Result res;
    if(R_FAILED(res = FSUSER_OpenArchive(&ArchiveSD, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return res;
    load_config();

    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE  "/cache"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/" APP_TITLE "/BadgeBackups"), FS_ATTRIBUTE_DIRECTORY);

    return 0;
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
        case 5:
            archive1 = 0x000002cf;
            archive2 = 0x000000a9;
            break;
        default:
            archive1 = 0x00;
            archive2 = 0x00;
    }

    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, main_paths[REMOTE_MODE_THEMES]), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, main_paths[REMOTE_MODE_SPLASHES]), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, main_paths[REMOTE_MODE_BADGES]), FS_ATTRIBUTE_DIRECTORY);

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

Result open_badge_extdata()
{
    Handle test_handle;
    FS_Path badge;

    Result res = 0;

    u32 badgePath[3] = {MEDIATYPE_SD, 0x000014d1, 0};
    badge.type = PATH_BINARY;
    badge.size = 0xC;

    badge.data = badgePath;
    if(R_FAILED(res = FSUSER_OpenArchive(&ArchiveBadgeExt, ARCHIVE_EXTDATA, badge)))
    {
        if (R_SUMMARY(res) == RS_NOTFOUND) 
        {
            DEBUG("Extdata not found - creating\n");
            createExtSaveData(0x000014d1);
            FSUSER_OpenArchive(&ArchiveBadgeExt, ARCHIVE_EXTDATA, badge);
        } else
        {
            DEBUG("Unknown extdata error\n");
            return res;
        }
    }

    if (R_FAILED(res = FSUSER_OpenFile(&test_handle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeData.dat"), FS_OPEN_READ, 0)))
    {
        if (R_SUMMARY(res) == RS_NOTFOUND)
        {
            FSUSER_CreateFile(ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeData.dat"), 0, BADGE_DATA_SIZE);
            FSUSER_OpenFile(&test_handle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeData.dat"), FS_OPEN_WRITE, 0);
            FSFILE_Flush(test_handle);
        }
        DEBUG("Error 0x%08ld opening BadgeData.dat, retrying\n", res);
    }
    FSFILE_Close(test_handle);

    if(R_FAILED(res = FSUSER_OpenFile(&test_handle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), FS_OPEN_READ, 0)))
    {
        DEBUG("Error 0x%08ld opening BadgeMngFile.dat, retrying\n", res);
        if (R_SUMMARY(res) == RS_NOTFOUND)
        {
            remake_file(fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), ArchiveBadgeExt, BADGE_MNG_SIZE);
        }
    }
    FSFILE_Close(test_handle);

    char tp_path[0x106] = {0};
    sprintf(tp_path, "%sThemePlaza Badges", main_paths[REMOTE_MODE_BADGES]);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, tp_path), FS_ATTRIBUTE_DIRECTORY);
    strcat(tp_path, "/_seticon.png");

    if(R_FAILED(res = FSUSER_OpenFile(&test_handle, ArchiveSD, fsMakePath(PATH_ASCII, tp_path), FS_OPEN_READ, 0)))
    {
        FILE *fp = fopen("romfs:/tp_set.png", "rb");
        fseek(fp, 0L, SEEK_END);
        ssize_t size = ftell(fp);
        char *icon_buf = malloc(size);
        fseek(fp, 0L, SEEK_SET);
        fread(icon_buf, 1, size, fp);
        fclose(fp);
        remake_file(fsMakePath(PATH_ASCII, tp_path), ArchiveSD, size);
        buf_to_file(size, fsMakePath(PATH_ASCII, tp_path), ArchiveSD, icon_buf);
        DEBUG("res: 0x%08lx\n", res);
        free(icon_buf);
    }

    return 0;
}

Result close_archives(void)
{
    Result res;

    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveSD))) return res;
    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveHomeExt))) return res;
    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveThemeExt))) return res;
    if(R_FAILED(res = FSUSER_CloseArchive(ArchiveBadgeExt))) return res;

    return 0;
}

Result load_parental_controls(Parental_Restrictions_s *restrictions)
{
    char parental_data[0xC0] = {0};
    Result res;

    if (R_FAILED(res = CFGU_GetConfigInfoBlk2(0xC0, 0x000C0000, &parental_data))) return res;
    memcpy(restrictions, parental_data, 4);

    return 0;
}

u32 file_to_buf(FS_Path path, FS_Archive archive, char ** buf)
{
    Handle file;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&file, archive, path, FS_OPEN_READ, 0)))
    {
        DEBUG("file_to_buf failed - 0x%08lx\n", res);
        return 0;
    }

    u64 size;
    FSFILE_GetSize(file, &size);
    if(size != 0)
    {
        *buf = calloc(1, size);
        if (*buf == NULL)
        {
            DEBUG("Error allocating buffer - out of memory??\n");
            return 0;
        }
        FSFILE_Read(file, NULL, 0, *buf, size);
    }
    FSFILE_Close(file);
    return (u32)size;
}

s16 for_each_file_zip(u16 *zip_path, u32 (*zip_iter_callback)(char *filebuf, u64 file_size, const char *name, void *userdata), void *userdata)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    ssize_t len = strulen(zip_path, 0x106);
    char * path = calloc(len, sizeof(u16));
    utf16_to_utf8((u8 *)path, zip_path, len * sizeof(u16));
    DEBUG("Attempting to open zip %s\n", path);

    int r = archive_read_open_filename(a, path, 0x4000);
    free(path);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened\n");
        char path[0x128] = {0};
        utf16_to_utf8((u8 *) path, zip_path, 0x128);
        DEBUG("%s\n", path);
        return -1;
    }

    struct archive_entry *entry;
    u64 file_size = 0;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        file_size = archive_entry_size(entry);
        char *buf = calloc(file_size, sizeof(char));
        archive_read_data(a, buf, file_size);
        zip_iter_callback(buf, file_size, archive_entry_pathname(entry), userdata);
        free(buf);
    }

    archive_read_free(a);
    return 0;
}

static u32 zip_to_buf(struct archive * a, const char * file_name, char ** buf)
{
    struct archive_entry * entry;

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

u32 zip_memory_to_buf(const char * file_name, void * zip_memory, size_t zip_size, char ** buf)
{
    struct archive * a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_memory, zip_size);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened from memory\n");
        return 0;
    }

    return zip_to_buf(a, file_name, buf);
}

u32 zip_file_to_buf(const char * file_name, const u16 * zip_path, char ** buf)
{
    ssize_t len = strulen(zip_path, 0x106);
    char * path = calloc(len, sizeof(u16));
    utf16_to_utf8((u8 *)path, zip_path, len * sizeof(u16));

    struct archive * a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_filename(a, path, 0x4000);
    free(path);
    if(r != ARCHIVE_OK)
    {
        DEBUG("Invalid zip being opened\n");
        char path[0x128] = {0};
        utf16_to_utf8((u8 *) path, zip_path, 0x128);
        DEBUG("%s\n", path);
        return 0;
    }

    return zip_to_buf(a, file_name, buf);
}

Result buf_to_file(u32 size, FS_Path path, FS_Archive archive, char * buf)
{
    Handle handle;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&handle, archive, path, FS_OPEN_WRITE, 0))) return res;
    if (R_FAILED(res = FSFILE_Write(handle, NULL, 0, buf, size, FS_WRITE_FLUSH))) return res;
    if (R_FAILED(res = FSFILE_Close(handle))) return res;
    return 0;
}

u32 decompress_lz_file(FS_Path file_name, FS_Archive archive, char ** buf)
{
    Handle handle;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&handle, archive, file_name, FS_OPEN_READ, 0))) {
        DEBUG("%lu\n", res);
        return 0;
    }
    u64 size;
    FSFILE_GetSize(handle, &size);

    char * temp_buf = NULL;

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

u32 compress_lz_file_fast(FS_Path path, FS_Archive archive, char * in_buf, u32 size)
{
    char * output_buf = calloc(1, size * 2);
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
    Result res = FSUSER_CreateFile(archive, path, 0, size);
    DEBUG("Remake file res: 0x%08lx\n", res);
    char * buf = calloc(size, 1);
    if (buf == NULL)
    {
        DEBUG("out of memory - not overwriting file?\n");
    } else {
        buf_to_file(size, path, archive, buf);
        free(buf);
    }
}

Result zero_handle_memeasy(Handle handle)
{
    u64 size = 0;
    u64 cur = 0;
    FSFILE_GetSize(handle, &size);
    char *zero_buf = calloc(1, 0x10000);
    while (size > 0x10000)
    {
        FSFILE_Write(handle, NULL, cur, &zero_buf, 0x10000, 0);
        cur += 0x10000;
        size -= 0x10000;
    }
    FSFILE_Write(handle, NULL, cur, &zero_buf, size, 0);
    free(zero_buf);
    return 0;
}

static SwkbdCallbackResult fat32filter(void * user, const char ** ppMessage, const char * text, size_t textlen)
{
    (void)textlen;
    (void)user;

    *ppMessage = language.fs.illegal_input;
    if(strpbrk(text, ILLEGAL_CHARS))
    {
        DEBUG("illegal filename: %s\n", text);
        return SWKBD_CALLBACK_CONTINUE;
    }

    return SWKBD_CALLBACK_OK;
}

// assumes the input buffer is a ZIP. if it isn't, why are you calling this?
void save_zip_to_sd(char * filename, u32 size, char * buf, RemoteMode mode)
{
    static char path_to_file[32761]; // FAT32 paths can be quite long.
    const int max_chars = 250;
    char new_filename[max_chars + 5]; // .zip + \0
renamed:
    char * curr_filename;
    if (mode == REMOTE_MODE_BADGES)
    {
        sprintf(path_to_file, "%sThemePlaza Badges/%s", main_paths[REMOTE_MODE_BADGES], filename);
        DEBUG("Remote mode badges! Saving to %s/\n", path_to_file);
        curr_filename = path_to_file + strlen(main_paths[REMOTE_MODE_BADGES]) + strlen("ThemePlaza Badges/");
    } else
    {
        sprintf(path_to_file, "%s%s", main_paths[mode], filename);
        curr_filename = path_to_file + strlen(main_paths[mode]);
    }

    DEBUG("Filtering out illegal chars...\n");
    // filter out characters illegal in FAT32 filenames
    char * illegal_char = curr_filename;
    while ((illegal_char = strpbrk(illegal_char, ILLEGAL_CHARS)))
    {
        DEBUG("Illegal char found in filename: %c\n", *illegal_char);
        if (*illegal_char == '.')
        {
            // skip initial . (this is allowed)
            if (illegal_char == curr_filename)
                continue;
            // skip extension delimiter
            if (strpbrk(illegal_char + 1, ".") == NULL)
            {
                illegal_char++;
                continue;
            }
        }
        *illegal_char = '-';
    }

    DEBUG("Checking extension\n");
    // ensure the extension is .zip
    char * extension = strrchr(path_to_file, '.');
    if (extension == NULL || strcmp(extension, ".zip"))
        strcat(path_to_file, ".zip");

    DEBUG("path: %s\n", path_to_file);
    u16 utf16path[0x106] = {0};
    utf8_to_utf16(utf16path, (u8 *) path_to_file, 0x106);
    FS_Path path = fsMakePath(PATH_UTF16, utf16path);

    // check if file already exists, and if it does, prompt the user
    // to overwrite or change name (or exit)
    Result res = FSUSER_CreateFile(ArchiveSD, path, 0, size);
    if (R_FAILED(res))
    {
        if (res == (long)0xC82044BE)
        {
            DEBUG("File already exists\n");

            SwkbdState swkbd;

            swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 3, max_chars / 2);
            swkbdSetHintText(&swkbd, language.fs.new_or_overwrite);
            swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT | SWKBD_DARKEN_TOP_SCREEN);

            swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, language.fs.cancel, false);
            swkbdSetButton(&swkbd, SWKBD_BUTTON_MIDDLE, language.fs.overwrite, false);
            swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, language.fs.rename, true);
            swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_CALLBACK, -1);
            swkbdSetFilterCallback(&swkbd, &fat32filter, NULL);

            SwkbdButton button = swkbdInputText(&swkbd, new_filename, max_chars);

            switch (button)
            {
            case SWKBD_BUTTON_RIGHT:
                DEBUG("Renaming to %s\n", new_filename);
                strcat(new_filename, ".zip");
                filename = new_filename;
                goto renamed;
            case SWKBD_BUTTON_MIDDLE:
                // we good
                DEBUG("Overwriting %s\n", filename);
                break;
            case SWKBD_BUTTON_LEFT:
                // do nothing
                DEBUG("File rename cancelled\n");
                return;
            case SWKBD_BUTTON_NONE:
                DEBUG("SWKBD broke wtf??? :- %x\n", swkbdGetResult(&swkbd));
                return throw_error(language.fs.swkbd_fail, ERROR_LEVEL_WARNING);
            }
        }
        else if (res == (long)0xC86044D2)
        {
            DEBUG("SD card is full\n");
            return throw_error(language.fs.sd_full, ERROR_LEVEL_WARNING);
        }
        else
        {
            DEBUG("error: %lx\n", res);
            return throw_error(language.fs.fs_error, ERROR_LEVEL_ERROR);
        }
    }

    DEBUG("Saving to SD: %s\n", path_to_file);
    remake_file(path, ArchiveSD, size);
    buf_to_file(size, path, ArchiveSD, buf);
}
