#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <pspuser.h>
#include <pspdisplay.h>
#include <pspctrl.h>
// USB Cam headers
#include <psputility_usbmodules.h>
#include <psputility_avmodules.h>
#include <pspusb.h>
#include <pspusbacc.h>
#include <pspusbcam.h>
#include <pspjpeg.h>
//Network headers
#include <pspnet.h>
#include <pspwlan.h>
#include <psputility_netmodules.h>
#include <psputility_sysparam.h>
#include <psputility_netparam.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// Standard headers
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"

#define printf pspDebugScreenPrintf
#define MAX_VIDEO_FRAME_SIZE (32*1024)
#define MAX_STILL_IMAGE_SIZE (512*1024)
#define SERVER_PORT 23
#define HELLO_MSG "Welcome to the skyPSP pspWebcam server!"

PSP_MODULE_INFO("skyPSP Webcam Server", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

/* Forward declarations. */
void StopApp();
int uploadFile(const char *filename, const char *ftpserver, const char *destname);

int bgSize, bufsize;
static SceUID waitphoto;
static int takephoto = 0;
static u8  work[68*1024] __attribute__((aligned(64)));
static u8  buffer[MAX_STILL_IMAGE_SIZE] __attribute__((aligned(64)));
//static u8  tampon[MAX_STILL_IMAGE_SIZE] __attribute__((aligned(64)));
//static u32 bgBuffer[480*272] __attribute__((aligned(64)));
static u32 shotBuffer[480*272] __attribute__((aligned(64)));
static u32 framebuffer[480*272] __attribute__((aligned(64)));

char msg_buffer[256], err_buffer[256], stat_buffer[50], read_buffer[55], localpath[256];
int err = 0, kill_threads = 0, connected = 0, server = 0, msgdelay = 0, saveimages = 0;
int showvideo = 1, showmask = 0, autorefresh = 1;
float camdelay = 0.5;

// FTP connection format: ftp://username:password@ftp.server.com/
unsigned char FTP_USERNAME[32] = { };
unsigned char FTP_PASSWORD[32] = { };
unsigned char FTP_HOSTADDR[256] = { };

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
	StopApp(); kill_threads = 1;
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp) {
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", (void *) exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void) {
	int thid = 0;
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0) sceKernelStartThread(thid, 0, 0);
	return thid;
}

void makeDirectory(const char *dir_path) {
	int d = sceIoDopen(dir_path); if (d >= 0) sceIoDclose(d); else sceIoMkdir(dir_path, 0777);
}

int fileSize(const char *filename) {
	int filesize = 0; int srcFile = sceIoOpen(filename, PSP_O_RDONLY, 0777);
	if (srcFile) { filesize = sceIoLseek(srcFile, 0, SEEK_END); sceIoClose(srcFile); }
	return filesize;
}

int InitialiseNetwork(void) { 
  int err;
  printf("Loading network modules... ");
  err = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
  if (err != 0) { printf("\nError, could not load PSP_NET_MODULE_COMMON %08X", err); return 1; }
  err = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
  if (err != 0) { printf("\nError, could not load PSP_NET_MODULE_INET %08X", err); return 1; }
  err = pspSdkInetInit();
  if (err != 0) { printf("\nError, could not initialise the network %08X", err); return 1; }
  printf("Done!\n");
  return 0;
}

int LoadModules() {
	int result = sceUtilityLoadUsbModule(PSP_USB_MODULE_ACC);
	if (result < 0) { printf("Error 0x%08X loading usbacc.prx.\n", result); return result; }
	result = sceUtilityLoadUsbModule(PSP_USB_MODULE_CAM);	
	if (result < 0) { printf("Error 0x%08X loading usbcam.prx.\n", result); return result; }
	result = sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC); // For jpeg decoding
	if (result < 0) { printf("Error 0x%08X loading avcodec.prx.\n", result); }
	return result;
}

int UnloadModules() {
	int result = sceUtilityUnloadUsbModule(PSP_USB_MODULE_CAM);
	if (result < 0) { printf("Error 0x%08X unloading usbcam.prx.\n", result); return result; }
	result = sceUtilityUnloadUsbModule(PSP_USB_MODULE_ACC);
	if (result < 0) { printf("Error 0x%08X unloading usbacc.prx.\n", result); return result; }
	result = sceUtilityUnloadAvModule(PSP_AV_MODULE_AVCODEC);
	if (result < 0) { printf("Error 0x%08X unloading avcodec.prx.\n", result); }
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
	if (result < 0) { printf("Error 0x%08X starting usbcammic driver.\n", result); }
	return result;
}

