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

#include "preview.h"
#include "draw.h"
#include <png.h>

static u16 nextPow2(u16 v)
{
    #define TEX_MIN_SIZE 64
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v++;
    return (v >= TEX_MIN_SIZE ? v : TEX_MIN_SIZE);
}

PreviewImage::PreviewImage()
{
    this->image = new(std::nothrow) C2D_Image;
    C3D_Tex* tex = new(std::nothrow) C3D_Tex;
    this->image->tex = tex;
}

PreviewImage::PreviewImage(char* png_buf, u32 png_size) : PreviewImage()
{
    if(png_buf == nullptr)
    {
        draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_NO_PREVIEW);
        return;
    }

    if(png_sig_cmp(reinterpret_cast<u8*>(png_buf), 0, 8))
    {
        draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_INVALID_PREVIEW_PNG);
        return;
    }

    FILE* fh = fmemopen(png_buf, png_size, "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fh);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);

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

    png_bytep* row_pointers = new(std::nothrow) png_bytep[height];
    for(int y = 0; y < height; y++)
        row_pointers[y] = new(std::nothrow) png_byte[png_get_rowbytes(png,info)];

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fh);

    Tex3DS_SubTexture* subt3x = new(std::nothrow) Tex3DS_SubTexture;
    subt3x->width = width;
    subt3x->height = height;
    subt3x->left = 0.0f;
    subt3x->top = 1.0f;
    subt3x->right = width/512.0f;
    subt3x->bottom = 1.0-(height/512.0f);
    this->image->subtex = subt3x;

    C3D_TexInit(this->image->tex, 512, 512, GPU_RGBA8);

    for(int j = 0; j < height; j++)
    {
        png_bytep row = row_pointers[j];
        for(int i = 0; i < width; i++)
        {
            png_bytep px = &(row[i * sizeof(u32)]);
            u32 dst = ((((j >> 3) * (512 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy(static_cast<u8*>(this->image->tex->data) + dst, px, sizeof(u32));
        }
        delete[] row;
    }
    delete[] row_pointers;

    this->ready = true;
}

PreviewImage::~PreviewImage()
{
    C3D_TexDelete(this->image->tex);
    delete this->image->tex;
    delete this->image->subtex;
    delete this->image;
}

void PreviewImage::draw() const
{
    start_frame(COLOR_BLACK);
    switch_screen(GFX_TOP);
    const float x_offset = -(this->image->subtex->width - 400.0f)/2.0f;
    C2D_DrawImageAt(*(this->image), x_offset, 0.0f, 0.0f);
    switch_screen(GFX_BOTTOM);
    C2D_DrawImageAt(*(this->image), x_offset - 40.0f, -240.0f, 0.0f);
    end_frame();
}

BadgePreviewImage::BadgePreviewImage(const fs::path& path)
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

    png_bytep* row_pointers = new(std::nothrow) png_bytep[height];
    for(int y = 0; y < height; y++)
        row_pointers[y] = new(std::nothrow) png_byte[png_get_rowbytes(png,info)];

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fh);

    u16 w_nextPow2 = nextPow2(width);
    u16 h_nextPow2 = nextPow2(height);

    Tex3DS_SubTexture* subt3x = new(std::nothrow) Tex3DS_SubTexture;
    subt3x->width = width;
    subt3x->height = height;
    subt3x->left = 0.0f;
    subt3x->top = 1.0f;
    subt3x->right = static_cast<float>(width)/w_nextPow2;
    subt3x->bottom = 1.0-(static_cast<float>(height)/h_nextPow2);
    this->image->subtex = subt3x;

    C3D_TexInit(this->image->tex, w_nextPow2, h_nextPow2, GPU_RGBA8);

    for(int j = 0; j < height; j++)
    {
        png_bytep row = row_pointers[j];
        for(int i = 0; i < width; i++)
        {
            png_bytep px = &(row[i * sizeof(u32)]);
            u32 dst = ((((j >> 3) * (w_nextPow2 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy(static_cast<u8*>(this->image->tex->data) + dst, px, sizeof(u32));
        }
        delete[] row;
    }
    delete[] row_pointers;

    this->ready = true;
}

void BadgePreviewImage::draw() const
{
    start_frame(COLOR_BLACK);
    switch_screen(GFX_TOP);
    C2D_DrawImageAt(*(this->image), 0.0f, 0.0f, 0.0f);
    switch_screen(GFX_BOTTOM);
    C2D_DrawImageAt(*(this->image), 0.0f, -240.0f, 0.0f);
    end_frame();
}
