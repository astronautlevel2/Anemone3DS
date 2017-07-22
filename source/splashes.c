#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"
#include "unicode.h"
#include "splashes.h"
#include "linked_lists.h"

Result prepare_splashes(node* first_node)
{
    Handle splashes_dir;
    FSUSER_OpenDirectory(&splashes_dir, ArchiveSD, fsMakePath(PATH_ASCII, "/Splashes"));; // Open up splashes directory
    while (true) // Honestly this repeated code triggers me but I'm too lazy to refactor (now at least)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        u32 entries_read;
        FSDIR_Read(splashes_dir, &entries_read, 1, entry);
        if (entries_read) // If there is a new entry
        {
            if (!strcmp(entry->shortExt, "ZIP")) // if that entry is a zip
            {
                u16 sanitized_zip[sizeof(entry->name)] = {0};
                bool changed = strip_unicode(sanitized_zip, entry->name, sizeof(entry->name)); // Here we strip out the non-ASCII characters in the zip name, as minizip doesn't like them
                if (changed) // If there were any non-ascii characters
                {
                    u16 zip_path[550] = {0};
                    ssize_t len = atow(zip_path, "/Splashes/"); // Copy the unicode equivalent of "/Splashes/" into zip_path
                    memcpy(&zip_path[len], entry->name, sizeof(entry->name)); // Copy the name of the zip (with unicode chars) into zip_path
                    u16 sanitized_zip_path[550] = {0}; // Same thing as above, this time with sanitized names
                    atow(sanitized_zip_path, "/Splashes/");
                    memcpy(&sanitized_zip_path[len], sanitized_zip, sizeof(entry->name));
                    FSUSER_RenameFile(ArchiveSD, fsMakePath(PATH_UTF16, zip_path), ArchiveSD, fsMakePath(PATH_UTF16, sanitized_zip_path)); // Rename the zip to the sanitized one
                    unzip_file("/Splashes/", entry, sanitized_zip); // And unzip it
                } else unzip_file("/Splashes/", entry, entry->name); // If it's the same, unzip it anyway
            }
            free(entry);
        } else {
            free(entry);
            break;
        }
    }
    FSDIR_Close(splashes_dir);
    FSUSER_OpenDirectory(&splashes_dir, ArchiveSD, fsMakePath(PATH_ASCII, "/Splashes"));
    while (true)
    {
        FS_DirectoryEntry *entry = malloc(sizeof(FS_DirectoryEntry));
        u32 entries_read;
        FSDIR_Read(splashes_dir, &entries_read, 1, entry);
        if (entries_read)
        {
            if (entry->attributes == 1)
            {
                u16 *splash_path = malloc(533);
                memset(splash_path, 0, 533);
                atow(splash_path, "/Splashes/");
                strucat(splash_path, entry->name);
                node *current_splash = malloc(sizeof(node));
                current_splash->data = splash_path;
                current_splash->next = NULL;
                add_node(first_node, current_splash);
            }
            free(entry);
        } else {
            free(entry);
            break;
        }
    }
    return 0;
}

Result install_splash(u16 *path)
{
    printu(path);
    int splashes_installed = 0;
    u16 file_path[0x106] = {0};
    strucpy(file_path, path);
    struacat(file_path, "/splash.bin");

    Handle file_handle;
    Result res = FSUSER_OpenFile(&file_handle, ArchiveSD, fsMakePath(PATH_UTF16, file_path), FS_OPEN_READ, 0);
    if (R_FAILED(res) && R_SUMMARY(res) != RS_NOTFOUND) return res;
    else if (R_SUCCEEDED(res))
    {
        u64 file_size;
        u32 bytes;
        FSFILE_GetSize(file_handle, &file_size);
        char *splash_data = malloc(file_size);
        memset(splash_data, 0, file_size);
        FSFILE_Read(file_handle, &bytes, 0, splash_data, file_size);
        FSFILE_Close(file_handle);
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_ASCII, "/Luma/splash.bin"));
        FSUSER_CreateFile(ArchiveSD, fsMakePath(PATH_ASCII, "/Luma/splash.bin"), 0, file_size);
        FSUSER_OpenFile(&file_handle, ArchiveSD, fsMakePath(PATH_ASCII, "/Luma/splash.bin"), FS_OPEN_WRITE, 0);
        Result res = FSFILE_Write(file_handle, &bytes, 0, splash_data, file_size, FS_WRITE_FLUSH);
        if (R_FAILED(res)) printf("Write: %lX\n", res);
        free(splash_data);
        FSFILE_Close(file_handle);
        splashes_installed++;
    }

    memset(file_path, 0, 0x106);
    strucpy(file_path, path);
    struacat(file_path, "/splashbottom.bin");

    res = FSUSER_OpenFile(&file_handle, ArchiveSD, fsMakePath(PATH_UTF16, file_path), FS_OPEN_READ, 0);
    if (R_FAILED(res) && R_SUMMARY(res) != RS_NOTFOUND) return res;
    else if (R_SUCCEEDED(res))
    {
        u64 file_size;
        u32 bytes;
        FSFILE_GetSize(file_handle, &file_size);
        char *splash_data = malloc(file_size);
        memset(splash_data, 0, file_size);
        FSFILE_Read(file_handle, &bytes, 0, splash_data, file_size);
        FSFILE_Close(file_handle);
        FSUSER_DeleteFile(ArchiveSD, fsMakePath(PATH_ASCII, "/Luma/splashbottom.bin"));
        FSUSER_CreateFile(ArchiveSD, fsMakePath(PATH_ASCII, "/Luma/splashbottom.bin"), 0, file_size);
        FSUSER_OpenFile(&file_handle, ArchiveSD, fsMakePath(PATH_ASCII, "/Luma/splashbottom.bin"), FS_OPEN_WRITE, 0);
        FSFILE_Write(file_handle, &bytes, 0, splash_data, file_size, FS_WRITE_FLUSH);
        FSFILE_Close(file_handle);
        free(splash_data);
        splashes_installed += 2;
    }

    switch(splashes_installed) 
    {
        case 0:
            printf("No splashes installed\n");
            break;

        case 1:
            printf("Top splash installed\n");
            break;

        case 2:
            printf("Bottom splash installed\n");
            break;

        case 3:
            printf("Both splashes installed\n");
            break;
    }

    return 0;
}