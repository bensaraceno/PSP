#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspcallbacks.h>
#include <pspgraphics.h>
#include <pspdebug.h>
#include <psppower.h>
#include <psptypes.h> 
#include <malloc.h> 
#include <stdio.h>
#include <stdlib.h>
#include <pspgu.h>
#include <png.h>
#include <pspctrl.h>

#define RGB(r, g, b) ((r)|((g)<<8)|((b)<<16))
#define black RGB(0, 0, 0)
#define white RGB(255, 255, 255)
#define light_gray RGB(200, 200, 200)
#define red RGB(255, 0, 0)
#define green RGB(0, 255, 0)
#define blue RGB(0, 0, 255)
#define yellow RGB(255, 255, 0)
#define cyan RGB(0, 255, 255)
#define purple RGB(255, 0, 255)
#define orange RGB(255, 97, 3)
#define dark_red RGB(139, 26, 26)
#define pink RGB(255, 105, 180)
#define white50 RGB(200, 200, 200)

PSP_MODULE_INFO("spriteTest", 0, 1, 1);

#ifndef RAM_INCLUDED
#define RAM_INCLUDED
#define RAM_BLOCK (1024 * 1024)
u32 ramAvailableLineareMax (void); 
u32 ramAvailable (void); 
#endif

int random(int max) { return (rand() % (max + 1)); }

//* Default game variables:
SceCtrlData newInput;
char stringBuffer[200]; char optionText[50];
int i; int x = 229; int y = 122;
int battLife; int battLeft;
int gameExit; int paused; int idleTime; int aiAction; int aiActionTime = 1;
int titleScreen = 1; int gameSetup; int gameRunning;
int charSelect = 1; int lvlSelect = 1;
int optSelect = 1; int optTotal = 1; int optTextX;
int boostStart = 20; int boostTotal = 20; float boost;
int energyStart = 50; int energyTotal = 50; float energy;
int roaming; int aiRoamSpeed;

//* Default level variables:
Image* bgTile; int bgX; int bgY;

//* Default object variables:
int objectX; int objectY;

//* Default sprite variables:
Image* playerSprite; int forward = 1; int refreshRate = 6;
int frame = 1; int frameStart = 1; int frameEnd = 3;
int spriteRow = 2; int spriteW = 24; int spriteH = 28;
int spriteX = 1; int spriteXpadding = 1; int spriteY = 1; int spriteYpadding = 1;

//* Default character variables:
int spriteSpeed = 1;
int stamina = 100; int energyAdder = 5;
int boostSpeed = 3; int boostAdder = 5;

//* Level tiles
char* levelNames[10] = { "unused", "brick", "grass", "water" };
Image* brickTile;
Image* grassTile;
Image* waterTile;

//* Character sprites:
Image* roboGreen;
Image* dracoBlue;
Image* tankBrown;

char* spriteNames[50] = { "name", "robo", "draco", "tank" };


//* Object sprites:
Image* woodSign;
Image* treeStump;
