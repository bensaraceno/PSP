#include <pspkernel.h>
#include <pspcallbacks.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <string.h>
#include <stdio.h>
#include "draw.h"
#include "font.c"

#define pbpSize 358493
#define bg_col 0xFFFFFF

#define PRX_WARPSP_XMB "ms0:/warPSP.prx"
#define PBP_WARPSP_EIP "EBOOT.PBP"

#define PRX_LFTVBACKUP "ms0:/lftv_plugin.prx"
#define PRX_LFTVPLAYER "flash0:/vsh/nodule/lftv_plugin.prx"

#define PRX_RPLYBACKUP "ms0:/premo_plugin.prx"
#define PRX_REMOTEPLAY "flash0:/vsh/nodule/premo_plugin.prx"

PSP_MODULE_INFO("warPSP^eip", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

int scePower_0442D852(int unknown); 
int quickinstall = 0, i = 0;
char buffer[256];

void clearScreenPrintHeader(int y) {
	i = 0;
	sceGuStart(GU_DIRECT, list);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	drawStringCenter("warPSP", y, 0xFF990000, 0);
	drawStringCenter("warXing Suite", y ? y + 20 : y + 10, 0xFF550000, 0);
	drawStringCenter("Easy Installation Program", y ? y + 40 : y + 30, 0xFF000000, 0);
	drawStringCenter("by caliFrag", y ? y + 80 : y + 20, 0xFF006600, 0);
}

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
		sprintf(buffer, "  %s > %s Done.", sFileSrc, sFileDest); swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter(buffer, 50 + (10 * i), 0xFF000000, 7); drawObjects(); i++;
		return 1;
	} else {
		sprintf(buffer, "  File not found: %s", sFileSrc); swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter(buffer, 50 + (10 * i), 0xFF000099, 7); drawObjects(); i++;
		return 0;
	}
	i++;
}

