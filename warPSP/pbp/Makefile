TARGET = warPSP
OBJS = main.o logo.o pspgraphics.o pspframebuffer.o nmeap01.o
#mp3.o  pgeFont.o
#USE_PSPSDK_LIBC = 1

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgum -lpspgu -lpng -lz -lm -lpspwlan -lpspnet -lpsprtc -lpsppower -lpsphprm
# -lmad -lpspaudiolib -lpspaudio -lfreetype
LDFLAGS =


EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = warPSP
PSP_EBOOT_ICON = ICON0.PNG 
PSP_EBOOT_PIC1 = PIC1.PNG 

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

LIBS += -lpsphprm_driver
#LIBS += -lstdc++  -lm -lpsphprm_driver -lpsprtc  -lpspvfpu 

#logo.c: ./images/logo.png
#	bin2c ./images/logo.png logo.c logo
