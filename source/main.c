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

	s8 ret = prepareThemes();
	s8 ret2 = themeInstall();

	printf("\x1b[20;10H%d", ret);
	printf("\x1b[29;15H%d", ret2);
	
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
