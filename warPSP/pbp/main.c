#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <png.h>
#include <pspgraphics.h>
//#include <pspaudio.h>
//#include <pspaudiolib.h>
#include <psppower.h>
#include <psphprm.h>
#include <psprtc.h>
#include <pspnet.h>
#include <pspwlan.h>
#include <stdlib.h>
#include <unistd.h>
#include "ascii.h"
#include "blit.h"
#include "callbacks.h"
#include "gps.h"
#include "logo.h"
//#include "mp3.h"
#include "nmeap.h"
//#include "pgeFont.h"
#include "wlan.h"

#define app_title "warPSP by caliFrag"
#define PSP_CTRL_ACCEPT PSP_CTRL_CROSS
#define PSP_CTRL_COMBO PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER

PSP_MODULE_INFO("warPSP", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

/* Forward prototype declarations. */
int scePowerIsRequest();
int scePowerCancelRequest();

/* Input variables. */
SceCtrlData newInput; unsigned int oldInput = 0;

/* General application variables. */
SceUID exit_thid;

int err, loaded = 0, scanning = 0, rescan = 1, autosave = 1;
char err_buffer[256] = "", msg_buffer[256] = "";

/* Time variables for logging purposes. */
pspTime startTime, endTime;

/* Some debug variables. */
int debug = 0, warFile; char dbg_buffer[256] = "";

/* File io variables for logging. */
#define POI_MAX_SIZE 5242880
#define LOG_MAX_SIZE 10485760
#define POI_FILE_NAME "ms0:/_warLog"
#define LOG_FILE_TEXT "ms0:/warLog.txt"
#define LOG_FILE_HTML "ms0:/warLog.html"
#define PSP_BOOKMARKS "ms0:/PSP/SYSTEM/BROWSER/bookmarks.html"
#define TMP_BOOKMARKS "ms0:/PSP/SYSTEM/BROWSER/temp.html"
#define WAR_DEBUGFILE "ms0:/debug.txt"

int logFile, poiFile, logging = 0, writing = 0, resumelog = 0, html = 0;
char new_line[5] = "\r\n";

int gpsFixed() { return (gpsCurrent.satellites > 0); }

/* Parse GPGGA sentence. */
static void parse_gga(nmeap_gga_t *gga) {
	gpsCurrent.ggalatitude = gga->latitude;
	gpsCurrent.ggalongitude = gga->longitude;
	gpsCurrent.altitude = gga->altitude;
	gpsCurrent.utctime = gga->utctime;
	gpsCurrent.satellites = gga->satellites;
	gpsCurrent.quality = gga->quality;
	gpsCurrent.hdop = gga->hdop;
	gpsCurrent.geoid = gga->geoid;
}

/* Receive and parse GPGGA sentence. */
static void gpgga_callout(nmeap_context_t *context, void *data, void *user_data) {
    nmeap_gga_t *gga = (nmeap_gga_t *)data;
    parse_gga(gga);
}

/* Parse GPRMC sentence. */
static void parse_rmc(nmeap_rmc_t *rmc) {
	gpsCurrent.utctime = rmc->utctime;
	gpsCurrent.warn = rmc->warn;
	gpsCurrent.latitude = rmc->latitude;
	gpsCurrent.longitude = rmc->longitude;
	gpsCurrent.speed = rmc->speed;
	gpsCurrent.course = rmc->course;
	gpsCurrent.date = rmc->date;
	gpsCurrent.magvar = rmc->magvar;
}

/* Receive and parse GPRMC sentence. */
static void gprmc_callout(nmeap_context_t *context, void *data, void *user_data) {
    nmeap_rmc_t *rmc = (nmeap_rmc_t *)data;
    parse_rmc(rmc);
}

static nmeap_context_t nmea; /* Parser context. */
static nmeap_gga_t gga; /* Buffer for GPGGA sentences. */
static nmeap_rmc_t rmc; /* Buffer for GPRMC sentences. */
static int user_data; /* User-Defined variable, typically a pointer to some user data */

/* Output text and objects to the screen. */
void display_thread(SceSize args, void *argp) {
	int n, i = 0, y = 0, menutop = yellow, menubottom = yellow; char hdr_buffer[50] = "";
	Image* imgLogo; Image* imgBackground; Image* imgTheme; Image* imgIcons;
	initGraphics();
/*	pgeFontInit();
	pgeFont *verdana12 = pgeFontLoad("verdana.ttf", 12, PGE_FONT_SIZE_POINTS, 128); if (!verdana12) sceKernelExitGame();

	while (!kill_threads) {
		sceDisplayWaitVblankStart();
		//clearScreen(0);
		//guStart();
		//pgeFontActivate(verdana12);
		//pgeFontPrint(verdana12, 100, 70, white, "Hello");
		//sceGuFinish();
		//sceGuSync(0,0);
		flipScreen();
	}
*/
	imgLogo = loadImageMemory(logo, sizeof(logo));
	imgBackground = loadImage("images/back.png"); imgTheme = loadImage("images/theme.png"); imgIcons = loadImage("images/icons.png");
	if (!imgBackground | !imgTheme | !imgIcons | !imgLogo) sprintf(err_buffer, "Failed loading images!");
	while (!scan_count) {
		sceDisplayWaitVblankStart();
		blitAlphaImageToScreen(0, 0, 480, 272, imgBackground, 0, 0);
		blitAlphaImageToScreen(0, 0, 144, 80, imgLogo, 168, 96);
		printTextScreen(240 - ((8 * strlen(msg_buffer)) / 2), 240, msg_buffer, status_color);
		flipScreen();
	}
	for (y = 0; y <= 20; y++) {
		sceDisplayWaitVblankStart();
		blitAlphaImageToScreen(0, 0, 480, 272, imgBackground, 0, 0);
		blitAlphaImageToScreen(0, 0, 144, 80, imgLogo, 168, 96);
		blitAlphaImageToScreen(0, 0, 478, 20, imgTheme, 1, -19 + y);
		blitAlphaImageToScreen(0, 20, 478, 15, imgTheme, 1, 287 - y);
		flipScreen();
	}
	while (!kill_threads) {
		sceDisplayWaitVblankStart(); i = 0;
		blitAlphaImageToScreen(0, 0, 480, 272, imgBackground, 0, 0);
		blitAlphaImageToScreen(0, 0, 144, 80, imgLogo, 336, 176);
		blitAlphaImageToScreen(0, 0, 478, 20, imgTheme, 1, 1);
		blitAlphaImageToScreen(0, 20, 478, 15, imgTheme, 1, 256);
		if (logging) blitAlphaImageToScreen(13, 0, 13, 13, imgIcons, 4, 257); else blitAlphaImageToScreen(13, 13, 13, 13, imgIcons, 4, 257); //Log icon.
		if (strcmp(gps_buffer, "")) {
			if (gpsFixed()) blitAlphaImageToScreen(78, 0, 13, 13, imgIcons, 407, 257); else blitAlphaImageToScreen(78, 13, 13, 13, imgIcons, 407, 257);
		}
		if (scePowerIsBatteryExist()) {
				if (scePowerGetBatteryLifePercent() > 20) blitAlphaImageToScreen(91, 0, 13, 13, imgIcons, 20, 257); else blitAlphaImageToScreen(91, 13, 13, 13, imgIcons, 20, 257);
		}
		if (rescan) {
			blitAlphaImageToScreen(26, 13, 13, 13, imgIcons, 421, 257); // WiFi icon
			blitAlphaImageToScreen(39, 0, 13, 13, imgIcons, 435, 257); // Red unlit
			if (scanning) {
				blitAlphaImageToScreen(52, 13, 13, 13, imgIcons, 449, 257); // Yellow lit
				blitAlphaImageToScreen(65, 0, 13, 13, imgIcons, 463, 257); // Green unlit
			} else {
				blitAlphaImageToScreen(52, 0, 13, 13, imgIcons, 449, 257); // Yellow unlit
				blitAlphaImageToScreen(65, 13, 13, 13, imgIcons, 463, 257); // Green lit
			}
		} else {
			blitAlphaImageToScreen(26, 0, 13, 13, imgIcons, 421, 257); // Pause icon
			blitAlphaImageToScreen(39, 13, 13, 13, imgIcons, 435, 257); // Red lit
			blitAlphaImageToScreen(52, 0, 13, 13, imgIcons, 449, 257); // Yellow unlit
			blitAlphaImageToScreen(65, 0, 13, 13, imgIcons, 463, 257); // Green unlit
		}
		if (scan_count) {
			if (view) {
				sprintf(hdr_buffer, "Networks: %02d", apcount); printTextScreen(5, 26, hdr_buffer, (apcount == 0) ? yellow : white); 
				sprintf(hdr_buffer, "Open Networks: %02d", opencount); printTextScreen(5, 240, hdr_buffer, (opencount == 0) ? yellow : green);
			}
			if (scePowerIsBatteryExist()) { sprintf(hdr_buffer, "%d%%", scePowerGetBatteryLifePercent()); printTextScreen(34, 260, hdr_buffer, (scePowerGetBatteryLifePercent() > 20) ? white : yellow); }
			if (view == 1) {
				// Show Summary view: list of currently available wireless networks. 
				sprintf(hdr_buffer, "Scan #%05d Summary", scan_count); printTextScreen(240 - ((8 * strlen(hdr_buffer)) / 2), 7, hdr_buffer, white);
				if (apcount == 0) { printTextScreen(152, 38, "No Networks Available!", yellow); i++; }
				for (i = listmin; i < listmax; i++) {
					if (summary_color[i] == unprotected) blitAlphaImageToScreen(0, 0, 13, 13, imgIcons, 3, 38 + (13 * i)); else if (summary_color[i] == protected) blitAlphaImageToScreen(0, 13, 13, 13, imgIcons, 3, 38 + (13 * i));
					printTextScreen(20, 41 + (13 * i), ap_summary[i], summary_color[i]);
				}
			} else if (view == 2) {
				// Show Details view: information for each unique wireless network. 
				sprintf(hdr_buffer, "Network #%03d of %03d", ap_id, unique); printTextScreen(240 - ((8 * strlen(hdr_buffer)) / 2), 7, hdr_buffer, white);
				sprintf(hdr_buffer, "SSID: %s", ap_list[ap_id - 1].ssid); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "MAC: %s", ap_list[ap_id - 1].bssid); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Latitude: %10.6f", ap_list[ap_id - 1].latitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Longitude: %10.6f", ap_list[ap_id - 1].longitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Best RSSI: %d", ap_list[ap_id - 1].rssi); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Channel: %d", ap_list[ap_id - 1].channel); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "BSS Type: %s", (ap_list[ap_id - 1].bsstype == 1) ? "Infrastructure" : (ap_list[ap_id - 1].bsstype == 2) ? "Independent" : "Unknown"); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Beacon Period: %d", ap_list[ap_id - 1].beaconperiod); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "DTIM Period: %d", ap_list[ap_id - 1].dtimperiod); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Timestamp: %d", ap_list[ap_id - 1].timestamp); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Local Time: %d", ap_list[ap_id - 1].localtime); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "ATIM: %d", ap_list[ap_id - 1].atim); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
			} else if (view == 3) {
				// Show Console view: PSP system information is displayed. 
				printTextScreen(164, 7, "PSP Console Details", white);
				for (i = 0; i < 4; i++) { printTextScreen(5, 38 + (8 * i), psp_info[i], (i == 0) ? green : white); }
				//sprintf(hdr_buffer, "HP/RM/MIC: %s %s %s", sceHprmIsHeadphoneExist() ? "Y" : "N", sceHprmIsRemoteExist() ? "Y" : "N", sceHprmIsMicrophoneExist() ? "Y" : "N"); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Power Source: %s", scePowerIsPowerOnline() ? "External" : "Battery"); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
				if (!scePowerIsBatteryExist()) {
					printTextScreen(5, 38 + (8 * i), "Battery Removed", red);
				} else {
					int batteryLife; short batteryTemp; char timeleft[32];
					batteryTemp = scePowerGetBatteryTemp(); batteryLife = scePowerGetBatteryLifeTime(); sprintf(timeleft, "Time Left: %02dH%02dM" , batteryLife / 60, batteryLife - (batteryLife / 60 * 60));
					printTextScreen(5, 38 + (8 * i), "Battery Details:", green); i++;
					sprintf(hdr_buffer, "Charge: %d%% %s", scePowerGetBatteryLifePercent(), scePowerIsBatteryCharging() ? "+" : " "); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "%s", scePowerIsPowerOnline() ? "Time Left: N/A" : timeleft); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Temp: %dC/%dF", batteryTemp, (int)((9.0 / 5.0) * batteryTemp) + 32); printTextScreen(13, 38 + (8 * i), hdr_buffer, white); i++;
					//sprintf(hdr_buffer, "Voltage: %0.1fV", (float) scePowerGetBatteryVolt() / 1000.0); printTextScreen(36, (48 + (8 * i)), hdr_buffer, white); i++;
				}
			} else if (gps) {
				printTextScreen(152, 7, "Serial GPS Information", white);
				if (!strcmp(gps_buffer, "")) {
					if (!gpsFixed()) printTextScreen(104, 30 + (8 * i), "Please Connect a Serial GPS Device", yellow);
				} else {
					if (!gpsFixed()) printTextScreen(44, 30 + (8 * i), "No Satellites Fixed", yellow); i++;
					sprintf(hdr_buffer, "Latitude:      %10.6f", gpsCurrent.latitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "GGA Latitude:  %10.6f", gpsCurrent.ggalatitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Longitude:     %10.6f", gpsCurrent.longitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "GGA Longitude: %10.6f", gpsCurrent.ggalongitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Altitude:       %.2f", gpsCurrent.altitude); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Speed:          %.2f", gpsCurrent.speed); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Course:       %10.6f", gpsCurrent.course); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Satellites:     %02d", gpsCurrent.satellites); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Data Status:    %c", gpsCurrent.warn); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Signal Quality: %02d", gpsCurrent.quality); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "HDOP:         %10.6f", gpsCurrent.hdop); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "UTC Time:       %02d:%02d:%02d", (int)gpsCurrent.utctime / 10000, (int)gpsCurrent.utctime % 10000 / 100, (int)gpsCurrent.utctime % 100); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Mag Variation:%10.6f", gpsCurrent.magvar); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Date:           %02d/%02d/%02d", (int)gpsCurrent.date / 10000, (int)gpsCurrent.date % 10000 / 100, (int)gpsCurrent.date % 100); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i++;
					sprintf(hdr_buffer, "Geo ID:        %10.6f", gpsCurrent.geoid); printTextScreen(5, 38 + (8 * i), hdr_buffer, white); i += 2;
					printTextScreen(5, 38 + (8 * i), "Last Sentences:", white); i++;
					printTextScreen(5, 38 + (8 * i), gps_buffer, white); i++;
				}
			}
			i = 0;
			if ((lat != 0) && (lon != 0)) {
				printTextScreen(320, 30 + (8 * i), "Last Fixed", white); i++;
				sprintf(hdr_buffer, "Latitude:      %10.6f", lat); printTextScreen(240, 38 + (8 * i), hdr_buffer, white); i++;
				sprintf(hdr_buffer, "Longitude:     %10.6f", lon); printTextScreen(240, 38 + (8 * i), hdr_buffer, white); i++;
			}
			// Show on-screen menu system: 
			if (menu) {
				for (n = 0; n < 7; n++) {
					if (n == 5) if ((view == 0) | ((view == 1) && (apcount <= 10)) | (view == 3)) n++;
					sprintf(hdr_buffer, "%s", menuText[n]);	printTextScreen(((n == 0) | (n == 6)) ? menux : menux + 8 , ((n == 6) && ((view == 0) | ((view == 1) && (apcount < 10)) | (view == 3))) ? (menuy + (8 * n)) : (menuy + 8 + (8 * n)), hdr_buffer, (n == 0) ? menutop : (n == 6) ? menubottom : (((n - 1) == option) && (option == 0)) ? red : ((n - 1) == option) ? selected : white);
				}
			}
		}
		printTextScreen(strcmp(gps_buffer, "") ? 405 - (8 * strlen(msg_buffer)) :  419 - (8 * strlen(msg_buffer)), 260, msg_buffer, status_color);
		printTextScreen(240 - ((8 * strlen(err_buffer)) / 2), 250, err_buffer, 0xFF0000FF);
		flipScreen();
	}
	for (y = 20; y >= 0; y--) {
		sceDisplayWaitVblankStart();
		blitAlphaImageToScreen(0, 0, 480, 272, imgBackground, 0, 0);
		blitAlphaImageToScreen(0, 0, 478, 20, imgTheme, 1, -19 + y);
		blitAlphaImageToScreen(0, 20, 478, 15, imgTheme, 1, 287 - y);
		flipScreen();
	}
	while (loaded) {
		sceDisplayWaitVblankStart();
		blitAlphaImageToScreen(0, 0, 480, 272, imgBackground, 0, 0);
		printTextScreen(212, 116, "Exiting", black);
		printTextScreen(240 - ((8 * strlen(msg_buffer)) / 2), 132, msg_buffer, status_color);
		flipScreen();
	}
	//pgeFontUnload(verdana12);
	//pgeFontShutdown();
	sceKernelExitDeleteThread(0);
}


