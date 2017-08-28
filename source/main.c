#include <stdlib.h>
#include <stdio.h>
#include <3ds.h>
#include <string.h>

#include "fs.h"
#include "themes.h"
#include "unicode.h"

int init_services(void)
{
    gfxInitDefault();
    cfguInit();
    srvInit();  
    hidInit();
    fsInit();   
    ptmSysmInit();
    open_archives();
    return 0;
}

int de_init_services(void)
{
    gfxExit();
    cfguExit();
    srvExit();
    hidExit();
    fsExit();
    ptmSysmExit();
    close_archives();
    return 0;
}

int main(void)
{
    init_services();
    consoleInit(GFX_TOP, NULL);

    int theme_count = get_number_entries("/Themes");
    printf("Theme count: %i\n", theme_count);
    theme **themes_list = calloc(theme_count, sizeof(theme));
    scan_themes(themes_list, theme_count);
    
    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A)
        {
            for (int i = 0; i < theme_count; i++)
            {
                printu(themes_list[i]->name);
                printu(themes_list[i]->path);
            }
        } 
        if (kDown & KEY_B)
        {
            shuffle_install(themes_list, theme_count);
            close_archives();
            PTMSYSM_ShutdownAsync(0);
            ptmSysmExit();
        }
        if (kDown & KEY_START)
        {
            close_archives();   
            PTMSYSM_ShutdownAsync(0);
            ptmSysmExit();
        }
    }

    de_init_services();
    return 0;
}
