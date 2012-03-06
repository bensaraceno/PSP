#include <pspdisplay.h>
#include <pspdebug.h>
#include <string.h>

#define ALPHA_BLEND 1

extern u8 msx[];
extern void pspDebugScreenSetInit();
/*
#if ALPHA_BLEND
static u32 adjust_alpha(u32 col) {
	u32 alpha = col >> 24;
	u8 mul; u32 c1, c2;

	if (alpha == 0x00)    return col;
	if (alpha == 0xFF) return col;

	c1 = col & 0x00FF00FF;
	c2 = col & 0x0000FF00;
	mul = (u8)(255 - alpha);
	c1 = ((c1 * mul) >> 8) & 0x00FF00FF;
	c2 = ((c2 * mul) >> 8) & 0x0000FF00;
	return (alpha << 24) | c1 | c2;
}
#endif
*/
int blit_string(int sx, int sy, const char *msg, int fg_col, int bg_col, int fontSize, int hSpacing, int vSpacing, int addShadow) {
	int x, y, p, offset, offset2, pwidth, pheight, bufferwidth, pixelformat, unk;
	char code; unsigned char font; unsigned int* vram32;

#if ALPHA_BLEND
	u32 col, c1, c2, alpha;
	//fg_col = adjust_alpha(fg_col);
	//bg_col = adjust_alpha(bg_col);
#endif

	// Text flickers if sx = 0 or if sy <= 16.
	if (sx <= 0) sx = 1;
	if (sy <= 0) sy = 1;
   	sceDisplayGetMode(&unk, &pwidth, &pheight);
   	sceDisplayGetFrameBuf((void*)&vram32, &bufferwidth, &pixelformat, &unk);
    if (bufferwidth == 0) return 0;
    if (pixelformat == 3) {
	    sy += vSpacing;

		/* This is the main font blitting portion offset 1x1 to product a drop shadow. */
		if (addShadow) {
	    	for (x = 0; msg[x] && x < (pwidth / 8); x++) {
	    		code = msg[x] & 0x7f; // 7bit ANK
	    		for (y = 0; y < 8; y++) {
	    			offset2 = (sy + y + 1) * bufferwidth + ((sx + x) * (8 * hSpacing)) + 1;
	    			font = msx[code * 8 + y];
					for (p = 0; p < 8; p++) {
					    if (font & 0x80) vram32[offset2] = bg_col;
					    font <<= 1; offset2++;
					}
	    		}
	    	}
		}

		/* This is the main font blitting portion. */
    	for (x = 0; msg[x] && x < (pwidth / 8); x++) {
    		code = msg[x] & 0x7f; // 7bit ANK
    		for (y = 0; y < 8; y++) {
    			offset = (sy + y) * bufferwidth + (sx + x) * (8 * hSpacing);
    			font = msx[code * 8 + y];
#if ALPHA_BLEND
				for (p = 0; p < 8; p++) {
					col = fg_col; alpha = col >> 24;
					if ((alpha == 0x00) | (alpha == 0xFF)) {
						if (font & 0x80)  vram32[offset] = col;
					} else if (alpha != 0xFF) {
						c2 = vram32[offset];
						c1 = c2 & 0x00FF00FF;
						c2 = c2 & 0x0000FF00;
						c1 = ((c1 * alpha) >> 8) & 0x00FF00FF;
						c2 = ((c2 * alpha) >> 8) & 0x0000FF00;
						if (font & 0x80) vram32[offset] = (col & 0xFFFFFF) + c1 + c2;
					}
				    font <<= 1; offset++;
				}
#else
				for (p = 0; p < 8; p++) {
				    if (font & 0x80) vram32[offset] = fg_col;
				    font <<= 1; offset++;
				}
#endif
    		}
    	}
    return x;
    }
	return 0;
}

int blit_string_ctr(int sy, const char *msg, int fg_col, int bg_col, int fontSize, int hSpacing, int vSpacing, int addShadow) {
	int sx = 480 / 2 - strlen(msg) * (8 / 2);
	return blit_string(sx, sy, msg, fg_col, bg_col, fontSize, hSpacing, vSpacing, addShadow);
}

