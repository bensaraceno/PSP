/* Webcam variables. */
int urlindex = 0, cyclecam = 0, lastindex = 0, grayscale = 0, scaleimage = 1; double camdelay = 1.5;// Webcam options
int saveimages = 0, f = 1, filenum = 1, snapshot = 0, snapnum = 1, autosnapshot = 0; // File variables

/*typedef struct {
	char *url;
	char *info;
	int camdelay;
} camInfo;
camInfo webcam[] = {};*/

/* Webcam URLs */
int camcount = 43;
const char *CAM_URL[] = {
	// NORTH AMERICA
	"http://63.229.55.11/ht/htzone1.jpg",							// NA, US - New York, New York: Hawaiian Tropic bar and catwalk (640x480) (Live)
	//"http://images.earthcam.com/ec_metros/ourcams/htzone1.jpg",						// NA, US - New York, New York: Hawaiian Tropic bar and catwalk (640x480) (Live)
	"http://63.229.55.11/ht/htzone2.jpg",							// NA, US - New York, New York: Hawaiian Tropic restaurant (640x480) (Live)
	//"http://images.earthcam.com/ec_metros/ourcams/htzone2.jpg",						// NA, US - New York, New York: Hawaiian Tropic restaurant (640x480) (Live)
	"http://63.229.55.11/ht/htzone3.jpg",							// NA, US - New York, New York: Hawaiian Tropic front desk (640x480) (Live)
	//"http://images.earthcam.com/ec_metros/ourcams/htzone3.jpg",						// NA, US - New York, New York: Hawaiian Tropic front desk (640x480) (Live)
	"http://63.229.55.11/ht/htzone4.jpg",							// NA, US - New York, New York: Hawaiian Tropic front entrance (640x480) (Live)
	//"http://images.earthcam.com/ec_metros/ourcams/htzone4.jpg",						// NA, US - New York, New York: Hawaiian Tropic front entrance (640x480) (Live)
	//"http://www.fallsview.com/Stream/camera0.jpg", 								// NA, US - Niagara, New York: View of Niagara Falls (1024x768) (?)
	//"http://www.laavenue.com/la/la.jpg",										// NA, US - Los Angeles, California: View of Los Angeles Avenue (320x240) (20s-40s) // Randomly crashes the program.
	"http://www.electricearl.com/LAcam-2007e.jpg", 					// NA, US - Los Angeles, California: View of Los Angeles skyline (448x336) (1h)
	"http://abclocal.go.com/three/kabc/webcam/web2-1.jpg",			// NA, US - Los Angeles, California: View of downtown LA (426x240) (?)
	"http://abclocal.go.com/three/kabc/webcam/web1-2.jpg",			// NA, US - Los Angeles, California: View of LAX airport (426x240) (?)
	"http://abclocal.go.com/three/kabc/webcam/web1-1.jpg",			// NA, US - Long Beach, California: View of the Queen Mary (426x240) (?)
	"http://abclocal.go.com/three/kabc/webcam/web2-2.jpg",			// NA, US - Burbank, California: View of Burbank (426x240) (?)
	//"http://watchthewater.co.la.ca.us/images/011_MDR_46.jpg",						// NA, US - Marina del Rey, California: View of the marina (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_45.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach East (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_46.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach Southeast 640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_47.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach South (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_48.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach Southwest (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_49.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach Southwest (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_50.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach pier West (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_51.jpg",						// NA, US - Hermosa Beach, California: View of Hermosa Beach pier Northwest (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_52.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach Northwest (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_41.jpg",						// NA, US - Hermosa Beach, California: View of Hermosa Beach North (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_42.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach Northeast (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_43.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach (640x480) (?)
	//"http://watchthewater.co.la.ca.us/images/002_Hermosa_44.jpg",					// NA, US - Hermosa Beach, California: View of Hermosa Beach East (640x480) (?)
	//"http://64.241.25.110/yell/webcams/oldfaith2.jpg", 								// NA, US - Yellowstone, Wyoming: View of Old Faithful (800x608) (30s)
	//"http://www.yosemite.org/vryos/ahwahnee.jpg", 								// NA, US - Yellowstone, Wyoming: View from Ahwahnee Meadow (800x608) (30s)
	//"http://www.yosemite.org/vryos/turtleback1.jpg", 								// NA, US - Yellowstone, Wyoming: View from Turtleback Dome (640x480) (30s)
	//"http://www.yosemite.org/vryos/sentinel.jpg", 								// NA, US - Yellowstone, Wyoming: View from below Sentinel Dome (640x480) (30s)
	//"http://meteora.ucsd.edu/cap/tioga/tioga_current.jpg", 							// NA, US - Yellowstone, Wyoming: View from Tioga Pass (800x560) (30s)
	//"http://maestro.haarp.alaska.edu/data/haarpcam/images/latest.jpg" 					//NA, US - Gakona, Alaska: HAARP Research Center (640x480) (5m)
	//"http://science.ksc.nasa.gov/shuttle/countdown/video/chan15large.jpg", 				// NASA US 704x480
	"http://reno.vs.oiccam.com/ftp/capcom/lvip/image.jpg", 			// Las Vegas NV 400x273
	"http://gsnus.vs.oiccam.com/ftp/capcom/mcgeecars/image.jpg", 	// Provincetown MA 320x240
	"http://reno.vs.oiccam.com/ftp/capcom/pducla/image.jpg", 		// CA State Univerisity 400x273
	"http://gsnus.vs.oiccam.com/ftp/capcom/sfggatekpix/image.jpg", 	// Golden Gate Bridge San Francisco CA 352x240
	"http://gsnus.vs.oiccam.com/ftp/capcom/sfalcatraz/image.jpg", 	// Alcatraz Island San Francisco CA 400x300
	"http://gsnus.vs.oiccam.com/ftp/capcom/waileaelua/image.jpg", 	// Waileaelua Village Maui Hawaii US 400x300
	"http://gsnus.vs.oiccam.com/ftp/capcom/sergio/image.jpg", 		// Sergio's Coffeeshop ?? US 400x300
	//"http://depot.vs.oiccam.com/ftp/capcom/depotphoto/image.jpg", 					// Depot Railcam ?? US 640x480
	
	// AFRICA
	
	//"http://www.pyramidcam.com/netcam.jpg", 									// AF, EG - Giza, Cairo: View of The Great Pyramids (800x608) (?)
	//"http://tommy-friedl.de/webcam.jpg",										// AF, EG - Hurghada, Cairo: View of Hurghada Beach (640x240) (15m)
	"http://www.colona.com/hurghada/cam/cdc.jpg",					// AF, EG - Hurghada, Cairo: View of Colona Hurghada Jetty (500x260) (15s-20s)
	//"http://ncd.aucegypt.edu/Last_Snapshot.jpg",									// AF, EG - Cairo, Cairo: View from The American University (1024x405) (1h)
	"http://www.wernerlau.com/ftpcam/egypt.jpg",					// AF, EG - Sharm El Sheikh, Sinai: View of Helnan Marina (352x288) (20m)
	//"http://www.socotra.info/webcam/webcam.jpg",								// AF, EG - Dahab Masbat, South Sinai: View from Lighthouse Cape to Masbat Bay (640x480) (30s)
	//"http://www.dahab-info.com/camoutput/camera.jpg",							// AF, EG - Dahab Masbat, South Sinai: View of Dahab Masbat (640x480) (15s)
	//"http://www.the-islander.org.ac/webcam/photos/pierhead.jpg",						// AF, UK - Ascension Island, Saint Helena: Georgetown Pierhead (640x480) (5m)
	//"http://www.caboverde24.cv/mindelocam/mindelo.jpg",							// AF, CV - Mindelo Harbour, São Vicente: View of the Harbor (640x480) (60s)
	//"http://www.caboverde24.cv/mindelocamzoom/mindelo.jpg",						// AF, CV - Mindelo Harbour, São Vicente: View of the Harbor zoomed (640x480) (60s)
	//"http://www.caboverde24.cv/webcam/caboverde.jpg",							// AF, CV - Santa Maria Beach, Sal Island: View of Santa Maria Beach Pier (640x480) (60s)
	//"http://www.caboverde24.cv/webcam/zoom/caboverde.jpg",						// AF, CV - Santa Maria Beach, Sal Island: View of Santa Maria Beach Pier zoomed (640x480) (60s)
	//"http://www.caboverde24.cv/surfcabocam/cape_verde_webcam.jpg",					// AF, CV - Santa Maria Beach, Sal Island: View of Santa Maria Beach (640x480) (60s)
	//"http://www.caboverde24.cv/surfcabocam/cape_verde_webcam_zoom.jpg",				// AF, CV - Santa Maria Beach, Sal Island: View of Santa Maria Beach zoomed (640x480) (60s)
	"http://www.info-mauritius.com/weather-wetter/weather.jpg",		// AF, MU - Beau-Bassin, Mauritius Island: View of the street (320x240) (15m-30m)
	"http://www.info-mauritius.com/weather-wetter/weather-flic-en-flac.jpg", // AF, MU - Flic en Flac, Mauritius Island: View from balcony (320x240) (15m-30m)
	//"http://www.marrakech-info.com/M1/img.jpg",								// AF, MA - Essaouira Maroc, Medina: View of Essaouira Maroc Beach (1280x480) (2m)
	"http://www.virtualseychelles.sc/webcam/beach/video.jpg",		// AF, SC - Beau-Vallon Beach, Mahé: View of Beau-Vallon Beach (320x240) (15s)
	"http://www.virtualseychelles.sc/webcam/market/video.jpg",		// AF, SC - Victoria Market, Seychelles: View of the Victoria Market (320x240) (15s)
	//"http://www.kilicam.com/cam_image/tuskerpic.jpg",								// AF, TZ - Mount Kilimanjaro, Moshi: View from Mount Kilimanjaro (640x480) (5m)
	//"http://www.midafricam.co.za/webcams/oct/capture.jpg",							// AF, ZA - Cape Town, Western Cape: Outeniqua Choo Tjoe Railcam (640x480) (5m)
	//"http://www.midafricam.co.za/webcams/george/capture.jpg",						// AF. ZA - Cape Town, Western Cape: View of George CBD (640x480) (5m)
	//"http://www.midafricam.co.za/webcams/ssbeach/ri/capture.jpg",					// AF, ZA - Cape Town, Western Cape: View of Robben Island (650x490) (10m)
	//"http://www.midafricam.co.za/webcams/ssbeach/tm/capture.jpg",					// AF, ZA - Cape Town, Western Cape: View of Tablemountain (650x490) (10m)
	//"http://www.midafricam.co.za/webcams/kaaimans/capture.jpg",						// AF, ZA - Cape Town, Western Cape: View of Kaaimans Pass (746x490) (5m)
	//"http://www.midafricam.co.za/webcams/knysna/zoom/capture.jpg",					// AF, ZA - Cape Town, Western Cape: View of Knysna Heads (650x490) (?)
	//"http://www.midafricam.co.za/webcams/vicbay/zoom/current.jpg",					// AF, ZA - Cape Town, Western Cape: View of Victoria Bay zoomed (650x490)
	//"http://www.midafricam.co.za/webcams/vicbay/wide/current.jpg",					// AF, ZA - Cape Town, Western Cape: View of Victoria Bay wide (650x490)
	//"http://www.midafricam.co.za/webcams/wilderness/capture.jpg",					// AF, ZA - Cape Town, Western Cape: View of Wilderness Beach (650x490) (10m)
	//"http://www.kapstadt.de/webcam.jpg", 										// AF, ZA - Cape Town, Western Cape: View of Tablemountain (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.125.5.11",	// AF, ZA - Cape Town, Western Cape: View of Century City (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.14",	// AF, ZA - Cape Town, Western Cape: View of Jeffery's Bay (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.15",	// AF, ZA - Cape Town, Western Cape: View of Strand and Gordons Bay (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.16,	// AF, ZA - Cape Town, Western Cape: View of Hermanus Beach (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.18",	// AF, ZA - Cape Town, Western Cape: View of Bloubergstrand (Beach Boulevard) (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.19",	// AF, ZA - Cape Town, Western Cape: View of Muizenberg Beach (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.20",	// AF, ZA - Cape Town, Western Cape: View of Tablemountain lower cable station (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.21",	// AF, ZA - Cape Town, Western Cape: View of Tablemountain upper cable station (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.23",	// AF, ZA - Cape Town, Western Cape: View of Port Elizabeth (704x576) (?)
	//"http://www.vodacom4me.co.za/vodacom4me-personal-resources/infocam/Cam@10.113.41.24",	// AF, ZA - Cape Town, Western Cape: View of Plettenberg Bay (704x576) (?)
	//"http://media.baysider.com/galleycam/ispy.jpg",								// AF, ZA - Cape Town, Western Cape: View of the bay from Fish Hoek (600x450) (?)
	//"http://www.laibach.co.za/axis/laibach_0000000001.jpg",						// AF, ZA - Cape Town, Western Cape: View of the Laibach Wineyards (320x240) (1h)
	//"http://tablemountain.cybercapetown.com/latest-600.jpg",							// AF, ZA - Cape Town, Western Cape: View of Tablemountain from Citybowl (600x453) (5m)
	//"http://mystical.eu.org/webcam.jpg",										// AF, ZA - Cape Town, Western Cape: View of Tablemountain from Citybowl (320x240) (15s)
	//"http://www.grapevine-cottage.co.za/webcam/grapevine-cottage-webcam01.jpg",			// AF, ZA - Cape Town, Western Cape: View of Grapevine Cottage (640x480)
	//"http://www.theheads.co.za/CamPic.jpg",									// AF, ZA - Cape Town, Western Cape: View of Knysna Heads (640x480) (45s)
	//"http://www.pretoria-astronomy.co.za/webcam/webcamimage.php?camimage=erwat_cam.jpg",	// AF, ZA - Kempton Park, Johannesburg: View of Kempton Park (640x480) (2m)
	//"http://www.aad.gov.au/asset/webcams/casey/default.asp",						// AN, AN - Casey Station, Australian Antarctic Division: View from Casey Station (640x480) (10m)
	//"http://www.aad.gov.au/asset/webcams/mawson/default.asp",						
	//"http://www.aad.gov.au/asset/webcams/davis/default.asp",						

	// ANTARCTICA
	
	//"http://www.aad.gov.au/asset/webcams/macca/small/I0709112117s.jpg",				// AN, AN - Macquarie Island Station, Australian Antarctic Division: View from Macquarie Island Station Northeast (640x480) (15m)
	/*The picture is captured using a video camera at the station and sent via a permanent satellite link to the Australian Antarctic Division's Headquarters located at Kingston, Tasmania.
	They aim to update the picture every 15 minutes. However, much of the time the weather conditions at Macquarie Island can prevent reasonable pictures being taken - the camera lens can be completely covered in water drops due to rain or low cloud or mist.
	The date/time on the picture shows local Macquarie Island time, which is the same as Australian Eastern Standard Time (normally 10 hours ahead of UTC, 11 hours ahead during Daylight Savings Time, over summer.)*/

	// EUROPE
	
	"http://www.parislive.net/eiffelcam2.jpg",						// EU, FR - Paris, Île-de-France: View of Eiffel Tower (352x288)  (?)
	//"http://www.parislive.net/eiffelcam3.jpg", 									// EU, FR - Paris, Île-de-France: View of Eiffel Tower (800x608) (?)
	"http://images.earthcam.com/ec_metros/ourcams/trafalgarsq.jpg",	// EU, UK - London, England: Trafalgar Square (352x288) (Live)
	"http://images.earthcam.com/ec_metros/ourcams/sharks.jpg",		// EU, UK - London, England: Aquarium Sharks (352x288) (?)
	"http://www.bbc.co.uk/radio1/webcam/images/live/webcam.jpg",	// EU, UK - London, England: BBC Radio 1 Studio A (384x288) (10m)
	"http://www.bbc.co.uk/radio1/webcam/images/live/webcam2.jpg",	// EU, UK - London, England: BBC Radio 1 Studio B (384x288) (10m)
	"http://www.belfastcity.gov.uk/webcam/cityhall_00002.jpg", 		// EU, UK - Belfast, England: View from the front of City Hall (352x288) (10s)
	//"http://213.123.140.128/-wvhttp-01-/GetStillImage?",			// EU, UK - Belfast, England: View from the Royal Mail building on Tomb Street (320x240) (10s)

	"http://www.askjob.ch/webcam.jpg", 								// Switzerland Geneve 320x240
	//"http://62.131.20.108:8888/video/pull?", 									// Holland Den Helder 320x240
	//"http://sharon.esrac.ele.tue.nl/cgi-bin/webcam3-slow.jpg", 							// Holland Eindhoven 320x240
	//"http://sharon.esrac.ele.tue.nl/cgi-bin/webcam1.jpg", 							// Holland Eindhoven 320x240
	//"http://sharon.esrac.ele.tue.nl/cgi-bin/webcam2.jpg", 							// Holland Eindhoven View of an Office 320x240 Unknown Crash
	//"http://sharon.esrac.ele.tue.nl/cgi-bin/webcam4.jpg", 							// Holland Eindhoven View of an Office 320x240 Unknown Crash
	"http://www.bbc.co.uk/england/webcams/live/norfolk_spare.jpg", 	// London UK Norfolk 384x288
	"http://www.bbc.co.uk/england/webcams/live/manchester2.jpg", 	// London UK Manchester 384x288
	//"http://www.nafc.ac.uk/SpyCam.jpg", // London UK Shetland Island 640x480
	"http://millenniumsquare.a-q.co.uk/images/image.jpg", 			// Millenium Square Leeds 352x288
	"http://www.abbeyroad.com/webcam/crossing.jpg", 				// London Abbey Road Crossing 384x284
	"http://www.ek.fi/kamera/tn_palace00.jpg", 						// Helsinki Finland 352x264
	"http://www.tampere.fi/live/kuva.jpg", 							// Tempere Finland 352x288
	
	// AUSTRALIA
	
	//"http://www.toorakplace.net.au/cam_1.jpg", // Toorak Palace AU 640x480
	"http://www.dpi.wa.gov.au/imarine/coastaldata/coastcam/livegfx/camtrigg2/live.jpg", // Perth AU 352x288
	"http://www.dpi.wa.gov.au/imarine/coastaldata/coastcam/livegfx/camswan/live.jpg", // Perth AU 352x288
	
	// ASIA
	
	//"http://www.ds-shanghai.org.cn/webcam/image.jpg", // Shanghai CN 640x480
	"http://reno.oiccam.com/ftp/capcom/panda188/image.jpg", // JP Panda Pen 320x240
	"http://www.manilaview.com/video1.jpg", // Manila AS 320x240
	"http://www.discoverhongkong.com/eng/interactive/webcam/images/ig_webc_harb1.jpg", // Victoria Harbor Hong Kong KO 352x288
	"http://www.discoverhongkong.com/eng/interactive/webcam/images/ig_webc_caus1.jpg", // Causeway Bay Hong Kong KO 352x288
	"http://www.discoverhongkong.com/eng/interactive/webcam/images/ig_webc_vict1.jpg", // Victoria Park Hong Kong KO 352x288
	"http://www.discoverhongkong.com/eng/interactive/webcam/images/ig_webc_petr1.jpg", // Hong Kong Skyline Hong Kong KO 352x288
	"http://www.discoverhongkong.com/eng/interactive/webcam/images/ig_webc_peak1.jpg", // View from the Peak Hong Kong KO 352x288



// Satellite Weather Images
//"http://pda.meteox.com/images.aspx?jaar=-3&voor=&soort=loop3uur256&c=uk&n=", // UK weather 256x256 Animated GIF
//"http://pda.meteox.com/images.aspx?jaar=-3&voor=&soort=loop3uur256&c=fr&n=", // France weather 256x256 Animated GIF
//"http://pda.meteox.com/images.aspx?jaar=-3&voor=&soort=loop3uur256&c=nl&n=", // Benelux weather 256x256 Animated GIF
//"http://pda.meteox.com/images.aspx?jaar=-3&voor=&soort=loop3uur256&c=de&n=", // Germany weather 256x256 Animated GIF
//"http://image.weather.com/images/sat/mideastsat_440x297.jpg", //Egypt Weather (440x297) (15m)


};