int StopUsb() {
	int result = sceUsbStop(PSP_USBCAMMIC_DRIVERNAME, 0, 0);	
	if (result < 0) { printf("Error 0x%08X stopping usbcammic driver.\n", result); return result; }
	result = sceUsbStop(PSP_USBCAM_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X stopping usbcam driver.\n", result); return result; }
	result = sceUsbStop(PSP_USBACC_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X stopping usbacc driver.\n", result); return result; }
	result = sceUsbStop(PSP_USBBUS_DRIVERNAME, 0, 0);
	if (result < 0) { printf("Error 0x%08X stopping usbbus driver.\n", result); }
	return result;
}

int InitJpegDecoder() {
	int result = sceJpegInitMJpeg();
	if (result < 0) { printf("Error 0x%08X initing MJPEG library.\n", result); return result; }
	result = sceJpegCreateMJpeg(480, 272);
	if (result < 0) { printf("Error 0x%08X creating MJPEG decoder context.\n", result); }
	return result;
}

int FinishJpegDecoder() {
	int result = sceJpegDeleteMJpeg();
	if (result < 0) { printf("Error 0x%08X deleting MJPEG decoder context.\n", result); return result; }
	result = sceJpegFinishMJpeg();
	if (result < 0) { printf("Error 0x%08X finishing MJPEG library.\n", result); }
	return result;
}

int SaveFile(const char *path, void *buf, int size) {
	SceUID fd; int w;
	fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (fd < 0) return fd;
	w = sceIoWrite(fd, buf, size);
	sceIoClose(fd);
	return w;
}

void StopApp() {
	sceUsbDeactivate(PSP_USBCAM_PID); StopUsb();
	FinishJpegDecoder(); UnloadModules();
}

int take_photo(int n) {
	PspUsbCamSetupStillParam stillparam;
	int  result;
	char path[128];
	// Video has to be stopped before taking a still image
	result = sceUsbCamStopVideo();
	if (result < 0) { printf("Error 0x%08X stopping video.\n", result); /*sceKernelExitDeleteThread(result);*/ }
	memset(&stillparam, 0, sizeof(stillparam));	
	stillparam.size = sizeof(stillparam);
	stillparam.resolution = PSP_USBCAM_RESOLUTION_480_272; // Image size
	stillparam.jpegsize = MAX_STILL_IMAGE_SIZE;
	stillparam.delay = PSP_USBCAM_NODELAY;
	stillparam.complevel = 1;
	stillparam.reverseflags = PSP_USBCAM_MIRROR;
	if (sceUsbCamGetLensDirection() == PSP_USBCAM_DIRECTION_OUTSIDE) { stillparam.reverseflags |= PSP_USBCAM_FLIP; }
	result = sceUsbCamSetupStill(&stillparam);	
	if (result < 0) { printf("Error 0x%08X in sceUsbCamSetupStill.\n", result); return result; }
	result = sceUsbCamStillInputBlocking(buffer, MAX_STILL_IMAGE_SIZE);
	if (result < 0) { printf("Error 0x%08X in sceUsbCamStillInput.\n", result); return result; }
	if (saveimages) sprintf(path, "ms0:/PICTURE/skyPSPcam/skyPSPcam%03d.jpg", n); else sprintf(path, "ms0:/PICTURE/skyPSPcam/skyPSPcam.jpg");
	strcpy(localpath, path); bufsize = result;
	if (saveimages) SaveFile(path, buffer, result);
	result = sceUsbCamStartVideo();
	if (result < 0) { printf("Error 0x%08X in sceUsbCamStartVideo.\n", result); /*sceKernelExitDeleteThread(result);*/ }
	return 0;
}

int video_thread(SceSize args, void *argp) {
	PspUsbCamSetupVideoParam videoparam;
	int result;
	u32 *vram = (u32 *)0x04000000;
	int photonumber = 0;
	memset(&videoparam, 0, sizeof(videoparam));
	videoparam.size = sizeof(videoparam);
	videoparam.resolution = PSP_USBCAM_RESOLUTION_480_272;
	videoparam.framerate = PSP_USBCAM_FRAMERATE_15_FPS;
	videoparam.wb = PSP_USBCAM_WB_AUTO;
	videoparam.saturation = 125;
	videoparam.brightness = 128;
	videoparam.contrast = 64;
	videoparam.sharpness = 0;
	videoparam.effectmode = PSP_USBCAM_EFFECTMODE_NORMAL;
	videoparam.framesize = MAX_VIDEO_FRAME_SIZE;
	videoparam.evlevel = PSP_USBCAM_EVLEVEL_0_0;	
	result = sceUsbCamSetupVideo(&videoparam, work, sizeof(work));	
	if (result < 0) { printf("Error 0x%08X calling sceUsbCamSetupVideo.\n", result); }
	
	sceUsbCamAutoImageReverseSW(1);
	result = sceUsbCamStartVideo();	
	if (result < 0) { printf("Error 0x%08X calling sceUsbCamStartVideo.\n", result); }
	sceDisplaySetMode(0, 480, 272);
	sceDisplaySetFrameBuf((void *)0x04000000, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
	while (1) {
		int i, j, m, n;
		result = sceUsbCamReadVideoFrameBlocking(buffer, MAX_VIDEO_FRAME_SIZE);
		if (result < 0) { printf("\nError 0x%08X calling sceUsbCamReadVideoFrameBlocking!", result); }	
		result = sceJpegDecodeMJpeg(buffer, result, framebuffer, 0);
		if (result < 0) { printf("\nError 0x%08X decoding frame %d!", result, photonumber); }
		if (showvideo) {
			for (i = 0; i < 272; i++) {
				m = i * 480; n = i * 512;
				for (j = 0; j < 480; j++) {
					vram[n + j] = framebuffer[m + j];
					shotBuffer[m + j] = framebuffer[m + j];
					//if (showmask) { if (bgBuffer[m + j] > 0x00000000) { vram[n + j] = bgBuffer[m + j]; shotBuffer[m + j] = bgBuffer[m + j]; } }
				}
			}
		}
		if (takephoto) {
			err = take_photo(photonumber++);
			if (err) printf("\nError 0x%08X saving image %d", err, photonumber);
			takephoto = 0; sceKernelSignalSema(waitphoto, 1);
		}
	}
	sceKernelExitDeleteThread(0);
	return 0;	
}

/* Make a new socket for connection. */
int make_socket(uint16_t port){
	int sock, ret; struct sockaddr_in name;
	sock = socket(PF_INET, SOCK_STREAM, 0); if (sock < 0) return -1;
	name.sin_family = AF_INET; name.sin_port = htons(port); name.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sock, (struct sockaddr *) &name, sizeof(name)); if (ret < 0) return -1;
	return sock;
}

char *getconfname(int confnum) { 
  static char confname[128]; 
  sceUtilityGetNetParam(confnum, PSP_NETPARAM_NAME, (netData *)confname); 
  return confname; 
}

size_t curlReadFunctionCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t total = size * nmemb;
	if (total > bufsize) total = bufsize;
	if (total > 0) {
		memcpy(ptr, stream, total);
		if (bufsize > total) memcpy(stream, ((char*)stream) + total, bufsize - total);
		bufsize -= total;
	}
	return total;
}

int stream_cam(const char *ftpserver, const char *destname) {
	CURL *curl; CURLcode res;
	struct curl_slist *headerlist = NULL;
	char ftpCommand[5][256], REMOTE_URL[256];
	sprintf(REMOTE_URL, "%s/skyPSP.upload", ftpserver); // File destination: remoteftp/skyPSP.upload.
	sprintf(ftpCommand[0], "RNTO %s", destname); // FTP command to rename from skyPSP.uplod to destname.
	curl = curl_easy_init();
	if(curl) {
		headerlist = curl_slist_append(headerlist, "RNFR skyPSP.upload");
		headerlist = curl_slist_append(headerlist, ftpCommand[0]);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1); // Enable uploading
		curl_easy_setopt(curl, CURLOPT_URL, REMOTE_URL); // Destination file
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist); // FTP commands to run after the transfer
		curl_easy_setopt(curl, CURLOPT_READDATA, buffer);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, &curlReadFunctionCallback);
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, bufsize);
		res = curl_easy_perform(curl);
		curl_slist_free_all(headerlist);
		curl_easy_cleanup(curl); // Cleanup any garbage.
	}
	return 0;
}

