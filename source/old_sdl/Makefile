
# Makefile for sms+sdl
#
# Copyright (C) 1998, 1999, 2000  Charles Mac Donald
# SDL version by Gregory Montoir <cyx@frenchkiss.net>
#
# Defines :
# LSB_FIRST : for little endian systems.
# X86_ASM   : enable x86 assembly optimizations
# USE_ZLIB  : enable ZIP file support

NAME	  = sms_sdl

CC        = gcc
CFLAGS    = `sdl-config --cflags` -Wall -O0 -g
DEFINES   = -DLSB_FIRST -DUSE_ZLIB -DDEBUG
INCLUDES  = -I. -I.. -I../cpu -I../sound -I../unzip -I../ntsc
LIBS	  = `sdl-config --libs` -lz -lm

OBJ	=       obj/z80.o	\
		obj/sms.o	\
		obj/pio.o	\
		obj/memz80.o	\
		obj/render.o	\
		obj/tms.o	\
		obj/vdp.o	\
		obj/system.o	\
		obj/error.o
	        
OBJ	+=	obj/fileio.o	\
		obj/state.o	\
		obj/loadrom.o

OBJ	+=	obj/ioapi.o	\
		obj/unzip.o

OBJ	+=	obj/sms_ntsc.o	

OBJ	+=      obj/sound.o	\
		obj/sn76489.o	\
		obj/emu2413.o	\
		obj/ym2413.o	\
		obj/fmintf.o	

OBJ	+=	obj/main.o	\
		obj/sdlsms.o	\
		obj/filters.o	

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

clean:
	rm -f $(OBJ) $(NAME)

obj/%.o : 	%.c %.h
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o : 	%.c
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o : 	../%.c
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o : 	../%.c ../%.h
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o :	../sound/%.c ../sound/%.h
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o	:	../unzip/%.c ../unzip/%.h
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o :	../cpu/%.c ../cpu/%.h
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

obj/%.o :	../ntsc/%.c ../ntsc/%.h
		$(CC) -c $(CFLAGS) $(INCLUDES) $(DEFINES) $< -o $@
