// Graphics library

#include <pspkernel.h> 
#include <pspdebug.h> 
#include <stdlib.h> 
#include <string.h> 
#include "pg.h"
#include "font.c"


//system call
void sceDisplayWaitVblankStart();
void sceDisplaySetMode(long,long,long);
void sceDisplaySetFrameBuf(char *topaddr,long linesize,long pixelsize,long);


//variables
char *pg_vramtop=(char *)0x04000000;
long pg_screenmode;
long pg_showframe;
long pg_drawframe;



void pgWaitVn(unsigned long count)
{
  for (; count>0; --count) {
    sceDisplayWaitVblankStart();
  }
}


void pgWaitV()
{
  sceDisplayWaitVblankStart();
}


char *pgGetVramAddr(unsigned long x,unsigned long y)
{
  return pg_vramtop+(pg_drawframe?FRAMESIZE:0)+x*PIXELSIZE*2+y*LINESIZE*2+0x40000000;
}


void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
  while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
    pgPutChar(x*32,y*32,color,0,*str,1,0,4);
    str++;
    x++;
    if (x>=CMAX4_X) {
      x=0;
      y++;
    }
  }
}


void pgFillvram(unsigned long color)
{
  unsigned char *vptr0;   //pointer to vram
  unsigned long i;

  vptr0=pgGetVramAddr(0,0);
  for (i=0; i<FRAMESIZE/2; i++) {
    *(unsigned short *)vptr0=color;
    vptr0+=PIXELSIZE*2;
  }
}

void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
  unsigned char *vptr0;   //pointer to vram
  unsigned char *vptr;    //pointer to vram
  const unsigned char *cfont;   //pointer to font
  unsigned long cx,cy;
  unsigned long b;
  char mx,my;

  if (ch>255) return;
  cfont=font+ch*8;
  vptr0=pgGetVramAddr(x,y);
  for (cy=0; cy<8; cy++) {
    for (my=0; my<mag; my++) {
      vptr=vptr0;
      b=0x80;
      for (cx=0; cx<8; cx++) {
        for (mx=0; mx<mag; mx++) {
          if ((*cfont&b)!=0) {
            if (drawfg) *(unsigned short *)vptr=color;
          } else {
            if (drawbg) *(unsigned short *)vptr=bgcolor;
          }
          vptr+=PIXELSIZE*2;
        }
        b=b>>1;
      }
      vptr0+=LINESIZE*2;
    }
    cfont++;
  }
}


void pgScreenFrame(long mode,long frame)
{
  pg_screenmode=mode;
  frame=(frame?1:0);
  pg_showframe=frame;
  if (mode==0) {
    //screen off
    pg_drawframe=frame;
    sceDisplaySetFrameBuf(0,0,0,1);
  } else if (mode==1) {
    //show/draw same
    pg_drawframe=frame;
    sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
  } else if (mode==2) {
    //show/draw different
    pg_drawframe=(frame?0:1);
    sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,1);
  }
}


void pgScreenFlip()
{
  pg_showframe=(pg_showframe?0:1);
  pg_drawframe=(pg_drawframe?0:1);
  sceDisplaySetFrameBuf(pg_vramtop+(pg_showframe?FRAMESIZE:0),LINESIZE,PIXELSIZE,0);
}


void pgScreenFlipV()
{
  pgWaitV();
  pgScreenFlip();
}

// functions used directly by superpong.c

void pgInit()
{
  sceDisplaySetMode(0,SCREEN_WIDTH,SCREEN_HEIGHT);
  pgScreenFrame(0,0);
}

void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
  while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
    pgPutChar(x*8,y*8,color,0,*str,1,0,1);
    str++;
    x++;
    if (x>=CMAX_X) {
      x=0;
      y++;
    }
  }
}


void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
  while (*str!=0 && x<CMAX2_X && y<CMAX2_Y) {
    pgPutChar(x*16,y*16,color,0,*str,1,0,2);
    str++;
    x++;
    if (x>=CMAX2_X) {
      x=0;
      y++;
    }
  }
}

void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,unsigned long h,unsigned long mag,const unsigned short *d)
{
  unsigned char *vptr0;   //pointer to vram
  unsigned char *vptr;    //pointer to vram
  unsigned long xx,yy,mx,my;
  const unsigned short *dd;
  
  vptr0=pgGetVramAddr(x,y);
  for (yy=0; yy<h; yy++) {
    for (my=0; my<mag; my++) {
      vptr=vptr0;
      dd=d;
      for (xx=0; xx<w; xx++) {
        for (mx=0; mx<mag; mx++) {
          *(unsigned short *)vptr=*dd;
          vptr+=PIXELSIZE*2;
        }
        dd++;
      }
      vptr0+=LINESIZE*2;
    }
    d+=w;
  }
}