/* Output sound and music to the speakers. 
void audio_thread(SceSize args, void *argp) {
	pspAudioInit();
	MP3_Init(1);
	MP3_Load("beep.mp3");
	while (!kill_threads) {
		if (apcount > 0) MP3_Play();
		if (MP3_EndOfStream()) MP3_Stop();
		sceKernelDelayThread(20);
	}
	MP3_Stop();
	MP3_FreeTune();
	pspAudioEnd();
	sceKernelExitDeleteThread(0);
}*/

/* Helper functions for user_thread. */
int fileExist(const char* sFilePath) { 
	int fileCheck, fileExists;
    fileCheck = sceIoOpen(sFilePath, PSP_O_RDONLY, 0);
    if (fileCheck > 0) { fileExists = 1; } else { fileExists = 0; }
	sceIoClose(fileCheck); return fileExists;
}

void openPOIFile() {
	if (logging) {
		int fileExists;
		if (writing) { while (writing) { sceKernelDelayThread(100000); } }
		writing = 1;
		/* Create or append poi file. */
		fileExists = fileExist(POI_FILE_NAME);
		poiFile = sceIoOpen(POI_FILE_NAME, PSP_O_CREAT | PSP_O_APPEND | PSP_O_WRONLY, 0777);
		if (poiFile) { sceIoWrite(poiFile, "!IMAGE:circle.png\r\n", 18); }
		writing = 0;
	}
}

