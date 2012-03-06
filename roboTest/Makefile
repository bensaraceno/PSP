TARGET = rm
OBJS = main.o pspgraphics.o pspframebuffer.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgu -lpng -lz -lm -lpsppower
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = spriteTest[TBD]

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak