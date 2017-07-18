#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"

Color makeColor(char r, char g, char b)
{
	Color n;
	n.rC = r;
	n.gC = g;
	n.bC = b;

	return n;
}

s8 drawPixel(u32 x, u32 y, Color c, u8* screen)
{
	u8 height=240;
	u32 v=(height-1-y+x*height)*3;
	screen[v]=c.bC;
	screen[v+1]=c.gC;
	screen[v+2]=c.rC;

 return 0;
}

s8 drawLine(u32 x1, u32 y1, u32 x2, u32 y2, Color c, u8* screen)
{
	u32 x, y;
	if(x1 == x2)
	{
		if (y1<y2) for (y = y1; y < y2; y++) drawPixel(x1,y,c,screen);
			else for (y = y2; y < y1; y++) drawPixel(x1,y,c,screen);
	}else{
		if (x1<x2) for (x = x1; x < x2; x++) drawPixel(x,y1,c,screen);
			else for (x = x2; x < x1; x++) drawPixel(x,y1,c,screen);
	}
 return 0;
}

s8 drawRect(u32 x1, u32 y1, u32 x2, u32 y2, Color c, u8* screen)
{
	drawLine( x1, y1, x2, y1, c, screen);
	drawLine( x2, y1, x2, y2, c, screen);
	drawLine( x1, y2, x2, y2, c, screen);
	drawLine( x1, y1, x1, y2, c, screen);
 return 0;
}
