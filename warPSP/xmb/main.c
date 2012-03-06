#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspsdk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pspctrl.h>
#include <pspthreadman.h>
#include <pspmodulemgr.h>
#include <psppower.h>
#include <psprtc.h>
#include <psphprm.h>
#include "ascii.h"
#include "blit.h"
#include "wlan.h"
#include "nmeap.h"

#define app_title "warPSP^xmb by caliFrag"
#define PSP_CTRL_ACCEPT PSP_CTRL_CROSS
#define PSP_CTRL_COMBO PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER
#define RGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))
#define RGBA(r, g, b, a) ((r) | ((g) << 8) | ((b) << 16) | ((a) >> 24 & 0xFF))

PSP_MODULE_INFO("warPSP", 0x0800, 1, 1);
//PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_VSH | PSP_THREAD_ATTR_VFPU);
PSP_MAIN_THREAD_ATTR(0);
//PSP_MAIN_THREAD_STACK_SIZE_KB(128);
PSP_HEAP_SIZE_KB(1024);

/* Forward prototype declarations. */
int scePowerIsRequest();
int scePowerCancelRequest();
//int sceHprmEnd(void);
//int sceSysregUartIoEnable(int uart);
//int sceSysconCtrlHRPower(int power);

/* Input variables. */
SceCtrlData newInput; unsigned int oldInput = 0;

/* General application variables. */
SceUID main_thid, exit_thid;

int err, kill_threads = 0, loaded = 0, visible = 1, scanning = 0, rescan = 1, resumescan = 0, autosave = 1;
char err_buffer[256] = "", msg_buffer[256] = "";

/* Time variables for logging purposes. */
pspTime startTime, endTime;

/* Some debug variables. */
int debug = 0, warFile, thid_count; char thread_list[50][50], dbg_buffer[256] = "";

/* File io variables for logging. */
#define POI_MAX_SIZE 5242880
#define LOG_MAX_SIZE 10485760
#define POI_FILE_NAME "ms0:/_warLog"
#define LOG_FILE_TEXT "ms0:/warLog.txt"
#define LOG_FILE_HTML "ms0:/warLog.html"
#define PSP_BOOKMARKS "ms0:/PSP/SYSTEM/BROWSER/bookmarks.html"
#define TMP_BOOKMARKS "ms0:/PSP/SYSTEM/BROWSER/temp.html"
#define WAR_DEBUGFILE "ms0:/debug.txt"

int logFile, poiFile, logging = 0, writing = 0, resumelog = 0, html = 0, gps = 0;
char new_line[5] = "\r\n";

/* Scan variables for networks. */
int summary_color[50]; accessPoint ap_list[50];
int apcount, unique = 0, ap_id, scan_count, status_color;
char psp_info[5][50], ap_summary[50][50];

/* Variables for user interface and display_thread. */
#define MAX_OPTIONS 4

int cursorx = 240, cursory = 136;
int view = 0, menu = 0, menux = 34, menuy = 142, listmin = 0, listmax = 0, option = 3;

char menuText[7][32] = {
    ",-' warPSP^xmb Menu `-.",
	 "Terminate Application", // Option 0
	 "Toggle Network Rescan", // Option 1
	 "Open and Start warLog", // Option 2
	 "New Unassigned Option", // Option 3
	 "Cycle to Next Network", // Option 4
	"`-..     .'X`.     ..-'"
};

/* GPS Variables. */
nmeap_gga_t g_gga;
double        ggalatitude;
double        ggalongitude;
double        altitude;
unsigned long time;
int           satellites;
int           quality;
double        hdop;
double        geoid;
char          warn;
double        latitude;
double        longitude;
double        speed;
double        course;
unsigned long date;
double        magvar;
char gps_buffer[512]; 

/* Handle GGA data. */
static void print_gga(nmeap_gga_t *gga) {
	ggalatitude = gga->latitude;
	ggalongitude = gga->longitude;
	altitude = gga->altitude;
	time = gga->time;
	satellites = gga->satellites;
	quality = gga->quality;
	hdop = gga->hdop;
	geoid = gga->geoid;
}

/* gpgga message received and parsed */
static void gpgga_callout(nmeap_context_t *context, void *data, void *user_data) {
    nmeap_gga_t *gga = (nmeap_gga_t *)data;
    print_gga(gga);
}

