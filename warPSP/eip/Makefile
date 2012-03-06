TARGET = warPSP
OBJS = font.c main.o draw.o scepower.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = warPSP Easy Installer
PSP_EBOOT_ICON = ICON0.PNG 
PSP_EBOOT_PIC1 = PIC1.PNG 
LIBS = -lpsppower -lpspgum -lpspgu -lm

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak 

font.c : font.raw
	bin2c font.raw font.c font
