#include <pspkernel.h>
#include <pspdebug.h>
#include <pspcallbacks.h>
#include <pspctrl.h>
#include <psppower.h>
#include <string.h>
#include <scepower.h>

#define printf pspDebugScreenPrintf

//#define prxSize 62790
#define pbpSize 209524
#define bg_col 0xFFFFFF

#define PRX_WARPSP_XMB "ms0:/warPSP.prx"
#define PBP_WARPSP_EIP "ms0:/PSP/GAME/warPSP^installer/EBOOT.PBP"

#define PRX_LFTVBACKUP "ms0:/lftv_plugin.prx"
#define PRX_LFTVPLAYER "flash0:/vsh/nodule/lftv_plugin.prx"

#define PRX_RPLYBACKUP "ms0:/premo_plugin.prx"
#define PRX_REMOTEPLAY "flash0:/vsh/nodule/premo_plugin.prx"

PSP_MODULE_INFO("warPSP^eip", 0, 1, 1);

int quickinstall = 0;

/* Helper function for file io. */
int fileExist(const char* sFilePath) { 
	int fileCheck, fileExists;
    fileCheck = sceIoOpen(sFilePath, PSP_O_RDONLY, 0);
    if (fileCheck > 0) { fileExists = 1; } else { fileExists = 0; }
	sceIoClose(fileCheck); return fileExists;
}

