#include <psputility_netmodules.h>
#include <psputility_sysparam.h>
#include <pspwlan.h>
#include <pspnet.h>

int sceNet_lib_5216CBF5(const char *name); /* Init the scan */
int sceNet_lib_7BA3ED91(const char *name, void *type, u32 *size, void *buf, u32 *unk); /* Scan APs */
int sceNet_lib_D2422E4D(const char *name); /* Terminate the scan */

#define InitScan sceNet_lib_5216CBF5
#define ScanAPs  sceNet_lib_7BA3ED91
#define TermScan sceNet_lib_D2422E4D

/* Global buffer to store the scan data */
unsigned char scan_data[0xA80];

typedef struct {
	char ssid[32];
	char bssid[32];
	int ssidlength;
	short rssi;
	int channel;
	int bsstype;
	int beaconperiod;
	int dtimperiod;
	int timestamp;
	int localtime;
	short atim;
	short capabilities;
	char rates[8];
	double latitude;
	double longitude;
} accessPoint;

/* Returned scan data structure. */
struct ScanData {
	struct ScanHead *pNext;
	char bssid[6];
	char channel;
	unsigned char ssidlength;
	char ssid[32];
	unsigned int bsstype;
	unsigned int beaconperiod;
	unsigned int dtimperiod;
	unsigned int timestamp;
	unsigned int localtime;
	unsigned short atim;
	unsigned short capabilities;
	char rate[8];
	unsigned short rssi;
	unsigned char  sizepad[6];
} __attribute__((packed));

/* Capability flags */
const char *caps[8] = {
	"ESS",
	"IBSS",
	"CF Pollable",
	"CF Pollreq",
	"Privacy (WEP)",
	"Short Preamble",
	"PBCC",
	"Channel Agility"
};

/* Scan variables for networks. */
int summary_color[50]; accessPoint ap_list[50];
int apcount, opencount, unique = 0, ap_id, scan_count, status_color;
char psp_info[5][50], ap_summary[50][50];
