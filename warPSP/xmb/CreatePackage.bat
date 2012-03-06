@echo off

chdir "C:\cygwin\projects\warPSP\xmb"

if not exist "warPSP.prx" echo "Error! warPSP.prx not found!"
if exist "warPSP.prx" copy "warPSP.prx" "C:\cygwin\projects\warPSP\eip"

chdir "C:\cygwin\projects\warPSP\eip"
if exist "warPSP.prx" copy /b "warPSP.prx"+"bookmarks.html"
if not exist "EBOOT.PBP" echo "Error! EBOOT.PBP not found!"
if exist "EBOOT.PBP" copy /b EBOOT.PBP + warPSP.prx "K:\PSP\GAME\warPSP^installer"

if exist "K:\lftv_plugin.prx" del "K:\lftv_plugin.prx"
if exist "K:\warLog.txt" del "K:\warLog.txt"

echo.
echo.
echo            Done!
echo.
echo.

pause