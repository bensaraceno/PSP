#release: all
#	mksfo 'skyPSP' PARAM.SFO
#	pack-pbp EBOOT.PBP PARAM.SFO NULL NULL NULL NULL NULL skypspmain.prx NULL

TARGET = skyPSP
OBJS = main.o config.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -g
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PSP_FW_VERSION = 271

LIBDIR =
LDFLAGS =
#PSPSDK_LIBC_LIB = 
#PSPSDK_LIBS = -lpspdebug -lpspdisplay_driver -lpspctrl_driver -lpspsdk
LIBS =  -lz -lm -lcurl -lpspwlan -lpspusb -lpsputility -lpspusbcam -lpspjpeg

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = skyPSP
PSP_EBOOT_ICON = ICON0.PNG 
PSP_EBOOT_PIC1 = PIC1.PNG 


PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