/* Handle RMC data. */
static void print_rmc(nmeap_rmc_t *rmc) {
	time = rmc->time;
	warn = rmc->warn;
	latitude = rmc->latitude;
	longitude = rmc->longitude;
	speed = rmc->speed;
	course = rmc->course;
	date = rmc->date;
	magvar = rmc->magvar;
}

/* gprmc message received and parsed */
static void gprmc_callout(nmeap_context_t *context, void *data, void *user_data) {
    nmeap_rmc_t *rmc = (nmeap_rmc_t *)data;
    print_rmc(rmc);
}

static nmeap_context_t nmea; /* parser context */
static nmeap_gga_t gga; /* this is where the data from GGA messages will show up */
static nmeap_rmc_t rmc; /* this is where the data from RMC messages will show up */
static int user_data; /* user can pass in anything. typically it will be a pointer to some user data */

/* Handle GPS data.  */
void gps_thread(SceSize args, void *argp) {
	int ch = 0, status;
	//pspDebugSioInit(); //and sceHprmEnd(); fails
	//pspDebugSioSetBaud(38400);
	//sceKernelDelayThread(2000000);

	err = nmeap_init(&nmea, (void *)&user_data);
	if (err) { sprintf(err_buffer, "nmeap_init %d", err); }
    
    err = nmeap_addParser(&nmea, "GPGGA", nmeap_gpgga, gpgga_callout, &gga);
    if (err) { sprintf(err_buffer, "nmeap_add %d", err); }

    err = nmeap_addParser(&nmea, "GPRMC", nmeap_gprmc, gprmc_callout, &rmc);
    if (err) { sprintf(err_buffer, "nmeap_add %d", err); }

	/*int counter = 0;
	int linecounter = 0;*/

	while(!kill_threads) {
		sceKernelDelayThread(20);
		//ch = kernel_ent((u32) &pspDebugSioGetchar);
		if (ch > 0) {
			/*if (ch == '\n') linecounter++;
			if (counter < 510 || linecounter < 4) {
				gps_buffer[counter++] = ch;
			} else {
				memset(gps_buffer, 0, sizeof(gps_buffer));
				counter = 0; linecounter=0;
			}*/
			status = nmeap_parse(&nmea, ch);
			switch(status) {
				case NMEAP_GPGGA: print_gga(&gga); break; /* GOT A GPGGA MESSAGE */
				case NMEAP_GPRMC: print_rmc(&rmc); break; /* GOT A GPRMC MESSAGE */
				default: break;
			}
		}
	}
}

