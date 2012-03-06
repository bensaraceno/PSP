#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <math.h>
// Display headers
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <png.h>
#include <pspgraphics.h>
// Standard headers
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
// Net headers
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

// My headers
#include "cam.h"
#include "callbacks.h"
#include "config.h"
#include "back.h"
#include "pgeFont.h"
#include "verdana.h"

#define white		0xFFFFFFFF
#define black		0xFF000000
#define red 		0xFF0000FF
#define green		0xFF00FF00
#define blue		0xFFFF0000
#define yellow		0xFF00FFFF
#define selected	green

#define SERVER_PORT 23
#define HELLO_MSG   "Welcome to the skyPSP pspWebcam server!"

PSP_MODULE_INFO("skyPSP Webcam Client", 0x1000, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

/* Global variables. */
Image* imgWebcam; int imgFile; 
struct MemoryStruct { char *memory; size_t size; };
char err_buffer[256] = "", msg_buffer[256] = "", stat_buffer[50] = "", read_buffer[55] = "", username[40] = "", szMyIPAddr[32];
int err = 0, paused = 0;

/* Net variables */
int network = 0, networkcount = 0, timeout = 0, connectstate = 0, server = 0, curling = 0, urlchange = 0;

// Our webcam variables.
// FTP connection format: ftp://username:password@ftp.server.com/
unsigned char FTP_USERNAME[32] = { 0x73, 0x6B, 0x79, 0x70, 0x73, 0x70, 0x73, 0x6E, 0x61, 0x70, 0x73 };
unsigned char FTP_PASSWORD[32] = { 0x47, 0x75, 0x65, 0x73, 0x74, 0x4C, 0x6F, 0x67, 0x69, 0x6E, 0x35, 0x36, 0x37, 0x38 };
unsigned char FTP_HOSTADDR[256] = { 0x66, 0x74, 0x70, 0x2E, 0x63, 0x61, 0x6C, 0x69, 0x66, 0x72, 0x61, 0x67, 0x2E, 0x63, 0x6F, 0x6D };

/* Input variables. */
SceCtrlData newInput; unsigned int oldInput = 0;

/* Display variables. */
#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)

//unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int __attribute__((aligned(16))) list[262144];
int menu = 0, menux = 148, menuy = 160, msgdelay = 0, camonly = 0, waitframe = 0, option = 0;

typedef struct {
	unsigned short u, v;
	short x, y, z;
} Vertex;

/* Helper functions for File IO. */
/* Check for existance of file. */
int fileExist(const char* sFilePath) { 
	int fileCheck, fileExists;
    fileCheck = sceIoOpen(sFilePath, PSP_O_RDONLY, 0);
    if (fileCheck > 0) { fileExists = 1; } else { fileExists = 0; }
	sceIoClose(fileCheck); return fileExists;
}

/*int freespace() {
	int free_space;
	int buf[5];	
	int *buf_pointer = buf;	
	sceIoDevctl("ms0:", 0x02425818, &buf_pointer, sizeof(buf_pointer), 0, 0);
	free_space = buf[2] * buf[3] * buf[4];
	free_space = free_space/1024;//to kb
	free_space = free_space/1024;//to mb
	return free_space;
}*/

/* Get configuration information from file. 
char *getconfig(const char *filename, const char searchstr[]) {
	int cfgFile;
	char *w[100], string[100];//, line[100], c;
	int x[100], i = 0;//, z[100], v;
	cfgFile = sceIoOpen(filename, PSP_O_RDONLY, 0777);
	if (!cfgFile) { return NULL; } else {
		while (sceIoRead(cfgFile, string, 100)) {
			//x[i] = strlen(string) - 1;
			if (string[0] == '#') continue;
			w[i] = strchr(string, '=') - 1;
			*w[i] = 0;
			if (!strcmp(searchstr, string)) {
				char *crlf = strpbrk(w[i] + 3, "\r\n");
				if (crlf) *crlf = 0;
				return w[i] + 3;
			}
			//i++;
		}
		sceIoClose(cfgFile);
		return NULL;
	}
	return "Test";
}*/

/* Helper functions for input_thread. */
/* Check if button is being pressed. */
int checkInput(int PSP_CTRL_BUTTON) { return (newInput.Buttons & PSP_CTRL_BUTTON); }

/* Helper functions for networking. */
/* Connect to an access point. */
int connectToNetwork(int index, const char *name) {
	int stateLast = -1;
	err = sceNetApctlConnect(index);
	if (err) { sprintf(err_buffer, "sceNetApctlConnect Error: 0x%08X", err); return 1; } else {
		while (!err) {
			int state;
			err = sceNetApctlGetState(&state); if (err) { sprintf(err_buffer, "sceNetApctlGetState Error: 0x%08X", err); break; }
			if (state > stateLast) { stateLast = state; connectstate = state; timeout = 0; sprintf(msg_buffer, "Connection State: %d of 4", state); msgdelay = 100; }
			if (state == 4) break; // Connected with a static ip address.
			sceKernelDelayThread(50000); // Wait before polling again.
		}
	}
	if (!err) { connected = 1; sprintf(msg_buffer, "Connected to network: %s!", name); msgdelay = 100; } else return 1; 
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

/*#define RESOLVE_NAME "google.com"
void net_resolver(void) {
	int rid = -1;
	char buf[1024];
	struct in_addr addr;
	char name[1024];
	while(!kill_threads);
		// Create a resolver 
		if(sceNetResolverCreate(&rid, buf, sizeof(buf)) < 0) { printf("Error creating resolver\n"); break; }
		printf("Created resolver %08x\n", rid);
		// Resolve a name to an ip address
		if(sceNetResolverStartNtoA(rid, RESOLVE_NAME, &addr, 2, 3) < 0) { printf("Error resolving %s\n", RESOLVE_NAME); break; }
		printf("Resolved %s to %s\n", RESOLVE_NAME, inet_ntoa(addr));
		// Resolve the ip address to a name
		if(sceNetResolverStartAtoN(rid, &addr, name, sizeof(name), 2, 3) < 0) { printf("Error resolving ip to name\n"); break; }
		printf("Resolved ip to %s\n", name);
	}
	if (rid >= 0) sceNetResolverDelete(rid);
}*/

/* Workaround for any realloc() that may not like NULL pointers */
void *myrealloc(void *ptr, size_t size) { if (ptr) return realloc(ptr, size); else return malloc(size); }

/* Load data to memory. */
size_t curlWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;
	//bytes += (size * nmemb);
	mem->memory = (char *)myrealloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

/* Write data to file. */
size_t curlWriteDataCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
	//bytes += (size * nmemb);
	int written = sceIoWrite(imgFile, ptr, nmemb);
	return written;
}

/* Read size of file we are downloading. */
size_t curlReadDataCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	return fread(ptr, size, nmemb, stream);
}

