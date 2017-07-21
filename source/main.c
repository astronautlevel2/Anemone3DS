#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"
#include "unicode.h"

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

		if (kDown & KEY_A) {
			node *first_node = malloc(sizeof(node));
			first_node->data = NULL;
			first_node->next = NULL;
			prepareThemes(first_node);
			node *current_node = first_node->next;
    		while (current_node != NULL)
    		{
        		printu(((theme_data *)current_node->data)->title);
        		current_node = current_node->next;
    		}
    		puts("Done!");
		}
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
