#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"

int main(int argc, char **argv)
{
	gfxInitDefault();
	cfguInit();
	srvInit();	
	hidInit();
	fsInit();	
	ptmSysmInit();

	consoleInit(GFX_TOP, NULL);


	printf("\x1b[20;10HN3DSThemeManager");

	
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
		{
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
