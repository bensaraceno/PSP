TARGET = warPSP
OBJS = main.o blit.o nmeap01.o

BUILD_PRX = 1
#PRX_EXPORTS = exports.exp
#EXTRA_TARGETS = EBOOT.PBP
#PSP_EBOOT_TITLE = warPSP^pbp

USE_PSPSDK_LIBS = 1
USE_PSPSDK_LIBC = 1
# Use the kernel's small inbuilt libc
#USE_KERNEL_LIBC = 1
# Use only kernel libraries
#USE_KERNEL_LIBS = 1

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

#LIBS = -lm -lc -lpsplibc -lpspuser -lpspkernel -lpsppower -lpsppower_driver -lpsprtc -lpsprtc_driver -lpsphprm -lpsphprm_driver -lpspwlan -lpspnet -lpsputility
LIBS = -lpsppower -lpsprtc -lpsphprm -lpspwlan -lpspnet

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