/* Progress meter for curl operation (t = total, d = downloaded). */
int curlProgressMeter(void *ptr, double t, double d, double ultotal, double ulnow) {
	if (camonly) sprintf(stat_buffer, "%d (%d) %d%%", urlindex + 1, f, (int)d * 100 / (int)t); else sprintf(stat_buffer, "Cam %d: Frame (%d) %d/%d (%d%%)", urlindex + 1, f, (int)d, (int)t, (int)d * 100 / (int)t);
	return 0;
}

int uploadFile(const char *filename, const char *ftpserver, const char *destname) {
	int srcFile; long filesize = 0;
	CURL *curl; CURLcode res; FILE* fSrc;
	struct curl_slist *headerlist = NULL;
	char ftpCommand[5][256], REMOTE_URL[256];

	sprintf(REMOTE_URL, "%s/skyPSP.upload", ftpserver); // File destination: remoteftp/skyPSP.upload.
	sprintf(ftpCommand[0], "RNTO %s", destname); // FTP command to rename from temp.uplod to destname.

	// File size of the local file.
	srcFile = sceIoOpen(filename, PSP_O_RDONLY, 0777); filesize = sceIoLseek(srcFile, 0, SEEK_END); sceIoClose(srcFile);
	fSrc = fopen(filename, "rb"); curl = curl_easy_init();
	if(curl) {
		headerlist = curl_slist_append(headerlist, "RNFR skyPSP.upload");
		headerlist = curl_slist_append(headerlist, ftpCommand[0]);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1); // Enable uploading
		curl_easy_setopt(curl, CURLOPT_URL, REMOTE_URL); // Destination file
		curl_easy_setopt(curl, CURLOPT_POSTQUOTE, headerlist); // FTP commands to run after the transfer
		curl_easy_setopt(curl, CURLOPT_READDATA, fSrc); // File to upload
		curl_easy_setopt(curl, CURLOPT_INFILESIZE, filesize);
		res = curl_easy_perform(curl);
		curl_slist_free_all(headerlist);
		curl_easy_cleanup(curl); // Cleanup any garbage.
	}
	fclose(fSrc); // Close the local file
	return 0;
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

