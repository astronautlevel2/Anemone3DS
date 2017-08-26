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
    
    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A)
        {        
            theme *theme = malloc(sizeof(theme));
            u16 path[262] = {0};
            struacat(path, "/Themes/Saber Lily");
            parse_smdh(theme, path);
            printu(theme->name);
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