/* Write files to or from the flash0. */
int fileCopy(const char* sFileSrc , const char* sFileDest) {
	if (fileExist(sFileSrc)) {
		int fd1, fd2, len; char buf[128*1024];
		sceIoUnassign("flash0:"); 
		sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", 0, NULL, 0);
		fd1 = sceIoOpen(sFileSrc, PSP_O_RDONLY, 0);
		fd2 = sceIoOpen(sFileDest, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
		while(1) {
			len = sceIoRead(fd1, buf, sizeof(buf));
			if (len == 0) break;
			sceIoWrite(fd2, buf, len);
		}
		sceIoClose(fd1); sceIoClose(fd2);
		sceIoUnassign("flash0:");
		pspDebugScreenSetTextColor(0x006600); printf("  %s > %s Done.\n", sFileSrc, sFileDest); 
		return 1;
	} else {
		pspDebugScreenSetTextColor(0x000099); printf("  %s does not exist!\n", sFileSrc); 
		return 0;
	}
}

void extractPRXFromPBP() {
	int pbpFile, prxFile, pkgSize = 0, prxSize = 0; char buf[1024*1024];
	pspDebugScreenSetTextColor(0x000000);
	printf("  Extracting warPSP.prx to the memory stick root.\n\n");
	// Open PBP file and read contents into the buffer.
	pbpFile = sceIoOpen(PBP_WARPSP_EIP, PSP_O_RDONLY, 0);
	sceKernelDelayThread(1000000);
	if (pbpFile) {
		// Get size of entire package.
		pkgSize = sceIoRead(pbpFile, buf, sizeof(buf));
		sceKernelDelayThread(1000000);
		printf("  EBOOT.PBP loaded into memory successfully!\n\n");
		if (pkgSize > 0) {
			// Calculate size of prx to extract (size of entire package - size of eboot.pbp).
			prxSize = pkgSize - pbpSize;
			// Open PRX file and write buffer into the contents.
			prxFile = sceIoOpen(PRX_WARPSP_XMB, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
			sceKernelDelayThread(100000);
			if (prxFile) {
				// Write prx file from end of eboot.pbp.
				sceIoWrite(prxFile, buf + pbpSize, prxSize);
				sceKernelDelayThread(1000000);
				sceIoClose(prxFile);
				sceKernelDelayThread(1000000);
				printf("  warPSP.prx extracted from memory successfully!\n\n");
			} else {
				printf("  warPSP.prx extraction from memory failed!\n\n");
			}
		}
		sceIoClose(pbpFile);
		sceKernelDelayThread(1000000);
	} else {
		printf("  EBOOT.PBP load into memory failed!\n\n");
	}
	buf[0] = (char)"\0";
}

void clearScreenPrintHeader() {
	pspDebugScreenClear();
	pspDebugScreenSetTextColor(0x880000);
	printf("\n");
	printf(" warPSP^xmb\n");
	printf("  xmb extension\n");
	printf("  by caliFrag\n\n");
	printf(" ------------------- Easy Installation Program -------------------\n\n");
	pspDebugScreenSetTextColor(0x000000);
}

int main() {
	SceCtrlData pad;
	int cancel = 0, uninstall = 0, reinstall = 0, lftv = 0, ok = 0, installed = 0, uninstalled = 0, autoExit = 0;
	SetupCallbacks(); pspDebugScreenInit();
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	sceCtrlReadBufferPositive(&pad, 1);
	if (pad.Buttons & PSP_CTRL_LTRIGGER) { quickinstall = 1; lftv = 1; } else if (pad.Buttons & PSP_CTRL_RTRIGGER) { quickinstall = 1; }
	pspDebugScreenSetBackColor(bg_col);
	clearScreenPrintHeader();
	sceKernelDelayThread(100000);

	if ((scePowerGetBatteryLifePercent() < 25) & !scePowerIsPowerOnline()) {
		// Check for low battery.
		pspDebugScreenSetTextColor(0x000099);
		printf(" Battery should be at least 25%% charged before writing to flash0!\n"); 
		printf(" Connect AC adapter to ignore this warning message and continue!\n");
		printf(" Press SELECT to cancel %sstallation of warPSP^xmb.\n\n", uninstall ? "unin" : "in");
		while (!scePowerIsPowerOnline()) {
			sceCtrlReadBufferPositive(&pad, 1);
			if (pad.Buttons & PSP_CTRL_SELECT) { cancel = 1; break; }
		}
		if (scePowerIsPowerOnline()) clearScreenPrintHeader();
	}
	if (!cancel & !quickinstall) {
		// Show disclaimer and options.
		clearScreenPrintHeader();
		pspDebugScreenSetTextColor(0x000099);
		printf(" READ THIS DISCLAIMER:\n");
		printf("  This program flashes files to the flash0 NAND drive of your PSP.\n");
		printf("  If you select an installation option, you accept risks involved.\n\n");
		printf("  DO NOT REMOVE THE MEMORY STICK WHILE THE ORANGE LED IS FLASHING.\n");
		printf("  DO NOT TURN OFF THE PSP SYSTEM WHILE THE ORANGE LED IS FLASHING.\n\n");
		printf("  If you do choose to accept the risk, press START to acknowledge.\n");
		printf("  If you do not choose to accept the risk, press SELECT to cancel.\n\n\n");
		printf("  THE AUTHOR ACCEPTS NO RESPONSIBILITY FOR DAMAGES THAT MAY OCCUR.\n");
		printf("  THIS SOFTWARE IS PRESENTED AS IS WITH NO WARRANTY OR GUARANTEES.\n\n");
		sceKernelDelayThread(200000);
		while (1) {
			// Handle selected option from user.
		    sceCtrlReadBufferPositive(&pad, 1);
			if (pad.Buttons & PSP_CTRL_START) { break; }
			if (pad.Buttons & PSP_CTRL_SELECT) { cancel = 1; break; }
			sceKernelDelayThread(100000);
		}
	}

	if (!cancel & !quickinstall) {
		// Check if backup file exists.
		if (fileExist(PRX_LFTVBACKUP) | fileExist(PRX_RPLYBACKUP)) uninstall = 1;
		sceKernelDelayThread(100000); clearScreenPrintHeader();
		printf(" Thanks for choosing the warPSP software suite for the Sony PSP!\n\n");
		printf(" Briefing:\n");
		printf("  warPSP is the most advanced warXing software suite for the PSP.\n\n");
		printf("  Please see the README.TXT file for more details and information.\n\n\n");
		printf(" Options:\n");
		if (uninstall) {
			printf("  Press SQUARE to uninstall warPSP and restore backup files.\n");
			printf("  Press CIRCLE to reinstall warPSP to the last slot selected.\n");
		} else {
			printf("  Press SQUARE to install warPSP to the LFTV Player slot.\n");
			printf("  Press CIRCLE to install warPSP to the Remote Play slot.\n");
		}
		pspDebugScreenSetTextColor(0x000099);
		printf("  Press SELECT to cancel %sstallation of warPSP^xmb.\n\n", uninstall ? "unin" : "in");
		sceKernelDelayThread(200000);
		while (1) {
			// Handle selected option from user.
		    sceCtrlReadBufferPositive(&pad, 1);
			if (uninstall) {
				if (pad.Buttons & PSP_CTRL_SQUARE) { break; }
				if (pad.Buttons & PSP_CTRL_CIRCLE) {
					uninstall = 0; reinstall = 1;
					if (fileExist(PRX_LFTVBACKUP)) lftv = 1; else lftv = 0;
					break;
				}
			} else {
				if (pad.Buttons & PSP_CTRL_SQUARE) { lftv = 1; break; }
				if (pad.Buttons & PSP_CTRL_CIRCLE) { lftv = 0; break; }
			}
			if (pad.Buttons & PSP_CTRL_SELECT) { cancel = 1; break; }
			sceKernelDelayThread(100000);
		}
	}

	if (!cancel) { if ((scePowerGetBatteryLifePercent() < 25) & !scePowerIsPowerOnline()) { pspDebugScreenSetTextColor(0x000099); printf(" Battery is below 25%% and AC adapter is not connected!\n\n"); cancel = 1; } }

	if (cancel) { pspDebugScreenSetTextColor(0x000099); printf(" %sstallation cancelled!\n\n", uninstall ? "Unin" : "In"); }

	if (!cancel) {
		clearScreenPrintHeader(); scePowerLock(0);
		if (quickinstall) { printf(" Quick installing warPSP to the %s slot.\n\n", lftv ? "Location Free Player" : "Remote Play"); }
		// Perform installation, uninstallation or reinstallation.
		pspDebugScreenSetTextColor(0x000000); printf(" %sstallation:\n\n", uninstall ? "Unin" : "In");
		if (uninstall) {
			if (fileExist(PRX_LFTVBACKUP)) { lftv = 1; ok = 1; } else if (fileExist(PRX_RPLYBACKUP)) { lftv = 0; ok = 1; }
			if (ok) {
				pspDebugScreenSetTextColor(0x006600); printf(" Backup prx found in the root of the memory stick. Ok to uninstall!\n\n");
				printf("  The backup file will be copied to the flash0 drive of the PSP!\n\n");
				if (fileCopy(lftv ? PRX_LFTVBACKUP : PRX_RPLYBACKUP, lftv ? PRX_LFTVPLAYER : PRX_REMOTEPLAY)) { pspDebugScreenSetTextColor(0x006600); printf("   Backup file reinstalled successfully!\n\n"); sceIoRemove(lftv ? PRX_LFTVBACKUP : PRX_RPLYBACKUP); uninstalled = 1; } else { pspDebugScreenSetTextColor(0x000099); printf("   Backup file reinstallation failed!\n\n");}
				if (uninstalled) printf("  To reinstall warPSP^xmb, just rerun the Easy Installion Program.\n\n");
			}
		} else {
			// Check if warPSP file exists.
			if (!fileExist(PRX_WARPSP_XMB)) { extractPRXFromPBP(); }
			if (!fileExist(PRX_WARPSP_XMB)) {
				pspDebugScreenSetTextColor(0x000066); printf("  warPSP.prx not in the root of the memory stick! Cancelling!\n\n");
			} else {
				pspDebugScreenSetTextColor(0x006600); printf("  warPSP.prx found in the root of the memory stick. Continuing!\n\n"); ok = 1;
			}
			if (ok) {
				// Create backup of original file and install warPSP.
				if (!reinstall) printf("  The backup file will be copied to the root of the memory stick!\n\n");
				if (!reinstall) {
					if (fileCopy(lftv ? PRX_LFTVPLAYER : PRX_REMOTEPLAY, lftv ? PRX_LFTVBACKUP : PRX_RPLYBACKUP)) { pspDebugScreenSetTextColor(0x006600); printf("   Original prx file backed up successfully!\n\n"); } else { pspDebugScreenSetTextColor(0x000099); printf("   Original prx file back up failed!\n\n");}
				}
				if (fileCopy(PRX_WARPSP_XMB, lftv ? PRX_LFTVPLAYER : PRX_REMOTEPLAY)) { pspDebugScreenSetTextColor(0x006600); printf("   warPSP^xmb installed successfully!\n\n"); sceIoRemove(PRX_WARPSP_XMB); installed = 1; } else { pspDebugScreenSetTextColor(0x000099); printf("   warPSP^xmb installation failed!\n\n"); installed = 0; }
			}
		}
		scePowerUnlock(0);
	}
	if (installed | uninstalled) sceKernelDelayThread(1000000); 
	if (!quickinstall) { printf(" Press START to "); if (installed | uninstalled) printf("restart the PSP. (Auto-Exit in 10 Seconds).\n %s!\n", installed ? "Happy warXing" : "Thank you for using warPSP"); else printf("return to the xmb!\n"); }

	while (1) {
		// Wait for exit.
		if (quickinstall) { sceKernelDelayThread(1000000); break; }
		if (autoExit >= 1000) break;
    	sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons & PSP_CTRL_START) break;
		sceKernelDelayThread(10000);
		autoExit++;
	}
	printf(" Exiting...");
	if (installed | uninstalled) { sceKernelExitGame(); scePower_0442D852(50000); } else { sceKernelExitGame(); }
	return 0;
}
