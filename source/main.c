#include <stdlib.h>

#include "fs.h"
#include "themes.h"

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

    int theme_count = get_number_entries("/Themes");
    theme** theme_list = calloc(theme_count, sizeof(theme*));
    
    free(theme_list);
    de_init_services();
    return 0;
}
