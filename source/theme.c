#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"
#include "unicode.h"
#include "minizip/unzip.h"
#include "linked_lists.h"

Result extract_current_file(unzFile zip_handle, u16 *theme_path)
{
    unz_file_info *file_info = malloc(sizeof(unz_file_info)); // Make the file_info struct
    char filename[64]; 
    unzGetCurrentFileInfo(zip_handle, file_info, filename, 64, NULL, 0, NULL, 0); // Get file info, as well as filename
    u32 file_size = file_info->uncompressed_size;
    u16 ufilename[128] = {0};
    atow(ufilename, filename); // Make the filename Unicode, as the folder name is unicode.
    
    u16 file_path[256] = {0};
    strucpy(file_path, theme_path); // Copy the base theme folder name into the file
    u16 slash[2] = {0};
    atow(slash, "/");
    strucat(file_path, slash); // Put a / so it recognizes a directory
    strucat(file_path, ufilename); // Copy the file name into the file path

    Result ret;
    ret = FSUSER_CreateFile(ArchiveSD, fsMakePath(PATH_UTF16, file_path), 0, file_size); // Create the file
    if (R_FAILED(ret))
    {
        free(file_info);
        return ret;
    }

    char *file; // Read the compressed file into a buffer
    file = malloc(file_size);
    unzOpenCurrentFile(zip_handle);
    unzReadCurrentFile(zip_handle, file, file_size);
    unzCloseCurrentFile(zip_handle);

    Handle console_file; // And write it onto the SD card
    ret = FSUSER_OpenFile(&console_file, ArchiveSD, fsMakePath(PATH_UTF16, file_path), FS_OPEN_WRITE, 0);
    
    if (R_FAILED(ret))
    {
        free(file_info);
        free(file);
        return ret;
    }

    ret = FSFILE_Write(console_file, NULL, 0, file, file_size, FS_WRITE_FLUSH);
    
    if (R_FAILED(ret))
    {
        free(file_info);
        free(file);
        return ret;
    }
   
    ret = FSFILE_Close(console_file);
    
    if (R_FAILED(ret))
    {
        free(file_info);
        free(file);
        return ret;
    }

    free(file_info);
    free(file);

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS);
}

// TODO: There's a lot of duplicated code here, especially considering that we already built the paths in prepare_themes(). Maybe clean it up a bit later
Result unzip_theme(FS_DirectoryEntry *entry, u16 *sanitized_name)
{
    char *base_path = "/Themes/";
    u16 zip_path[sizeof(entry->name) + 18] = {0}; // Make two u16*s that are big enough to hold the entire path
    u16 uzipfile[sizeof(entry->name) + 18] = {0};
    atow(zip_path, base_path); // Copy "/Themes/" unicode equivalent into a path
    strucat(zip_path, entry->name); // And copy the unsanitized path in
    atow(uzipfile, base_path);
    strucat(uzipfile, sanitized_name);
    u16 theme_path_u16[128] = {0};
    trim_extension(theme_path_u16, zip_path); // Get rid of the extension on the unsanitized one, which later becomes the basis for the folder

    FS_Path theme_path = fsMakePath(PATH_UTF16, theme_path_u16); // Turn the unsanizited name into a new directory

    Result res = FSUSER_OpenDirectory(NULL, ArchiveSD, theme_path);
    if (R_SUMMARY(res) == RS_NOTFOUND) // If it doesn't exist, make it
    {
        res = FSUSER_CreateDirectory(ArchiveSD, theme_path, FS_ATTRIBUTE_DIRECTORY);
        if (R_FAILED(res)) return res;
    } else if(R_FAILED(res)) return res;

    char *zipfile = malloc(524); // Plop the char* equivalent into here
    wtoa(zipfile, uzipfile);

    unzFile zip_handle = unzOpen(zipfile); // Open up the zip file
    if (zip_handle == NULL)
    {
        printf("Failed to open zip: %s\n\n", zipfile);
        return MAKERESULT(RL_FATAL, RS_NOTFOUND, RM_COMMON, RD_NOT_FOUND);
    }

    unzGoToFirstFile(zip_handle); // Go to the first file and unzip it

    extract_current_file(zip_handle, theme_path_u16);
    while(unzGoToNextFile(zip_handle) == UNZ_OK) extract_current_file(zip_handle, theme_path_u16); // While next file exists, unzip it
    unzClose(zip_handle);
    free(zipfile);
    FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_ASCII, zipfile));
    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS); // And return success \o/
}

