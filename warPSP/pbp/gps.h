#include "nmeap.h"

/* GPS Variables. */
nmeap_gga_t g_gga;
char gps_buffer[512];
double lat, lon;

struct gpsData {
	double        ggalatitude;
	double        ggalongitude;
	double        altitude;
	unsigned long utctime;
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
};

struct gpsData gpsCurrent;
