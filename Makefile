TARGET_LIB=libpspav.a

C_OBJS = \
	src/at3.o \
	src/pspav.o \
	src/pspav_audio.o \
	src/pspav_decoder.o \
	src/pspav_reader.o \
	src/pspav_video.o \
	

OBJS = $(C_OBJS)

all: $(TARGET_LIB)

PSPSDK=$(shell psp-config --pspsdk-path)

CC=psp-gcc
INCDIR = include
CFLAGS = -std=c99 -Wall -Os -G0 -fno-pic
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =

include $(PSPSDK)/lib/build.mak