/* Output text and objects to the screen. */
void display_thread(SceSize args, void *argp) {
	int n, i = 0, menutop = yellow, menubottom = yellow;
	char buffer[256] = "", hdr_buffer[50] = "";
    while(!kill_threads) {
		i = 0;
		/* Test blitting stuff here. */
		if (debug) {
			//sprintf(buffer, "x: %d", cursorx); blit_string(10, 28, buffer, white, shadow, 8, 1, 1, 1);
			//blit_pixel(cursorx, cursory, yellow, shadow);
			//blit_circle(cursorx, cursory, 10, yellow, shadow);
			//drawSquare(cursorx, cursory, 10, white, shadow);
			//drawCircle(cursorx, cursory, 10, white, shadow);
			//for (i = 0; i < thid_count; i++) { sprintf(buffer, thread_list[i]); blit_string(20, (20 + (8 * i)), buffer, white, shadow, 8, 1, 1, 1); }
			if (view == 1) { sprintf(buffer, "Networks: %02d to %02d", listmin, listmax); blit_string(35, 260, buffer, white, shadow, 8, 1, 1, 1); }
			i = 0;
		}
		if (visible) {
			if (!scan_count) { blit_string((59 - strlen(app_title)), 24, app_title, white, shadow, 8, 1, 1, 1); }
			if (!loaded && !scan_count) { sprintf(msg_buffer, "Logging %sabled", logging ? "En" : "Dis"); status_color = logging ? green : yellow;  }

			if (scan_count) { sprintf(buffer, ",-' Networks: %02d", apcount); blit_string(41, 24, buffer, apcount == 0 ? red : white, shadow, 8, 1, 1, 1); }

			if (strcmp(msg_buffer,"")) { blit_string((!scan_count) ? (59 - strlen(msg_buffer)) : (57 - strlen(msg_buffer)), 32, msg_buffer, status_color, shadow, 8, 1, 1, 1); }

			if (scan_count) {
				if (!view) {
					/* Show Minimal view: count of wireless networks and battery %. */
					if (scePowerIsBatteryExist()) { sprintf(hdr_buffer, "%d%%", scePowerGetBatteryLifePercent()); blit_string((57 - strlen(hdr_buffer)), 40, hdr_buffer, (scePowerGetBatteryLifePercent() > 20) ? white : yellow, shadow, 8, 1, 1, 1); }
				} else if (view == 1) {
					/* Show Summary view: list of currently available wireless networks. */
					sprintf(hdr_buffer, ",-' Scan #%05d Summary", scan_count); blit_string(34, 40, hdr_buffer, white, shadow, 8, 1, 1, 1);
					if (apcount == 0) { blit_string(35, 48, "No Networks Available!", yellow, shadow, 8, 1, 1, 1); i++; }
					for (i = listmin; i < listmax; i++) { blit_string(35, menu ? (48 + ((i - listmin) * 8)) : (48 + (i * 8)), ap_summary[i], summary_color[i], (summary_color[i] == protected) ? shadow : shadow, 8, 1, 1, 1); }
				} else if (view == 2) {
					/* Show Details view: information for each unique wireless network. */
					sprintf(buffer, ",-' Network #%03d of %03d", ap_id, unique); blit_string(34, 40, buffer, white, shadow, 8, 1, 1, 1);
					sprintf(buffer, "SSID: %s", ap_list[ap_id - 1].ssid); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "MAC: %s", ap_list[ap_id - 1].bssid); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "RSSI: %d", ap_list[ap_id - 1].rssi); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "Channel: %d", ap_list[ap_id - 1].channel); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "BSS Type: %s", (ap_list[ap_id - 1].bsstype == 1) ? "Infrastructure" : (ap_list[ap_id - 1].bsstype == 2) ? "Independent" : "Unknown"); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "Beacon Period: %d", ap_list[ap_id - 1].beaconperiod); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "DTIM Period: %d", ap_list[ap_id - 1].dtimperiod); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "Timestamp: %d", ap_list[ap_id - 1].timestamp); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "Local Time: %d", ap_list[ap_id - 1].localtime); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "ATIM: %d", ap_list[ap_id - 1].atim); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
				} else if (view == 3) {
					/* Show Console view: PSP system information is displayed. */
					blit_string(34, 40, ",-' PSP Console Details", white, shadow, 8, 1, 1, 1);
					for (i = 0; i < 4; i++) { blit_string(35, (48 + (8 * i)), psp_info[i], (i == 0) ? green : white, shadow, 8, 1, 1, 1); }
					//sprintf(buffer, "HP/RM/MIC: %s %s %s", sceHprmIsHeadphoneExist() ? "Y" : "N", sceHprmIsRemoteExist() ? "Y" : "N", sceHprmIsMicrophoneExist() ? "Y" : "N"); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					sprintf(buffer, "Power Source: %s", scePowerIsPowerOnline() ? "External" : "Battery"); blit_string(35, 48 + (8 * i), buffer, white, shadow, 8, 1, 1, 1); i++;
					if (!scePowerIsBatteryExist()) {
						blit_string(35, (48 + (8 * i)), "Battery Removed", red, shadow, 8, 1, 1, 1);
					} else {
						int batteryLife; short batteryTemp; char timeleft[6];
						batteryTemp = scePowerGetBatteryTemp(); batteryLife = scePowerGetBatteryLifeTime(); sprintf(timeleft, "%02dH%02dM" , batteryLife / 60, batteryLife - (batteryLife / 60 * 60));
						blit_string(35, (48 + (8 * i)), "Battery Details:", green, shadow, 8, 1, 1, 1); i++;
					    sprintf(buffer, "Charge: %d%% %s", scePowerGetBatteryLifePercent(), scePowerIsBatteryCharging() ? "+" : " "); blit_string(36, (48 + (8 * i)), buffer, white, shadow, 8, 1, 1, 1); i++;
					    sprintf(buffer, "Time Left: %s", scePowerIsPowerOnline() ? "N/A" : timeleft);	blit_string(36, (48 + (8 * i)), buffer, white, shadow, 8, 1, 1, 1); i++;
					    sprintf(buffer, "Temp: %dC / %dF", batteryTemp, (int)((9.0 / 5.0) * batteryTemp) + 32); blit_string(36, (48 + (8 * i)), buffer, white, shadow, 8, 1, 1, 1); i++;
					    //sprintf(buffer, "Voltage: %0.1fV", (float) scePowerGetBatteryVolt() / 1000.0); blit_string(36, (48 + (8 * i)), buffer, white, shadow, 8, 1, 1, 1); i++;
					}
				}

				/* Show on-screen menu system:*/
				if (menu) {
					for (n = 0; n < 7; n++) {
						if (n == 5) if ((view == 0) | ((view == 1) && (apcount <= 10)) | (view == 3)) n++;
						sprintf(buffer, "%s", menuText[n]);	blit_string(((n == 0) | (n == 6)) ? menux : menux + 1 , ((n == 6) && ((view == 0) | ((view == 1) && (apcount < 10)) | (view == 3))) ? (menuy + (8 * n)) : (menuy + 8 + (8 * n)), buffer, (n == 0) ? menutop : (n == 6) ? menubottom : (((n - 1) == option) && (option == 0)) ? red : ((n - 1) == option) ? selected : white, shadow, 8, 1, 1, 1);
					}
				}
			}
		} else {
			if (menu) {
				blit_string(41, 24, "warPSP^xmb Cloaked", white, shadow, 8, 1, 1, 1); 
				blit_string(47, 32, "Press Select", yellow, shadow, 8, 1, 1, 1); 
			}
		}
		if (strcmp(err_buffer, "")) { i++; blit_string((59 - strlen(err_buffer)), (scan_count < 1) ? 48 : (!view) ? 56 : (view == 1) ? (56 + (apcount * 8)) : (view == 2) ? 80 : 48 + (8 * i), err_buffer, red, shadow, 8, 1, 1, 1); }
		 sceKernelDelayThread(1000); sceDisplayWaitVblankStart();
    }
	while (loaded) {
		if (strcmp(msg_buffer, "")) { blit_string((59 - strlen(msg_buffer)), 24, msg_buffer, status_color, shadow, 8, 1, 1, 1); }
		if (strcmp(err_buffer, "")) { blit_string((59 - strlen(err_buffer)), 48, err_buffer, red, shadow, 8, 1, 1, 1); }
		sceKernelDelayThread(1000); sceDisplayWaitVblankStart();
	}
	sceKernelExitDeleteThread(0);
}

