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

#include "entry.h"
#include "file.h"
#include "draw.h"

#include <png.h>

struct SMDH {
    u8 _padding1[4 + 2 + 2];

    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];

    u8 _padding2[0x2000 - 0x200 + 0x30 + 0x8];
    u16 small_icon[24*24];

    u16 big_icon[48*48];
} PACKED;

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
            u32 dst = ((((j >> 3) * (512 >> 3) + (i >> 3)) << 6) + ((i & 1) | ((j & 1) << 1) | ((i & 2) << 1) | ((j & 2) << 2) | ((i & 4) << 2) | ((j & 4) << 3))) * 4;

            memcpy(static_cast<u8*>(this->image->tex->data) + dst, px, sizeof(u32));
        }
    }

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fh);
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

}

void BadgePreviewImage::draw() const
{

}

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

Entry::Entry(const fs::path& path, bool is_zip) : path(path), is_zip(is_zip)
{
    char* buf = nullptr;
    if(is_zip && path.extension() == ".png")  // Badge
    {
        this->title = path.stem();
    }
    else  // Theme or Splash
    {
        u32 size = this->get_file("info.smdh", &buf);
        if(buf != nullptr)
        {
            if(size == 0x36c0)
            {
                SMDH* smdh_buf = reinterpret_cast<SMDH*>(buf);

                char utf_title[0x40] = {0};
                utf16_to_utf8((u8*)utf_title, smdh_buf->name, 0x40);
                this->title = std::string(utf_title, 0x40);

                char utf_description[0x80] = {0};
                utf16_to_utf8((u8*)utf_description, smdh_buf->desc, 0x80);
                this->description = std::string(utf_description, 0x80);

                char utf_author[0x40] = {0};
                utf16_to_utf8((u8*)utf_author, smdh_buf->author, 0x40);
                this->author = std::string(utf_author, 0x40);
            }
            else
            {
                this->color = C2D_Color32(rand() % 256, rand() % 256, rand() % 256, 255);
            }
            delete[] buf;
        }
        else
        {
            this->color = C2D_Color32(rand() % 256, rand() % 256, rand() % 256, 255);
        }
    }

    if(this->author.empty())
        this->author = "Unknown author";
    if(this->description.empty())
        this->description = "No description";
    if(this->title.empty())
        this->title = "No title";
}

void Entry::draw() const
{
    static constexpr float x = 20.0f;
    float y = 35.0f;
    draw_text(TEXT_GENERAL, TEXT_ENTRY_BY, COLOR_WHITE, x, y, 0.2f, 0.5f, 0.5f);
    draw_text(this->author, COLOR_WHITE, x + get_text_width(TEXT_GENERAL, TEXT_ENTRY_BY, 0.5f), y, 0.2f, 0.5f, 0.5f);
    y += 30*0.5f + 2.0f;
    draw_text(this->title, COLOR_WHITE, x, y, 0.2f, 0.7f, 0.7f);
}

u32 Entry::get_file(const std::string& file_path, char** buf) const
{
    if(this->is_zip)
    {
        return zip_file_to_buf(file_path.c_str(), this->path, buf);
    }
    else
    {
        std::string full_path = this->path / file_path;
        return file_to_buf(fsMakePath(PATH_ASCII, full_path.c_str()), SD_CARD, buf);
    }
}

EntryIcon* Entry::load_icon() const
{
    if(this->color)
        return nullptr;

    EntryIcon* out = nullptr;

    if(this->is_zip && this->path.extension() == ".png")
    {
        out = new BadgeIcon(this->path);
    }
    else
    {
        char* buf = nullptr;
        u32 size = this->get_file("info.smdh", &buf);
        if(buf != nullptr)
        {
            if(size == 0x36c0)
            {
                SMDH* smdh_buf = reinterpret_cast<SMDH*>(buf);
                out = new EntryIcon(smdh_buf->big_icon);
            }
            delete[] buf;
        }
    }

    return out;
}

PreviewImage* Entry::load_preview() const
{
    if(this->is_zip && this->path.extension() == ".png")
    {
        return new BadgePreviewImage(this->path);
    }
    else
    {
        char* png_buf = nullptr;
        u32 png_size = this->get_file("preview.png", &png_buf);
        PreviewImage* preview = new PreviewImage(png_buf, png_size);

        if(png_buf != nullptr)
            delete[] png_buf;

        return preview;
    }
}

void Entry::delete_entry()
{

}

RemoteEntry::RemoteEntry(int entry_id) : Entry("/3ds/"  APP_TITLE  "/cache/" + std::to_string(entry_id), false)
{

}
