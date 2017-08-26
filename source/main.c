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
    // close_archives(); // TODO: Implement
    return 0;
}

int main(void)
{
    init_services();
    consoleInit(GFX_TOP, NULL);

    int theme_count = get_number_entries("/Themes");
    theme *theme = calloc(1, sizeof(theme));
    u16 path[262] = {0};
    utf8_to_utf16(path, (u8*)"/Themes/[11115] Saber Lily by kiss7938.zip", 262 * sizeof(u16));
    memcpy(theme->path, path, 262 * sizeof(u16));
    theme->is_zip = true;
    single_install(*theme);
    
    while(aptMainLoop())
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
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
