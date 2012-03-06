 ###--------------------------------------------------------
##                                          warPSP v1.0 BETA
##                                             warXing suite
##                                               by caliFrag
##
##
 ###--------------------------------------------------------

  ###-------------------------------------------------------
 ##   Brief:
  ###-------------------------------------------------------

      warPSP is a general warXing application for the PSP.

  ###-------------------------------------------------------
 ##   Uses:
  ###-------------------------------------------------------

      warDriving \ warWalking \ warBiking \ warFlying \ 
              and any other warXing activity!

      More info @ http://en.wikipedia.org/wiki/WarXing

  ###-------------------------------------------------------
 ##   General:
  ###-------------------------------------------------------

       warPSP puts the system into a wifi network discovery
      mode that continually scans and rescans for networks.

  ###-------------------------------------------------------
 ##   Logging:
  ###-------------------------------------------------------

       The results from each scan are written to a log file
      at the root of the memory stick. The log file can be
      opened in the PSP's built-in web browser after exiting
      the application. A bookmark is added to the PSP's built
      -in web browser after creating a log file.

       POI files can also be created and loaded into common
      GPS mapping software programs, including MapThis! by
      Deniska, for the PSP.

  ###-------------------------------------------------------
 ##   Display:
  ###-------------------------------------------------------

     pbp standalone:
      TBA

     easy installer:
      TBA

     xmb extension:

       The on-screen user interface has four general views.
      All four views show the number of available wireless
      networks in the area.

       Minimal: The percent of battery life left is shown.
         ex.
	               ,-' Networks: 03
	                    Scanning...
	                           100%


       Summary: A summary of available wireless networks 
      found during the last scan are shown.
         ex.
	               ,-' Networks: 03
	                    Scanning...
	        ,-' Scan #03513 Summary
	         1. X linksys (90)
	         2.   home_wireless (75)
	         3. X netgear (65)


       Network: The information for each unique wireless
     network found since application start is shown.
         ex.
                      ,-'  Networks: 03
                            Scanning...
               ,-' Network: #003 of 078
                SSID: linksys
                MAC: 08:23:d9:57:c0:16
                RSSI: 100
                Channel: 6


       Console: The PSP system details are shown.
          ex.
	              ,-'  Networks: 03
	                    Scanning...
                ,-' PSP Console Details
                 Name: caliFrag
                 MAC: 00:00:00:00:00:00
                 CPU/BUS: 222/111 mHz
                 Free RAM: 3.78 MB
                 Power Source: External
                 Battery Details:
                  Charge Level: 100%
                  Time Left: N/A
                  Temperature: 33C/91F
                  Voltage: 4.2V


  ###-------------------------------------------------------
 ##   Controls:
  ###-------------------------------------------------------

     pbp standalone:

       During Startup: Hold L to DISABLE logging features.
      Logging features are ENABLED by default.
   
       After Startup: Hold L or R to show the on-screen menu.
      Use Up and Down on the D-Pad to scroll through the
      options and press Cross to select an option.

          ex.
                 ,-' warPSP^xmb Menu `-.
                  Terminate Application
                  Toggle Network Rescan
                  Toggle Logging to ms0
                  Unknown New Option
                  Display More Networks
                  Cycle to Next Network
                          Etc...
                 `-..  X to Select  ..-'


      To switch between views: Press Left or Right on the 
     directional pad.

     easy installer:

       During Startup: Hold L to ENABLE quick install to
      the LFTV_PLUGIN slot. Hold R to ENABLE quick install
      to the PREMO_PLUGIN slot.

       Press SELECT anytime to abort (except during install).

     xmb extension:

       During Startup: Hold L to DISABLE logging features.
      Logging features are ENABLED by default.
   
       After Startup: Hold L+R to show the on-screen menu.
      Use Up and Down on the D-Pad to scroll through the
      options and press Cross to select an option.

          ex.
                 ,-' warPSP^xmb Menu `-.
                  Terminate Application
                  Toggle Network Rescan
                  Toggle Logging to ms0
                  Unknown New Option
                  Display More Networks
                  Cycle to Next Network
                          Etc...
                 `-..  X to Select  ..-'


      To switch between views: While holding L+R, press Left
     or Right on the directional pad.

      To enable or disable "Cloaked" mode: While holding L+R,
     press the Select button.

  ###-------------------------------------------------------
 ##   Installation:
  ###-------------------------------------------------------

     pbp standalone:

       Copy the EBOOT.PBP into the GAME\warPSP\ folder. All
      image files must be placed within GAME\warPSP\images.
      Optionally you can install to the GAME150 folder.

     easy installer:
       Copy the EBOOT.PBP into the GAME\warPSP^installer\ folder.
      Optionally you can install to the GAME150 folder.

     xmb extension:

       Just run the Easy Installer Program and the prx will
      be extracted to the root of the memory stick. You will
      then be presented with the standard disclaimer and a
      chance to cancel installation by pressing Select.

       If you accept the risks involved with flashing the PSP
      press Start and you will be presented with installation
      options and are also given a chance to cancel again.

       You can choose to install the prx to the location free
      player slot or the remote play slot. I chose these slots
      because they are both under the "Network" icon in the XMB.

       If you would like to install to a different slot There 
      are plenty of flashing programs and recovery mode that
      you can use to put it wherever you want.

       To uninstall or reinstall warPSP just run the program
      again and follow the on screen instructions.

       If your PSP battery is below 25% you will not be able
      to install unless you plug in your AC Adapter. This is
      done as a precaution during flash write operations.

       A "quick install" method has been implemented. Holding
      the L Trigger during startup will quick install the prx
      to the location free player slot. Holding the R Trigger
      during startup will quick install the prx to the remote
      play slot.

  ###-------------------------------------------------------
 ##   Known Bugs:
  ###-------------------------------------------------------

     pbp standalone:

     easy installer:

     xmb extension:

       Only the first access point is displayed correctly.
      This issue has been present since the first release and
      is due to the fact that I haven't figured out all the
      offsets to pull the data out of the structures. Right
      now the SSID and BSSID for each network is displayed
      correctly, but only for networks whose SSID length is
      shorter than 18 characters. If the SSID is longer than
      18 characters then it wraps around the screen and can
      mess up some of the offsets.

       Function hooking has only been partially implemented
      which means random stuff happens when you do stuff you
      shouldn't do like trying to open the internet browser or
      rss and log on to a network while in the middle of a scan
      or opening the usb connection while you are logging. I'm
      not responsible for anything that might happen if you do
      stuff that common sense tells you not to do!

       So far this is what has been documented:
       Launching the internet browser while warPSP is running
      will results in the internet browser being unavailable
      after exiting.

       Opening the USB connection while warPSP is logging to
      the memory stick can result in no log file being created.
       A partial fix has been put in place that will close the
      log file if it detects that usb mode has been activated,
      then reopen it once usb mode has been deactivated.

       Launching an application from the memory stick will 
      cause the prx to lock up, no log file will be created
      but the application will launch normally.

       Turning off the PSP while the application is running
      can result in the PSP locking up and crashing and not
      creating a log file.
       The power switch trigger has been hooked and if the
      psp is put into standby the program will automatically
      stop itself and close.

       Launching a video from the memory stick will crash the
      psp. This is possibly due to the PRX being over 50kb.
       Future versions may be compressed to fix this.

  ###-------------------------------------------------------
 ##   Future Plans:
  ###-------------------------------------------------------

     pbp standalone:

     easy installer:

     xmb extension:

       GPS integration including POI file creation.

       Auto-Connect that will add the strongest unprotected
      network to the PSP and automatically connect to it.

       As stated before, a WEP cracker will NOT be added as
      there is no known way to put the PSP's wLan card into
      "Promiscuous" mode which allows the PSP to sniff for
      wireless packets that are not specifically destined
      for each PSP's unique MAC address.

  ###-------------------------------------------------------
 ##   Changelog:
  ###-------------------------------------------------------

     pbp standalone:

       09/01/2007: v1.0 Alpha
        General program ported to EBOOT.PBP version. Enhanced
       on-screen user-interface and functionality. Many bugs
       present in xmb extension fixed and gps functionality was
       integrated.

     easy installer:

       08/03/2007: v1.0 Alpha
        Created an Easy Installation Program to help new PSP
       users who are not comfortable entering recovery or
       flashing their PSP to do it easily with the program.

     xmb extension:

       09/03/2007: v1.0 Beta
        Decreased start up time before scan information is
       displayed.
        Logging and POI logging features were implemented.
        Added more views for scan and console details.
        Added a "Cloaked" mode for no screen output.
        Added an auto-save feature that will save logfiles
       every 100 scans in case of power failure/other bugs.
        Added a low battery check terminating the program
       when the battery falls below 10% charge.
        Added an auto-bookmark feature that will create or
       add a new bookmark to the PSP's web browser after
       creating a log file.

       Bugfixes:
        The internet browser seems to always work now after
       exit and the random crashes have been reduced.
        Solved a bug issue which locked up xmb on exit during
       "Scan Completed" status.
        Solved a bug which prevented VSH Rebooter from being
       able to properly reboot the PSP thus forcing a crash.
        Seemed to fix an issue with the scan counter getting
       messed up and displaying the incorrect number.
        Added "Hooking" for:
         Controls so input is not sent to the xmb.
         USB so log is closed before usb mode is activated.
         Power switch so program is closed before turning off.


       05/10/2007: Bugfix
        Fixed bug issue which made the built-in internet browser
       and rss feeds unavailable after exit.

       04/26/2007: v1.0 Alpha
        Added a summary view and ability to toggle the rescan
       feature on and off.

       04/20/2007: Proof of Concept
        Scans local wireless networks and shows number found.
