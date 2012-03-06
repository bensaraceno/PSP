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
/* Variables for user interface and display_thread. */
#define MAX_OPTIONS 4

int cursorx = 240, cursory = 136;
int view = 1, lastview = 0, gps = 0, menu = 0, menux = 148, menuy = 160, listmin = 0, listmax = 0, option = 3;

char menuText[7][32] = {
    ",-' . warPSP Menu . `-.",
	 "Terminate Application", // Option 0
	 "Toggle Network Rescan", // Option 1
	 "Open and Start warLog", // Option 2
	 "View Serial GPS Input", // Option 3
	 "Cycle to Next Network", // Option 4
	"`-..      .-.      ..-'"
};

#endif
