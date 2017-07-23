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
Result unzip_file(char* base_path, FS_DirectoryEntry *entry, u16 *sanitized_name)
{
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
    res = FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_ASCII, zipfile));
    if (R_FAILED(res))
    {
        free(zipfile);
        return res;
    }
    free(zipfile);
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
                    unzip_file("/Themes/", entry, sanitized_zip); // And unzip it
                } else unzip_file("/Themes/", entry, entry->name); // If it's the same, unzip it anyway
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
                /*
                FOR TESTING PURPOSES ONLY, REMOVE THIS LINE LATER!!!!
                */
                theme_info->selected = true;
                /*
                FOR TESTING PURPOSES ONLY, REMOVE ABOVE LINE LATER!!!
                */
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

Result shuffle_install(node *first_node)
{
    node *current_node = first_node->next;
    u8 count = 0;
    theme_data *themes_to_be_shuffled[10] = {0};
    u32 body_sizes[10] = {0};
    u32 bgm_sizes[10] = {0};
    
    // Load themes that are selected for shuffle
    while (current_node != NULL) 
    {
        if (((theme_data*)current_node->data)->selected) count++;
        
        if (count > 10) return(MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_COMMON, RD_INVALID_SELECTION));

        themes_to_be_shuffled[count - 1] = (theme_data *) current_node->data; // -1 because arrays are zero indexed, so the pos is one less than the count
        current_node = current_node->next;
    }

    // Load and edit SaveData
    u64 save_data_size;
    Handle save_data_handle;
    Result res = FSUSER_OpenFile(&save_data_handle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if (R_FAILED(res)) return res;
    FSFILE_GetSize(save_data_handle, &save_data_size);
    char *save_data_buffer = malloc(save_data_size);
    res = FSFILE_Read(save_data_handle, NULL, 0, save_data_buffer, save_data_size);
    
    if (R_FAILED(res))
    {
        free(save_data_buffer);
        return res;
    }

    save_data_buffer[0x141b] = 1; // shuffle flag
    memset(&save_data_buffer[0x13b8], 0, 8); // clear non-shuffle theme
    save_data_buffer[0x13bd] = 3; // 3 flag makes it persistent (?)
    save_data_buffer[0x13b8] = 0xff;

    for (int i = 0; i < 10; i++) // We have to set all 10 even if we're installing less than 10 because we may need to clear previous shuffles
    {
        memset(&save_data_buffer[0x13c0 + 0x8 * i], 0, 8); // clear any existing theme structure. 8 is the length of the theme structure, so 8 * i is the pos of the current one
        if (count > i) // if we are still installing themes...
        {
            save_data_buffer[0x13c0 + (8 * i)] = i; // index
            save_data_buffer[0x13c0 + (8 * i) + 5] = 3; // persistence (?)
        }
    }

    res = FSFILE_Write(save_data_handle, NULL, 0, save_data_buffer, save_data_size, FS_WRITE_FLUSH);
    free(save_data_buffer);
    if (R_FAILED(res)) return res;
    FSFILE_Close(save_data_handle);

    // Writing themes bodies to extdata
    Handle body_cache_handle;
    res = FSUSER_OpenFile(&body_cache_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BodyCache_rd.bin"), FS_OPEN_WRITE, 0); // 3DS uses _rd for shuffle themes
    if (R_FAILED(res)) return res;
    for (int i = 0; i < 10; i++)
    {
        if (count > i) // if we still have themes to install...
        {
            Handle body_lz; // This kind of code is repeated a ton, there has to be a better way. Either way; loading body_lz.bin into file
            u16 body_path[524] = {0};
            u64 body_size;
            strucpy(body_path, themes_to_be_shuffled[i]->path);
            struacat(body_path, "/body_LZ.bin");
            res = FSUSER_OpenFile(&body_lz, ArchiveSD, fsMakePath(PATH_UTF16, body_path), FS_OPEN_READ, 0);
            if (R_FAILED(res)) return res;
            FSFILE_GetSize(body_lz, &body_size);
            char *body_data = malloc(body_size);
            res = FSFILE_Read(body_lz, NULL, 0, body_data, body_size);
            if (R_FAILED(res))
            {
                free(body_data);
                return res;
            }
            FSFILE_Close(body_lz);
            body_sizes[i] = body_size; // We need to keep track of each theme's body size for when we write to ThemeManage.bin
            res = FSFILE_Write(body_cache_handle, NULL, 0x150000 * i, body_data, body_size, FS_WRITE_FLUSH); // Each theme has 0x150000 bytes allocated for it and no less
            free(body_data);
            if (R_FAILED(res)) return res;
        } else { // All themes have been written, write blank data to overwrite previous shuffles
            char empty[0x150000] = {0};
            FSFILE_Write(body_cache_handle, NULL, 0x150000 * i, empty, 0x150000, FS_WRITE_FLUSH);
        }
    }

    FSFILE_Close(body_cache_handle);

    // Unlike body data, the 3DS uses a separate file for each piece of BGM data. Don't ask me why.
    for (int i = 0; i < 10; i++)
    {
        Handle bgm_cache_handle;
        char bgm_cache_path[17] = {0}; // BGM Path is "/BgmCache_0i.bin" (16 chars) where i is the index number, + 1 for proper null termination
        sprintf(bgm_cache_path, "/BgmCache_0%i.bin", i);
        res = FSUSER_OpenFile(&bgm_cache_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, bgm_cache_path), FS_OPEN_WRITE, 0);
        if (R_FAILED(res)) return res;

        if (count > i && themes_to_be_shuffled[i]->bgm) // We write the bgm data if we still have themes to install and the current theme has bgm data, otherwise we write blank
        {
            Handle bgm_handle;
            u16 bgm_path[524] = {0};
            u64 bgm_size;
            strucpy(bgm_path, themes_to_be_shuffled[i]->path);
            struacat(bgm_path, "/bgm.bcstm");
            res = FSUSER_OpenFile(&bgm_handle, ArchiveSD, fsMakePath(PATH_UTF16, bgm_path), FS_OPEN_READ, 0);
            
            if (R_SUMMARY(res) == RS_NOTFOUND) // Despite the flag saying "install with bgm" there's no bgm for this theme -_-
            {
                char empty[3371008] = {0};
                res = FSFILE_Write(bgm_cache_handle, NULL, 0, empty, 3371008, FS_WRITE_FLUSH);
                if (R_FAILED(res)) return res;
                continue; // No need to do anything else
            }
            if (R_FAILED(res)) return res;
            
            FSFILE_GetSize(bgm_handle, &bgm_size); // Copy bgm data into buffer and write out to the bgm cache
            if (bgm_size > 3371008)
            {
                FSFILE_Close(bgm_handle);
                return MAKERESULT(RL_FATAL, RS_INVALIDARG, RM_COMMON, RD_TOO_LARGE);
            }

            char *bgm_data = malloc(bgm_size);
            Result res = FSFILE_Read(bgm_handle, NULL, 0, bgm_data, bgm_size);
            if (R_FAILED(res))
            {
                free(bgm_data);
                return res;
            }
            res = FSFILE_Write(bgm_cache_handle, NULL, 0, bgm_data, 3371008, FS_WRITE_FLUSH);
            free(bgm_data);
            if (R_FAILED(res)) return res;

            bgm_sizes[i] = bgm_size; // As with body we need the bgm_size for ThemeManage.bin
            FSFILE_Close(bgm_cache_handle);
            FSFILE_Close(bgm_handle);
        } else { // out of themes or this theme was specified not to use BGM
            char empty[3371008] = {0};
            res = FSFILE_Write(bgm_cache_handle, NULL, 0, empty, 3371008, FS_WRITE_FLUSH);
            FSFILE_Close(bgm_cache_handle);
            if (R_FAILED(res)) return res;
        }
    }

    // Finally we need to write data to ThemeManage.bin
    Handle thememanage_handle;
    char thememanage_buf[0x800];
    res = FSUSER_OpenFile(&thememanage_handle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
    if (R_FAILED(res)) return res;

    res = FSFILE_Read(thememanage_handle, NULL, 0, thememanage_buf, 0x800);
    if (R_FAILED(res)) return res;

    // 3dbrew doesn't know what most of these values do 
    thememanage_buf[0x00] = 1; // Unknown, normally 0x1 with size of 4
    thememanage_buf[0x01] = 0;
    thememanage_buf[0x02] = 0;
    thememanage_buf[0x03] = 0;
    thememanage_buf[0x04] = 0; // Unknown, normally 0 with size of 4
    thememanage_buf[0x05] = 0;
    thememanage_buf[0x06] = 0;
    thememanage_buf[0x07] = 0;
    thememanage_buf[0x10] = 0xFF; // Unknown
    thememanage_buf[0x14] = 0x01; // Unkown
    thememanage_buf[0x18] = 0xFF; // DLC theme index - 0xFF indicates no DLC theme
    thememanage_buf[0x1D] = 0x02; // Unknown, usually 0x200 to indicate theme-cache is being used

    u32 *bodysizeloc = (u32*) (&thememanage_buf[0x08]); // Set non-shuffle theme sizes to 0
    u32 *bgmsizeloc = (u32*) (&thememanage_buf[0x0C]);
    *bodysizeloc = (u32) 0;
    *bgmsizeloc = (u32) 0;

    for (int i = 0; i < 10; i++)
    {
        bodysizeloc = (u32*) (&thememanage_buf[0x338 + (4 * i)]); // body size info for shuffle themes starts at 0x338 and is 4 bytes for each theme
        bgmsizeloc = (u32*) (&thememanage_buf[0x360 + (4 * i)]); // same thing for bgm but starting at 0x360
        *bodysizeloc = body_sizes[i]; // We don't need to check if we've already installed all the themes because all sizes initialized to 0
        *bgmsizeloc = bgm_sizes[i];
    }

    res = FSFILE_Write(thememanage_handle, NULL, 0, thememanage_buf, 0x800, FS_WRITE_FLUSH);
    if (R_FAILED(res)) return res;

    FSFILE_Close(res);

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_COMMON, RD_SUCCESS);
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