int blit_pixel(int sx, int sy, int fg_col, int bg_col) {
	int offset, offset2, pwidth, pheight, bufferwidth, pixelformat, unk; unsigned int* vram32;

#if ALPHA_BLEND
	u32 col, c1, c2, alpha;
	//fg_col = adjust_alpha(fg_col);
	//bg_col = adjust_alpha(bg_col);
#endif

	// Text flickers if sx = 0 or if sy <= 16.
	if (sx <= 0) sx = 1;
	if (sy <= 0) sy = 1;
   	sceDisplayGetMode(&unk, &pwidth, &pheight);
   	sceDisplayGetFrameBuf((void*)&vram32, &bufferwidth, &pixelformat, &unk);
    if (bufferwidth == 0) return 0;
    if (pixelformat == 3) {
		/* This is the main pixel blitting portion offset 1x1 to product a drop shadow. */
	    offset2 = (sy + 1) * bufferwidth + (sx + 1);
		vram32[offset2] = bg_col;
		/* This is the main pixel blitting portion. */
    	offset = (sy) * bufferwidth + (sx);
#if ALPHA_BLEND
		col = fg_col; alpha = col >> 24;
		if ((alpha == 0x00) | (alpha == 0xFF)) {
			vram32[offset] = col;
		} else if (alpha != 0xFF) {
			c2 = vram32[offset];
			c1 = c2 & 0x00FF00FF;
			c2 = c2 & 0x0000FF00;
			c1 = ((c1 * alpha) >> 8) & 0x00FF00FF;
			c2 = ((c2 * alpha) >> 8) & 0x0000FF00;
			vram32[offset] = (col & 0xFFFFFF) + c1 + c2;
		}
#else
		vram32[offset] = fg_col;
#endif
    return 1;
    }
	return 0;
}

/*int blit_circle(int sx, int sy, int r, int fg_col, int bg_col) {
	int i, j;
	for (i = (sy - r); i < (sy + r); i++) {
		for (j = (sx - r); j < (sx + r); j++) {
			if (i * i + j * j < r * r) blit_pixel(j, i, fg_col, bg_col);
		}
	}
	return 1;
}

void drawLine(int x1, int y1, int x2, int y2, int fg_col, int bg_col) {
	float k, b, y;
	k = (float)(y2 - y1) / (x2 - x1);
	b = y1 - k * x1;
	if (x2 * x2; x++) {
		y = k * x + b;
		blit_pixel(x, int(y), fg_col, bg_col);
	}
}*/

void drawCircle(int Xc, int Yc, int radius, int fg_col, int bg_col) {
	int x, y, d;
	d = 3 - (radius << 1);
	x = 0;
	y = radius;

	while(x != y) {
		blit_pixel(Xc + x, Yc + y, fg_col, bg_col);
		blit_pixel(Xc + x, Yc - y, fg_col, bg_col);
		blit_pixel(Xc - x, Yc + y, fg_col, bg_col);
		blit_pixel(Xc - x, Yc - y, fg_col, bg_col);
		blit_pixel(Xc + y, Yc + x, fg_col, bg_col);
		blit_pixel(Xc + y, Yc - x, fg_col, bg_col);
		blit_pixel(Xc - y, Yc + x, fg_col, bg_col);
		blit_pixel(Xc - y, Yc - x, fg_col, bg_col);

		if (d > 0) {
		    d = d + (x << 2) + 6;
		} else {
		    d = d + ((x - y) << 2) + 10;
		    y--;
		}
		x++;
  }
}

void drawSquare(int x1, int y1, int size, int fg_col, int bg_col) {
	int x, y;
	for (x = x1; x < x1 + size; x++) { blit_pixel(x, y1, fg_col, bg_col); } // Top
	for (y = y1; y < y1 + size; y++) { blit_pixel(x1, y, fg_col, bg_col); } // Left
	for (y = y1; y < y1 + size; y++) { blit_pixel(x1 + size, y, fg_col, bg_col); } // Right
	for (x = x1; x < x1 + size; x++) { blit_pixel(x, y1 + size, fg_col, bg_col); } // Bottom
}

void drawRect(int x1, int y1, int x2, int y2, int fg_col, int bg_col) {
	int x, y, width, height;
	width = x2 - x1; height = y2 - y1;
	for (x = x1; x < x1 + width; x++) { blit_pixel(x, y1, fg_col, bg_col); } // Top
	for (y = y1; y < y1 + height; y++) { blit_pixel(x1, y, fg_col, bg_col); } // Left
	for (y = y1; y < y1 + height; y++) { blit_pixel(x1 + width, y, fg_col, bg_col); } // Right
	for (x = x1; x < x1 + width; x++) { blit_pixel(x, y1 + height, fg_col, bg_col); } // Bottom
}
