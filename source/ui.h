#pragma once

struct rgbColor
{
	char rC;
	char gC;
	char bC;
};

typedef struct rgbColor Color;

Color makeColor(char r, char g, char b);
s8 drawPixel(u32 x, u32 y, Color c, u8* screen);
s8 drawLine(u32 x1, u32 y1, u32 x2, u32 y2, Color c, u8* screen);
s8 drawRect(u32 x1, u32 y1, u32 x2, u32 y2, Color c, u8* screen);
