#ifndef __draw__
#define __draw__

#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <string.h>
#include "draw.h"

void drawObjects() {
	sceGuFinish();
	sceGuSync(0, 0);
	sceDisplayWaitVblankStart();
	swapBuffers();
}

void swapBuffers() { sceGuSwapBuffers(); }

void drawString(const char* text, int x, int y, unsigned int color, int fw) {
	int len = (int)strlen(text);
	if (!len) return;

	typedef struct {
		float s, t;
		unsigned int c;
		float x, y, z;
	} VERT;

	VERT* v = sceGuGetMemory(sizeof(VERT) * 2 * len);

	int i;
	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char)text[i];
		if(c < 32) { c = 0; } else if(c >= 128) { c = 0; }

		int tx = (c & 0x0F) << 4;
		int ty = (c & 0xF0);

		VERT* v0 = &v[i*2+0];
		VERT* v1 = &v[i*2+1];
		
		v0->s = (float)(tx + (fw ? ((16 - fw) >> 1) : ((16 - fontwidthtab[c]) >> 1)));
		v0->t = (float)(ty);
		v0->c = color;
		v0->x = (float)(x);
		v0->y = (float)(y);
		v0->z = 0.0f;

		v1->s = (float)(tx + 16 - (fw ? ((16 - fw) >> 1) : ((16 - fontwidthtab[c]) >> 1)));
		v1->t = (float)(ty + 16);
		v1->c = color;
		v1->x = (float)(x + (fw ? fw : fontwidthtab[c]));
		v1->y = (float)(y + 16);
		v1->z = 0.0f;

		x += (fw ? fw : fontwidthtab[c]);
	}

	sceGumDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D, len * 2, 0, v);
}

void drawStringCenter(const char* text, int y, unsigned int color, int fw) {
	int len = (int)strlen(text), xx, width = 0;
	if (!len) return;
	int i;
	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char)text[i];
		if(c < 32) { c = 0; } else if(c >= 128) { c = 0; }
		width = width + (fw ? fw : fontwidthtab[c]);
	}
	xx = (480 - width) / 2;
	drawString(text, xx, y, color, fw);
}
#endif
