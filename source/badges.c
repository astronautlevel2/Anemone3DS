/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2024 Contributors in CONTRIBUTORS.md
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

// Badge implementation adapted from GYTB by MrCheeze and ABE by AntiMach
// https://github.com/AntiMach/advanced-badge-editor
// https://github.com/MrCheeze/GYTB

#include "badges.h"
#include "draw.h"
#include "ui_strings.h"

Handle badgeDataHandle;
char *badgeMngBuffer;
u16 *rgb_buf_64x64;
u16 *rgb_buf_32x32;
u8 *alpha_buf_64x64;
u8 *alpha_buf_32x32;
u64 progress_finish;
u64 progress_status;

void remove_exten(u16 *filename)
{
    for (int i = 0; i < strulen(filename, 0x8A); ++i)
    {
        if (filename[i] == '.') 
        {
            filename[i] = 0x00;
            return;
        }
    }
}

u64 getShortcut(char *filename)
{
    u64 shortcut = 0xFFFFFFFFFFFFFFFF;
    char *p1 = strchr(filename, '.');

    if (p1 == NULL) return shortcut;
    ++p1;

    char *p2 = strchr(p1, '.');
    if (p2 == NULL) return shortcut;
    if (p2-p1 != 8) return shortcut;

    unsigned int lowpath;
    if (sscanf(p1, "%08x", &lowpath) != 1) return shortcut;

    shortcut = 0x0004001000000000 + lowpath;
    DEBUG("Shortcut %16llx found for %s\n", shortcut, filename);
    return shortcut;
}

int install_badge_generic(char *file_buf, u64 file_size, u16 *name, int *badge_count, int set_id)
{
    int badges_in_image = pngToRGB565(file_buf, file_size, rgb_buf_64x64, alpha_buf_64x64, rgb_buf_32x32, alpha_buf_32x32, false);
    
    char utf8_name[512] = {0};
    utf16_to_utf8((u8 *) utf8_name, name, 0x8A);
    u64 shortcut = getShortcut(utf8_name);
    int badges_installed = 0;

    for (int badge = 0; badge < badges_in_image && *badge_count < 1000; ++badge)
    {
        remove_exten(name);
        for (int j = 0; j < 16; ++j) // Copy name for all 16 languages
        {
            FSFILE_Write(badgeDataHandle, NULL, 0x35E80 + *badge_count * 16 * 0x8A + j * 0x8A, name, 0x8A, 0);
        }
        FSFILE_Write(badgeDataHandle, NULL, 0x318F80 + *badge_count * 0x2800, rgb_buf_64x64 + badge * 64 * 64, 64 * 64 * 2, 0);
        FSFILE_Write(badgeDataHandle, NULL, 0x31AF80 + *badge_count * 0x2800, alpha_buf_64x64 + badge * 64 * 64/2, 64 * 64/2, 0);
        FSFILE_Write(badgeDataHandle, NULL, 0xCDCF80 + *badge_count * 0xA00, rgb_buf_32x32 + badge * 32 * 32, 32 * 32 * 2, 0);
        FSFILE_Write(badgeDataHandle, NULL, 0xCDD780 + *badge_count * 0xA00, alpha_buf_32x32 + badge * 32 * 32/2, 32 * 32/2, 0);

        int badge_id = *badge_count + 1;
        memcpy(badgeMngBuffer + 0x3E8 + *badge_count * 0x28 + 0x4, &badge_id, 4);
        memcpy(badgeMngBuffer + 0x3E8 + *badge_count * 0x28 + 0x8, &set_id, 4);
        memcpy(badgeMngBuffer + 0x3E8 + *badge_count*0x28 + 0xC, badge_count, 2);
        badgeMngBuffer[0x3E8 + *badge_count*0x28 + 0x12] = 255; // Quantity Low
        badgeMngBuffer[0x3E8 + *badge_count*0x28 + 0x13] = 255; // Quantity High
        
        memcpy(badgeMngBuffer + 0x3E8 + *badge_count*0x28 + 0x18, &shortcut, 8);
        memcpy(badgeMngBuffer + 0x3E8 + *badge_count*0x28 + 0x20, &shortcut, 8); // u64 shortcut[2], not sure what second is for

        badgeMngBuffer[0x358 + *badge_count/8] |= 1 << (*badge_count % 8); // enabled badges bitfield
        badges_installed++;

        *badge_count += 1;
    }

    return badges_installed;
}