/* Refresh the webcam image. */
void webcam_thread(SceSize args, void *argp) {
	CURL *curl; CURLcode res; struct MemoryStruct chunk;
	char fileext[10], ftpserver[256];
	sprintf(ftpserver, "ftp://%s:%s@%s", FTP_USERNAME, FTP_PASSWORD, FTP_HOSTADDR);
	while (!kill_threads && connected) {
		while (paused) { sceKernelDelayThread(1000); }
		curl = curl_easy_init(); //bytes = 0;
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, CAM_URL[urlindex]); urlchange = 0; // Set the url.
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk); // Pass our 'chunk' struct to the callback function
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback); // Function to handle data stream.
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, curlReadDataCallback);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0); // Enable progress meter.
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curlProgressMeter); // Progress meter.
			//curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, Bar);
			curl_easy_setopt(curl, CURLOPT_MUTE, 1); // Silent operation.
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0"); // Some servers require a user agent.
			sprintf(fileext, "%s", strrchr(CAM_URL[urlindex], '.'));
			if (stricmp(fileext, ".jpg") && stricmp(fileext, ".jpeg")) sprintf(err_buffer, "Webcam image is not JPEG!");
			while (!urlchange && !paused) {
				if (!urlchange) { curling = 1; res = 0; chunk.size = 0; chunk.memory = NULL; res = curl_easy_perform(curl); } // Download to memory.
				if (imgWebcam && (snapshot || saveimages)) { // Save last webcam image before loading the new one.
					char filename[256], snapfilename[256], snapdestname[256];
					if (snapshot) {
						sceIoMkdir("ms0:/PICTURE", 0777); sceIoMkdir("ms0:/PICTURE/snapshots", 0777);
						do { sprintf(snapfilename, "ms0:/PICTURE/snapshots/snapshot%d%s", snapnum, fileext); snapnum++; } while (fileExist(snapfilename));
						sprintf(msg_buffer, "Saving Snapshot #%02d", snapnum); msgdelay = 100;
					} else {
						sceIoMkdir("ms0:/PICTURE", 0777); sceIoMkdir("ms0:/PICTURE/skyPSP", 0777); 
						do { sprintf(filename, "ms0:/PICTURE/skyPSP/image%d%s", filenum, fileext); filenum++; } while (fileExist(filename));
					}
					saveImage(snapshot ? snapfilename : filename, imgWebcam->data, imgWebcam->imageWidth, imgWebcam->imageHeight, PSP_LINE_SIZE, 0);
					//saveImage(filename, getVramDisplayBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT, PSP_LINE_SIZE, 0); // Screenshot
					sprintf(snapdestname, "%s%d.jpg", username, snapnum); uploadFile(snapfilename, ftpserver, snapdestname); // Upload snapshot to FTP.
					sceKernelDelayThread(100000); if (snapshot) snapshot = 0;
				}
				// If download fails or changing webcams, free memory chunk, otherwise if a valid jpeg or png file, load it.
				if (CURLE_OK != res || urlchange) { sceKernelDelayThread(100000); free(chunk.memory); } else {
					char *ct; res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
					if ((CURLE_OK == res) && ct) {
						if (!strcmp(ct, "image/jpeg") || !strcmp(ct, "image/png")) { // If the content-type matches valid jpeg or png, load the new image.
							if (imgWebcam) freeImage(imgWebcam);
							imgWebcam = loadImageFromMemory((unsigned char*)chunk.memory, chunk.size); free(chunk.memory);
							if (imgWebcam && grayscale) GreyScale(imgWebcam);
							//Negative(imgWebcam);
							//if (imgWebcam) imageFlipH(imgWebcam);
						}
					}
				}
				if (f >= waitframe) waitframe = 0;
				f++; curling = 0; if (!urlchange) sceKernelDelayThread(1000000 * camdelay); // Delay 1s * camdelay to prevent unnecessary refresh.
				if (cyclecam) { urlchange = 1; urlindex += cyclecam; if (urlindex < 0) urlindex = camcount - 1; else if (urlindex > camcount - 1) urlindex = 0; }
			}
			curl_easy_cleanup(curl); // Cleanup any garbage.
		}
	}
}