/* Connect to an access point */
int connect_to_apctl(int config) {
	int err; int stateLast = -1;
	// Connect using the first profile
	if (sceWlanGetSwitchState() != 1)
    pspDebugScreenClear();

	while (sceWlanGetSwitchState() != 1) {
	    pspDebugScreenSetXY(0, 0);
	    printf("Please enable WLAN to continue.\n");
	    sceKernelDelayThread(1000 * 1000);
	}
	err = sceNetApctlConnect(config);
	if (err != 0) { printf("sceNetApctlConnect returns %08X\n", err); return 0; }
	printf("Connecting...\n");
	while (1) {
		int state;
		err = sceNetApctlGetState(&state);
		if (err != 0) { printf("sceNetApctlGetState returns $%x\n", err); break; }
		if (state > stateLast) { printf("  Connection State: %d of 4\n", state); stateLast = state; }
		if (state == 4) break;  // connected with static IP
		// wait a little before polling again
		sceKernelDelayThread(50*1000); // 50ms
	}
	if (err != 0) { return 0; }
	return 1;
}

/* Start a simple tcp echo server */
void server_thread(SceSize args, void *argp) {
	int ret, sock, readbytes, n = 0, resettimer = 0, new = -1;
	struct sockaddr_in client; char data[1024];
	size_t size; fd_set set; fd_set setsave;

	sock = make_socket(SERVER_PORT); // Create a socket for listening
	if (sock < 0) { sprintf(err_buffer, "Error creating server socket"); } else {
		ret = listen(sock, 1);
		if (ret < 0) { sprintf(err_buffer, "Error calling listen"); } else {
			FD_ZERO(&set); FD_SET(sock, &set); setsave = set;
			while (!kill_threads && connected) {
				int i; set = setsave; server = 1; resettimer++;
				if ((strlen(read_buffer) > 50) || (resettimer > 500)) { resettimer = 0; strcpy(read_buffer, "\0"); } //strrchr(read_buffer, ' ')); }
				if (select(FD_SETSIZE, &set, NULL, NULL, NULL) < 0) { sprintf(err_buffer, "Select error!"); break; }
				for (i = 0; i < FD_SETSIZE; i++) {
					if (FD_ISSET(i, &set)) {
						int val = i;
						if (val == sock) {
							new = accept(sock, (struct sockaddr *) &client, &size);
							if (new < 0) { sprintf(err_buffer, "Error in accept %s", strerror(errno)); close(sock); break; }
							sprintf(msg_buffer, "New Connection: (%d) %d:%s", val, ntohs(client.sin_port), inet_ntoa(client.sin_addr)); msgdelay = 100;
							write(new, HELLO_MSG, strlen(HELLO_MSG)); FD_SET(new, &setsave);
						} else {
							readbytes = read(val, data, sizeof(data));
							if (readbytes <= 0) { sprintf(msg_buffer, "Socket %d closed", val); FD_CLR(val, &setsave); close(val); } else {
								strncat(read_buffer, data, readbytes); n += readbytes;
							}
						}
					}
				}
			}
		}
		close(sock);
	}
	server = 0;
	sceKernelExitDeleteThread(0);
}

