#pragma once

void wtoa(char*, u16*);
ssize_t atow(u16*, char*);
ssize_t trim_extension(u16*, u16*);
bool strip_unicode(u16*, u16*, ssize_t);
u16 *strucpy(u16*, const u16*);
ssize_t strulen(const u16*);
u16 *strucat(u16*, const u16*);