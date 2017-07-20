#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"

int main(void)
{
	gfxInitDefault();
	cfguInit();
	srvInit();	
	hidInit();
	fsInit();	
	ptmSysmInit();
	consoleInit(GFX_TOP, NULL);

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_A) prepareThemes();
		if (kDown & KEY_START)
		{
			closeThemeArchives();	
			PTMSYSM_ShutdownAsync(0);
			ptmSysmExit();
		}
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}
