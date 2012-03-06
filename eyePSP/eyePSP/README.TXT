eyePSP Control Demo
   Alpha v0.2c
   by caliFrag


Program Installation:
 Copy the PSP folder to the root of your memory stick: it will create the necessary folders and will copy the demo program.

 Alternatively, copy the ..\PSP\GAME\eyePSP folder directly to your PSP\GAME or PSP\GAMEXXX folder on your memory stick.


Version Changelog:
 Alpha v0.2c:
  Fixed bug that crashed the program when the cursor was drawn off-screen.
  Updated to work with 5.00 M33.

 Alpha v0.2b:
  Added extra control methods (direct, gesture), on-screen buttons and button press detection functions.

 Alpha v0.2a:
  Can now see what camera actually views and tracks, and added a new control method (mimic).

 Alpha v0.1:
  Basic laser tracking, with on-screen cursor position



Source Installation:
 Copy the ..\src\cygwin folder to your cygwin directory. The eyePSP source can be found in the ..\src\cygwin\projects\eyePSP folder.

 This will also install the PSP Camera header files directly into the sdk include folder.

 If you do not want to do this, then:
  1) Copy the .h files from the ..\src\cygwin\usr\local\pspdev\psp\sdk\include into the ..\src\cygwin\projects\eyePSP folder.
  2) Open the main.c file and change the USB cam headers by changing the <'s and >'s to "'s:
     You can find the USB cam headers under the comment "// USB cam headers".

     IE. Change #include <psputility_usbmodules.h> to #include "psputility_usbmodules.h"

