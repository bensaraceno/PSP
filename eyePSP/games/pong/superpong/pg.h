// Graphics library for Super "Cool" Pong
// Special thanks to the writer of PSP Hello World for creating this library
// A few new functions have been added to support the extreme graphics 
// rendering required by Super Pong

//constants
#define PIXELSIZE           1    //in short
#define LINESIZE          512    //in short
#define FRAMESIZE     0x44000    //in byte
#define SCREEN_WIDTH      480
#define SCREEN_HEIGHT     272
#define CMAX_X             60    //480*272 = 60*38
#define CMAX_Y             38
#define CMAX2_X            30
#define CMAX2_Y            19
#define CMAX4_X            15
#define CMAX4_Y             9


// Original graphics library
void pgWaitV();
void pgWaitVn(unsigned long count);
void pgScreenFrame(long mode,long frame);
void pgScreenFlip();
void pgScreenFlipV();
void pgPrint4(unsigned long x,unsigned long y,unsigned long color,
              const char *str);
void pgFillvram(unsigned long color);
void pgPutChar(unsigned long x,unsigned long y,unsigned long color,
               unsigned long bgcolor,unsigned char ch,char drawfg,
               char drawbg,char mag);

// functions explicity used by superpong.c
void pgInit();
void pgPrint(unsigned long x,unsigned long y,unsigned long color,
             const char *str);
void pgPrint2(unsigned long x,unsigned long y,unsigned long color,
              const char *str);
void pgBitBlt(unsigned long x,unsigned long y,unsigned long w,
              unsigned long h,unsigned long mag,const unsigned short *d);

// new functions created for Super Pong
void pgDrawRect(unsigned long x,unsigned long y,unsigned long w,
                unsigned long h, const unsigned long color);
void pgRestoreBackground(unsigned long x,unsigned long y, unsigned long w, 
                         unsigned long h, const unsigned short *d);
void pgDrawTransparentImage(unsigned long x,unsigned long y,unsigned long w,
                            unsigned long h, const unsigned short *p);
                            
