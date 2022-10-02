#include "conversion.h"
#include "draw.h"

#include <png.h>

static void rotate_agbr_counterclockwise(char** bufp, size_t size, size_t width)
{
    uint32_t* buf = (uint32_t*)*bufp;
    uint32_t* out = malloc(size);

    size_t pixel_count = (size/4);
    size_t height = pixel_count/width;

    for (uint32_t h = 0; h < height; ++h)
    {
        for (uint32_t w = 0; w < width; ++w)
        {
            size_t buf_index = (w + (h * width));
            size_t out_index = (height * (width-1)) - (height * w) + h;

            out[out_index] = buf[buf_index];
        }
    }

    free(*bufp);
    *bufp = (char*)out;
}

size_t bin_to_agbr(char** bufp, size_t size)
{
    size_t out_size = (size / 3) * 4;
    char* buf = malloc(out_size);

    for (size_t i = 0, j = 0; i < size; i+=3, j+=4)
    {
        (buf+j)[0] = 0xFF;         // A
        (buf+j)[1] = (*bufp+i)[0]; // B
        (buf+j)[2] = (*bufp+i)[1]; // G
        (buf+j)[3] = (*bufp+i)[2]; // R
    }
    free(*bufp);
    *bufp = buf;

    // splash screens contain the raw framebuffer to put on the screen
    // because the screens are mounted at a 90 degree angle we also need to rotate
    // the output
    rotate_agbr_counterclockwise(bufp, out_size, 240);

    return out_size;
}

size_t png_to_abgr(char** bufp, size_t size)
{
    size_t out_size = 0;
    if(size < 8 || png_sig_cmp((png_bytep)*bufp, 0, 8))
    {
        throw_error("Invalid preview.png", ERROR_LEVEL_WARNING);
        return out_size;
    }

    uint32_t* buf = (uint32_t*)*bufp;

    FILE* fp = fmemopen(buf, size, "rb");;
    png_bytep* row_pointers = NULL;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fp);
    png_read_info(png, info);

    u32 width = png_get_image_width(png, info);
    u32 height = png_get_image_height(png, info);

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
        png_set_add_alpha(png, 0xFF, PNG_FILLER_BEFORE);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    //output ABGR
    png_set_bgr(png);
    png_set_swap_alpha(png);

    png_read_update_info(png, info);

    row_pointers = malloc(sizeof(png_bytep) * height);
    out_size = sizeof(u32) * (width * height);
    u32* out = malloc(out_size);
    for(u32 y = 0; y < height; y++)
    {
        row_pointers[y] = (png_bytep)(out + (width * y));
    }

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);

    if (fp) fclose(fp);
    if (row_pointers) free(row_pointers);

    free(*bufp);
    *bufp = (char*)out;

    return out_size;
}