void display_thread(SceSize args, void *argp) {
	int /*i, tmpColor, */blinktimer;
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
	sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	// Load images
	Image* imgBackground = loadImageMemory(back, sizeof(back));
	sceKernelDelayThread(10000);

	// Load fonts
	pgeFontInit();
	pgeFont *verdana12 = pgeFontLoadMemory(verdana, size_verdana, 12, PGE_FONT_SIZE_POINTS, 128); // Load verdana font at 12 point (128x128 texture)

	while (!kill_threads) {
		sceKernelDcacheWritebackInvalidateAll();
		sceGuStart(GU_DIRECT, list);
		sceGumMatrixMode(GU_PROJECTION);
		sceGumLoadIdentity();
		sceGumPerspective(75.0f, 16.0f / 9.0f, 0.5f, 1000.0f);
		sceGumMatrixMode(GU_VIEW);
		sceGumLoadIdentity();
		sceGumMatrixMode(GU_MODEL);
		sceGumLoadIdentity();
		sceGuClearColor(0);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

		// Draw background graphics here.
		if (!camonly) blitAlphaGUImageToScreen(0, 0, 480, 272, imgBackground, 0, 0);
		if (imgWebcam && !waitframe) {
			if (!paused) { blinktimer++; if (blinktimer == 1000) { blinktimer = 0; if (autosnapshot) snapshot = 1; } }
			blitAlphaGUImageToScreen(0, 0, imgWebcam->imageWidth, imgWebcam->imageHeight, imgWebcam, (480 - imgWebcam->imageWidth) / 2, (272 - imgWebcam->imageHeight) / 2);
			//drawRectScreen(black, imgWebcam->imageWidth < 480 ? (480 - imgWebcam->imageWidth) / 2 : 0, imgWebcam->imageHeight < 272 ? (272 - imgWebcam->imageHeight) / 2 : 0, imgWebcam->imageWidth < 480 ? imgWebcam->imageWidth : 480, imgWebcam->imageHeight < 272 ? imgWebcam->imageHeight : 272, imgWebcam);
		}

		//Draw any text (on top of graphics).
		pgeFontActivate(verdana12);
		pgeFontPrintCenter(verdana12, 245, red, err_buffer); // Print any errors
		if (strcmp(read_buffer, " ")) pgeFontPrintCenter(verdana12, 245, red, read_buffer); // Received messages
		if (!connected) {
			pgeFontPrintfCenter(verdana12, 122, white, "Hello %s!", username);
			pgeFontPrintCenter(verdana12, 146, yellow, msg_buffer);
			/*for (i = 0; i < connectstate; i++) {
				switch (i) {
					case 0: tmpColor = 0xFF000099; break;
					case 1: tmpColor = 0xFF00BBBB; break;
					case 2: tmpColor = 0xFF00FF00; break;
					default: tmpColor = white; break;
				}
				fillScreenRect(tmpColor, 182 + 50 * i, 152, 10, 10);
				drawRectScreen(black, 182 + 50 * i, 152, 10, 10);
			}*/
		} else {
			if (msgdelay) msgdelay--;
			if (f < 10) pgeFontPrintfCenter(verdana12, 12, white, "Telnet: %s:%d", szMyIPAddr, SERVER_PORT); // Server IP
			if (camonly) pgeFontPrint(verdana12, 5, 268, yellow, stat_buffer); else pgeFontPrintCenter(verdana12, 268, yellow, stat_buffer); // Print progress
			if (waitframe || msgdelay) {
				pgeFontPrintCenter(verdana12, (!imgWebcam || waitframe || msgdelay) ? 132 : 260, (waitframe || snapshot) ? yellow : white, msg_buffer);
				//if (f > 1) { sprintf(hdr_buffer, "Frame: %d", f - 1); printTextScreen(5, 5, hdr_buffer, white); }
			}
		}

		// Draw foreground graphics (on top of text).

		
		sceGuFinish(); // Finish drawing
		sceGuSync(0,0); // Sync buffers
		sceGuSwapBuffers(); // Swap buffers
		sceDisplayWaitVblankStart(); // Wait for new frame.
	}
	pgeFontUnload(verdana12); // Unload font
	// pgeFontShutdown(); Shutdown font library (Does nothing)
	sceKernelExitDeleteThread(0);
}

