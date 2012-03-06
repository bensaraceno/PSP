#include <pspsdk.h>
#include <pspuser.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <math.h>
#include <time.h>
// USB cam headers
#include <psputility_usbmodules.h>
#include <psputility_avmodules.h>
#include <pspusb.h>
#include <pspusbacc.h>
#include <pspusbcam.h>
#include <pspjpeg.h>
// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "targets.h"
#include "graphics.h"

PSP_MODULE_INFO("eyePSP_Demo", 0, 1, 0);

PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define printf pspDebugScreenPrintf

#define CAM_LINE_SIZE 480
#define MAX_VIDEO_FRAME_SIZE (32*1024)
#define MAX_STILL_IMAGE_SIZE (512*1024)

// Program states.
int running = 1, connected = 0, aligned = 0, menu = 0;

// Behavior oriented variables.
int followcursor = 0;
int direction = 0, lastdirection = 0, method = 0;

// Global coordinates and screen areas.
Coord cursor, old, travel;

// Screen map for area detection.
union {
	struct {
		Color gridcolor, fillcolor, selcolor;
		int x, y, w, h;
	};
	float sm[4];
} screenmap;

int LoadModules() {
	int result = sceUtilityLoadUsbModule(PSP_USB_MODULE_ACC);
	if (result < 0) { printf("Error 0x%08X loading usbacc.prx.\n", result); return result; }

	result = sceUtilityLoadUsbModule(PSP_USB_MODULE_CAM);	
	if (result < 0) { printf("Error 0x%08X loading usbcam.prx.\n", result); return result; }

	result = sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC); // For jpeg decoding
	if (result < 0) printf("Error 0x%08X loading avcodec.prx.\n", result);
	return result;
}

int UnloadModules() {
	int result = sceUtilityUnloadUsbModule(PSP_USB_MODULE_CAM);
	if (result < 0) { printf("Error 0x%08X unloading usbcam.prx.\n", result); }

	result = sceUtilityUnloadUsbModule(PSP_USB_MODULE_ACC);
	if (result < 0) { printf("Error 0x%08X unloading usbacc.prx.\n", result); }

	result = sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	if (result < 0) printf("Error 0x%08X unloading avcodec.prx.\n", result);
	return result;
}

