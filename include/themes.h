#ifndef THEMES_H
#define THEMES_H

typedef struct {
    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];
    char icon_data[0x1200];
    u16 path[262];
    ssize_t path_len;
    bool is_zip;
} theme;

Result single_install(theme*);

#endif