Result prepareThemes(node *first_node)
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

    // This is where the fun begins
    Handle themes_dir;
    FSUSER_OpenDirectory(&themes_dir, ArchiveSD, fsMakePath(PATH_ASCII, "/Themes")); // Open up the Themes directory and iterate over each file
    while (true)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        u32 entries_read;
        FSDIR_Read(themes_dir, &entries_read, 1, entry);
        if (entries_read) // If there is a new entry
        {
		    if (!strcmp(entry->shortExt, "ZIP")) // if that entry is a zip
            {
                u16 sanitized_zip[sizeof(entry->name)] = {0};
                bool changed = strip_unicode(sanitized_zip, entry->name, sizeof(entry->name)); // Here we strip out the non-ASCII characters in the zip name, as minizip doesn't like them
                if (changed) // If there were any non-ascii characters
                {
                    u16 zip_path[550] = {0};
                    ssize_t len = atow(zip_path, "/Themes/"); // Copy the unicode equivalent of "/Themes/" into zip_path
                    memcpy(&zip_path[len], entry->name, sizeof(entry->name)); // Copy the name of the zip (with unicode chars) into zip_path
                    u16 sanitized_zip_path[550] = {0}; // Same thing as above, this time with sanitized names
                    atow(sanitized_zip_path, "/Themes/");
                    memcpy(&sanitized_zip_path[len], sanitized_zip, sizeof(entry->name));
                    FSUSER_RenameFile(ArchiveSD, fsMakePath(PATH_UTF16, zip_path), ArchiveSD, fsMakePath(PATH_UTF16, sanitized_zip_path)); // Rename the zip to the sanitized one
                    unzip_theme(entry, sanitized_zip); // And unzip it
                } else unzip_theme(entry, entry->name); // If it's the same, unzip it anyway
            }
		    free(entry);
        } else {
            free(entry);
            break;
        }
    }
    FSDIR_Close(themes_dir);
    FSUSER_OpenDirectory(&themes_dir, ArchiveSD, fsMakePath(PATH_ASCII, "/Themes"));
    while (true)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        u32 entries_read;
        FSDIR_Read(themes_dir, &entries_read, 1, entry);
        if (entries_read)
        {
            if (entry->attributes == 1)
            {
                theme_data *theme_info = malloc(sizeof(theme_data));
                u16 theme_path[533] = {0};
                atow(theme_path, "/Themes/");
                strucat(theme_path, entry->name);
                parseSmdh(theme_info, theme_path);
                node *current_theme = malloc(sizeof(node));
                current_theme->data = theme_info;
                current_theme->next = NULL;
                add_node(first_node, current_theme);
            }
            free(entry);
        } else {
            free(entry);
            break;
        }
    }
    return 0;
}