int install_badge_png(FS_Path badge_path, FS_DirectoryEntry badge_file, int *badge_count, int set_id)
{
    u64 res;
    char *file_buf = NULL;
    res = file_to_buf(badge_path, ArchiveSD, &file_buf);
    if (res != badge_file.fileSize)
    {
        return -1;
    }

    int badges = install_badge_generic(file_buf, badge_file.fileSize, badge_file.name, badge_count, set_id);
    free(file_buf);
    return badges;
}

typedef struct {
    int *badge_count;
    int set_id;
    int installed;
} zip_userdata;

u32 zip_callback(char *file_buf, u64 file_size, const char *name, void *userdata)
{
    progress_finish += 1;
    zip_userdata *data = (zip_userdata *) userdata;
    u16 *utf16_name = calloc(strlen(name), sizeof(u16));
    utf8_to_utf16(utf16_name, (u8 *) name, strlen(name));
    data->installed += install_badge_generic(file_buf, file_size, utf16_name, data->badge_count, data->set_id);
    free(utf16_name);
    progress_status += 1;

    return 0;
}

int install_badge_zip(u16 *path, int *badge_count, int set_id)
{
    zip_userdata data = {0};
    data.set_id = set_id;
    data.badge_count = badge_count;
    for_each_file_zip(path, zip_callback, &data);

    return data.installed;
}

int install_badge_dir(FS_DirectoryEntry set_dir, int *badge_count, int set_id)
{
    Result res;
    Handle folder;
    int start_idx = *badge_count;
    char *icon_buf = NULL;
    int icon_size = 0;
    
    FS_DirectoryEntry *badge_files = calloc(1024, sizeof(FS_DirectoryEntry));
    u16 path[512] = {0};
    u16 set_icon[17] = {0};
    utf8_to_utf16(set_icon, (u8 *) "_seticon.png", 16);
    struacat(path, main_paths[REMOTE_MODE_BADGES]);
    strucat(path, set_dir.name);
    res = FSUSER_OpenDirectory(&folder, ArchiveSD, fsMakePath(PATH_UTF16, path));
    if (R_FAILED(res))
    {
        return -1;
    }
    u32 entries_read;
    res = FSDIR_Read(folder, &entries_read, 1024, badge_files);
    int badges_in_set = 0;
    progress_finish += entries_read;
    for (u32 i = 0; i < entries_read && *badge_count < 1000; ++i)
    {
        if (!strcmp(badge_files[i].shortExt, "PNG"))
        {
            memset(path, 0, 512 * sizeof(u16));
            struacat(path, main_paths[REMOTE_MODE_BADGES]);
            strucat(path, set_dir.name);
            struacat(path, "/");
            strucat(path, badge_files[i].name);
            if (!memcmp(set_icon, badge_files[i].name, 16))
            {
                DEBUG("Found set icon for folder set %d\n", set_id);
                icon_size = file_to_buf(fsMakePath(PATH_UTF16, path), ArchiveSD, &icon_buf);
                continue;
            }
            badges_in_set += install_badge_png(fsMakePath(PATH_UTF16, path), badge_files[i], badge_count, set_id);
        } else if (!strcmp(badge_files[i].shortExt, "ZIP"))
        {
            memset(path, 0, 512 * sizeof(u16));
            struacat(path, main_paths[REMOTE_MODE_BADGES]);
            strucat(path, set_dir.name);
            struacat(path, "/");
            strucat(path, badge_files[i].name);
            badges_in_set += install_badge_zip(path, badge_count, set_id);
        }
        progress_status += 1;
        draw_loading_bar(progress_status, progress_finish, INSTALL_BADGES);
    }

    if (!badges_in_set)
    {
        goto end;
    }

    int set_index = set_id - 1;
    u32 total_count = 0xFFFF * badges_in_set;
    for (int i = 0; i < 16; ++i)
    {
        FSFILE_Write(badgeDataHandle, NULL, set_index * 0x8A0 + i * 0x8A, set_dir.name, strulen(set_dir.name, 0x45) * 2 + 2, 0);
    }
    badgeMngBuffer[0x3D8 + set_index/8] |= 0 << (set_index % 8);

    memset(badgeMngBuffer + 0xA028 + set_index * 0x30, 0xFF, 8);
    badgeMngBuffer[0xA028 + 0xC + set_index * 0x30] = 0x10; // bytes 13 and 14 are 0x2710
    badgeMngBuffer[0xA028 + 0xD + set_index * 0x30] = 0x27;
    memcpy(badgeMngBuffer + 0xA028 + 0x10 + set_index * 0x30, &set_id, 4);
    memcpy(badgeMngBuffer + 0xA028 + 0x14 + set_index * 0x30, &set_index, 4);
    memset(badgeMngBuffer + 0xA028 + 0x18 + set_index * 0x30, 0xFF, 4);
    memcpy(badgeMngBuffer + 0xA028 + 0x1C + set_index * 0x30, &badges_in_set, 4);
    memcpy(badgeMngBuffer + 0xA028 + 0x20 + set_index * 0x30, &total_count, 4);
    memcpy(badgeMngBuffer + 0xA028 + 0x24 + set_index * 0x30, &start_idx, 4);

    int icon = pngToRGB565(icon_buf, icon_size, rgb_buf_64x64, alpha_buf_64x64, rgb_buf_32x32, alpha_buf_32x32, true);

    if (icon == 0)
    {
        DEBUG("Falling back on default icon\n");
        if (icon_buf) free(icon_buf);
        FILE *fp = fopen("romfs:/hb_set.png", "rb");
        fseek(fp, 0L, SEEK_END);
        icon_size = ftell(fp);
        icon_buf = malloc(icon_size);
        fseek(fp, 0L, SEEK_SET);
        fread(icon_buf, 1, icon_size, fp);
        fclose(fp);
        pngToRGB565(icon_buf, icon_size, rgb_buf_64x64, alpha_buf_64x64, rgb_buf_32x32, alpha_buf_32x32, true);
    }

    free(icon_buf);
    FSFILE_Write(badgeDataHandle, NULL, 0x250F80 + set_index * 0x2000, rgb_buf_64x64, 64 * 64 * 2, 0);
    badgeMngBuffer[0x3D8 + set_index/8] |= 0 << (set_index % 8);
    end:
    free(badge_files);
    FSDIR_Close(folder);
    return badges_in_set;
}

