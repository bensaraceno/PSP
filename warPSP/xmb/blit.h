#ifndef BLIT_H
#define BLIT_H

/*Shades */
#define white		0xFFFFFFFF
#define black		0xFF000000
#define gray		0xFF555555

/* Primaries */
#define red 		0xFF0000FF
#define blue		0xFFFF0000
#define green		0xFF00FF00

/* Darks */
#define darkgreen	0xFF00AA00

/* Mixes */
#define purple		0xFFFF00FF
#define yellow		0xFF00FFFF
#define olive		0xFF00B0B0

#define cyan		0xFFFFFF00
#define cyan90		0xFFE6E600
#define cyan75		0xFFC0C000
#define cyan50		0xFF808000

/* Helpers */
#define selected	green
#define protected	red
#define unprotected	green
#define shadow		0xFF000000

/* Alphas */
#if ALPHA_BLEND
#define white75		0xEEFFFFFF
#define white50		0x77FFFFFF
#define black50		0x77000000
#define red50 		0x770000FF
#define yellow50	0x7700FFFF
#endif

int blit_string(int sx, int sy, const char *msg, int fg_col, int bg_col, int fontSize, int hSpacing, int vSpacing, int addShadow);
int blit_string_ctr(int sy, const char *msg, int fg_col, int bg_col, int fontSize, int hSpacing, int vSpacing, int addShadow);
int blit_pixel(int sx, int sy, int fg_col, int bg_col);
int blit_circle(int sx, int sy, int r, int fg_col, int bg_col);
void drawCircle(int Xc, int Yc, int radius, int fg_col, int bg_col);
void drawSquare(int x1, int y1, int size, int fg_col, int bg_col);
void drawRect(int x1, int y1, int x2, int y2, int fg_col, int bg_col);

#endif