int net_thread(SceSize args, void *argp) {
	int selComponent = 1;
	printf("Connecting to Network #%d - %s...\n", selComponent, getconfname(selComponent));
	if (connect_to_apctl(selComponent)) {
		char szMyIPAddr[32]; connected = 1;
		if (sceNetApctlGetInfo(8, szMyIPAddr) != 0) strcpy(szMyIPAddr, "Unknown");
		pspDebugScreenClear(); printf("Connected! IP Address: %s\n", szMyIPAddr);
	}
	return sceKernelExitDeleteThread(0);
}

int main() {
	char ftpserver[256], cfg_buffer[256];
	int *cfgvartype = 0; int *cfgvarsize = 0;
	SetupCallbacks(); pspDebugScreenInit();
	SceCtrlData pad, oldpad; oldpad.Buttons = 0xFFFFFFFF;
	SceUID net_thid, vid_thid, server_thid;

	/* Load options from config file. */
	configLoad("skyPSP.cfg");
	err = configRead("FTP", "HOSTADDRESS", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) strcpy((char*)FTP_HOSTADDR, cfg_buffer);
	err = configRead("FTP", "USERNAME", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) strcpy((char*)FTP_USERNAME, cfg_buffer);
	err = configRead("FTP", "PASSWORD", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) strcpy((char*)FTP_PASSWORD, cfg_buffer);
	err = configRead("CAM", "CAMDELAY", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) {
		
	
	}printf("\n\n%s\n\n", cfg_buffer);//camdelay = atoi(cfg_buffer);

	sprintf(ftpserver, "ftp://%s:%s@%s", FTP_USERNAME, FTP_PASSWORD, FTP_HOSTADDR);

	if (InitialiseNetwork() != 0) { sceKernelSleepThread(); }

	// Create a network thread to connect to the network.
	net_thid = sceKernelCreateThread("net_thread", net_thread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, NULL);
	if (net_thid > 0) sceKernelStartThread(net_thid, 0, NULL);

	while (!connected && !kill_threads) { sceKernelDelayThread(1000); }
	
	if (connected) {
		/* Start server_thread to serve our own webcam image. */
		server_thid = sceKernelCreateThread("server_thread", (SceKernelThreadEntry)server_thread, 0x18, 0x10000, THREAD_ATTR_USER, NULL);
		if (server_thid > 0) sceKernelStartThread(server_thid, 0, NULL);
	}

	printf("\nPlease connect the (GoCam!/ChottoShot) camera and press X.\n");

	while (1) {
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons & PSP_CTRL_CROSS) { break; }
		sceKernelDelayThread(50000);
	}

	printf("Starting skyPSP Webcam Server...\n");
	if (LoadModules() < 0) sceKernelSleepThread();
	if (InitJpegDecoder() < 0) sceKernelSleepThread();

	/*int fd = sceIoOpen("./mask.jpg", PSP_O_RDONLY, 0777);
	if (fd < 0) printf("The image mask could not be opened!\n"); else {
		bgSize = sceIoRead(fd, tampon, MAX_STILL_IMAGE_SIZE);
		int result = sceJpegDecodeMJpeg(tampon, bgSize, bgBuffer, 0); // Load mask only once.
		if (result < 0) { printf("The jpeg data could not be decoded!\n"); sceKernelExitDeleteThread(result); }
	}*/
	
	if (StartUsb() < 0) sceKernelSleepThread();
	if (sceUsbActivate(PSP_USBCAM_PID) < 0) { printf("The camera could not be activated!\n"); sceKernelSleepThread(); }

	while (1) { sceKernelDelayThread(50000); if ((sceUsbGetState() & 0xF) == PSP_USB_CONNECTION_ESTABLISHED) break;	}

	waitphoto = sceKernelCreateSema("WaitPhotoSema", 0, 0, 1, NULL);
	if (waitphoto < 0) { printf("Cannot create semaphore.\n"); sceKernelSleepThread(); }

	vid_thid = sceKernelCreateThread("video_thread", video_thread, 0x16, 256*1024, 0, NULL);
	if (vid_thid > 0) sceKernelStartThread(vid_thid, 0, NULL);

	makeDirectory("ms0:/PICTURE"); makeDirectory("ms0:/PICTURE/skyPSPcam"); 

	printf("skyPSP Webcam Server started!\n");
	sceKernelDelayThread(2000000);
	pspDebugScreenClear();
	while (1) {
		sceCtrlPeekBufferPositive(&pad, 1);
		//sceKernelDelayThread(1000000 * camdelay);
		//if (pad.Buttons != oldpad.Buttons) {
			//if (pad.Buttons & PSP_CTRL_CROSS) { if (showvideo) showvideo = 0; else showvideo = 1; }
		//}
		if (autorefresh) {
			//printf("Saving and uploading new image.");
			takephoto = 1; sceKernelWaitSema(waitphoto, 1, NULL);
			stream_cam(ftpserver, "skyPSPcam.jpg");
		} else {
			if (pad.Buttons != oldpad.Buttons) {
				if (pad.Buttons & PSP_CTRL_RTRIGGER) {
					//printf("Saving and uploading new image.");
					takephoto = 1; sceKernelWaitSema(waitphoto, 1, NULL);
					stream_cam(ftpserver, "skyPSPcam.jpg");
				}
			}
		}
		oldpad.Buttons = pad.Buttons;
	}
}
