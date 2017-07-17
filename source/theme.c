#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"
#include "unicode.h"
#include "minizip/unzip.h"

Result extract_current_file(unzFile zip_handle, u16 *theme_path, ssize_t len)
{
    unz_file_info *file_info = malloc(sizeof(unz_file_info)); // Make the file_info struct
    char filename[64]; 
    unzGetCurrentFileInfo(zip_handle, file_info, filename, 64, NULL, 0, NULL, 0); // Get file info, as well as filename
    u32 file_size = file_info->uncompressed_size;
    u16 ufilename[128] = {0};
    ssize_t filename_len = atow(ufilename, filename);
    
    u16 file_path[256] = {0};
    memcpy(file_path, theme_path, len * sizeof(u16));
    memset(&file_path[len], '/', 1);
    len += 1;
    memcpy(&file_path[len], ufilename, filename_len * sizeof(u16));

    Result ret;
    ret = FSUSER_CreateFile(ArchiveSD, fsMakePath(PATH_UTF16, file_path), 0, file_size); // Create the file
    if (R_FAILED(ret)) return ret;

    char *file; // Read the compressed file into a buffer
    file = malloc(file_size);
    unzOpenCurrentFile(zip_handle);
    unzReadCurrentFile(zip_handle, file, file_size);
    unzCloseCurrentFile(zip_handle);

    Handle console_file; // And write it onto the SD card
    ret = FSUSER_OpenFile(&console_file, ArchiveSD, fsMakePath(PATH_UTF16, file_path), FS_OPEN_WRITE, 0);
    if (R_FAILED(ret)) return ret;
    ret = FSFILE_Write(console_file, NULL, 0, file, file_size, FS_WRITE_FLUSH);
    if (R_FAILED(ret)) return ret;
    ret = FSFILE_Close(console_file);
    if (R_FAILED(ret)) return ret;

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS);
}

Result unzip_theme(FS_DirectoryEntry *entry, u16 *sanitized_zip)
{
    char *base_path = "/Themes/";
    u16 zip_path[sizeof(entry->name) + (strlen(base_path) * 2) + 1];
    u16 uzipfile[sizeof(entry->name) + (strlen(base_path) * 2) + 1];
    memset(zip_path, 0, sizeof(entry->name) + (strlen(base_path) * 2 * sizeof(u16)));
    atow(zip_path, base_path);
    atow(uzipfile, base_path);
    memcpy(&zip_path[strlen(base_path)], entry->name, sizeof(entry->name));
    memcpy(&uzipfile[strlen(base_path)], sanitized_zip, sizeof(entry->name));
    u16 theme_path_u16[128] = {0};
    ssize_t len = trim_extension(theme_path_u16, zip_path, sizeof(entry->name));

    FS_Path theme_path = fsMakePath(PATH_UTF16, theme_path_u16); // Turn it into a 3ds directory

    Result res = FSUSER_OpenDirectory(NULL, ArchiveSD, theme_path);
    if (R_SUMMARY(res) == RS_NOTFOUND) // If it doesn't exist, make it
    {
        res = FSUSER_CreateDirectory(ArchiveSD, theme_path, FS_ATTRIBUTE_DIRECTORY);
        if (R_FAILED(res)) printf("Failed to make directory!\nSummary: %lu\n", R_SUMMARY(res));
    } else if(R_FAILED(res)) printf("Failed to make directory!\nSummary: %lu\n", R_SUMMARY(res));

    char *zipfile = malloc(524);
    wtoa(zipfile, uzipfile);

    unzFile zip_handle = unzOpen(zipfile); // Open up the zip file
    if (zip_handle == NULL)
    {
        printf("Failed to open zip: %s\n\n", zipfile);
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_COMMON, RD_NOT_FOUND);
    }

    unzGoToFirstFile(zip_handle); // Go to the first file and unzip it

    extract_current_file(zip_handle, theme_path_u16, len);
    while(unzGoToNextFile(zip_handle) == UNZ_OK) extract_current_file(zip_handle, theme_path_u16, len); // While next file exists, unzip it
    unzClose(zip_handle);
    FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_ASCII, zipfile));
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS); // And return success \o/
}

s8 prepareThemes()
{
    printf("Preparing themes...\n");
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
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);

    u32 homeMenuPath[3] = {MEDIATYPE_SD, archive2, 0};
    home.type = PATH_BINARY;
    home.size = 0xC;
    home.data = homeMenuPath;
    retValue = FSUSER_OpenArchive(&ArchiveHomeExt, ARCHIVE_EXTDATA, home);  
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);

    u32 themePath[3] = {MEDIATYPE_SD, archive1, 0};
    theme.type = PATH_BINARY;
    theme.size = 0xC;
    theme.data = themePath;
    retValue = FSUSER_OpenArchive(&ArchiveThemeExt, ARCHIVE_EXTDATA, theme);    
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);

    // This is where the fun begins
    Handle themes_dir;
    FSUSER_OpenDirectory(&themes_dir, ArchiveSD, fsMakePath(PATH_ASCII, "/Themes"));
    while (true)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        u32 entries_read;
        FSDIR_Read(themes_dir, &entries_read, 1, entry);
        if (entries_read)
        {
		    if (!strcmp(entry->shortExt, "ZIP")) 
            {
                u16 sanitized_zip[sizeof(entry->name)] = {0};
                bool changed = strip_unicode(sanitized_zip, entry->name, sizeof(entry->name));
                if (changed)
                {
                    u16 zip_path[550] = {0};
                    ssize_t len = atow(zip_path, "/Themes/");
                    memcpy(&zip_path[len], entry->name, sizeof(entry->name));
                    u16 sanitized_zip_path[550] = {0};
                    atow(sanitized_zip_path, "/Themes/");
                    memcpy(&sanitized_zip_path[len], sanitized_zip, sizeof(entry->name));
                    FSUSER_RenameFile(ArchiveSD, fsMakePath(PATH_UTF16, zip_path), ArchiveSD, fsMakePath(PATH_UTF16, sanitized_zip_path));
                    unzip_theme(entry, sanitized_zip);
                } else unzip_theme(entry, entry->name);
            }
		    free(entry);
        } else {
            free(entry);
            break;
        } 
    }
    printf("Done!\n");
    return 0;
}