/* Helper functions for user_thread. */
int fileExist(const char* sFilePath) { 
	int fileCheck, fileExists;
    fileCheck = sceIoOpen(sFilePath, PSP_O_RDONLY, 0);
    if (fileCheck > 0) { fileExists = 1; } else { fileExists = 0; }
	sceIoClose(fileCheck); return fileExists;
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
		if (gps) {
			fileExists = fileExist(POI_FILE_NAME);
			poiFile = sceIoOpen(POI_FILE_NAME, PSP_O_CREAT | PSP_O_APPEND | PSP_O_WRONLY, 0777);
			if (poiFile) { sceIoWrite(poiFile, "!IMAGE:circle.png\r\n", 19); }
		}
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
			addBookmark();
		}
		if (poiFile) sceIoClose(poiFile);
		writing = 0;strcpy(menuText[3], "Open and Start warLog");
	}
}

/* Write unique wireless network list to log file.
void writeNetworkList() {
	int i; char log_buffer[256];
	for (i = 0; i < unique; i++) {
		sprintf(log_buffer, " SSID: %s\r\n", ssid); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  RSSI: %02d\r\n", rssi); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  Channel: %d\r\n", channel); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  BSSID: %s\r\n", bssid); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sceIoWrite(logFile, "  BSS Type: ", 12);
		sprintf(log_buffer, "%s\r\n", (scanInfo[x].bsstype == 1) ? "Infrastructure" : (scanInfo[x].bsstype == 2) ? "Independent" : "Unknown"); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  Beacon Period: %d\r\n", scanInfo->beaconperiod); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  DTIM Period: %d\r\n", scanInfo->dtimperiod); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  Timestamp: %d\r\n", scanInfo->timestamp); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  Local Time: %d\r\n", scanInfo->localtime); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sprintf(log_buffer, "  ATIM: %d\r\n", scanInfo->atim); sceIoWrite(logFile, log_buffer, strlen(log_buffer));
		sceIoWrite(logFile, "  Capabilities:\r\n", 17);
		for (i = 0; i < 8; i++) {
			if (scanInfo[x].capabilities & (1 << i)) { sprintf(log_buffer, "   %s\r\n", caps[i]); sceIoWrite(logFile, log_buffer, strlen(log_buffer)); } 
		}
		sceIoWrite(logFile, "  Rates:\r\n", 10);
		for (i = 0; i < 8; i++) {
			const char *type;
			if (scanInfo[x].rate[i] & 0x80) { type = "Basic"; } else { type = "Operational"; }
			if (scanInfo[x].rate[i] & 0x7F) { sprintf(log_buffer, "   %s %d kbps\r\n", type, (scanInfo[x].rate[i] & 0x7F) * 500); sceIoWrite(logFile, log_buffer, strlen(log_buffer)); }
		}
	}
}*/

