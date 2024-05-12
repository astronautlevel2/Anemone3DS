#ifndef CONVERISON_H
#define CONVERISON_H

#include "common.h"

size_t bin_to_abgr(char ** bufp, size_t size);
size_t png_to_abgr(char ** bufp, size_t size, u32 *height);

#endif
