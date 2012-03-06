TARGET = eyePSP
OBJS = main.o graphics.o

INCDIR = ./include
CFLAGS = -O2 -G0 -Wall 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PSP_FW_VERSION=271

LIBDIR = ./lib
LDFLAGS =
LIBS = -lpspgum -lpspgu -lpng -ljpeg -lz -lm -lpspusb -lpsputility -lpspusbcam -lpspjpeg

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = eyePSP
PSP_EBOOT_ICON = ICON0.PNG 


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

#back.c: ./back.png
#	bin2c ./back.png back.c back

#verdana.c: ./verdana.ttf
#	bin2c ./verdana.ttf verdana.c verdana