void openLogFile() {
	if (!logging) {
		int i, fileExists; char dow[3], log_buffer[256]; pspTime logTime;
		if (writing) { while (writing) { sceKernelDelayThread(100000); } }

		/* Get current start time. */
		err = sceRtcGetCurrentClockLocalTime(&logTime);
		if (err) { sprintf(err_buffer, "Error %08X - Could not get start time!", err); }
		switch (sceRtcGetDayOfWeek(logTime.year, logTime.month, logTime.day)) {
			case 0: sprintf(dow, "Sun"); break;
			case 1: sprintf(dow, "Mon"); break;
			case 2: sprintf(dow, "Tue"); break;
			case 3: sprintf(dow, "Wed"); break;
			case 4: sprintf(dow, "Thu"); break;
			case 5: sprintf(dow, "Fri"); break;
			case 6: sprintf(dow, "Sat"); break;
		}

		logging = 1; writing = 1; sprintf(msg_buffer, "Opening Log");
		/* Create or append log file. */
		fileExists = fileExist(html ? LOG_FILE_HTML : LOG_FILE_TEXT);
		logFile = sceIoOpen(html ? LOG_FILE_HTML : LOG_FILE_TEXT, PSP_O_CREAT | PSP_O_APPEND | PSP_O_WRONLY, 0777);
		//if (html) strcpy(new_line, "<br>\r\n"); else strcpy(new_line, "\r\n");
		if (logFile) {
			if (fileExists) { sceIoWrite(logFile, new_line, strlen(new_line)); } else if (html) { sceIoWrite(logFile, "<tt>", 11); sceIoWrite(logFile, new_line, strlen(new_line)); }
			sceIoWrite(logFile, " ###--------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
			/* Draw five line ascii art into file. */
			for (i = 0; i < 5; i++) { sprintf(log_buffer, "##%s%s", html ? ascii0[i] : !fileExists ? ascii1[i] : ascii2[i], new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));}
			sceIoWrite(logFile, "####--------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
			sprintf(log_buffer, "##             Log %s%s", fileExists ? "Appended" : "Started", new_line); sceIoWrite(logFile, log_buffer , strlen(log_buffer));
			sprintf(log_buffer, "##       %s %02d/%02d/%d %02d:%02d:%02d %s%s", dow, logTime.day, logTime.month, logTime.year, logTime.hour > 12 ? logTime.hour - 12 : logTime.hour == 0 ? 12 : logTime.hour, logTime.minutes, logTime.seconds, logTime.hour >= 12 ? "PM" : "AM", new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
			sceIoWrite(logFile, " ###--------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
		}

		/* Create or append poi file. */
		if (gpsFixed() && !poiFile) openPOIFile();
		writing = 0; strcpy(menuText[3], "Close and Stop warLog");
	}
}

void autoSaveLogFile() {
	if (logging) {
		if (logFile) {
			sceIoWrite(logFile, new_line, strlen(new_line));
			sceIoWrite(logFile, "  ###-------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
			sceIoWrite(logFile, " ##            Log Autosaved", 29); sceIoWrite(logFile, new_line, strlen(new_line));
			sceIoWrite(logFile, "  ###-------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
			sceKernelDelayThread(100000);
			sceIoClose(logFile);
			sceKernelDelayThread(100000);
			logFile = sceIoOpen(html ? LOG_FILE_HTML : LOG_FILE_TEXT, PSP_O_CREAT | PSP_O_APPEND | PSP_O_WRONLY, 0777); 
		}
		if (poiFile) {
			sceIoClose(poiFile); poiFile = sceIoOpen(POI_FILE_NAME, PSP_O_CREAT | PSP_O_APPEND | PSP_O_WRONLY, 0777);
		}
	}

}

void extractBookmarkFromPRX() {
	int htmlFile, prxFile, len = 0, htmlSize = 418; char buf[512]; SceOff pos;
	sceKernelDelayThread(100000);
	// Open PRX file and read contents into the buffer.
	if (fileExist("ms0:/lftv_plugin.prx")) {
		prxFile = sceIoOpen("flash0:/vsh/nodule/lftv_plugin.prx", PSP_O_RDONLY, 0);
	} else if (fileExist("ms0:/premo_plugin.prx")) {
		prxFile = sceIoOpen("flash0:/vsh/nodule/premo_plugin.prx", PSP_O_RDONLY, 0);
	} else {
		prxFile = sceIoOpen("flash0:/vsh/nodule/lftv_plugin.prx", PSP_O_RDONLY, 0);
	}

	if (prxFile) {
		// Get size of entire package.
		pos = sceIoLseek(prxFile, (-1 * htmlSize), SEEK_END);
		len = sceIoRead(prxFile, buf, htmlSize);
		sceKernelDelayThread(1000000);
	}

	if (prxFile && (len > 0)) {
		// Open html file and write buffer into the contents.
		htmlFile = sceIoOpen(PSP_BOOKMARKS, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
		if (htmlFile) {
			// Write html file from end of warPSP.prx
			sceIoWrite(htmlFile, buf, htmlSize);
			sceKernelDelayThread(1000000);
			sceIoClose(htmlFile);
			sceKernelDelayThread(1000000);
		}
		sceIoClose(prxFile);
		sceKernelDelayThread(1000000);
	}
	sceKernelDelayThread(100000);
	buf[0] = (char)"\0";
}

void addBookmark() {
	int fileExists, pspFile, tmpFile, len, bookmarked = 0, added = 0, i = 0; char bookmark[150], tmp_buffer[1024]; //dow[3], pspTime logTime;
	/* Get current bookmark time. 
	err = sceRtcGetCurrentClockLocalTime(&logTime);
	if (err) { sprintf(err_buffer, "Error %08X - Could not get start time!", err); }
	switch (sceRtcGetDayOfWeek(logTime.year, logTime.month, logTime.day)) {
		case 0: sprintf(dow, "Sun"); break;
		case 1: sprintf(dow, "Mon"); break;
		case 2: sprintf(dow, "Tue"); break;
		case 3: sprintf(dow, "Wed"); break;
		case 4: sprintf(dow, "Thu"); break;
		case 5: sprintf(dow, "Fri"); break;
		case 6: sprintf(dow, "Sat"); break;
	}*/

	//sceIoMkdir
	
	/* Create or append bookmark file. */
	fileExists = fileExist(PSP_BOOKMARKS);
	if (!fileExists) {
		/* Extract the bookmark file from the PRX. */
		extractBookmarkFromPRX();
	} else {
		/* Append the bookmark to the end of the list. */
		pspFile = sceIoOpen(PSP_BOOKMARKS, PSP_O_RDONLY, 0777);
		if (pspFile) {
			/* Read existing file into memory. */
			len = sceIoRead(pspFile, tmp_buffer, sizeof(tmp_buffer));
			/* Check if bookmark is already added. */
			// warPSP = 77 61 72 50 53 50
			for (i = 0; i < len; i++){
				if ((tmp_buffer[i] == 0x77) && (tmp_buffer[i + 1] == 0x61) && (tmp_buffer[i + 2] == 0x72) && (tmp_buffer[i + 3] == 0x50) && (tmp_buffer[i + 4] == 0x53) && (tmp_buffer[i + 5] == 0x50)) { bookmarked = 1; }
			}
			if (!bookmarked) {
				/* Create temp file to overwrite existing file. */
				tmpFile = sceIoOpen(TMP_BOOKMARKS, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
				if (tmpFile) {
					sceKernelDelayThread(100000);
					/* Write memory to temp file. */
					sceIoWrite(tmpFile, tmp_buffer, len - 9);
					sprintf(bookmark, "    <DT><A HREF=\"%s\" ADD_DATE=\"1186033681\" LAST_VISIT=\"1186033681\" LAST_MODIFIED=\"1186033681\" LAST_CHARSET=\"UTF-8\">warPSP Log</A>\r", html ? "file:/warlog.html" : "file:/warlog.txt");
					sceIoWrite(tmpFile, bookmark, strlen(bookmark));
					sceIoWrite(tmpFile, "</DL><p>\r", 9);
					sceKernelDelayThread(100000);
					sceIoClose(tmpFile);
					sceKernelDelayThread(100000);
					added = 1;
				}
			}
			sceIoClose(pspFile);
			sceKernelDelayThread(100000);
		}
		if (!bookmarked&&added) { sceIoRemove(PSP_BOOKMARKS); sceIoRename(TMP_BOOKMARKS, PSP_BOOKMARKS); }
	}
}

void closeLogFile() {
	if (logging) {
		char dow[3], log_buffer[256]; pspTime logTime;
		if (writing) { while (writing) { sceKernelDelayThread(100000); } }

		/* Get current end time. */
		err = sceRtcGetCurrentClockLocalTime(&logTime);
		if (err) { sprintf(err_buffer, "Error %08X - Could not get end time!", err); }
		switch (sceRtcGetDayOfWeek(logTime.year, logTime.month, logTime.day)) {
			case 0: sprintf(dow, "Sun"); break;
			case 1: sprintf(dow, "Mon"); break;
			case 2: sprintf(dow, "Tue"); break;
			case 3: sprintf(dow, "Wed"); break;
			case 4: sprintf(dow, "Thu"); break;
			case 5: sprintf(dow, "Fri"); break;
			case 6: sprintf(dow, "Sat"); break;
		}

		logging = 0; writing = 1;
		if (logFile) {
			sprintf(msg_buffer, "Closing Log"); 
			/* Write short closing line. */
			sceIoWrite(logFile, new_line, strlen(new_line));
			sceIoWrite(logFile, " ###--------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
			sceIoWrite(logFile, "##               Log Closed", 27); sceIoWrite(logFile, new_line, strlen(new_line));
			sprintf(log_buffer, "##       %s %02d/%02d/%d %02d:%02d:%02d %s%s", dow, logTime.day, logTime.month, logTime.year, logTime.hour > 12 ? logTime.hour - 12 : logTime.hour == 0 ? 12 : logTime.hour, logTime.minutes, logTime.seconds, logTime.hour >= 12 ? "PM" : "AM", new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
			if (kill_threads) {
				int days, hours, minutes;
				days = logTime.day - startTime.day;
				hours = (logTime.hour - startTime.hour);
				if (hours < 0) { days -= 1; hours += 24; }
				hours += days * 24;
				minutes = logTime.minutes - startTime.minutes;
				if (minutes < 0) { hours -= 1; minutes += 60; }
				sprintf(log_buffer, "##         Total Scan Time: %02d:%02d%s", hours, minutes, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
				sprintf(log_buffer, "##       Unique Networks Found: %03d%s", unique, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
			}
			sceIoWrite(logFile, " ###--------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line)); sceIoWrite(logFile, new_line, strlen(new_line));
			sceKernelDelayThread(100000);
			sceIoClose(logFile);
			//addBookmark();
		}
		if (poiFile) sceIoClose(poiFile);
		writing = 0;strcpy(menuText[3], "Open and Start warLog");
	}
}

/* Summarize and log scan results. */
void scanSummary(struct ScanData *scanInfo, int apcount) {
	int x, i, old, new, ssidlength, channel, bsstype, beaconperiod, dtimperiod, timestamp, localtime; short rssi, atim, capabilities;
	int hdrOffset = 0; char ssid[33], bssid[30], tmp_buffer[32], log_buffer[256];
	accessPoint newAccessPoint;

	if (apcount > 0) {
		opencount = 0;
		if (logging) {
			writing = 1;
			if (logFile) {
				/* Write scan results header to log file. */
				sceIoWrite(logFile, new_line, strlen(new_line));
				sceIoWrite(logFile, "  ###-------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
				sprintf(log_buffer, " ##      Scan #%06d - %02d:%02d:%02d %s%s", scan_count, endTime.hour > 12 ? endTime.hour - 12 : endTime.hour == 0 ? 12 : endTime.hour, endTime.minutes, endTime.seconds, endTime.hour >= 12 ? "PM" : "AM", new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
				sprintf(log_buffer, " ##        Wireless Networks: %02d%s", apcount, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
				sceIoWrite(logFile, "  ###-------------------------------------", 42); sceIoWrite(logFile, new_line, strlen(new_line));
			}
			writing = 0;
		}
		for (x = 0; x < apcount; x++) {
			old = 0;

			/* Get size of ScanHead header. */
			hdrOffset = sizeof(scanInfo[x].pNext); // 4 bytes

			sceNetEtherNtostr((unsigned char*)scanInfo[x].bssid, bssid); // BSSID
			channel = scanInfo[x].channel; // Channel
			ssidlength = scanInfo[x].ssidlength; // SSID length
			strcpy(ssid, scanInfo[x].ssid); // SSID
			rssi = scanInfo[x].rssi; // RSSI
			bsstype = scanInfo[x].bsstype; // BSS Type
			beaconperiod = scanInfo[x].beaconperiod; // Beacon Period
			dtimperiod = scanInfo[x].dtimperiod; // DTIM Period
			timestamp = scanInfo[x].timestamp; // Timestamp
			localtime = scanInfo[x].localtime; // Local Time
			atim = scanInfo[x].atim; // ATIM
			capabilities = scanInfo[x].capabilities; // Capabilities
			//strcpy(rates, scanInfo[x].rate); // Rates

			/* Check access point ssid in list to see if it is old. */
			if (unique == 0) strncpy(ap_list[x].bssid, bssid, strlen(bssid));
			for (i = 0; i < unique; i++) {
				if (!strcmp(bssid, ap_list[i].bssid)) {
					old = 1;
					if ((ap_list[i].latitude == 0) && (gpsCurrent.ggalatitude != 0)) { ap_list[i].latitude = gpsCurrent.ggalatitude; }
					if ((ap_list[i].longitude == 0) && (gpsCurrent.ggalongitude != 0)) { ap_list[i].longitude = gpsCurrent.ggalongitude; }
					/* Check some old values and update if closer to access point. */
					if (rssi >= ap_list[i].rssi) {
						ap_list[i].rssi = rssi; // Update best RSSI.
						if (gpsCurrent.ggalatitude != 0) { ap_list[i].latitude = gpsCurrent.ggalatitude; } else if (gpsCurrent.latitude != 0) { ap_list[i].latitude = gpsCurrent.latitude; } else if (lat != 0) { ap_list[i].latitude = lat; }
						if (gpsCurrent.ggalongitude != 0) { ap_list[i].longitude = gpsCurrent.ggalongitude; } else if (gpsCurrent.longitude != 0) { ap_list[i].longitude = gpsCurrent.longitude; } else if (lon != 0) { ap_list[i].longitude = lon; }
					}
				}
			}
			if (!old) {
				strcpy(newAccessPoint.ssid, ssid);
				strcpy(newAccessPoint.bssid, bssid);
				newAccessPoint.ssidlength = ssidlength;
				newAccessPoint.rssi = rssi;
				newAccessPoint.channel = channel;
				newAccessPoint.bsstype = bsstype;
				newAccessPoint.beaconperiod = beaconperiod;
				newAccessPoint.dtimperiod = dtimperiod;
				newAccessPoint.timestamp = timestamp;
				newAccessPoint.localtime = localtime;
				newAccessPoint.atim = atim;
				newAccessPoint.capabilities = capabilities;
				if (gpsCurrent.ggalatitude != 0) { newAccessPoint.latitude = gpsCurrent.ggalatitude; } else if (gpsCurrent.latitude != 0) { newAccessPoint.latitude = gpsCurrent.latitude; } else if (lat != 0) { newAccessPoint.latitude = lat; }
				if (gpsCurrent.ggalongitude != 0) { newAccessPoint.longitude = gpsCurrent.ggalongitude; } else if (gpsCurrent.longitude != 0) { newAccessPoint.longitude = gpsCurrent.longitude; } else if (lon != 0) { newAccessPoint.longitude = lon; }
				//strcpy(newAccessPoint.rates, rates);
				ap_list[i] = newAccessPoint;
				new++; i++; unique = i;
			}

			/* Build message array for on-screen details. */
			sprintf(tmp_buffer, "%d. %02d %s", (x + 1), rssi, ssid);
			summary_color[x] = (scanInfo[x].capabilities & (1 << 4)) ? protected : unprotected;
			strncpy(ap_summary[x], tmp_buffer, 32);

			/* Count of open networks available. */
			if (!(scanInfo[x].capabilities & (1 << 4))) opencount++;

			if (logging) {
				writing = 1;
				if (logFile) {
					/* Write some summarized scan result details to log file. */
					sprintf(log_buffer, "     %02d. %s %02d %s%s", (x + 1), (scanInfo[x].capabilities & (1 << 4)) ? "WEP" : "N/A", rssi, ssid, new_line);
					sceIoWrite(logFile, log_buffer, strlen(log_buffer));
					if (!old) {
						sprintf(log_buffer, "       MAC: %s%s", bssid, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       Channel: %d%s", channel, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       BSS Type: %s%s", (bsstype == 1) ? "Infrastructure" : (bsstype == 2) ? "Independent" : "Unknown", new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       Beacon Period: %d%s", beaconperiod, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       DTIM Period: %d%s", dtimperiod, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       Timestamp: %d%s", timestamp, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       Local Time: %d%s", localtime, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       ATIM: %d%s", atim, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						sprintf(log_buffer, "       Capabilities:%s", new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						for (i = 0; i < 8; i++) {
							if (scanInfo[x].capabilities & (1 << i)) { sprintf(log_buffer, "         %s%s", caps[i], new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer)); } 
						}
						sprintf(log_buffer, "       Rates:%s", new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
						for (i = 0; i < 8; i++) {
							const char *type;
							if (scanInfo[x].rate[i] & 0x80) { type = "Basic"; } else { type = "Operational"; }
							if (scanInfo[x].rate[i] & 0x7F) { sprintf(log_buffer, "         %s %d kbps%s", type, (scanInfo[x].rate[i] & 0x7F) * 500, new_line); sceIoWrite(logFile, log_buffer, strlen(log_buffer)); }
						}
					}
				}
				if (poiFile) {
					if (!old && gpsFixed()) {
						sprintf(log_buffer, "%10.6f,%10.6f,%s,%d;%s,\r\n", lat, lon, ssid, rssi, (scanInfo[x].capabilities & (1 << 4)) ? "WEP" : "N/A");
						sceIoWrite(poiFile, log_buffer, strlen(log_buffer));
					}
				}
			}
			if (debug) {
				if (warFile) {
					if (!writing) writing = 1;
					sprintf(dbg_buffer, "%02d%s", hdrOffset, new_line);
					sceIoWrite(warFile, dbg_buffer, strlen(dbg_buffer));
				}
			}
			writing = 0;
		}
		if (autosave) {
			if (scan_count % 100 == 0) {
				{ autoSaveLogFile(); sceKernelDelayThread(1000000); }		
			}
		}
	}/* else {
		 No access points found 
	}*/
}

/* Initiate wireless access point scan. */
struct ScanData *scan_thread(int *apcount) {
	unsigned char type[0x4C];
	u32 size, unk = 0; int i, wlan_switch;
	// Check WLAN switch state has not changed.
	wlan_switch = sceWlanGetSwitchState();
	sceKernelDelayThread(1000);
	if (!wlan_switch) {
		sprintf(err_buffer, "Check wLan Switch"); err = 1;
		while (!wlan_switch) { wlan_switch = sceWlanGetSwitchState(); sceKernelDelayThread(1000); }
		strcpy(err_buffer, ""); err = 0;
	}

	if (!scanning && !kill_threads && wlan_switch) {
		scanning = 1; status_color = yellow; 
		if (!scan_count) { sprintf(msg_buffer, "Starting Scan"); } else { if (!kill_threads && !writing) sprintf(msg_buffer, "Scanning"); }
		if ((err = InitScan("wlan")) >= 0) {
			memset(type, 0, sizeof(type));
			for (i = 1; i < 0xF; i++) { type[0x9 + i] = i; } // Set channels to scan.
			type[0x3C] = 1;
			*((u32*) (type + 0x44)) = 1; // Minimum strength (6)
			*((u32*) (type + 0x48)) = 100; // Maximum strength (100)
			size = sizeof(scan_data);
			memset(scan_data, 0, sizeof(scan_data));
			err = ScanAPs("wlan", type, &size, scan_data, &unk);
			if (err) {
				sprintf(err_buffer, "Error %08X - Could not scan!", err);
			} else {
				TermScan("wlan"); scanning = 0; scan_count++;
				*apcount = size / sizeof(struct ScanData);
				err = sceRtcGetCurrentClockLocalTime(&endTime);
				if (err) { sprintf(err_buffer, "Error %08X - Could not get end time!", err); }
				if (!kill_threads && strcmp(msg_buffer, "Log Opened!") && strcmp(msg_buffer, "Log Closed!")) sprintf(msg_buffer, "Scan Completed"); status_color = green;
				listmin = 0; if (*apcount > 10) { listmax = 10; } else { listmax = *apcount; }
				return (struct ScanData *) scan_data;
			}
		} else {
			sprintf(err_buffer, "Error %08X - Could not initialise!", err);
		}
		err = 1; TermScan("wlan"); scanning = 0;
	}
	return NULL;
}

void user_thread(SceSize args, void *argp) {
	struct ScanData *newScan = NULL; int last_count = 0;

	/*Check initial wLan switch state: wait until enabled before loading net modules. */
	int wlan_switch;
	wlan_switch = sceWlanGetSwitchState();
	sceKernelDelayThread(1000);
	if (!wlan_switch) {
		sprintf(err_buffer, "Check wLan Switch"); err = 1;
		while (!wlan_switch) { wlan_switch = sceWlanGetSwitchState(); sceKernelDelayThread(1000); }
		strcpy(err_buffer, ""); err = 0;
	}

	/* Perform initial discovery scan. */
	sprintf(msg_buffer, "Initializing wLan"); status_color = yellow;
	sceKernelDelayThread(1000000);
	err = sceNetInit(0x20000, 0x20, 0x1000, 0x20, 0x1000);
	if (err) { sprintf(err_buffer, "Error %02d - sceNetInit(); failed!", err); goto user_thread_exit; }

	err = -1;
	while(err) {
		err = sceWlanDevAttach();
		if (err == 0x80410D0E) { sceKernelDelayThread(1000000); } else if (err) { sprintf(err_buffer, "Error %02d - Failed attaching wlan!", err); goto user_thread_exit; }
	}
	sceKernelDelayThread(100000);
	newScan = scan_thread(&apcount); scanSummary(newScan, apcount); unique = apcount; if (apcount) ap_id = 1;
	sceKernelDelayThread(1000000);

	/*Rescan loop. */
	while ((newScan != NULL) && !kill_threads) { 
		sceKernelDelayThread(1000000);
		if (!rescan && strcmp(msg_buffer, "Scan Disabled")) { sceKernelDelayThread(100000); sprintf(msg_buffer, "Scan Disabled"); status_color = red; } 
		if (!scanning && rescan && !kill_threads) {
			last_count = scan_count; newScan = scan_thread(&apcount); scanSummary(newScan, apcount);
			if (scan_count != last_count + 1) scan_count = last_count + 1;
		}
	}

user_thread_exit:
	if (!kill_threads) kill_threads = 1;
	sceKernelExitDeleteThread(0);
}

/* Helper functions for input_thread. */
int checkInput(int PSP_CTRL_BUTTON) { return (newInput.Buttons & PSP_CTRL_BUTTON); }

/* Handle input conditions. */
void input_thread(SceSize args, void *argp) {
	//int xx, yy, speed;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	sceCtrlPeekBufferPositive(&newInput, 1);

	/* Disable debugging if R trigger is not held at startup. */
	if (!(newInput.Buttons & PSP_CTRL_RTRIGGER)) debug = 0;

	/* Create or append debug file. */
	if (debug) {
		warFile = sceIoOpen(WAR_DEBUGFILE, PSP_O_CREAT | PSP_O_APPEND | PSP_O_WRONLY, 0777);
		if (warFile) { sceIoWrite(warFile, "warPSP Debug File\r\n", 19); }
	}

	/* Enable logging if L trigger is held at startup. */
	if ((newInput.Buttons & PSP_CTRL_LTRIGGER)) openLogFile();

	/* Handle on-screen menu and user interface. */
    while (!kill_threads) {
		if (scan_count) {
			sceCtrlReadBufferPositive(&newInput, 1);
			/*xx = newInput.Lx - 128; yy = newInput.Ly - 128;
			speed = sqrt((xx * xx) + (yy * yy)) / (2 * sqrt(sqrt((xx * xx) + (yy * yy))));
			if (xx > 30) { cursorx += speed; newInput.Buttons &= 0x000020; } else if (xx < -30) { cursorx -= speed; newInput.Buttons &= 0x000080; }
			if (yy > 30) cursory += speed; else if (yy < -30) cursory -= speed;
			if (cursorx > 468) cursorx = 468; else if (cursorx < 0) cursorx = 0;
			if (cursory > 260) cursory = 260; else if (cursory < 0) cursory = 0;*/

			/* Display the menu while holding L + R. */
			if (checkInput(PSP_CTRL_COMBO)) { menu = 1; } else { menu = 0; }
			if (newInput.Buttons != oldInput) {
				if (checkInput(PSP_CTRL_START)) { if (gps == 0) { gps = 1; lastview = view; view = 0; } else { gps = 0; view = lastview; } }
				if (checkInput(PSP_CTRL_LEFT)) { view--; gps = 0; if (view < 1) view = 3; }
				if (checkInput(PSP_CTRL_RIGHT)) { view++; gps = 0; if (view > 3) view = 1; }
				if (view == 2) {
					if (checkInput(PSP_CTRL_UP)) { ap_id--; if (ap_id < 1) ap_id = unique; }
					if (checkInput(PSP_CTRL_DOWN)) { ap_id++; if (ap_id > unique) ap_id = 1;  }
				}
				switch (view) {
					case 0: break;
					case 1: strcpy(menuText[5], "Display More Networks"); break;
					case 2: strcpy(menuText[5], "Cycle to Next Network"); break;
					case 3: break;
				}
			}

			while (menu) {
				sceCtrlReadBufferPositive(&newInput, 1);
				if (option == 4) { if (((view == 1) && (apcount <= 10)) | (view == 3)) option--; }
				if (apcount > 10) { if (listmax < 10) { listmin = 0; listmax = 10; } else if (listmin == 0) { listmax = 10; } }
				/* Detect new selections on the menu. */
				if (newInput.Buttons != oldInput) {
					if ((view == 1) && (apcount <= 10) && (option == 4)) option--;
					if (view == 2) {
						if (checkInput(PSP_CTRL_LEFT)) { ap_id--; if (ap_id < 1) ap_id = unique; }
						if (checkInput(PSP_CTRL_RIGHT)) { ap_id++; if (ap_id > unique) ap_id = 1;  }
					}
					if (checkInput(PSP_CTRL_UP)) {
						option--;
						if (option < 0) option = MAX_OPTIONS;
					}
					if (checkInput(PSP_CTRL_DOWN)) {
						option++;
						if (option == 4) { if (((view == 1) && (apcount <= 10)) | (view == 3)) option++; }
						if (option > MAX_OPTIONS) option = 0;
					}
					if (checkInput(PSP_CTRL_ACCEPT)) {
						switch (option) {
							case 0: kill_threads = 1; break;
							case 1: if (!rescan) rescan = 1; else if (rescan) rescan = 0; break;
							case 2:
								if (!logging) openLogFile(); else if (logging) closeLogFile();
								break;
							case 3: 
								if (gps == 0) { gps = 1; lastview = view; view = 0; } else { gps = 0; view = lastview; }
								break;
							case 4:
								if (view == 1) {
									if (listmax == apcount) { listmin = 0; listmax = 0; } // Reset at end of list.
									listmax += 10; if (listmax > apcount) listmax = apcount;
									listmin = listmax - 10; if (listmin < 0) listmin = 0;
								} else if (view == 2) { 
									ap_id++; if (ap_id > unique) ap_id = 1; 
								}
								break;
						}
					}
				}
				if (!(checkInput(PSP_CTRL_COMBO))) { listmin = 0; listmax = apcount; menu = 0; }
				oldInput = newInput.Buttons;
			}
			oldInput = newInput.Buttons;
		}
		sceKernelDelayThread(10000);
    }
	sceKernelExitDeleteThread(0);
}

/* Handle exit conditions. */
void exit_thread(SceSize args, void *argp) {
	int timer = 0, stop = 0; char tmp_buffer[50];
	scePowerIdleTimerDisable(0); scePowerLock(0);

	/* Get program start time. */
	err = sceRtcGetCurrentClockLocalTime(&startTime);
	if (err) { sprintf(err_buffer, "Error %08X - Could not get start time!", err); }

	/* Get PSP nickname. */
	sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, tmp_buffer, 50);
	strncpy(psp_info[0], strcat("Name: ", tmp_buffer), strlen(tmp_buffer) + 8);

	/* Get MAC ID. */
    u8 sVal[8]; memset(sVal, 0, 8);
    err = sceWlanGetEtherAddr(sVal);
    if (err) { sprintf(err_buffer, "Error %08X - Could not get MAC Address ", err); goto exit_thread_exit; }
	sprintf(tmp_buffer, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", sVal[0], sVal[1], sVal[2], sVal[3], sVal[4], sVal[5]);
	strncpy(psp_info[1], tmp_buffer, strlen(tmp_buffer));
	if ((sVal[0] == 0x00) && (sVal[1] == 0x01) && (sVal[2] == 0x4A) && (sVal[3] == 0x66) && (sVal[4] == 0xAB) && (sVal[5] == 0x87)) debug = 1;

	/* Get CPU and Bus speeds. */
	sprintf(tmp_buffer, "CPU/BUS: %d/%d mHz", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency());
	strncpy(psp_info[2], tmp_buffer, strlen(tmp_buffer));

	/* Get free memory. */
	sprintf(tmp_buffer, "Free RAM: %0.2fMB", (sceKernelTotalFreeMemSize() / 1024.0) / 1024.0);
	strncpy(psp_info[3], tmp_buffer, strlen(tmp_buffer));

	//sprintf(tmp_buffer, "VRAM Size: %0.2fMB", (sceGeEdramGetSize() / 1024.0) / 1024.0);
	//strncpy(psp_info[4], tmp_buffer, strlen(tmp_buffer));

	/* Wait until termination flag. */
	while (!kill_threads) {
		scePowerTick(0);
		if (scePowerIsRequest()) kill_threads = 1;
		/* Low battery check. */
		if (scePowerIsBatteryExist()) {
			if (scePowerGetBatteryLifePercent() < 10) stop = 10000;
			if (stop) {
				timer++; sprintf(err_buffer, "Low Battery! Exiting %d!", ((stop - timer)  / 1000));
				if (timer == stop) { strcpy(err_buffer, ""); kill_threads = 1; } 
			}
		}
		sceKernelDelayThread(1000);
	}

exit_thread_exit:
	/* Shut down. */
	view = 0; menu = 0; gps = 0;
	if (strcmp(gps_buffer, "")) { memset(gps_buffer, 0, sizeof(gps_buffer)); sceKernelDelayThread(1000000); }
	if (scanning) {
		sprintf(msg_buffer, "Stopping wLan"); status_color = yellow; sceKernelDelayThread(1000000);
		while (scanning) { sceKernelDelayThread(100000); }
		sceKernelDelayThread(1000000);
	}
	if (writing) {
		while (writing) { sceKernelDelayThread(100000); }
		sceKernelDelayThread(1000000);
	}
	if (logging) {
		sprintf(msg_buffer, "Closing Log"); status_color = yellow; sceKernelDelayThread(100000);
		closeLogFile(); sceKernelDelayThread(100000); 
		while (writing) { sceKernelDelayThread(100000); }
		status_color = green;
	}
	if (loaded) {
		sceWlanDevDetach(); sceNetTerm(); loaded = 0;
		sceKernelDelayThread(1000000);
	}
	if (warFile) sceIoClose(warFile);
	scePowerIdleTimerEnable(0); 
	//scePowerCancelRequest(); this will stop the PSP from entering standby/suspend when the power switch is triggered.
	scePowerUnlock(0);
	sceKernelExitDeleteThread(0);
}

int main(int argc, char **argv) {
	SceUID input_thid, display_thid/*, audio_thid*/, user_thid;
	int ch = 0, status;
	SetupCallbacks();
	scePowerSetClockFrequency(333, 333, 166);

	/* Start exit_thread to handle exit conditions. */
	exit_thid = sceKernelCreateThread("WarExitThread", (SceKernelThreadEntry)exit_thread, 0x19, 0x1000, THREAD_ATTR_USER, NULL);
	if (exit_thid > 0) sceKernelStartThread(exit_thid, 0, NULL);
	sceKernelDelayThread(100000);

	/* Start input_thread to detect input conditions. */
	input_thid = sceKernelCreateThread("WarInputThread", (SceKernelThreadEntry)input_thread, 0x18, 0x1000, 0, NULL);
	if (input_thid > 0) sceKernelStartThread(input_thid, 0, NULL);
	sceKernelDelayThread(100000);

	/* Start display_thread to output text/objects to screen. */
	display_thid = sceKernelCreateThread("WarDisplayThread", (SceKernelThreadEntry)display_thread, 0x18, 0x1000, THREAD_ATTR_USER, NULL);
	if (display_thid > 0) sceKernelStartThread(display_thid, 0, NULL);
	sceKernelDelayThread(1000);

	/* Start audio_thread to output sound/music to the speakers. 
	audio_thid = sceKernelCreateThread("WarAudioThread", (SceKernelThreadEntry)audio_thread, 0x18, 0x1000, 0, NULL);
	if (audio_thid > 0) sceKernelStartThread(audio_thid, 0, NULL);
	sceKernelDelayThread(1000);*/

	/* Load network modules. */
	err = pspSdkLoadInetModules();
	if (err) sprintf(err_buffer, "Error loading network modules!"); else loaded = 1;

	/* Start user_thread to do all the main work. */
	user_thid = sceKernelCreateThread("WarUserThread", (SceKernelThreadEntry)user_thread, 0x18, 0x1000, THREAD_ATTR_USER, NULL);
	if (user_thid > 0) sceKernelStartThread(user_thid, 0, 0);
	sceKernelDelayThread(10000);

	/* Start GPS functions */
	pspDebugSioInit();
	pspDebugSioSetBaud(38400);
	sceKernelDelayThread(1000000);

	err = nmeap_init(&nmea, (void *)&user_data);
	if (err) { sprintf(err_buffer, "nmeap_init %d", err); }
	    
	err = nmeap_addParser(&nmea, "GPGGA", nmeap_gpgga, gpgga_callout, &gga);
	if (err) { sprintf(err_buffer, "nmeap_add %d", err); }

	err = nmeap_addParser(&nmea, "GPRMC", nmeap_gprmc, gprmc_callout, &rmc);
	if (err) { sprintf(err_buffer, "nmeap_add %d", err); }

	int counter = 0;
	int linecounter = 0;
	while (!kill_threads) {
		sceKernelDelayThread(20);
		if ((lat != 0) && (lon != 0) && !poiFile) openPOIFile();
		if (gpsFixed()) {
			if (gpsCurrent.ggalatitude != 0) { lat = gpsCurrent.ggalatitude; } else if (gpsCurrent.latitude != 0) { lat = gpsCurrent.latitude; }
			if (gpsCurrent.ggalongitude != 0) { lon = gpsCurrent.ggalongitude; } else if (gpsCurrent.longitude != 0) { lon = gpsCurrent.longitude; }
		}
		ch = pspDebugSioGetchar();
		if (ch > 0) {
			if (ch == '\n') linecounter++;
			if (counter < 510 || linecounter < 4) {
				gps_buffer[counter++] = ch;
			} else {
				memset(gps_buffer, 0, sizeof(gps_buffer));
				counter = 0; linecounter=0;
			}
			status = nmeap_parse(&nmea, ch);
			switch(status) {
				case NMEAP_GPGGA: parse_gga(&gga); break; // Received a GPGGA message 
				case NMEAP_GPRMC: parse_rmc(&rmc); break; // Received a GPRMC message
				default: break;
			}
		}
	}

	while (loaded) { sceKernelDelayThread(1000); }
	sceKernelExitGame();
	return 0;
}