int StartUsb() {
	int result = sceUsbStart(PSP_USBBUS_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X starting usbbus driver.\n", result); return result; }

	result = sceUsbStart(PSP_USBACC_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X starting usbacc driver.\n", result); return result; }
	
	result = sceUsbStart(PSP_USBCAM_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X starting usbcam driver.\n", result); return result; }

	result = sceUsbStart(PSP_USBCAMMIC_DRIVERNAME, 0, 0);
	if (result < 0) printf("Error 0x%08X starting usbcammic driver.\n", result);
	return result;
}

int StopUsb() {
	int result = sceUsbStop(PSP_USBCAMMIC_DRIVERNAME, 0, 0);	
	if (result < 0) { printf("Error 0x%08X stopping usbcammic driver.\n", result); }

	result = sceUsbStop(PSP_USBCAM_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X stopping usbcam driver.\n", result); }

	result = sceUsbStop(PSP_USBACC_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X stopping usbacc driver.\n", result); }

	result = sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
	if (result < 0) printf("Error 0x%08X stopping usbbus driver.\n", result);
	return result;
}

int InitJpegDecoder() {
	int result = sceJpegInitMJpeg();
	if (result < 0) { printf("Error 0x%08X initing MJPEG library.\n", result); }

	result = sceJpegCreateMJpeg(480, 272);
	if (result < 0) { printf("Error 0x%08X creating MJPEG decoder context.\n", result); }
	return result;
}

int FinishJpegDecoder() {
	int result = sceJpegDeleteMJpeg();
	if (result < 0) { printf("Error 0x%08X deleting MJPEG decoder context.\n", result); }

	result = sceJpegFinishMJpeg();
	if (result < 0) { printf("Error 0x%08X finishing MJPEG library.\n", result); }

	return result;
}

void StopApp() {
	running = 0; sceUsbDeactivate(PSP_USBCAM_PID); StopUsb();
	FinishJpegDecoder(); UnloadModules();
}

// Exit callback
int exit_callback(int arg1, int arg2, void *common) {
	StopApp(); sceKernelExitGame();
	return 0;
}

// Callback thread
int CallbackThread(SceSize args, void *argp) {
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", (void *) exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

// Sets up the callback thread and returns its thread id
int SetupCallbacks(void) {
	int cb_thid = 0;
	cb_thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(cb_thid >= 0) sceKernelStartThread(cb_thid, 0, 0);
	return cb_thid;
}

// Check if a coordinate is within a region.
int checkCoordPosition(Coord check, int x0, int y0, int x1, int y1) {
	if ((check.x >= x0) && (check.x <= x1)) {
		if ((check.y >= y0) && (check.y <= y1)) return 1;
	}
	return 0;
}

// Check if a coordinate touches a button.
int checkCoordButton(Coord check, Button btn) {
	if ((check.x >= btn.x) && (check.x <= btn.x + btn.w)) {
		if ((check.y >= btn.y) && (check.y <= btn.y + btn.h)) return 1;
	}
	return 0;
}

// Where the magic happens ;)
void checkDirection(Coord current, Coord old) {
	Coord change; change.x = 0; change.y = 0;
	int sensitivity = 4; // Sensitivity variables.

	// Calculate change in cursor position and sprite speed based on change.
	change.x = current.x - old.x; change.y = current.y - old.y;
	travel.x = abs(change.x); travel.y = abs(change.y);

	if ((change.x > sensitivity * -1) && (change.x < sensitivity)) {
		// No horizontal movement along x-axis.
		if ((change.y > sensitivity * -1) && (change.y < sensitivity)) {
			direction = 0; // Dead Zone (No vertical movement along y-axis.)
		} else if (change.y <= sensitivity * -1) {
			direction = 3; // Up (North) (Negative vertical movement along y-axis)
		} else if (change.y >= sensitivity) {
			direction = 7; // Down (South) (Positive vertical movement along y-axis)
		}
	} else if (change.x <= sensitivity * -1) {
		// Negative horizontal movement along x-axis
		if ((change.y > sensitivity * -1) && (change.y < sensitivity)) {
			direction = 1; // Left (West) (No vertical movement along y-axis.)
		} else if (change.y <= sensitivity * -1) {
			direction = 2; // Left-Up (Northwest) (Negative vertical movement along y-axis)
		} else if (change.y >= sensitivity) {
			direction = 8; // Left-Down (Southwest) (Positive vertical movement along y-axis)
		}
	} else if (change.x >= sensitivity) {
		// Positive horizontal movement along x-axis.
		if ((change.y > sensitivity * -1) && (change.y < sensitivity)) {
			direction = 5; // Right (East) (No vertical movement along y-axis.)
		} else if (change.y <= sensitivity * -1) {
			direction = 4; // Right-Up (Northeast) (Negative vertical movement along y-axis)
		} else if (change.y >= sensitivity) {
			direction = 6; // Right-Down (Southeast) (Positive vertical movement along y-axis)
		}
	}
	travel.x -= sensitivity;
	travel.y -= sensitivity;
}

int video_thread(SceSize args, void *argp) {
	// Pixel coordinates: first = first luminant pixel to breach threshhold, last = ..., mid = ...
	Coord first, last, mid;
	int bufsize; int threshold = 200; // Luminance threshold.
	int showvideo = 0; // Show the actual video input? 0: No, 1: Yes

	// Camera buffers.
	PspUsbCamSetupVideoParam videoparam;
	static u8  buffer[MAX_STILL_IMAGE_SIZE] __attribute__((aligned(64)));
	static u8  work[68*1024] __attribute__((aligned(64)));
	static u32 framebuffer[480*272] __attribute__((aligned(64)));

	// Sprites and other images.
	Image* imgTargets = loadImageMemory(targets, sizeof(targets));

	// Startup cursor position.
	cursor.x = 237, cursor.y = 50; old.x = 237, old.y = 50;

	// Setup the screenmap size and position.
	screenmap.x = 20; screenmap.y = 200;
	screenmap.w = 60; screenmap.h = 60;
	screenmap.gridcolor = 0xFFC09090;
	screenmap.fillcolor = 0xFFF0F0F0;
	screenmap.selcolor = 0xFFC0FFFF;

	// Create a start button.
	Button btnStart;
	btnStart.x = 420; btnStart.y = 250;
	btnStart.w = 50; btnStart.h = 12;
	btnStart.fillcolor = 0xFF00FFFF;
	btnStart.textcolor = 0xFF000000;
	btnStart.bordercolor = 0xFF000000;
	btnStart.shadowcolor = 0xFF888888;
	btnStart.bordersize = 1;
	btnStart.borderbevel = 0;
	btnStart.shadowsize = 0;
	btnStart.shadowdistance = 0;
	strcpy(btnStart.text, "Start");
	strcpy(btnStart.name, "btnStart");

	// Wait for camera to be connected.
	while (!connected) {
		clearScreen(0xFFF0F0F0);
		printTextScreenCenter(132, "Please connect the camera and press any button.", 0xFF009900);
		flipScreen(); sceDisplayWaitVblankStart();
		sceKernelDelayThread(20000);
	}

	// Load the camera modules and start the decoder.
	if (LoadModules() < 0) sceKernelSleepThread();
	if (StartUsb() < 0) sceKernelSleepThread();
	if (sceUsbActivate(PSP_USBCAM_PID) < 0) sceKernelSleepThread();
	if (InitJpegDecoder() < 0) sceKernelSleepThread();
	while (1) {
		if ((sceUsbGetState() & 0xF) == PSP_USB_CONNECTION_ESTABLISHED) break;
		sceKernelDelayThread(50000);
	}

	//Setup video parameters and start video capture.
	memset(&videoparam, 0, sizeof(videoparam));
	videoparam.size = sizeof(videoparam);
	videoparam.resolution = PSP_USBCAM_RESOLUTION_480_272;
	videoparam.framerate = PSP_USBCAM_FRAMERATE_30_FPS;
	videoparam.wb = PSP_USBCAM_WB_INCANDESCENT;
	videoparam.saturation = 125;
	videoparam.brightness = 100;
	videoparam.contrast = 64;
	videoparam.sharpness = 0;
	videoparam.effectmode = PSP_USBCAM_EFFECTMODE_NORMAL;
	videoparam.framesize = MAX_VIDEO_FRAME_SIZE;
	videoparam.evlevel = PSP_USBCAM_EVLEVEL_0_0;	
	if (sceUsbCamSetupVideo(&videoparam, work, sizeof(work)) < 0) sceKernelExitDeleteThread(0);
	sceUsbCamAutoImageReverseSW(1);
	if (sceUsbCamStartVideo() < 0) sceKernelExitDeleteThread(0);

	while (running) {
		int i, j, lum = 0, tracking = 0;
		first.x = 0; first.y = 0; last.x = 0; last.y = 0; mid.x = old.x; mid.y = old.y;
		clearScreen(0xFFFFFFFF);

		// Capture the camera image into the framebuffer.
		bufsize = sceUsbCamReadVideoFrameBlocking(buffer, MAX_VIDEO_FRAME_SIZE);
		if (bufsize > 0) sceJpegDecodeMJpeg(buffer, bufsize, framebuffer, 0);

		// Analyze the camera image.
		for (i = 0; i < 272; i++) {
			for (j = 0; j < 480; j++) {
				if (showvideo) putPixelScreen(framebuffer[i * CAM_LINE_SIZE + j], j, i); // Show video input.
				// Calculate luminance (brightness as perceived by the eye) and compare versus threshhold. 
				lum = (299 * R(framebuffer[i * CAM_LINE_SIZE + j]) + 587 * G(framebuffer[i * CAM_LINE_SIZE + j]) + 114 * B(framebuffer[i * CAM_LINE_SIZE + j])) / 1000;
				if (lum > threshold) {
					tracking = 1; if (aligned) putPixelScreen(0xFF0000FF, j, i);
					if ((first.x == 0) || (j < first.x)) first.x = j;
					if ((first.y == 0) || (i < first.y)) first.y = i;
					if ((last.x == 0) || (j > last.x)) last.x = j;
					if ((last.y == 0) || (i > last.y)) last.y = i;
				}
			}
		}

		if (tracking) {
			// Calculate directional movement and determine cursor position.
			mid.x = first.x + (abs((last.x - first.x)) / 2); mid.y = first.y + (abs((last.y - first.y)) / 2);
			checkDirection(mid, old);
			switch (direction) {
				case 0: cursor.x = old.x; cursor.y = old.y; break;
				case 1: cursor.x = first.x; cursor.y = first.y + (abs((last.y - first.y)) / 2); break;
				case 2: cursor.x = first.x; cursor.y = first.y; break;
				case 3: cursor.x = first.x + (abs((last.x - first.x)) / 2); cursor.y = first.y; break;
				case 4: cursor.x = last.x; cursor.y = first.y; break;
				case 5: cursor.x = last.x; cursor.y = first.y + (abs((last.y - first.y)) / 2); break;
				case 6: cursor.x = last.x; cursor.y = last.y; break;
				case 7: cursor.x = first.x + (abs((last.x - first.x)) / 2); cursor.y = last.y; break;
				case 8: cursor.x = first.x; cursor.y = last.y; break;		
			};
			
			//Uncomment the following lines to draw 'directional' markers on screen.
			/*if ((abs(last.x - first.x) > 15) || (abs(last.y - first.y) > 15)) {
				if ((direction > 0) && (direction <= 4)) {
					drawLineScreen(first.x, first.y, last.x, last.y, 0xFFC0C0C0);
				} else {
					drawLineScreen(last.x, last.y, first.x, first.y, 0xFFC0C0C0);
				}
				switch (direction) {
					case 0: break;
					case 1: drawLineScreen(last.x, last.y + ((last.y - first.y) / 2), first.x, first.y + ((last.y - first.y) / 2), 0xFFC0C0C0); break; // W
					case 2: drawLineScreen(last.x, last.y, first.x, first.y, 0xFFC0C0C0); break; // NW
					case 3: drawLineScreen(first.x + ((last.x - first.x) / 2), last.y, first.x + ((last.x - first.x) / 2), first.y, 0xFFC0C0C0); break; // N
					case 4: drawLineScreen(first.x, last.y, last.x, first.y, 0xFFC0C0C0); break; // NE
					case 5: drawLineScreen(first.x, first.y + ((last.y - first.y) / 2), last.x, first.y + ((last.y - first.y) / 2), 0xFFC0C0C0); break; // E
					case 6: drawLineScreen(first.x, first.y, last.x, last.y, 0xFFC0C0C0); break; // SE
					case 7: drawLineScreen(first.x + ((last.x - first.x) / 2), first.y, first.x + ((last.x - first.x) / 2), last.y, 0xFFC0C0C0); break; // S
					case 8: drawLineScreen(last.x, first.y, first.x, last.y, 0xFFC0C0C0); break; // SW
				};
				drawLineScreen((first.x > last.x) ? last.x : first.x, (first.y > last.y) ? last.y : first.y, (first.x < last.x) ? last.x : first.x, (first.y < last.y) ? last.y : first.y, 0xFFC0C0C0);
			} else {
				drawRectScreen(0xFFC0C0C0, first.x, first.y, last.x - first.x, last.y - first.y);
			}*/
		} else {
			// printTextScreenCenter(10, "Please stay within the playing area.", 0xFF0000FF);
			if (lastdirection == 0) { cursor.x = old.x; cursor.y = old.y; }
		}

		if (!aligned) {
			showvideo = 1;
			// Alignment Screen: wait for camera to be aligned to the playing area.
			printTextScreenCenter(126, "Please align the camera to the playing area.", 0xFFFFFFFF);
			printTextScreenCenter(136, "Drag the cursor to the \"Start\" button to continue.", 0xFFFFFFFF);

			if (checkCoordButton(cursor, btnStart)) { btnStart.fillcolor = 0xFF00FF00; aligned = 1; menu = 1; }
			drawButtonScreen(btnStart);
			if (aligned) { btnStart.fillcolor = 0xFF00FFFF; btnStart.x = 240 - (btnStart.w / 2); btnStart.y = 200; }
		} else if (menu) {
			showvideo = 0;
			// Menu Screen: show a splash, logo, menu, etc.
			printTextScreenCenter(126, "eyePSP Shooting Gallery", 0xFF009900);
			printTextScreenCenter(136, "Please press the \"Start\" button to continue.", 0xFFFF0000);

			if (checkCoordButton(cursor, btnStart)) { btnStart.fillcolor = 0xFFC0FFC0; menu = 0; }
			drawButtonScreen(btnStart);
		} else {
			// Game Screen: allow user to control a sprite (blue box) using detection methods.
			// Draw any game objects here.
			blitAlphaImageToScreen(0, 0, 51, 51, imgTargets, 100, 100);
			blitAlphaImageToScreen(12, 52, 25, 25, imgTargets, 175, 150);
			blitAlphaImageToScreen(19, 79, 11, 11, imgTargets, 210, 200);
		}

		// Draw cursor (within boundaries) .
		if (tracking) {
			for (i = cursor.y - 5; i <= cursor.y + 5; i++) { if ((i > 0) && (i < 272)) putPixelScreen(!tracking ? 0xFF0000FF : 0xFF009900, cursor.x, i); } // y-axis
			for (j = cursor.x - 5; j <= cursor.x + 5; j++) { if ((j > 0) && (j < 480)) putPixelScreen(!tracking ? 0xFF0000FF : 0xFF009900, j, cursor.y); } // x-axis
		}

		old.x = cursor.x; old.y = cursor.y; lastdirection = direction;
		flipScreen(); sceDisplayWaitVblankStart();
		sceKernelDelayThread(20000);
	}
	sceKernelExitDeleteThread(0);
	return 0;	
}

int main() {
	SceUID video_thid; //int xx, yy, nubspeed; 
	SetupCallbacks(); pspDebugScreenInit(); initGraphics();
	SceCtrlData pad, oldpad; oldpad.Buttons = 0xFFFFFFFF;
	sceCtrlSetSamplingCycle(0); sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// Start the video thread to draw objects to screen and manage video input.
	video_thid = sceKernelCreateThread("video_thread", video_thread, 0x12, 512 * 1024, 0, NULL);
	if (video_thid > 0) sceKernelStartThread(video_thid, 0, NULL);
	
	while (!connected) { sceCtrlReadBufferPositive(&pad, 1); if (pad.Buttons) connected = 1; }

	// Wait for camera to be aligned to playing area and for game to start.
	while (running && (!aligned || menu)) {
		// Uncomment the following lines to include analog stick and button input during alignment.
		/*sceCtrlPeekBufferPositive(&pad, 1);
		xx = pad.Lx - 128; yy = pad.Ly - 128;
		nubspeed = sqrt((xx * xx) + (yy * yy)) / (2 * sqrt(sqrt((xx * xx) + (yy * yy))));
		if (xx > 30) cursorx += nubspeed; else if (xx < -30) cursorx -= nubspeed;
		if (yy > 30) cursory += nubspeed; else if (yy < -30) cursory -= nubspeed;
		if (cursorx > 480) cursorx = 480; else if (cursorx < 0) cursorx = 0;
		if (cursory > 272) cursory = 272; else if (cursory < 0) cursory = 0;

		// Detect changes in button input.
		if (pad.Buttons != oldpad.Buttons) {
		}
		oldpad.Buttons = pad.Buttons;*/
		sceKernelDelayThread(50000);
	}

	while (running) {
		// Uncomment the following lines to include analog stick and button input.
		// Defeats the purpose as holding the PSP or touching it shakes the camera
		// and makes the laser pointer input very inaccurate. Could be useful with
		// the PSP remote control to allow games to be played using the remote
		// for D-Pad movement and the laser pointer for looking/aiming.
		/*sceCtrlPeekBufferPositive(&pad, 1);
		xx = pad.Lx - 128; yy = pad.Ly - 128;
		nubspeed = sqrt((xx * xx) + (yy * yy)) / (2 * sqrt(sqrt((xx * xx) + (yy * yy))));
		if (xx > 30) cursorx += nubspeed; else if (xx < -30) cursorx -= nubspeed;
		if (yy > 30) cursory += nubspeed; else if (yy < -30) cursory -= nubspeed;
		if (cursorx > 480) cursorx = 480; else if (cursorx < 0) cursorx = 0;
		if (cursory > 272) cursory = 272; else if (cursory < 0) cursory = 0;

		// Detect changes in button input.
		if (pad.Buttons != oldpad.Buttons) {
		}
		oldpad.Buttons = pad.Buttons;*/

		// Change sprite speeds, positions and behaviors here.

		sceKernelDelayThread(20000);
	}

	StopApp();
	sceKernelExitGame();
	return 0;
}
