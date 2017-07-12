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

	retValue = FSUSER_OpenArchive(&ARCHIVE_SD, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);

	u32 homeMenuPath[3] = {MEDIATYPE_SD, archive2, 0};
	home.type = PATH_BINARY;
	home.size = 0xC;
	home.data = homeMenuPath;
	retValue = FSUSER_OpenArchive(&ARCHIVE_HOMEEXT, ARCHIVE_EXTDATA, home);	
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);

	u32 themePath[3] = {MEDIATYPE_SD, archive1, 0};
	theme.type = PATH_BINARY;
	theme.size = 0xC;
	theme.data = themePath;
	retValue = FSUSER_OpenArchive(&ARCHIVE_THEMEEXT, ARCHIVE_EXTDATA, theme);	
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);

	return 0;
}

s8 themeInstall()
{
	char* body;
	char* bgm;
	u64 bodySize;
	u64 bgmSize;
	Handle bodyHandle;
	Handle bgmHandle;
	
	Result retValue;

	// This fails, for some reason, 4 is returned aka NOT FOUND
	retValue = FSUSER_OpenFile(&bodyHandle, ARCHIVE_SDMC, fsMakePath(PATH_ASCII, u"/Themes/theme/body_LZ.bin"), FS_OPEN_READ, 0);
	if(R_FAILED(retValue)) return R_SUMMARY(retValue);
return 0;
}