void network_thread(SceSize args, void *argp) {
	while (!kill_threads) {
		/* Get network index to connect to. */
		netData networkData; char networkName[256]; network = 1;
		//err = sceUtilityCheckNetParam(1); // Check for at least one network.
		//if (err) { sprintf(msg_buffer, "No networks to connect to!"); } else {
			//err = sceUtilityCheckNetParam(2); // Check for more networks, default to the first if none.
			//if (err) { network = 1; } else {
				/* More than one network, get listing. */
				//while (!network) {
					//sceUtilityGetNetParam(option, PSP_NETPARAM_NAME, &networkData); strcpy(networkName, networkData.asString);
					//sprintf(msg_buffer, "Please select a network to connect to: %s", networkName);
					//sceKernelDelayThread(1000); }
			//}
			err = pspSdkInetInit(); if (err) { sprintf(err_buffer, "Failed initializing the network %08X", err); }
			sceUtilityGetNetParam(network, PSP_NETPARAM_NAME, &networkData); strcpy(networkName, networkData.asString);
			err = connectToNetwork(network, networkName);
			if (err) { sprintf(msg_buffer, "Could Not Connect to Network!"); } else {
				// Connected! Get an ip address and start.
				if (sceNetApctlGetInfo(8, szMyIPAddr)) strcpy(szMyIPAddr, "IP Address Unknown!");
				// Wait for our webcam server to start up.
				while (!server && !kill_threads) { sceKernelDelayThread(1000); }
				//uploadCapture("ms0:/capture.jpg", "pspWebcam.jpg"); // Upload our Cam image to FTP.
				while (connected && !kill_threads) { sceKernelDelayThread(1000); }
				while (curling) { sceKernelDelayThread(1000); }
				sceNetApctlDisconnect(); connected = 0;
			}
		//}
	}
	sceKernelExitDeleteThread(0);
}