typedef struct set_node_s {
    u32 set_id;
    u32 set_index;
    struct set_node_s *next;
} SetNode;

u32 get_set_index(SetNode *head, u32 set_id)
{
    SetNode *cursor = head;
    while (cursor != NULL)
    {
        if (cursor->set_id == set_id)
            return cursor->set_index;
        cursor = cursor->next;
    }
    return 0xFFFFFFFF;
}

void free_list(SetNode *head)
{
    SetNode *cursor = head;
    while (cursor)
    {
        SetNode *next = cursor->next;
        free(cursor);
        cursor = next;
    }
}

SetNode * extract_sets(char *badgeMngBuffer, Handle backupDataHandle)
{
    u32 setCount = *((u32 *) (badgeMngBuffer + 0x4));

    if (!setCount) // GYTB? make unknown set
    {
        u16 set_path[256] = {0};
        struacat(set_path, "/3ds/" APP_TITLE "/BadgeBackups/Unknown Set");
        FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_UTF16, set_path), FS_ATTRIBUTE_DIRECTORY);
        return NULL;
    }

    SetNode *head = calloc(1, sizeof(SetNode));
    SetNode *cursor = head;

    u16 *icon_rgb_buf = malloc(64 * 64 * 2);
    u8 *icon_alpha_buf = malloc(64 * 64 * 0.5);

    for (u32 i = 0; i < setCount; ++i)
    {
        memcpy(&(cursor->set_id), &badgeMngBuffer[0xA028 + 0x30 * i + 0x10], 4);
        memcpy(&(cursor->set_index), &badgeMngBuffer[0xA028 + 0x30 * i + 0x14], 4);

        if (cursor->set_id != 0xEFBE) // 0xEFBE is GYTB Set ID; GYTB doesn't properly create sets, so skip
        {
            DEBUG("Processing icon for set %lu at index %lu\n", cursor->set_id, cursor->set_index);
            u16 utf16SetName[0x46] = {0};
            FSFILE_Read(backupDataHandle, NULL, cursor->set_index * 16 * 0x8A, utf16SetName, 0x8A);
            replace_chars(utf16SetName, ILLEGAL_CHARS, u'-');
            u16 set_path[256] = {0};
            struacat(set_path, "/3ds/" APP_TITLE "/BadgeBackups/");
            size_t set_name_len = strucat(set_path, utf16SetName);
            if (!set_name_len)
            {
                struacat(set_path, "Unknown Set");
            }
            FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_UTF16, set_path), FS_ATTRIBUTE_DIRECTORY);
            memset(icon_alpha_buf, 255, 64 * 64 * 0.5);
            FSFILE_Read(backupDataHandle, NULL, 0x250F80 + cursor->set_index * 0x2000, icon_rgb_buf, 0x2000);
            char filename[256] = {0};
            utf16_to_utf8((u8 *) filename, set_path, 256);
            strcat(filename, "/_seticon.png");
            DEBUG("%s\n", filename);
            rgb565ToPngFile(filename, icon_rgb_buf, icon_alpha_buf, 48, 48);
        }

        cursor->next = calloc(1, sizeof(SetNode));
        cursor = cursor->next;
    }

    free(icon_rgb_buf);
    free(icon_alpha_buf);

    return head;
}