//------------------------------------------------------------------------------
// Name:     pgRestoreBackground
// Summary:  Takes an input image, and draws the specified portion of that
//           image to the specified location of the screen (I.E. it restores
//           the background at a given location on the screen)
// Inputs:   1. x position to start drawing from on PSP screen
//           2. y position to start drawing from on PSP screen
//           3. width of section to draw
//           4. height of section to draw
//           5. pointer to array containing image data to draw to screen
// Outputs:  none
// Globals:  none
// Returns:  none
// Cautions: The input image must be 480x272 in size or else it will not match
//           the exact dimensions of the PSP screen
//------------------------------------------------------------------------------
void pgRestoreBackground(unsigned long x,unsigned long y, unsigned long w, 
                         unsigned long h, const unsigned short *d)
{
  unsigned char *vptr0;   //pointer to vram
  unsigned char *vptr;    //pointer to vram
  unsigned long xx,yy;
  const unsigned short *dd;
  
  vptr0=pgGetVramAddr(x,y);
  for (yy=0; yy<h; yy++) 
  {
    vptr=vptr0;
    dd = &d[(y+yy) * SCREEN_WIDTH]; // go to correct row of image data
    for (xx=0; xx<w; xx++)          // draw a line of data
    {
      *(unsigned short *)vptr=dd[xx+x];
          vptr+=PIXELSIZE*2;
    }
    vptr0+=LINESIZE*2;
  }
}

//------------------------------------------------------------------------------
// Name:     pgDrawRect
// Summary:  Draws a rectangle, or square if need be, to the screen
// Inputs:   1. x position to start drawing from on PSP screen
//           2. y position to start drawing from on PSP screen
//           3. width of rectangle
//           4. height of rectangle
//           5. color of rectangle
// Outputs:  none
// Globals:  none
// Returns:  none
// Cautions: none
//------------------------------------------------------------------------------
void pgDrawRect(unsigned long x,unsigned long y,unsigned long w, 
                unsigned long h, const unsigned long color)
{
  unsigned char *vptr0;   //pointer to vram
  unsigned char *vptr;    //pointer to vram
  unsigned long xx,yy;
  
  vptr0=pgGetVramAddr(x,y);
  for (yy=0; yy<h; yy++)   // loop through each line
  {
    vptr=vptr0;
    for (xx=0; xx<w; xx++) // draw row of data
    {
        *(unsigned short *)vptr=color;
        vptr+=PIXELSIZE*2;
    }
    vptr0+=LINESIZE*2;
  }
}

//------------------------------------------------------------------------------
// Name:     pgDrawTransparentImage
// Summary:  Draws a image with a transparent background to the screen.  The 
//           transparent background color is 0xFFFF, or pure white. Pure white
//           can still be used in the image itself by using hex 0x7C00.  This
//           is becuase the first bit of the 16 bit color value is ignored by
//           the PSP when drawing image data to the screen.  Each RGB color
//           value gets exactly 5 bits.  A value of 0xFFFF has its first bit
//           set, so it is used to id transparent parts in an image.
// Inputs:   1. x position to start drawing from on PSP screen
//           2. y position to start drawing from on PSP screen
//           3. width of image
//           4. height of image
//           5. pointer to image data
// Outputs:  none
// Globals:  none
// Returns:  none
// Cautions: Use wisely.
//------------------------------------------------------------------------------
void pgDrawTransparentImage(unsigned long x,unsigned long y,unsigned long w,unsigned long h, const unsigned short *p)
{
  unsigned char *vptr0;   //pointer to vram
  unsigned char *vptr;    //pointer to vram
  unsigned long xx,yy;
  
  vptr0=pgGetVramAddr(x,y);
  for (yy=0; yy<h; yy++) 
  {
    vptr=vptr0;
    for (xx=0; xx<w; xx++) 
    {
        if (*p != 0xFFFF) // only draw pixel if its value is not 0xFFFF
          *(unsigned short *)vptr = *p;
        vptr+=PIXELSIZE*2;
        p++;
    }
    vptr0+=LINESIZE*2;
  }
}