/* Handle the configuration */
void user_thread(SceSize args, void *argp) {
	/* Get PSP nickname. */
	sceUtilityGetSystemParamString(PSP_SYSTEMPARAM_ID_STRING_NICKNAME, username, 50);
	//if (strcmp(username, "")) sprintf(username, "user_unknown%d", randInt);

	while (!kill_threads) { sceKernelDelayThread(10000); }
	sceKernelExitDeleteThread(0);
}

/* Handle input conditions. */
void input_thread(SceSize args, void *argp) {
	int changecam = 0;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	sceCtrlReadBufferPositive(&newInput, 1);
	if (checkInput(PSP_CTRL_LTRIGGER)) saveimages = 1; // Save webcam images.
	while (!network && !kill_threads) {
		sceCtrlReadBufferPositive(&newInput, 1);
		if (newInput.Buttons != oldInput) {
			if (checkInput(PSP_CTRL_UP)) option--; else if (checkInput(PSP_CTRL_DOWN)) option++;
			if (option < 1) option = networkcount; else if (option > networkcount) option = 1;
		}
		oldInput = newInput.Buttons;
	}
    while (!kill_threads) {
		sceCtrlReadBufferPositive(&newInput, 1);
		/*while (menu) {
			sceCtrlReadBufferPositive(&newInput, 1);
			if (newInput.Buttons != oldInput) {  }
			oldInput = newInput.Buttons;
		}*/
		if (newInput.Buttons != oldInput) {
			//if (checkInput(PSP_CTRL_START)) { if (paused) paused = 0; else paused = 1; }
			if (checkInput(PSP_CTRL_RTRIGGER)) snapshot = 1; // Take a snapshot
			if (checkInput(PSP_CTRL_CROSS)) { if (camonly) { camonly = 0; } else { camonly = 1; sprintf(msg_buffer, "Theatre Mode"); msgdelay = 100; } } // Show only the cam image (Theatre mode).
			if (checkInput(PSP_CTRL_CIRCLE)) { if (grayscale) { grayscale = 0; } else { grayscale = 1; GreyScale(imgWebcam); } } // Switch to grayscale mode.
			if (checkInput(PSP_CTRL_LTRIGGER)) { if (camdelay == 2.0) camdelay = 1.0; else camdelay = 2.0; } // Change webcam delay.
			if (checkInput(PSP_CTRL_UP)) cyclecam = 1; else if (checkInput(PSP_CTRL_DOWN)) cyclecam = -1; // Cycle through webcams.
			if (checkInput(PSP_CTRL_TRIANGLE)) cyclecam = 0;
			if (checkInput(PSP_CTRL_LEFT)) changecam = -1; else if (checkInput(PSP_CTRL_RIGHT)) changecam = 1; // Change webcam feed.
			if (changecam && !waitframe) {
				lastindex = urlindex; 
				urlindex += changecam; waitframe = f + curling + 1; changecam = 0; cyclecam = 0; urlchange = 1;
				if (urlindex < 0) urlindex = camcount - 1; else if (urlindex > camcount - 1) urlindex = 0;
				sprintf(msg_buffer, "Changing to Webcam #%02d", urlindex + 1);
			}
		}
		oldInput = newInput.Buttons;
	}
	sceKernelExitDeleteThread(0);
}