Result extract_badges(void)
{
    DEBUG("Dumping installed badges...\n");
    char *badgeMngBuffer = NULL;
    u32 size = file_to_buf(fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), ArchiveBadgeExt, &badgeMngBuffer);
    DEBUG("%lu bytes read\n", size);

    Result res = 0;
    SetNode *head;
    Handle backupDataHandle;
    u16 *badge_rgb_buf = malloc(64 * 64 * 2);
    u8 *badge_alpha_buf = malloc(64 * 64 * 0.5);
    u32 badge_count = 0;
    memcpy(&badge_count, badgeMngBuffer + 0x8, 4);
    DEBUG("%lu badges found\n", badge_count);
    if (badge_count > 0)
    {
        res = FSUSER_OpenFile(&backupDataHandle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeData.dat"), FS_OPEN_READ, 0);
        if (R_FAILED(res))
        {
            free(badgeMngBuffer);
            free(badge_rgb_buf);
            free(badge_alpha_buf);
            char err_string[128] = {0};
            sprintf(err_string, language.badges.extdata_locked, res);
            throw_error(err_string, ERROR_LEVEL_WARNING);
            DEBUG("backupDataHandle open failed\n");
            return -1;
        }
        head = extract_sets(badgeMngBuffer, backupDataHandle);
    } else {
        free(badgeMngBuffer);
        free(badge_rgb_buf);
        free(badge_alpha_buf);
        return 0;
    }
    for (u32 i = 0; i < badge_count; ++i)
    {
        u32 badgeId;
        memcpy(&badgeId, badgeMngBuffer + 0x3E8 + i * 0x28 + 0x4, 4);
        u32 badgeSetId;
        memcpy(&badgeSetId, badgeMngBuffer + 0x3E8 + i * 0x28 + 0x8, 4);
        u16 badgeSubId;
        memcpy(&badgeSubId, badgeMngBuffer + 0x3E8 + i * 0x28 + 0xE, 2);
        u32 shortcut;
        memcpy(&shortcut, badgeMngBuffer + 0x3E8 + i * 0x28 + 0x18, 4);

        char filename[512] = {0};

        u16 utf16Name[0x46] = {0};
        FSFILE_Read(backupDataHandle, NULL, 0x35E80 + i * 16 * 0x8A, utf16Name, 0x8A);
        replace_chars(utf16Name, ILLEGAL_CHARS, u'-');
        char utf8Name[256] = {0};
        res = utf16_to_utf8((u8 *) utf8Name, utf16Name, 256);

        char dir[256] = "/3ds/" APP_TITLE "/BadgeBackups/";
        if (badgeSetId != 0xEFBE) // 0xEFBE is GYTB Set ID; GYTB doesn't properly create sets, so skip
        {
            u32 set_index = get_set_index(head, badgeSetId);
            if (set_index == 0xFFFFFFFF) {
                sprintf(dir, "/3ds/" APP_TITLE "/BadgeBackups/Unknown Set");
            } else
            {
                u16 utf16SetName[0x46] = {0};
                FSFILE_Read(backupDataHandle, NULL, set_index * 16 * 0x8A, utf16SetName, 0x8A);
                replace_chars(utf16SetName, ILLEGAL_CHARS, u'-');
                char utf8SetName[128] = {0};
                res = utf16_to_utf8((u8 *) utf8SetName, utf16SetName, 128);
                if (!res)
                    strncpy(utf8SetName, "Unknown Set", 128);
                DEBUG("UTF-8 Set Name: %s; ID: %lx\n", utf8SetName, badgeSetId);
                sprintf(dir, "/3ds/" APP_TITLE "/BadgeBackups/%s", utf8SetName);
            }
        } else
        {
            sprintf(dir, "/3ds/" APP_TITLE "/BadgeBackups/Unknown Set");
        }

        if (shortcut == 0xFFFFFFFF)
        {
            sprintf(filename, "%s/%s.%lx.%x.png", dir, utf8Name, badgeId, badgeSubId);
        } else
        {
            sprintf(filename, "%s/%s.%08lx.%lx.%x.png", dir, utf8Name, shortcut, badgeId, badgeSubId);
        }
        DEBUG("Dump filename: %s\n", filename);

        FSFILE_Read(backupDataHandle, NULL, 0x318F80 + i * 0x2800, badge_rgb_buf, 0x2000);
        FSFILE_Read(backupDataHandle, NULL, 0x318F80 + i * 0x2800 + 0x2000, badge_alpha_buf, 0x800);
        rgb565ToPngFile(filename, badge_rgb_buf, badge_alpha_buf, 64, 64);
        draw_loading_bar(i + 1, badge_count, INSTALL_DUMPING_BADGES);
    }

    free(badgeMngBuffer);
    free(badge_rgb_buf);
    free(badge_alpha_buf);
    free_list(head);
    FSFILE_Close(backupDataHandle);

    return res;
}

