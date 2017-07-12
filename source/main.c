#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

u8 themeInstall()
{
	u8 region;
	u32 archive;
	u32 archive2;
	char* body;
	u64 bodySize;
	char* bgm;
	u64 bgmSize;

	u32 bytes;

	Handle bodyHandle;
	Handle bgmHandle;

	CFGU_SecureInfoGetRegion(&region);
	switch(region)
	{
		case 1:
			archive = 0x000002cd;
			archive2 = 0x0000008f;		
			break;
		case 2:
			archive = 0x000002ce;
			archive2 = 0x00000090;
			break;
		case 3:
			archive = 0x000002cc;
			archive2 = 0x00000082;
			break;
		default:
			archive = 0;
			archive2 = 0;
			break;
	}
	

	FSUSER_OpenFile(&bodyHandle, ARCHIVE_SDMC, fsMakePath(PATH_ASCII, "/Themes/theme/body_LZ.bin"), FS_OPEN_READ, 0);
	
	FSFILE_GetSize(bodyHandle, &bodySize);
	body = malloc(sizeof(char) * bodySize); 
	FSFILE_Read(bodyHandle, &bytes, 0, body, (u32)bodySize);

	FSUSER_OpenFile(&bgmHandle, ARCHIVE_SDMC, fsMakePath(PATH_ASCII, "/Themes/theme/bgm.bcstm"), FS_OPEN_READ, 0);
	FSFILE_GetSize(bgmHandle, &bgmSize);
	bgm = malloc(sizeof(char) * bgmSize);
	FSFILE_Read(bgmHandle, &bytes, 0, bgm, (u32)bgmSize);

	Handle saveDataHandle;
	u8* saveDataBuffer;
	u64 saveDataSize;

	Result ret;
	ret = FSUSER_OpenFile(&saveDataHandle, archive2, fsMakePath(PATH_ASCII, "/SaveData.dat"), FS_OPEN_READ | FS_OPEN_WRITE, 0);
	return (u8)ret;
	
	FSFILE_GetSize(saveDataHandle, &saveDataSize);

	saveDataBuffer = malloc(sizeof(u8) * saveDataSize);
	FSFILE_Read(saveDataHandle, &bytes, 0, saveDataBuffer, (u32)saveDataSize);

	memset(&saveDataBuffer[0x13b8], 0, 8);
		
	saveDataBuffer[0x141b] = 0;
	saveDataBuffer[0x13bd] = 3;
	saveDataBuffer[0x13b8] = 0xFF;

	FSFILE_Write(saveDataHandle, &bytes, 0, saveDataBuffer, saveDataSize, FS_WRITE_FLUSH);
		
	free(saveDataBuffer);
	FSFILE_Close(saveDataHandle);


	Handle bodyCache;
	FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, "/BodyCache.bin"));
	FSUSER_CreateFile(archive, fsMakePath(PATH_ASCII, "/BodyCache.bin"), 0, (u64)0x150000);
	FSUSER_OpenFile(&bodyCache, archive, fsMakePath(PATH_ASCII, "/BodyCache.bin"), FS_OPEN_WRITE, 0);
	FSFILE_Write(bodyCache, &bytes, 0, body, (u64)bodySize, FS_WRITE_FLUSH);
	FSFILE_Close(bodyCache);

	Handle bgmCache;
	FSUSER_DeleteFile(archive, fsMakePath(PATH_ASCII, "/BgmCache.bin"));
	FSUSER_CreateFile(archive, fsMakePath(PATH_ASCII, "/BgmCache.bin"), 0, (u64)3371008); 
	FSUSER_OpenFile(&bgmCache, archive, fsMakePath(PATH_ASCII, "/BgmCache.bin"), FS_OPEN_WRITE, 0);
	FSFILE_Write(bgmCache, &bytes, 0, body, (u64)bgmSize, FS_WRITE_FLUSH);
	FSFILE_Close(bgmCache);

	u8* themeManageBuffer = malloc((sizeof(u8) * 2048) * 8);
	Handle themeManageHandle;

	FSUSER_OpenFile(&themeManageHandle, 0x000002cc, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_WRITE, 0);
	FSFILE_Read(themeManageHandle, &bytes, 0, themeManageBuffer, (u32)2048);

	themeManageBuffer[0x00] = 1;
	themeManageBuffer[0x01] = 0;
	themeManageBuffer[0x02] = 0;
	themeManageBuffer[0x03] = 0;
	themeManageBuffer[0x04] = 0;
	themeManageBuffer[0x05] = 0;
	themeManageBuffer[0x06] = 0;
	themeManageBuffer[0x07] = 0;

	u32* bodySizeLocation = (u32*)(&themeManageBuffer[0x8]);
	u32* bgmSizeLocation = (u32*)(&themeManageBuffer[0xC]);
	*bodySizeLocation = (u32)bodySize;
	*bgmSizeLocation = (u32)bgmSize;

	themeManageBuffer[0x10] = 0xFF;
	themeManageBuffer[0x14] = 0x01;
	themeManageBuffer[0x18] = 0xFF;
	themeManageBuffer[0x1D] = 0x02;
	memset(&themeManageBuffer[0x338], 0, 4);
	memset(&themeManageBuffer[0x340], 0, 4);
	memset(&themeManageBuffer[0x360], 0, 4);
	memset(&themeManageBuffer[0x368], 0, 4);

	FSFILE_Write(themeManageHandle, &bytes, 0, themeManageBuffer, 2048, FS_WRITE_FLUSH);
	FSFILE_Close(themeManageHandle);

	return 0;
}


int main(int argc, char **argv)
{
	gfxInitDefault();
	cfguInit();
	
	consoleInit(GFX_TOP, NULL);

	u8 ret = themeInstall();

	printf("\x1b[29;15H%u", ret);
	
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