Result themeInstall(theme_data theme_to_install)
{
    
    u16 *u16path = theme_to_install.path;
    char *path = malloc(524);
    wtoa(path, u16path);
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

    if(R_FAILED(retValue)) 
    {
        free(path);
        return retValue;
    }

    retValue = FSFILE_GetSize(bodyHandle, &bodySize);

    if(R_FAILED(retValue)) 
    {
        free(path);
        return retValue;
    }

    body = malloc(bodySize);
    retValue = FSFILE_Read(bodyHandle, &bytes, 0, body, (u64)bodySize);
    
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        return retValue;
    }

    FSFILE_Close(bodyHandle);
    
    char bgmPath[128];
    strcpy(bgmPath, path);
    strcat(bgmPath, "/bgm.bcstm");
    retValue = FSUSER_OpenFile(&bgmHandle, ArchiveSD, fsMakePath(PATH_ASCII, bgmPath), FS_OPEN_READ, 0);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        return retValue;
    }

    retValue = FSFILE_GetSize(bgmHandle, &bgmSize);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        return retValue;
    }

    bgm = malloc(bgmSize);
    retValue = FSFILE_Read(bgmHandle, &bytes, 0, bgm, (u64)bgmSize);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        return retValue;
    }

    FSFILE_Close(bgmHandle);

    retValue = FSUSER_OpenFile(&saveDataHandle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ, 0);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        return retValue;
    }

    retValue = FSFILE_GetSize(saveDataHandle, &saveDataSize);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        return retValue;
    }

    saveData = malloc(saveDataSize);
    retValue = FSFILE_Read(saveDataHandle, &bytes, 0, saveData, (u32)saveDataSize);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    FSFILE_Close(saveDataHandle);   
    
    retValue = FSUSER_OpenFile(&themeManageHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    retValue = FSFILE_GetSize(themeManageHandle, &themeManageSize);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    themeManage = malloc(themeManageSize);
    retValue = FSFILE_Read(themeManageHandle, &bytes, 0, themeManage, (u32)themeManageSize);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

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
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    retValue = FSFILE_Write(bodyHandle, &bytes, 0, body, (u64)bodySize, FS_WRITE_FLUSH);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    FSFILE_Close(bodyHandle);
    free(body);

    retValue = FSUSER_OpenFile(&bgmHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BgmCache.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    if(!music) memset(bgm, 0x00, 337000 );
    retValue = FSFILE_Write(bgmHandle, &bytes, 0, bgm, (u64)bgmSize, FS_WRITE_FLUSH);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    FSFILE_Close(bgmHandle);
    free(bgm);

    retValue = FSUSER_OpenFile(&themeManageHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    retValue = FSFILE_Write(themeManageHandle, &bytes, 0, themeManage, (u64)themeManageSize, FS_WRITE_FLUSH);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    FSFILE_Close(themeManageHandle);
    free(themeManage);

    retValue = FSUSER_OpenFile(&saveDataHandle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    retValue = FSFILE_Write(saveDataHandle, &bytes, 0, saveData, (u64)saveDataSize, FS_WRITE_FLUSH);
        
    if(R_FAILED(retValue)) 
    {
        free(path);
        free(body);
        free(bgm);
        free(saveData);
        return retValue;
    }

    FSFILE_Close(saveDataHandle);
    free(path);
    free(body);
    free(bgm);
    free(saveData);

    return 0;
}

Result parseSmdh(theme_data *entry, u16 *path)
{
	Result retValue;
	u32 bytes;
	Handle infoHandle;
	u16 pathToInfo[534] = {0};
	u16 infoPath[12] = {0};
	atow(infoPath, "/info.smdh");
	strucpy(pathToInfo, path);
	strucat(pathToInfo, infoPath);
	char *infoContent = malloc(0x36C0);
	retValue = FSUSER_OpenFile(&infoHandle, ArchiveSD, fsMakePath(PATH_UTF16, pathToInfo), FS_OPEN_READ, 0);
	
    if(R_FAILED(retValue))
    {
        free(infoContent);
        return retValue;
    }

	retValue = FSFILE_Read(infoHandle, &bytes, 0, infoContent, (u32)14016);
    
    if(R_FAILED(retValue))
    {
        free(infoContent);
        return retValue;
    }
    
	retValue = FSFILE_Close(infoHandle);
    
    if(R_FAILED(retValue))
    {
        free(infoContent);
        return retValue;
    }
    

	memcpy(entry->title, infoContent + 0x08, 0x80);
	memcpy(entry->description, infoContent + 0x88, 0x100);
	memcpy(entry->author, infoContent + 0x188, 0x80);
	memcpy(entry->iconData, infoContent + 0x2040, 0x1200);
	
	free(infoContent);
	strucpy(entry->path, path);
	return 0;
}

Result closeThemeArchives()
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
