/* Variables for GPS_thread. */
nmeap_gga_t g_gga;

double ggalatitude;
double ggalongitude;
double altitude;
unsigned long time;
int satellites;
int quality;
double hdop;
double geoid;
char warn;
double latitude;
double longitude;
double speed;
double course;
unsigned long date;
double magvar;

static nmeap_context_t nmea; // Parser context.
static nmeap_gga_t gga;	// Data from GGA messages.
static nmeap_rmc_t rmc;	// Data from RMC messages.
static int user_data; // Pointer to user data.
