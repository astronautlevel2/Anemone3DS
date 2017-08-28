#ifndef THEMES_H
#define THEMES_H

#include <3ds.h>

typedef struct {
    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];
    char icon_data[0x1200];
    u16 path[262];
    bool is_zip;
    bool selected;
} theme;

void parse_smdh(theme *entry, u16 *path);
int scan_themes(theme **themes, int num_themes);
Result single_install(theme);
Result shuffle_install(theme **themes_list, int num_themes);

#endif