/* Summarize and log scan results. */
void scanSummary(struct ScanData *scanInfo, int apcount) {
	int x, i, old, new, size, ssidlength, channel, bsstype, beaconperiod, dtimperiod, timestamp, localtime; short rssi, atim, capabilities;
	int bssidOffset = 0, ssidOffset = 0, hdrOffset = 0; char ssid[33], bssid[30], tmp_buffer[32], log_buffer[256];
	accessPoint newAccessPoint;

	if (apcount > 0) {
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

			/* BSSID */
			size = 0; size = strlen(scanInfo[x].bssid + bssidOffset);
			sceNetEtherNtostr((unsigned char*)scanInfo[x].bssid + bssidOffset, bssid);
			bssidOffset = bssidOffset + size + (12 - size);

			/* Channel */
			channel = scanInfo[x].channel;

			/* SSID length */
			ssidlength = scanInfo[x].ssidlength;

			/* SSID */
			size = 0; size = strlen(scanInfo[x].ssid + ssidOffset);
			strncpy(ssid, scanInfo[x].ssid + ssidOffset, size);
			ssid[size] = 0; ssidOffset = ssidOffset + size + (12 - size);

			/* RSSI */
			rssi = scanInfo[x].rssi;

			/* BSS Type */
			bsstype = scanInfo[x].bsstype;

			/* Beacon Period */
			beaconperiod = scanInfo[x].beaconperiod;

			/* DTIM Period */
			dtimperiod = scanInfo[x].dtimperiod;

			/* Timestamp */
			timestamp = scanInfo[x].timestamp;

			/* Local Time */
			localtime = scanInfo[x].localtime;

			/* ATIM */
			atim = scanInfo[x].atim;

			/* Capabilities */
			capabilities = scanInfo[x].capabilities;

			/* Rates  
			size = 0; size = strlen(scanInfo[x].rate + rateOffset);
			strncpy(rates, scanInfo[x].rate + rateOffset, size);
			rates[size] = 0; rateOffset = rateOffset + size + (12 - size);*/

			/* Check access point ssid in list to see if it is old. */
			if (unique == 0) strncpy(ap_list[x].bssid, bssid, strlen(bssid));
			for (i = 0; i < unique; i++) {
				if (!strcmp(bssid, ap_list[i].bssid)) {
					old = 1;
					// Temporary fix until offsets are figured out.
					// Updates network list information for first found.
					if (x == 0) {
						if (rssi > ap_list[i].rssi) ap_list[i].rssi = rssi;
						if (ap_list[i].channel <= 0) ap_list[i].channel = channel;
						if (ap_list[i].ssidlength <= 0) ap_list[i].ssidlength = ssidlength;
						if (ap_list[i].bsstype <= 0) ap_list[i].bsstype = bsstype;
						if (ap_list[i].beaconperiod <= 0) ap_list[i].beaconperiod = beaconperiod;
						if (ap_list[i].dtimperiod <= 0) ap_list[i].dtimperiod = dtimperiod;
						if (ap_list[i].timestamp <= 0) ap_list[i].timestamp = timestamp;
						if (ap_list[i].localtime <= 0) ap_list[i].localtime = localtime;
						if (ap_list[i].atim <= 0) ap_list[i].atim = atim;
						if (ap_list[i].capabilities <= 0) ap_list[i].capabilities = capabilities;
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
				//strcpy(newAccessPoint.rates, rates);
				ap_list[i] = newAccessPoint;
				new++; i++; unique = i;
			}

			/* Build message array for on-screen details. */
			sprintf(tmp_buffer, "%d. %s %02d %s ", (x + 1), (scanInfo[x].capabilities & (1 << 4)) ? "WEP" : "N/A", rssi, ssid);
			summary_color[x] = (scanInfo[x].capabilities & (1 << 4)) ? protected : unprotected;
			strncpy(ap_summary[x], tmp_buffer, 32);

			if (logging) {
				writing = 1;
				if (logFile) {
					/* Write some summarized scan result details to log file. */
					sprintf(log_buffer, "     %02d. %s %02d %s%s", (x + 1), (scanInfo[x].capabilities & (1 << 4)) ? "WEP" : "N/A", rssi, ssid, new_line);
					sceIoWrite(logFile, log_buffer, strlen(log_buffer));
				}
				if (poiFile) {
					if (!old) {
						sprintf(log_buffer, "lat,lon,%s,%d;%s,\r\n", ssid, rssi, (scanInfo[x].capabilities & (1 << 4)) ? "P" : "O");
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
				TermScan("wlan"); scan_count++;
				*apcount = size / sizeof(struct ScanData);
				err = sceRtcGetCurrentClockLocalTime(&endTime);
				if (err) { sprintf(err_buffer, "Error %08X - Could not get end time!", err); }
				if (!kill_threads && strcmp(msg_buffer, "Log Opened!") && strcmp(msg_buffer, "Log Closed!")) sprintf(msg_buffer, "Scan Completed"); status_color = green;
				listmin = 0; if (*apcount > 10) { listmax = 10; } else { listmax = *apcount; }
				scanning = 0; return (struct ScanData *) scan_data;
			}
		} else {
			sprintf(err_buffer, "Error %08X - Could not initialise!", err);
		}
		sprintf(msg_buffer, "Scan Failed"); status_color = red;
		TermScan("wlan"); scanning = 0;
	}
	return NULL;
}

int loadNetworkModules(void) {
	if (!loaded) {
		err = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
		if (err) { sprintf(err_buffer, "Error %02d - PSP_NET_MODULE_COMMON failed!", err); goto loadNetworkModules_exit; }

		sceKernelDelayThread(2000);
		err = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
		if (err) { sprintf(err_buffer, "Error %02d - PSP_NET_MODULE_INET failed!", err); goto loadNetworkModules_exit; }
	}
loadNetworkModules_exit:
	if (err) { return 1; } else { loaded = 1; return 0; }
}

int unloadNetworkModules(void) {
	if (loaded) {
		err = -1;
		while (err) { err = sceWlanDevDetach(); if (err == 0x80410D0E) { sceKernelDelayThread(10000); } else if (err) { sprintf(err_buffer, "Error %02d - Failed detaching wlan!", err); goto unloadNetworkModules_exit; } }

		sceKernelDelayThread(2000);
		err = sceNetTerm();
		if (err) { sprintf(err_buffer, "Error %02d - sceNetTerm(); failed!", err); goto unloadNetworkModules_exit; }

		sceKernelDelayThread(2000);
		err = sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
		if (err) { sprintf(err_buffer, "Error %02d - PSP_NET_MODULE_INET failed!", err); goto unloadNetworkModules_exit; }

		sceKernelDelayThread(2000);
		err = sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
		if (err) { sprintf(err_buffer, "Error %02d - PSP_NET_MODULE_COMMON failed!", err); goto unloadNetworkModules_exit; }
	}
unloadNetworkModules_exit:
	if (err) { return 1; } else { loaded = 0; return 0; }
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

	/* Load network modules. */
	err = loadNetworkModules();
	if (err) { sprintf(err_buffer, "Error Loading!"); goto user_thread_exit; }
	sceKernelDelayThread(1000);

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
int checkInput(int PSP_CTRL_BUTTON) { return ((newInput.Buttons & (PSP_CTRL_COMBO | PSP_CTRL_BUTTON)) == (PSP_CTRL_COMBO | PSP_CTRL_BUTTON)); }

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
			sceCtrlPeekBufferPositive(&newInput, 1);
			/*xx = newInput.Lx - 128; yy = newInput.Ly - 128;
			speed = sqrt((xx * xx) + (yy * yy)) / (2 * sqrt(sqrt((xx * xx) + (yy * yy))));
			if (xx > 30) { cursorx += speed; newInput.Buttons &= 0x000020; } else if (xx < -30) { cursorx -= speed; newInput.Buttons &= 0x000080; }
			if (yy > 30) cursory += speed; else if (yy < -30) cursory -= speed;
			if (cursorx > 468) cursorx = 468; else if (cursorx < 0) cursorx = 0;
			if (cursory > 260) cursory = 260; else if (cursory < 0) cursory = 0;*/

			/* Display the menu while holding L + R. */
			if (checkInput(PSP_CTRL_COMBO)) { menu = 1; } else { menu = 0; }

			while (menu) {
				sceCtrlReadBufferPositive(&newInput, 1); // This stops the xmb from receiving input.
				if (option == 4) { if ((view == 0) | ((view == 1) && (apcount <= 10)) | (view == 3)) option--; }
				if (apcount > 10) { if (listmax < 10) { listmin = 0; listmax = 10; } else if (listmin == 0) { listmax = 10; } }
				/* Detect new selections on the menu. */
				if (newInput.Buttons != oldInput) {
					if ((view == 1) && (apcount <= 10) && (option == 4)) option--;
					if (checkInput(PSP_CTRL_SELECT)) { if (!visible) visible = 1; else visible = 0; }
					if (visible) {
						if (checkInput(PSP_CTRL_UP)) {
							option--;
							if (option < 0) option = MAX_OPTIONS;
						}
						if (checkInput(PSP_CTRL_DOWN)) {
							option++;
							if (option == 4) { if ((view == 0) | ((view == 1) && (apcount <= 10)) | (view == 3)) option++; }
							if (option > MAX_OPTIONS) option = 0;
						}
						if (checkInput(PSP_CTRL_LEFT)) { view--; if (view < 0) view = 3; }
						if (checkInput(PSP_CTRL_RIGHT)) { view++; if (view > 3) view = 0; }
							switch (view) {
								case 0: break;
								case 1: strcpy(menuText[5], "Display More Networks"); break;
								case 2: strcpy(menuText[5], "Cycle to Next Network"); break;
								case 3: break;
							}
						if (checkInput(PSP_CTRL_ACCEPT)) {
							switch (option) {
								case 0: kill_threads = 1; break;
								case 1: if (!rescan) rescan = 1; else if (rescan) rescan = 0; break;
								case 2:
									if (!logging) openLogFile(); else if (logging) closeLogFile();
									break;
								case 3: break;
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
				}
				if (!(checkInput(PSP_CTRL_COMBO))) { listmin = 0; listmax = apcount; menu = 0; }
				oldInput = newInput.Buttons;
			}
		}
		sceKernelDelayThread(10000);
    }
	/* This should prevent the user from clicking anything while exiting. */
	while (loaded) { sceCtrlReadBufferPositive(&newInput, 1); }
	sceCtrlReadBufferPositive(&newInput, 0); //
	memset(&newInput.Buttons, 0, sizeof(NULL));
	sceKernelExitDeleteThread(0);
}

/* Dump the thread status (0 to screen, 1 to debug file). */
void dumpThreadStatus(int dest) {
	SceUID thid_list[50]; SceKernelThreadInfo thinfo; int i;
	sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thid_list, 50, &thid_count);
	for(i = 0; i < thid_count; i++) {
		memset(&thinfo, 0, sizeof(SceKernelThreadInfo));
		thinfo.size = sizeof(SceKernelThreadInfo);
		sceKernelReferThreadStatus(thid_list[i], &thinfo);
		if (!dest) {
			strcpy(thread_list[i], &thinfo.name[0]);
		} else {
			if (warFile) {
				sceIoWrite(warFile, &thinfo.name[0], strlen(&thinfo.name[0]));
				sceIoWrite(warFile, new_line, strlen(new_line));
			}
		}
	}
	if (!dest && warFile) { sceIoWrite(warFile, new_line, strlen(new_line)); }
}

/* Handle unknown exits. */
void unknownExit() {
	dumpThreadStatus(1); kill_threads = 1;
	while (loaded) { sceKernelDelayThread(1000); }
}

/* Watch the system threads and handle problems. */
void watchThreads() {
	int i, usb = 0, exit = 0, kill_thread = 0, thid_count;
	SceUID thids[50]; SceKernelThreadInfo thinfo;
	sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, thids, 50, &thid_count);
	for(i = 0; i < thid_count; i++) {
		memset(&thinfo, 0, sizeof(SceKernelThreadInfo));
		thinfo.size = sizeof(SceKernelThreadInfo);
		sceKernelReferThreadStatus(thids[i], &thinfo);
		// "ScePafJob" means something bad is about to happen roffle. ;D
		if (!strcmp(&thinfo.name[0], "SceKernelLoadExec")) { exit = 1; }
		if (!strcmp(&thinfo.name[0], "SceHtmlViewer")) { kill_thread = 1; }
		if (!strcmp(&thinfo.name[0], "SceNetApctl")) { exit = 1; }
		if (!strcmp(&thinfo.name[0], "SceUsbstor")) { usb = 1; }
		if (!strcmp(&thinfo.name[0], "SceUsbstorMsMed")) { usb = 1; }
		if (!strcmp(&thinfo.name[0], "SceUsbstorMsCmd")) { usb = 1; }
		if (!strcmp(&thinfo.name[0], "SceVshSysconfThread")) { usb = 1; }

		if (usb && logging) {
			sceKernelSuspendThread(thids[i]);
			closeLogFile(); resumelog = 1;
			sceKernelDelayThread(100000);
			sceKernelResumeThread(thids[i]);
		/*} else if (browser && loaded) {
			sceKernelSuspendThread(thids[i]);
			rescan = 0; resumescan = 1;
			if (scanning) { while (scanning) { sceKernelDelayThread(1000); } }
			unloadNetworkModules(); sceKernelDelayThread(1000000);
			sceKernelResumeThread(thids[i]);*/
		} else if (kill_thread) {
			sceKernelTerminateDeleteThread(thids[i]);
		} else if (exit) {
			unknownExit();
		}
	}
	if (!usb && !logging && resumelog) { sceKernelDelayThread(100000); openLogFile(); resumelog = 0; }
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
		watchThreads(); scePowerTick(0);
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
	if (!visible) visible = 1;
	if (view) view = 0;
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
		unloadNetworkModules(); sceKernelDelayThread(1000000);
		while (loaded) { sceKernelDelayThread(100000); }
	}
	if (warFile) sceIoClose(warFile);
	scePowerIdleTimerEnable(0); 
	//scePowerCancelRequest(); this will stop the PSP from entering standby/suspend when the power switch is triggered.
	scePowerUnlock(0);
	sceKernelExitDeleteThread(0);
}

void startSIO() {
	//pspDebugSioInit();
	//pspDebugSioSetBaud(38400);
	sceKernelDelayThread(2000000);
}

/* Main thread to start all user threads. */
int main() {
	SceUID input_thid, display_thid, user_thid, gps_thid;

	//while(!sceKernelFindModuleByName("ScePafModule")) { sceKernelDelayThread(10000); }
	//sceKernelDelayThread(100000);

	/* Start exit_thread to handle exit conditions. */
	exit_thid = sceKernelCreateThread("WarExitThread", (SceKernelThreadEntry)exit_thread, 0x19, 0x1000, THREAD_ATTR_USER, NULL);
	if (exit_thid >= 0) sceKernelStartThread(exit_thid, 0, NULL);
	sceKernelDelayThread(100000);

	/* Start input_thread to detect input conditions. */
	input_thid = sceKernelCreateThread("WarInputThread", (SceKernelThreadEntry)input_thread, 0x18, 0x1000, THREAD_ATTR_USER, NULL);
	if (input_thid >= 0) sceKernelStartThread(input_thid, 0, NULL);
	sceKernelDelayThread(100000);

	/* Start display_thread to output text/objects to screen. */
	display_thid = sceKernelCreateThread("WarDisplayThread", (SceKernelThreadEntry)display_thread, 0x19, 0x0400, THREAD_ATTR_USER, NULL);
	if (display_thid >= 0) sceKernelStartThread(display_thid, 0, NULL);
	sceKernelDelayThread(1000000);

	/* Start gps_thread to detect gps conditions. */
	gps_thid = sceKernelCreateThread("WarGPSThread", (SceKernelThreadEntry)gps_thread, 0x16, 0x1000, 0, NULL);
	if (gps_thid >= 0) sceKernelStartThread(gps_thid, 0, NULL);
	sceKernelDelayThread(10000);

	/* Start user_thread to do all the main work. */
	user_thid = sceKernelCreateThread("WarUserThread", (SceKernelThreadEntry)user_thread, 0x18, 0x1000, THREAD_ATTR_USER, NULL);
	if (user_thid >= 0) sceKernelStartThread(user_thid, 0, 0);
	sceKernelDelayThread(10000);

	while (!kill_threads | loaded) { sceKernelDelayThread(1000); }

	//sceKernelSelfStopUnloadModule(1, 0, NULL); //Fails to unload
	return sceKernelExitDeleteThread(0);
}
/*
int module_start (SceSize args, void *argp) {
	main_thid = sceKernelCreateThread("WarMainThread", (SceKernelThreadEntry)main_thread, 0x16, 0x1000, 0, NULL);
	if (main_thid >= 0) { sceKernelStartThread(main_thid, args, argp); }
	return 0;
}

int module_stop(SceSize args, void *argp) { return 0; }

int module_reboot_before() { return 0; }

int module_reboot_phase(int arg1) {
	if (arg1 == 3) { unknownExit(); }
	return 0;
}
*/
