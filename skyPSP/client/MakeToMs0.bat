@echo off

chdir "C:\cygwin\projects\skyPSP\client"

if exist "EBOOT.PBP" del "EBOOT.PBP"
if exist "skyPSP.elf" del "skyPSP.elf"
if exist "main.o" del "main.o"

C:
chdir C:\cygwin\bin

set path=%path%;C:/cygwin/usr/local/pspdev/bin
set PSPSDK=C:/cygwin/usr/local/pspdev

bash --login -c 'make -C "skyPSP\client"'

chdir "C:\cygwin\projects\skyPSP\client"

if exist "EBOOT.PBP" copy EBOOT.PBP "K:\PSP\GAME\skyPSP"

echo.
echo.
echo            Done!
echo.
echo.

pause