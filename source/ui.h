#ifndef UI_H
#define UI_H

#include <errno.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <3ds.h>
#include <citro3d.h>

#define TOP_SCREEN_WIDTH 			400
#define TOP_SCREEN_HEIGHT 			240

#define BOTTOM_SCREEN_WIDTH 		320
#define BOTTOM_SCREEN_HEIGHT 		240

#define MAX_TEXTURES 				1024

#define TEXTURE_BOTTOM_SCREEN_BG 	0
#define TEXTURE_TOP_SCREEN_BG 		1
#define TEXTURE_ICON 				2
#define TEXTURE_DRIVE_ICON 			3

#define RGBA8(r, g, b, a) 			((((a)&0xFF)<<24) | (((b)&0xFF)<<16) | (((g)&0xFF)<<8) | (((r)&0xFF)<<0))

#define COLOUR_MAINMENU 			RGBA8(78, 74, 67, 255)
#define COLOUR_MAINMENU_HIGHLIGHT 	RGBA8(250, 237, 227, 255)
#define COLOUR_MENU 				RGBA8(0, 0, 0, 255)
#define COLOUR_SUBJECT 				RGBA8(120, 118, 115, 255)
#define COLOUR_VALUE 				RGBA8(67, 72, 66, 255)
#define CLEAR_COLOR 				0x000000FF

void screen_init(void);
void screen_exit(void);
void screen_set_base_alpha(u8 alpha);
u32 screen_allocate_free_texture(void);
void screen_load_texture_untiled(u32 id, void* data, u32 size, u32 width, u32 height, GPU_TEXCOLOR format, bool linearFilter);
void screen_load_texture_file(u32 id, const char* path, bool linearFilter);
void screen_load_texture_tiled(u32 id, void* data, u32 size, u32 width, u32 height, GPU_TEXCOLOR format, bool linearFilter);
void screen_unload_texture(u32 id);
void screen_get_texture_size(u32* width, u32* height, u32 id);
void screen_begin_frame(void);
void screen_end_frame(void);
void screen_select(gfxScreen_t screen);
void screen_draw_texture(u32 id, float x, float y) ;
void screen_draw_texture_crop(u32 id, float x, float y, float width, float height);
int screen_get_texture_width(u32 id);
int screen_get_texture_height(u32 id);
void screen_get_string_size(float* width, float* height, const char* text, float scaleX, float scaleY);
void screen_get_string_size_wrap(float* width, float* height, const char* text, float scaleX, float scaleY, float wrapWidth);
float screen_get_string_width(const char * text, float scaleX, float scaleY);
float screen_get_string_height(const char * text, float scaleX, float scaleY);
void screen_draw_string(float x, float y, float scaleX, float scaleY, u32 color, const char * text);
void screen_draw_stringf(float x, float y, float scaleX, float scaleY, u32 color, const char * text, ...);
void screen_draw_string_wrap(float x, float y, float scaleX, float scaleY, u32 color, float wrapX, const char * text);
void screen_draw_rect(float x, float y, float width, float height, u32 color);

#endif