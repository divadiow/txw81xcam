#ifndef __CUSTOM_FONT_H
#define __CUSTOM_FONT_H
#include "osal/string.h"
#include "../../../lvgl.h"
#if LV_USE_FREETYPE
struct custom_font_s
{
	const char ttf_name[32];
	const unsigned char *font;
	unsigned int size;
};
extern void *find_custom_font(char *name,unsigned int *size);
#endif
#endif