Result backup_badges_fast(void)
{
    char *badgeMng = NULL;

    DEBUG("writing badge data: making files...\n");
    char mng_path[128] = "/3ds/" APP_TITLE "/BadgeMngFile.dat";
    char data_path[128] = "/3ds/" APP_TITLE "/BadgeData.dat";
    DEBUG("mng_path: %s, data_path: %s\n", mng_path, data_path);

    Handle dataHandle = 0;
    Handle sdHandle = 0;

    DEBUG("loading existing badge mng file...\n");
    u32 mngRead = file_to_buf(fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), ArchiveBadgeExt, &badgeMng);
    DEBUG("loading existing badge data file\n");
    Result res = FSUSER_OpenFile(&dataHandle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeData.dat"), FS_OPEN_READ, 0);
    if (mngRead != BADGE_MNG_SIZE || R_FAILED(res))
    {
        char err_string[128] = {0};
        sprintf(err_string, language.badges.extdata_locked, res);
        throw_error(err_string, ERROR_LEVEL_WARNING);
        if (badgeMng) free(badgeMng);
        if (dataHandle) FSFILE_Close(dataHandle);
        FSFILE_Close(sdHandle);
        return -1;
    }
    remake_file(fsMakePath(PATH_ASCII, mng_path), ArchiveSD, BADGE_MNG_SIZE);

    FSUSER_CreateFile(ArchiveSD, fsMakePath(PATH_ASCII, data_path), 0, BADGE_DATA_SIZE);
    FSUSER_OpenFile(&sdHandle, ArchiveSD, fsMakePath(PATH_ASCII, data_path), FS_OPEN_WRITE, 0);

    DEBUG("writing badge data: writing BadgeMngFile...\n");
    res = buf_to_file(mngRead, fsMakePath(PATH_ASCII, mng_path), ArchiveSD, badgeMng);
    if (R_FAILED(res))
    {
        DEBUG("Failed to write badgemngfile: 0x%08lx\n", res);
        free(badgeMng);
        FSFILE_Close(dataHandle);
        FSFILE_Close(sdHandle);
        return -1;
    }
    DEBUG("writing badge data: writing badgedata...\n");
    char *buf = malloc(0x10000);
    u64 size = BADGE_DATA_SIZE;
    u64 cur = 0;
    while (size > 0)
    {
        u32 read = 0;
        res = FSFILE_Read(dataHandle, &read, cur, buf, min(0x10000, size));
        res = FSFILE_Write(sdHandle, NULL, cur, buf, read, FS_WRITE_FLUSH);
        size -= read;
        cur += read;
    }

    free(badgeMng);
    free(buf);
    FSFILE_Close(dataHandle);
    FSFILE_Close(sdHandle);
    return 0;
}

