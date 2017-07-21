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
            node *theme_node = malloc(sizeof(node));
            theme_node->data = NULL;
            theme_node->next = NULL;
            prepareThemes(theme_node);
            node *splashes_node = malloc(sizeof(node));
            splashes_node->data = NULL;
            splashes_node->next = NULL;
            prepareSplashes(splashes_node);
            node *current_node = splashes_node->next;
            while (current_node != NULL)
            {
                printu((u16*)current_node->data);
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
