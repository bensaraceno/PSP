@echo off

chdir "C:\cygwin\projects\warPSP\pbp"

if exist "EBOOT.PBP" del "EBOOT.PBP"
if exist "warPSP.elf" del "warPSP.elf"
if exist "main.o" del "main.o"

C:
chdir C:\cygwin\bin

set path=%path%;C:/cygwin/usr/local/pspdev/bin
set PSPSDK=C:/cygwin/usr/local/pspdev

bash --login -c 'make -C "warPSP\pbp"'

chdir "C:\cygwin\projects\warPSP\pbp"

if exist "EBOOT.PBP" copy EBOOT.PBP "K:\PSP\GAME\warPSP"

echo.
echo.
echo            Done!
echo.
echo.

pause