int main(int argc, char **argv) {
	SceUID input_thid, display_thid, user_thid, network_thid, webcam_thid, server_thid;
	char cfg_buffer[256]; 
	int *cfgvartype; int *cfgvarsize;
	SetupCallbacks();

	/* Start input_thread to detect input conditions. */
	input_thid = sceKernelCreateThread("input_thread", (SceKernelThreadEntry)input_thread, 0x18, 0x1000, 0, NULL);
	if (input_thid > 0) sceKernelStartThread(input_thid, 0, NULL);
	sceKernelDelayThread(10000);

	/* Load options from config file. */
	configLoad("skyPSP.cfg");
	err = configRead("FTP", "HOSTADDRESS", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) strcpy((char*)FTP_HOSTADDR, cfg_buffer);
	err = configRead("FTP", "USERNAME", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) strcpy((char*)FTP_USERNAME, cfg_buffer);
	err = configRead("FTP", "PASSWORD", cfg_buffer, cfgvartype, cfgvarsize);
	if (!err && strcmp(cfg_buffer, "")) strcpy((char*)FTP_PASSWORD, cfg_buffer);
	//err = configRead("CAM", "STARTUPCAM", cfg_buffer, cfgvartype, cfgvarsize);
	//if (!err && strcmp(cfg_buffer, "")) urlindex = atoi(cfg_buffer) + 1;
	//err = configRead("CAM", "SAVEIMAGES", cfg_buffer, cfgvartype, cfgvarsize);
	//if (!err && strcmp(cfg_buffer, "")) saveimages = atoi(cfg_buffer);
	//err = configRead("CAM", "AUTOSNAPSHOT", cfg_buffer, cfgvartype, cfgvarsize);
	//if (!err && strcmp(cfg_buffer, "")) autosnapshot = atoi(cfg_buffer);
	//err = configRead("CAM", "CYCLECAM", cfg_buffer, cfgvartype, cfgvarsize);
	//if (!err && strcmp(cfg_buffer, "")) cyclecam = atoi(cfg_buffer);
	configClose();

	/* Start user_thread to do handle configuration. */
	user_thid = sceKernelCreateThread("user_thread", (SceKernelThreadEntry)user_thread, 0x18, 0x10000, THREAD_ATTR_USER, NULL);
	if (user_thid > 0) sceKernelStartThread(user_thid, 0, NULL);
	sceKernelDelayThread(10000);

	/* Start display_thread to output text/objects to screen. */
	display_thid = sceKernelCreateThread("display_thread", (SceKernelThreadEntry)display_thread, 0x16, 0x08000, THREAD_ATTR_USER, NULL);
	if (display_thid > 0) sceKernelStartThread(display_thid, 0, NULL);
	sceKernelDelayThread(10000);

	err = pspSdkLoadInetModules(); if (err) { sprintf(err_buffer, "Failed loading inet modules."); sceKernelSleepThread(); }

	while (!kill_threads) {
		/* Start network_thread to handle network connection. */
		network_thid = sceKernelCreateThread("network_thread", (SceKernelThreadEntry)network_thread, 0x18, 0x10000, THREAD_ATTR_USER, NULL);
		if (network_thid > 0) sceKernelStartThread(network_thid, 0, NULL);
		sceKernelDelayThread(100000);
		while (!connected) {
			if (connectstate) {
				timeout = 1; sceKernelDelayThread(10*1000000); // 10s timeout between connection states.
				if (timeout) {
					if (connectstate) sprintf(msg_buffer, "Connection timed out!");
					sceKernelDelayThread(2000000); kill_threads = 1;
				}
			}
		}

		if (connected) {
			/* Start webcam_thread to refresh webcam image. */
			webcam_thid = sceKernelCreateThread("webcam_thread", (SceKernelThreadEntry)webcam_thread, 0x16, 0x10000, THREAD_ATTR_USER, NULL);
			if (webcam_thid > 0) sceKernelStartThread(webcam_thid, 0, NULL);
			sceKernelDelayThread(100000);

			/* Start server_thread to serve our own webcam image. */
			server_thid = sceKernelCreateThread("server_thread", (SceKernelThreadEntry)server_thread, 0x18, 0x10000, THREAD_ATTR_USER, NULL);
			if (server_thid > 0) sceKernelStartThread(server_thid, 0, NULL);
			sceKernelDelayThread(10000);
		}
		while (connected) { sceKernelDelayThread(10000); }
	}

	if (kill_threads) sceKernelExitGame();
	sceKernelExitDeleteThread(0);
	return 0;
}
