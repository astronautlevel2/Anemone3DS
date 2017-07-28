#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"
#include "splashes.h"
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
    prepare_archives();
    unzip_themes();
    int theme_count = get_number_entries("/Themes");
    theme_data **themes_list = calloc(theme_count, sizeof(theme_data));
    prepare_themes(themes_list);
    int splash_count = get_number_entries("/Splashes");
    u16 *splashes_list = calloc(splash_count, PATH_LENGTH);
    prepare_splashes(splashes_list);
    printf("Theme count: %i\nSplash count: %i\n", theme_count, splash_count);
    
    while (aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_A) {
            for (int iter = 0; iter < splash_count; iter++)
            {
                printu(&splashes_list[iter * PATH_LENGTH/sizeof(u16)]);
            }
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
