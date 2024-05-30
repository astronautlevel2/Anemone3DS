#ifndef CONVERISON_H
#define CONVERISON_H

#include "common.h"

size_t bin_to_abgr(char ** bufp, size_t size);
size_t png_to_abgr(char ** bufp, size_t size, u32 *height);
int pngToRGB565(char *png_buf, u64 fileSize, u16 *rgb_buf_64x64, u8 *alpha_buf_64x64, u16 *rgb_buf_32x32, u8 *alpha_buf_32x32, bool set_icon);

#endif