s8 themeInstall(theme theme_to_install)
{
    char *path = theme_to_install.path;
    bool music = theme_to_install.bgm;
    char *body;
    char *bgm;
    char *saveData;
    char *themeManage;
    u64 bodySize;
    u64 bgmSize;
    u64 saveDataSize;
    u64 themeManageSize;
    Handle bodyHandle;
    Handle bgmHandle;
    Handle saveDataHandle;
    Handle themeManageHandle;

    Result retValue;

    u32 bytes;

    // Opening relevant files
    char bodyPath[128];
    strcpy(bodyPath, path);
    strcat(bodyPath, "/body_LZ.bin");
    retValue = FSUSER_OpenFile(&bodyHandle, ArchiveSD, fsMakePath(PATH_ASCII, bodyPath), FS_OPEN_READ, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_GetSize(bodyHandle, &bodySize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    body = malloc(bodySize);
    retValue = FSFILE_Read(bodyHandle, &bytes, 0, body, (u64)bodySize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(bodyHandle);
    
    char bgmPath[128];
    strcpy(bgmPath, path);
    strcat(bgmPath, "/bgm.bcstm");
    retValue = FSUSER_OpenFile(&bgmHandle, ArchiveSD, fsMakePath(PATH_ASCII, bgmPath), FS_OPEN_READ, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_GetSize(bgmHandle, &bgmSize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    bgm = malloc(bgmSize);
    retValue = FSFILE_Read(bgmHandle, &bytes, 0, bgm, (u64)bgmSize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(bgmHandle);

    retValue = FSUSER_OpenFile(&saveDataHandle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_GetSize(saveDataHandle, &saveDataSize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    saveData = malloc(saveDataSize);
    retValue = FSFILE_Read(saveDataHandle, &bytes, 0, saveData, (u32)saveDataSize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(saveDataHandle);   
    
    retValue = FSUSER_OpenFile(&themeManageHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_GetSize(themeManageHandle, &themeManageSize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    themeManage = malloc(themeManageSize);
    retValue = FSFILE_Read(themeManageHandle, &bytes, 0, themeManage, (u32)themeManageSize);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(themeManageHandle);

    //Changing data in the buffers
    memset(&saveData[0x13b8], 0, 8);

    saveData[0x141b] = 0x00;
    saveData[0x13b8] = 0xFF;
    saveData[0x13b9] = 0x00;
    saveData[0x13ba] = 0x00;
    saveData[0x13bb] = 0x00;
    saveData[0x13bc] = 0x00;
    saveData[0x13bd] = 0x03;
    saveData[0x13be] = 0x00;
    saveData[0x13bf] = 0x00;


    themeManage[0x00] = 1;
    themeManage[0x01] = 0;
    themeManage[0x02] = 0;
    themeManage[0x03] = 0;
    themeManage[0x04] = 0;
    themeManage[0x05] = 0;
    themeManage[0x06] = 0;
    themeManage[0x07] = 0;

    u32* bodySizeLocation = (u32*)(&themeManage[0x8]);
    u32* bgmSizeLocation = (u32*)(&themeManage[0xC]);
    *bodySizeLocation = (u32)bodySize;
    *bgmSizeLocation = (u32)bgmSize;

    themeManage[0x10] = 0xFF;
    themeManage[0x14] = 0x01;
    themeManage[0x18] = 0xFF;
    themeManage[0x1D] = 0x02;

    memset(&themeManage[0x338], 0, 4);
    memset(&themeManage[0x340], 0, 4);
    memset(&themeManage[0x360], 0, 4);
    memset(&themeManage[0x368], 0, 4);


    // Writing to extdata
    retValue = FSUSER_OpenFile(&bodyHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BodyCache.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_Write(bodyHandle, &bytes, 0, body, (u64)bodySize, FS_WRITE_FLUSH);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(bodyHandle);
    free(body);

    retValue = FSUSER_OpenFile(&bgmHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BgmCache.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    if(!music) memset(bgm, 0x00, 337000 );
    retValue = FSFILE_Write(bgmHandle, &bytes, 0, bgm, (u64)bgmSize, FS_WRITE_FLUSH);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(bgmHandle);
    free(bgm);

    retValue = FSUSER_OpenFile(&themeManageHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_Write(themeManageHandle, &bytes, 0, themeManage, (u64)themeManageSize, FS_WRITE_FLUSH);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(themeManageHandle);
    free(themeManage);

    retValue = FSUSER_OpenFile(&saveDataHandle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSFILE_Write(saveDataHandle, &bytes, 0, saveData, (u64)saveDataSize, FS_WRITE_FLUSH);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    FSFILE_Close(saveDataHandle);
    free(saveData);

    return 0;
}

s8 closeThemeArchives()
{
    Result retValue;

    retValue = FSUSER_CloseArchive(ArchiveSD);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSUSER_CloseArchive(ArchiveHomeExt);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    retValue = FSUSER_CloseArchive(ArchiveThemeExt);
    if(R_FAILED(retValue)) return R_SUMMARY(retValue);
    
    return 0;
}
