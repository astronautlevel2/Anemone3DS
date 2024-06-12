#include "conversion.h"
#include "draw.h"

#include <png.h>

// don't be fooled - this function always expects 64x64 input buffers. Width/height only
// control output resolution
int rgb565ToPngFile(char *filename, u16 *rgb_buf, u8 *alpha_buf, int width, int height)
{
    FILE *fp = fopen(filename, "wb");
    u8 *image = malloc(64 * 64 * 4);
    if (!image) return -1;

    int i, x, y, r, g, b, a;

    for (i = 0; i <  64 * 64; ++i)
    {
        r = (rgb_buf[i] & 0xF800) >> 11;
        g = (rgb_buf[i] & 0x07E0) >> 5;
        b = (rgb_buf[i] & 0x001F);
        a = (alpha_buf[i/2] >> (4*(i%2))) & 0x0F;

        r = round(r * 255.0 / 31.0);
        g = round(g * 255.0 / 63.0);
        b = round(b * 255.0 / 31.0);
        a = a * 0x11;
        x = 8*((i/64)%8) + (((i%64)&0x01) >> 0) + (((i%64)&0x04) >> 1) + (((i%64)&0x10) >> 2);
        y = 8*(i/512) + (((i%64)&0x02) >> 1) + (((i%64)&0x08) >> 2) + (((i%64)&0x20) >> 3);
        image[y * 64 * 4 + x * 4 + 0] = r;
        image[y * 64 * 4 + x * 4 + 1] = g;
        image[y * 64 * 4 + x * 4 + 2] = b;
        image[y * 64 * 4 + x * 4 + 3] = a;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    setjmp(png_jmpbuf(png));
    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_bytep row = malloc(4 * width * sizeof(png_byte));
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            row[x * 4 + 0] = image[(y * 64 + x) * 4 + 0];
            row[x * 4 + 1] = image[(y * 64 + x) * 4 + 1];
            row[x * 4 + 2] = image[(y * 64 + x) * 4 + 2];
            row[x * 4 + 3] = image[(y * 64 + x) * 4 + 3];
        }

        png_write_row(png, row);
    }

    png_write_end(png, info);
    png_free_data(png, info, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png, NULL);
    free(row);
    free(image);
    fflush(fp);
    fclose(fp);

    return 0;
}

int pngToRGB565(char *png_buf, u64 fileSize, u16 *rgb_buf_64x64, u8 *alpha_buf_64x64, u16 *rgb_buf_32x32, u8 *alpha_buf_32x32, bool set_icon)
{
    if (png_buf == NULL) return 0;
    if (fileSize < 8 || png_sig_cmp((png_bytep) png_buf, 0, 8))
    {
        return 0;
    }

    u32 *buf = (u32 *) png_buf;
    FILE *fp = fmemopen(buf, fileSize, "rb");
    png_bytep *row_pointers = NULL;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fp);
    png_read_info(png, info);

    u32 width = png_get_image_width(png, info);
    u32 height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (set_icon && (width != 48 || height != 48))
    {
        DEBUG("Invalid set icon?\n");
        return 0;
    }
    if (!set_icon && (width < 64 || height < 64 || width % 64 != 0 || height % 64 != 0 || width > 12 * 64 || height > 6 * 64))
    {
        DEBUG("Invalid png found...\n");
        return 0;
    }
    
    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    u32 x, y, r, g, b, a;

    memset(alpha_buf_64x64, 0, 12*6*64*64/2);
    memset(alpha_buf_32x32, 0, 12*6*32*32/2);

    row_pointers = malloc(sizeof(png_bytep) * height);
    for (y = 0; y < height; y++)
    {
        row_pointers[y] = (png_byte *) malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, row_pointers);

    png_destroy_read_struct(&png, &info, NULL);

    if (fp) fclose(fp);
    for (y = 0; y < height; ++y)
    {
        png_bytep row = row_pointers[y];
        for (x = 0; x < width; ++x)
        {
            png_bytep px = &(row[x * 4]);
            r = px[0] >> 3;
            g = px[1] >> 2;
            b = px[2] >> 3;
            a = px[3] >> 4;
            
            // rgb565 conversion code adapted from GYTB
            int rgb565_index = 8*64*((y/8)%8) | 64*((x/8)%8) | 32*((y/4)%2) | 16*((x/4)%2) | 8*((y/2)%2) | 4*((x/2)%2) | 2*(y%2) | (x%2);
            rgb565_index |= 64*64*(height/64)*(x/64) + 64*64*(y/64); // account for multiple badges from 1 image
            rgb_buf_64x64[rgb565_index] = (r << 11) | (g << 5) | b;
            alpha_buf_64x64[rgb565_index / 2] |= a << (4 * (x % 2));
        }
    }

    for (y = 0; y < height; y += 2)
    {
        png_bytep row = row_pointers[y];
        png_bytep nextrow = row_pointers[y+1];
        for (x = 0; x < width; x += 2)
        {
            png_bytep px1 = &(row[x * 4]);
            png_bytep px2 = &(nextrow[x * 4]);
            png_bytep px3 = &(row[(x + 1) * 4]);
            png_bytep px4 = &(nextrow[(x + 1) * 4]);
            r = (px1[0] + px2[0] + px3[0] + px4[0]) >> 5;
            g = (px1[1] + px2[1] + px3[1] + px4[1]) >> 4;
            b = (px1[2] + px2[2] + px3[2] + px4[2]) >> 5;
            a = (px1[3] + px2[3] + px3[3] + px4[3]) >> 6;
            int x2 = x/2;
            int y2 = y/2;

            int rgb565_index = 4*64*((y2/8)%4) | 64*((x2/8)%4) | 32*((y2/4)%2) | 16*((x2/4)%2) | 8*((y2/2)%2) | 4*((x2/2)%2) | 2*(y2%2) | (x2%2);
            rgb565_index |= 32*32*(height/64)*(x/64) + 32*32*(y/64);

            rgb_buf_32x32[rgb565_index] = (r << 11) | (g << 5) | b;
            alpha_buf_32x32[rgb565_index / 2] |= a << (4 * (x2%2));
        }
    }

    for (y = 0; y < height; y++)
    {
        free(row_pointers[y]);
    }

    free(row_pointers);
    if (!set_icon)
        return (height/64)*(width/64);
    else
        return (height/48)*(width/48);
} 

static void rotate_agbr_counterclockwise(char ** bufp, size_t size, size_t width)
{
    uint32_t * buf = (uint32_t*)*bufp;
    uint32_t * out = malloc(size);

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

size_t bin_to_abgr(char ** bufp, size_t size)
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

size_t png_to_abgr(char ** bufp, size_t size, u32 *height)
{
    size_t out_size = 0;
    if(size < 8 || png_sig_cmp((png_bytep)*bufp, 0, 8))
    {
        throw_error("Invalid preview.png", ERROR_LEVEL_WARNING);
        return out_size;
    }

    uint32_t * buf = (uint32_t*)*bufp;

    FILE * fp = fmemopen(buf, size, "rb");;
    png_bytep * row_pointers = NULL;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fp);
    png_read_info(png, info);

    u32 width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);

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

    row_pointers = malloc(sizeof(png_bytep) * *height);
    out_size = sizeof(u32) * (width * *height);
    u32 * out = malloc(out_size);
    for(u32 y = 0; y < *height; y++)
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
