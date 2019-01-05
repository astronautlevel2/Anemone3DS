/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2018 Contributors in CONTRIBUTORS.md
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include "icons.h"

#include <png.h>

EntryIcon::EntryIcon()  // For badges
{
    this->image = new(std::nothrow) C2D_Image;
    C3D_Tex* tex = new(std::nothrow) C3D_Tex;
    static const Tex3DS_SubTexture subt3x = { 64, 64, 0.0f, 1.0f, 1.0f, 0.0f };
    this->image->tex = tex;
    this->image->subtex = &subt3x;
    C3D_TexInit(this->image->tex, 64, 64, GPU_RGBA8);
    memset(this->image->tex->data, 0, this->image->tex->size);

    C3D_TexSetFilter(image->tex, GPU_LINEAR, GPU_LINEAR);
    image->tex->border = 0xFFFFFFFF;
    C3D_TexSetWrap(image->tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
}

EntryIcon::EntryIcon(u16* pixels)  // For SMDHs
{
    this->image = new(std::nothrow) C2D_Image;
    C3D_Tex* tex = new(std::nothrow) C3D_Tex;
    static const Tex3DS_SubTexture subt3x = { 48, 48, 0.0f, 48/64.0f, 48/64.0f, 0.0f };
    this->image->tex = tex;
    this->image->subtex = &subt3x;
    C3D_TexInit(this->image->tex, 64, 64, GPU_RGB565);

    u16* dest = static_cast<u16*>(this->image->tex->data) + (64-48)*64;
    for(int j = 0; j < 48; j += 8)
    {
        memcpy(dest, pixels, 48*8*sizeof(u16));
        pixels += 48*8;
        dest += 64*8;
    }

    C3D_TexSetFilter(image->tex, GPU_LINEAR, GPU_LINEAR);
    image->tex->border = 0xFFFFFFFF;
    C3D_TexSetWrap(image->tex, GPU_CLAMP_TO_BORDER, GPU_CLAMP_TO_BORDER);
}

EntryIcon::~EntryIcon()
{
    C3D_TexDelete(this->image->tex);
    delete this->image->tex;
    delete this->image;
}

BadgeIcon::BadgeIcon(const fs::path& path)
{
    FILE* fh = fopen(path.c_str(), "rb");

    u8 sig[8];
    fread(sig, 1, 8, fh);
    if(png_sig_cmp(sig, 0, 8))
    {
        fclose(fh);
        return;
    }

    fseek(fh, 0, SEEK_SET);

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fh);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    if(width < 64 || height < 64)
    {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fh);
        return;
    }

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth  = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, ABGR format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    //output ABGR
    png_set_bgr(png);
    png_set_swap_alpha(png);

    png_read_update_info(png, info);

    png_size_t row_bytes = png_get_rowbytes(png,info);
    png_byte row[row_bytes];
    png_byte disp_row[row_bytes];
    for(int j = 0; j < 64; j++)
    {
        png_read_row(png, row, disp_row);
        for(int i = 0; i < 64; i++)
        {
            png_bytep px = &(row[i * sizeof(u32)]);
            u32 dst = ((((j >> 3) * (64 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy(static_cast<u8*>(this->image->tex->data) + dst, px, sizeof(u32));
        }
    }

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fh);
}