Result install_badges(void)
{
    Handle handle = 0;
    Handle folder = 0;
    Result res = 0;
    draw_loading_bar(0, 1, INSTALL_BADGES);
    {
        char testpath[128] = "/3ds/" APP_TITLE "/BadgeData.dat";
        if (R_FAILED(res = FSUSER_OpenFile(&handle, ArchiveSD, fsMakePath(PATH_ASCII, testpath), FS_OPEN_READ, 0)))
        {
            if (R_SUMMARY(res) == RS_NOTFOUND)
            {
                res = backup_badges_fast();
                if (R_FAILED(res)) return res;
            } else
            {
                DEBUG("????: 0x%08lx\n", res);
            }
        }
    }

    if (handle) FSFILE_Close(handle);

    DEBUG("Initializing ACT\n");
    res = actInit(true);
    if (R_FAILED(res))
    {
        DEBUG("actInit() failed!\n");
        return res;
    }

    DEBUG("Initializing ACT\n");
    res = ACT_Initialize(0xB0502C8, 0, 0);
    if (R_FAILED(res))
    {
        DEBUG("ACT_Initialize failed! %08lx\n", res);
        return res;
    }

    DEBUG("Getting NNID\n");
    u32 nnidNum = 0xFFFFFFFF;
    res = ACT_GetAccountInfo(&nnidNum, sizeof(nnidNum), ACT_DEFAULT_ACCOUNT, INFO_TYPE_PRINCIPAL_ID);
    if (R_FAILED(res))
    {
        DEBUG("ACT_GetAccountInfo failed! %08lx\n", res);
        return res;
    }
    DEBUG("NNID found: 0x%08lx\n", nnidNum);

    badgeMngBuffer = NULL;
    badgeDataHandle = 0;
    rgb_buf_64x64 = NULL;
    rgb_buf_32x32 = NULL;
    alpha_buf_64x64 = NULL;
    alpha_buf_32x32 = NULL;

    DEBUG("Opening badge directory\n");
    FS_DirectoryEntry *badge_files = calloc(1024, sizeof(FS_DirectoryEntry));
    res = FSUSER_OpenDirectory(&folder, ArchiveSD, fsMakePath(PATH_ASCII, main_paths[REMOTE_MODE_BADGES]));
    if (R_FAILED(res))
    {
        DEBUG("Failed to open folder: %lx\n", res);
        goto end;
    }

    u32 entries_read;
    res = FSDIR_Read(folder, &entries_read, 1024, badge_files);
    DEBUG("%lu files found\n", entries_read);
    rgb_buf_64x64 = malloc(12*6*64*64*2); //12x6 badges in sheet max, 64x64 pixel badges, 2 bytes per RGB data
    alpha_buf_64x64 = malloc(12*6*64*64/2); //Same thing, but 2 pixels of alpha data per byte
    rgb_buf_32x32 = malloc(12*6*32*32*2); //Same thing, but 32x32
    alpha_buf_32x32 = malloc(12*6*32*32/2);
    res = FSUSER_OpenFile(&badgeDataHandle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeData.dat"), FS_OPEN_WRITE, 0);
    badgeMngBuffer = calloc(1, BADGE_MNG_SIZE);

    if (!rgb_buf_64x64)
    {
        DEBUG("rgb_buf_64x64 alloc failed\n");
        goto end;
    }

    if (!alpha_buf_64x64)
    {
        DEBUG("alpha_buf_64x64 alloc failed\n");
        goto end;
    }

    if (!rgb_buf_32x32)
    {
        DEBUG("rgb_buf_32x32 alloc failed\n");
        goto end;
    }

    if (!alpha_buf_32x32)
    {
        DEBUG("alpha_buf_32x32 alloc failed\n");
        goto end;
    }

    if (R_FAILED(res))
    {
        badgeDataHandle = 0;
        char err_string[128] = {0};
        sprintf(err_string, language.badges.extdata_locked, res);
        throw_error(err_string, ERROR_LEVEL_WARNING);
        DEBUG("badgeDataHandle open failed\n");
        goto end;
    }

    if (!badgeMngBuffer)
    {
        DEBUG("badgeMngBuffer alloc failed\n");
        goto end;
    }

    zero_handle_memeasy(badgeDataHandle);

    int badge_count = 0;
    int set_count = 0;
    int default_set = 0;
    int default_set_count = 0;
    int default_idx = 0;

    progress_finish = entries_read + 12;
    progress_status = 12;
    draw_loading_bar(progress_status, progress_finish, INSTALL_BADGES);
    for (u32 i = 0; i < entries_read && badge_count < 1000; ++i)
    {
        if (!strcmp(badge_files[i].shortExt, "PNG"))
        {
            if (default_set == 0)
            {
                set_count += 1;
                default_set = set_count;
                default_idx = badge_count;
            }
            u16 path[0x512] = {0};
            struacat(path, main_paths[REMOTE_MODE_BADGES]);
            strucat(path, badge_files[i].name);
            default_set_count += install_badge_png(fsMakePath(PATH_UTF16, path), badge_files[i], &badge_count, default_set);
        } else if (!strcmp(badge_files[i].shortExt, "ZIP"))
        {
            if (default_set == 0)
            {
                set_count += 1;
                default_set = set_count;
            }
            u16 path[0x512] = {0};
            struacat(path, main_paths[REMOTE_MODE_BADGES]);
            strucat(path, badge_files[i].name);

            default_set_count += install_badge_zip(path, &badge_count, default_set);
        } else if (badge_files[i].attributes & FS_ATTRIBUTE_DIRECTORY)
        {
            set_count += 1;
            u32 count = install_badge_dir(badge_files[i], &badge_count, set_count);
            if (count == 0)
                set_count -= 1;
        }
        progress_status += 1;
        draw_loading_bar(progress_status, progress_finish, INSTALL_BADGES);
    }

    DEBUG("Badges installed - doing metadata\n");
    if (default_set != 0)
    {
        int default_index = default_set - 1;
        u32 total_count = 0xFFFF * default_set_count;
        for (int i = 0; i < 16; ++i)
        {
            u16 name[0x8A/2] = {0};
            utf8_to_utf16(name, (u8 *) "Other Badges", 0x8A);
            FSFILE_Write(badgeDataHandle, NULL, default_index * 0x8A0 + i * 0x8A, &name, strulen(name, 0x45) * 2, 0);
        }
        badgeMngBuffer[0x3D8 + default_index/8] |= 1 << (default_index % 8);

        memset(badgeMngBuffer + 0xA028 + default_index * 0x30, 0xFF, 8);
        badgeMngBuffer[0xA028 + 0xC + default_index * 0x30] = 0x10; // bytes 13 and 14 are 0x2710
        badgeMngBuffer[0xA028 + 0xD + default_index * 0x30] = 0x27;
        memcpy(badgeMngBuffer + 0xA028 + 0x10 + default_index * 0x30, &default_set, 4);
        memcpy(badgeMngBuffer + 0xA028 + 0x14 + default_index * 0x30, &default_index, 4);
        memset(badgeMngBuffer + 0xA028 + 0x18 + default_index * 0x30, 0xFF, 4);
        memcpy(badgeMngBuffer + 0xA028 + 0x1C + default_index * 0x30, &default_set_count, 4);
        memcpy(badgeMngBuffer + 0xA028 + 0x20 + default_index * 0x30, &total_count, 4);
        memcpy(badgeMngBuffer + 0xA028 + 0x24 + default_index * 0x30, &default_idx, 4);

        FILE *fp = fopen("romfs:/anemone_set.png", "rb");
        fseek(fp, 0L, SEEK_END);
        ssize_t size = ftell(fp);
        char *icon_buf = malloc(size);
        fseek(fp, 0L, SEEK_SET);
        fread(icon_buf, 1, size, fp);
        fclose(fp);
        pngToRGB565(icon_buf, size, rgb_buf_64x64, alpha_buf_64x64, rgb_buf_32x32, alpha_buf_32x32, true);
        free(icon_buf);
        FSFILE_Write(badgeDataHandle, NULL, 0x250F80 + default_index * 0x2000, rgb_buf_64x64, 64 * 64 * 2, 0);
    }

    FSFILE_Flush(badgeDataHandle);

    u32 total_badges = 0xFFFF * badge_count; // Quantity * unique badges?

    badgeMngBuffer[0x4] = set_count; // badge set count
    memcpy(badgeMngBuffer + 0x8, &badge_count, 4);
    badgeMngBuffer[0x10] = 0xFF; // Selected Set - 0xFFFFFFFF is all badges
    badgeMngBuffer[0x11] = 0xFF;
    badgeMngBuffer[0x12] = 0xFF;
    badgeMngBuffer[0x13] = 0xFF;
    memcpy(badgeMngBuffer + 0x18, &total_badges, 4);
    memcpy(badgeMngBuffer + 0x1C, &nnidNum, 4);

    for (int i = set_count; i < 100; ++i) // default values for remaining sets
    {
        memset(badgeMngBuffer + 0xA028 + 0x30 * i, 0xFF, 0x8);
        memset(badgeMngBuffer + 0xA028 + 0x30 * i + 0x8, 0x00, 0x4);
        badgeMngBuffer[0xA028 + 0x30 * i + 0xC] = 0x10;
        badgeMngBuffer[0xA028 + 0x30 * i + 0xD] = 0x27;
        memset(badgeMngBuffer + 0xA028 + 0x30 * i + 0xE, 0x0, 0x2);
        memset(badgeMngBuffer + 0xA028 + 0x30 * i + 0x10, 0xFF, 0xC);
        memset(badgeMngBuffer + 0xA028 + 0x30 * i + 0x1C, 0x00, 0x8);
        memset(badgeMngBuffer + 0xA028 + 0x30 * i + 0x24, 0xFF, 0x4);
        memset(badgeMngBuffer + 0xA028 + 0x30 * i + 0x28, 0x00, 0x8);
    }

    res = FSUSER_OpenFile(&handle, ArchiveBadgeExt, fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), FS_OPEN_READ, 0);
    if (res == 0)
    {
        FSFILE_Read(handle, NULL, 0xB2E8, badgeMngBuffer+0xB2E8, 360 * 0x18);
        FSFILE_Close(handle);
    }

    res = buf_to_file(BADGE_MNG_SIZE, fsMakePath(PATH_ASCII, "/BadgeMngFile.dat"), ArchiveBadgeExt, badgeMngBuffer);
    if (res)
    {
        DEBUG("Error writing badge manage data! %lx\n", res);
        char err_string[128] = {0};
        sprintf(err_string, language.badges.extdata_locked, res);
        throw_error(err_string, ERROR_LEVEL_WARNING);
        goto end; 
    }

    
    end:
    actExit();
    if (rgb_buf_64x64) free(rgb_buf_64x64);
    if (alpha_buf_64x64) free(alpha_buf_64x64);
    if (rgb_buf_32x32) free(rgb_buf_32x32);
    if (alpha_buf_32x32) free(alpha_buf_32x32);
    if (handle) FSFILE_Close(handle);
    if (folder) FSDIR_Close(folder);
    if (badgeDataHandle) FSFILE_Close(badgeDataHandle);
    if (badgeMngBuffer) free(badgeMngBuffer);
    if (badge_files) free(badge_files);
    return res;
}