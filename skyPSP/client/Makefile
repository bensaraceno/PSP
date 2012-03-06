TARGET = skyPSP
OBJS = main.o back.o verdana.o config.o pspgraphics.o pspframebuffer.o pgeFont.o

INCDIR = 
CFLAGS = -O0 -G0 -Wall -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgum -lpspgu -lpng -ljpeg -lz -lfreetype -lm -lcurl


EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = skyPSP
PSP_EBOOT_ICON = ICON0.PNG 
PSP_EBOOT_PIC1 = PIC1.PNG 

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

#back.c: ./back.png
#	bin2c ./back.png back.c back

#verdana.c: ./verdana.ttf
#	bin2c ./verdana.ttf verdana.c verdana
