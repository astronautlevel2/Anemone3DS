#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"

s8 prepareThemes()
{
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

	return 0;
}

s8 themeInstall()
{
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
	retValue = FSUSER_OpenFile(&bodyHandle, ArchiveSD, fsMakePath(PATH_ASCII, "/Themes/theme/body_LZ.bin"), FS_OPEN_READ, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_GetSize(bodyHandle, &bodySize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	body = malloc(sizeof(char) * bodySize);
	retValue = FSFILE_Read(bodyHandle, &bytes, 0, body, (u64)bodySize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	FSFILE_Close(bodyHandle);

	retValue = FSUSER_OpenFile(&bgmHandle, ArchiveSD, fsMakePath(PATH_ASCII, "/Themes/theme/bgm.bcstm"), FS_OPEN_READ, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_GetSize(bgmHandle, &bgmSize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	bgm = malloc(sizeof(char) * bgmSize);
	retValue = FSFILE_Read(bgmHandle, &bytes, 0, bgm, (u64)bgmSize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	FSFILE_Close(bgmHandle);

	retValue = FSUSER_OpenFile(&saveDataHandle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_GetSize(saveDataHandle, &saveDataSize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	saveData = malloc(sizeof(char) * saveDataSize);
	retValue = FSFILE_Read(saveDataHandle, &bytes, 0, saveData, (u32)saveDataSize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	FSFILE_Close(saveDataHandle);	
	
	retValue = FSUSER_OpenFile(&themeManageHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_GetSize(themeManageHandle, &themeManageSize);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	themeManage = malloc(sizeof(char) * themeManageSize);
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

	retValue = FSUSER_OpenFile(&bgmHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/BgmCache.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_Write(bgmHandle, &bytes, 0, bgm, (u64)bgmSize, FS_WRITE_FLUSH);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	FSFILE_Close(bgmHandle);
	
	retValue = FSUSER_OpenFile(&themeManageHandle, ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_Write(themeManageHandle, &bytes, 0, themeManage, (u64)themeManageSize, FS_WRITE_FLUSH);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	FSFILE_Close(themeManageHandle);

	retValue = FSUSER_OpenFile(&saveDataHandle, ArchiveHomeExt, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	retValue = FSFILE_Write(saveDataHandle, &bytes, 0, saveData, (u64)saveDataSize, FS_WRITE_FLUSH);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
	FSFILE_Close(saveDataHandle);


	FSUSER_CloseArchive(ArchiveSD);
	FSUSER_CloseArchive(ArchiveHomeExt);
	FSUSER_CloseArchive(ArchiveThemeExt);

return 0;
}