int main() {
	SceCtrlData pad;
	int cancel = 0, uninstall = 0, reinstall = 0, lftv = 0, ok = 0, installed = 0, uninstalled = 0, autoExit = 0;
	SetupCallbacks();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	sceCtrlReadBufferPositive(&pad, 1);
	if (pad.Buttons & PSP_CTRL_LTRIGGER) { quickinstall = 1; lftv = 1; } else if (pad.Buttons & PSP_CTRL_RTRIGGER) { quickinstall = 1; }
	if (fileExist(PRX_LFTVBACKUP) | fileExist(PRX_RPLYBACKUP)) uninstall = 1;

	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuClearColor(0xFFFFFFFF);
	sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
	sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuDepthRange(0xc350, 0x2710);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, 256, 128, 256, font);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexEnvColor(0x0);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexScale(1.0f / 256.0f, 1.0f / 128.0f);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuFinish();
	sceGuSync(0,0);
	sceGuDisplay(GU_TRUE);

	// Check for low battery.
	if ((scePowerGetBatteryLifePercent() < 25) & !scePowerIsPowerOnline()) {
		sceGuStart(GU_DIRECT, list);
		sceGuClear(GU_COLOR_BUFFER_BIT);
		drawStringCenter("Battery charge should be at least 25% when modifying flash!", 40 + (10 * i), 0xFF0000FF, 8); i += 2;
		drawStringCenter("Connect the AC adapter to ignore this warning and continue!", 40 + (10 * i), 0xFF0000FF, 8); i += 2;
		drawStringCenter(uninstall ? "Press any button to cancel uninstallation of warPSP." : "Press any button to cancel installation of warPSP.", 50 + (10 * i), 0xFF0000FF, 8); i += 2;
		drawObjects();
		while (!scePowerIsPowerOnline()) { sceCtrlReadBufferPositive(&pad, 1); if (pad.Buttons) { cancel = 1; break; } }
	}

	if (!cancel) {
		float c = 0.0;
		for (c = 10.0; c <= 100.0; c++) {
			unsigned int col = 0xFF000000 |
				(unsigned int)((c / 100.0) * 255.0f) << 16 |
				(unsigned int)((c / 100.0) * 255.0f) <<  8 |
				(unsigned int)((c / 100.0) * 255.0f) <<  0;
			sceGuClearColor(col);
			clearScreenPrintHeader(90);
			if (quickinstall & (c > 50)) drawStringCenter("Quick Install Activated!", 250, 0xFF006600, 8);
			drawObjects();
		}
		sceKernelDelayThread(3000000);
		for (c = 100.0; c >= 10.0; c--) {
			unsigned int col = 0xFF000000 |
				(unsigned int)((c / 100.0) * 255.0f) << 16 |
				(unsigned int)((c / 100.0) * 255.0f) <<  8 |
				(unsigned int)((c / 100.0) * 255.0f) <<  0;
			sceGuClearColor(col);
			clearScreenPrintHeader(90);
			drawObjects();
		}
	}

	sceGuClearColor(0xFFFFFFFF);

	// Show disclaimer and options.
	if (!cancel & !quickinstall) {
		sceGuStart(GU_DIRECT, list);
		sceGuClear(GU_COLOR_BUFFER_BIT);
		drawStringCenter("!!! DISCLAIMER !!!", 40 + (10 * i), 0xFF0000FF, 10); i += 2;
		drawStringCenter("This program modifies the flash drive of your Sony PSP.", 60 + (10 * i), 0xFF0000FF, 8); i++;
		drawStringCenter("You accept the risk when running this installation app.", 60 + (10 * i), 0xFF0000FF, 8); i += 2;
		drawStringCenter("DO NOT REMOVE THE MEMORY STICK WHEN ORANGE LED FLASHES.", 60 + (10 * i), 0xFF0000FF, 8); i++;
		drawStringCenter("DO NOT TURN OFF THE PSP SYSTEM WHEN ORANGE LED FLASHES.", 60 + (10 * i), 0xFF0000FF, 8); i += 2;
		drawStringCenter("Press START to acknowledge and to continue to briefing.", 60 + (10 * i), 0xFF0000FF, 8); i++;
		drawStringCenter("Press SELECT to decline and to abort installing warPSP.", 60 + (10 * i), 0xFF0000FF, 8); i += 2;
		drawStringCenter("THE AUTHOR DOES NOT HOLD RESPONSIBILITY FOR ANY DAMAGE.", 60 + (10 * i), 0xFF0000FF, 8); i++;
		drawStringCenter("THIS SOFTWARE IS PRESENTED WITHOUT WARRANTY/GUARANTEES.", 60 + (10 * i), 0xFF0000FF, 8); i += 4;
		drawObjects();
		while (1) {
			sceCtrlReadBufferPositive(&pad, 1);
			if (pad.Buttons & PSP_CTRL_START) { break; }
			if (pad.Buttons & PSP_CTRL_SELECT) { cancel = 1; break; }
		}
	}

	// Check if backup file exists.
	if (!cancel & !quickinstall) {
		swapBuffers(); clearScreenPrintHeader(0); drawObjects(); clearScreenPrintHeader(0);
		drawStringCenter("Briefing", 50 + (10 * i), 0xFF000000, 0); i+= 2;
		drawStringCenter("Thanks for your interest in the warPSP Software Suite!", 50 + (10 * i), 0xFF006600, 0); i += 2;
		drawStringCenter("warPSP is an advanced warXing utility for the Sony PSP.", 50 + (10 * i), 0xFF000000, 8); i += 2;
		drawStringCenter("Please see the README.TXT file for more information.", 50 + (10 * i), 0xFF660000, 8); i += 3;
		drawStringCenter("Options", 50 + (10 * i), 0xFF000000, 0); i += 2;
		if (uninstall) {
			drawStringCenter("Press SQUARE to uninstall warPSP and restore backup files.", 50 + (10 * i), 0xFF000000, 8); i++;
			drawStringCenter("Press CIRCLE to reinstall warPSP to the last slot selected.", 50 + (10 * i), 0xFF000000, 8); i++;
		} else {
			drawStringCenter("Press SQUARE to install warPSP to the LFTV Player slot.", 50 + (10 * i), 0xFF000000, 8); i++;
			drawStringCenter("Press CIRCLE to install warPSP to the Remote Play slot.", 50 + (10 * i), 0xFF000000, 8); i++;
		}
		drawStringCenter(uninstall ? "Press SELECT to cancel uninstallation of warPSP and exit." : "Press SELECT to cancel installation of warPSP and exit.", 50 + (10 * i), 0xFF000099, 8); i += 2;
		drawObjects();
		while (1) {
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
		}
	}

	if (!cancel) {
		if ((scePowerGetBatteryLifePercent() < 25) & !scePowerIsPowerOnline()) {
			swapBuffers(); sceGuStart(GU_DIRECT, list);
			drawStringCenter(" Battery is below 25%% and AC adapter is not connected!", 50 + (10 * i), 0xFF000099, 0); i += 2;
			cancel = 1; drawObjects();
		}
	}

	if (cancel) {
		swapBuffers(); sceGuStart(GU_DIRECT, list);
		sprintf(buffer, "%sstallation cancelled!", uninstall ? "Unin" : "In");
		drawStringCenter(buffer, 50 + (10 * i), 0xFF0000FF, 0); i += 2;
		drawObjects();
		sceKernelDelayThread(1000000);
	}

	// Perform installation, uninstallation or reinstallation.
	if (!cancel) {
		scePowerLock(0);
		swapBuffers(); clearScreenPrintHeader(0); drawObjects(); clearScreenPrintHeader(0); drawObjects(); swapBuffers(); sceGuStart(GU_DIRECT, list);
		drawStringCenter(uninstall ? "Uninstallation" : "Installation", 50 + (10 * i), 0xFF990000, 0); i += 2;
		if (quickinstall) { drawStringCenter("Quick installing warPSP to the location free player slot.", 50 + (10 * i), 0xFF990000, 0); i += 2; }
		drawObjects(); swapBuffers(); sceGuStart(GU_DIRECT, list);
		if (uninstall) {
			if (fileExist(PRX_LFTVBACKUP)) { lftv = 1; ok = 1; } else if (fileExist(PRX_RPLYBACKUP)) { lftv = 0; ok = 1; }
			if (ok) {
				drawStringCenter("Backup prx found. Ok to uninstall!", 50 + (10 * i), 0xFF990000, 8); i++;
				drawStringCenter("The backup prx will be copied to the flash drive of your PSP!", 50 + (10 * i), 0xFF000000, 8); i += 2; drawObjects();
				if (fileCopy(lftv ? PRX_LFTVBACKUP : PRX_RPLYBACKUP, lftv ? PRX_LFTVPLAYER : PRX_REMOTEPLAY)) { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("Backup file reinstalled successfully!", 50 + (10 * i), 0xFF006600, 8); sceIoRemove(lftv ? PRX_LFTVBACKUP : PRX_RPLYBACKUP); uninstalled = 1; } else { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("Backup file reinstallation failed!", 50 + (10 * i), 0xFF000099, 8); }
				i += 2; drawObjects();
				if (uninstalled) { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("To reinstall warPSP, rerun the Easy Installation Program.", 50 + (10 * i), 0xFF990000, 8); i += 2; drawObjects(); }
			}
		} else {
			if (fileExist(PRX_WARPSP_XMB)) { sceIoRemove(PRX_WARPSP_XMB); sceKernelDelayThread(1000000); }
			drawStringCenter("Extracting warPSP.prx to the root of the memory stick.", 50 + (10 * i), 0xFF000000, 8); i += 2; drawObjects();
			sceKernelDelayThread(2000000);
			// Open PBP file and read contents into the buffer.
			int pbpFile, prxFile, pkgSize = 0, prxSize = 0; char buf[1024*1024];
			pbpFile = sceIoOpen(PBP_WARPSP_EIP, PSP_O_RDONLY, 0);
			sceKernelDelayThread(1000000);
			if (pbpFile) {
				// Get size of entire package.
				pkgSize = sceIoRead(pbpFile, buf, sizeof(buf));
				sceKernelDelayThread(1000000);
				if (pkgSize > 0) {
					swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("EBOOT.PBP loaded into memory successfully!", 50 + (10 * i), 0xFF006600, 8); i += 2; drawObjects();
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
						swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("warPSP.prx extracted from memory successfully!", 50 + (10 * i), 0xFF006600, 8); drawObjects();
					} else {
						swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("warPSP.prx extraction from memory failed!", 50 + (10 * i), 0xFF000099, 8); drawObjects();
					}
					i += 2;
				}
				sceIoClose(pbpFile);
				sceKernelDelayThread(1000000);
			} else {
				swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("EBOOT.PBP load into memory failed!", 50 + (10 * i), 0xFF000099, 8); i += 2; drawObjects();
			}
			buf[0] = (char)"\0";
			swapBuffers(); sceGuStart(GU_DIRECT, list);
			if (!fileExist(PRX_WARPSP_XMB)) { drawStringCenter("warPSP.prx not found! Install cancelled!", 50 + (10 * i), 0xFF000099, 8); } else { drawStringCenter("warPSP.prx found. Ok to install!", 50 + (10 * i), 0xFF006600, 8); ok = 1; }
			i += 2; drawObjects();
			// Create backup of original file and install warPSP.
			if (ok) {
				if (!reinstall) {
					swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("The backup file will be copied to the memory stick!", 50 + (10 * i), 0xFF990000, 8); i++; drawObjects();
					if (fileCopy(lftv ? PRX_LFTVPLAYER : PRX_REMOTEPLAY, lftv ? PRX_LFTVBACKUP : PRX_RPLYBACKUP)) { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("Original prx file backed up successfully!", 50 + (10 * i), 0xFF006600, 8); } else { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("Original prx file back up failed!", 50 + (10 * i), 0xFF000099, 8); }
					i += 2; drawObjects();
				}
				if (fileCopy(PRX_WARPSP_XMB, lftv ? PRX_LFTVPLAYER : PRX_REMOTEPLAY)) { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("warPSP^xmb installed successfully!", 50 + (10 * i), 0xFF006600, 8); sceIoRemove(PRX_WARPSP_XMB); installed = 1; } else { swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("warPSP^xmb installation failed!", 50 + (10 * i), 0xFF000099, 8); installed = 0; }
				i += 2; drawObjects();
			}
		}
		scePowerUnlock(0);
	}

	if (installed | uninstalled) { sceKernelDelayThread(1000000); }
	if (!quickinstall) {
		swapBuffers(); sceGuStart(GU_DIRECT, list);
		sprintf(buffer, "Press any button to %s! (Auto-Exit in 10s).", (installed | uninstalled) ? "restart the PSP" : "return to the xmb"); drawStringCenter(buffer, 50 + (10 * i), 0xFF000000, 8); i++;
		if (installed) { drawStringCenter("Happy warXing!", 50 + (10 * i), 0xFF006600, 8); i++; } else if (uninstalled) { drawStringCenter("Thank you for using warPSP", 50 + (10 * i), 0xFF990000, 8); i++; }
		drawObjects();
		// Wait for exit.
		while (1) {
			if (autoExit >= 1000) break;
	    	sceCtrlReadBufferPositive(&pad, 1);
			if (pad.Buttons) break;
			sceKernelDelayThread(10000);
			autoExit++;
		}
	}

	if (quickinstall) { sceKernelDelayThread(1000000); }
	swapBuffers(); sceGuStart(GU_DIRECT, list); drawStringCenter("Exiting!", 50 + (10 * i), (installed | uninstalled) ? 0xFF990000 : 0xFF0000FF, 8); drawObjects();
	if (installed | uninstalled) { sceKernelExitGame(); scePower_0442D852(50000); } else { sceKernelExitGame(); }
	return 0;
}
