#pragma once

void wtoa(char*, u16*);
ssize_t atow(u16*, char*);
ssize_t trim_extension(u16*, u16*, ssize_t);
bool strip_unicode(u16*, u16*, ssize_t);