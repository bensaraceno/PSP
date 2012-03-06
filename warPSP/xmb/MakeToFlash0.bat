@echo off

chdir "C:\cygwin\projects\warPSP\xmb"

if exist "warPSP.prx" del "warPSP.prx"
if exist "warPSP.elf" del "warPSP.elf"
if exist "main.o" del "main.o"

C:
chdir C:\cygwin\bin

set path=%path%;C:/cygwin/usr/local/pspdev/bin
set PSPSDK=C:/cygwin/usr/local/pspdev

bash --login -c 'make -C "warPSP\xmb"'

echo.
echo.

chdir "C:\cygwin\projects\warPSP\xmb"

if not exist "warPSP.prx" echo "Error! warPSP.prx not found!"
if exist "warPSP.prx" copy "warPSP.prx" "K:\vsh\nodule\lftv_plugin.prx"

echo.
echo.
echo            Done!
echo.
echo.

pause