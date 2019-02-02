PRGNAME     = sms.elf

# define regarding OS, which compiler to use
EXESUFFIX = 
TOOLCHAIN = 
CC          = gcc
CCP         = g++
LD          = gcc

# add SDL dependencies

CFLAGS		= -O0 -g -std=gnu99 -DINLINE=inline -DLSB_FIRST
CFLAGS 		+= -I/usr/include/SDL -Isource -Isource/z80 -Isource/generic -I./source/sound -Isource/unzip -Isource/sdl -Isource/sound/crabemu_sn76489

CXXFLAGS	= $(CFLAGS) 
LDFLAGS     = -lSDL -lm -flto -lz -lportaudio

# Files to be r
SRCDIR    = ./source ./source/unzip ./source/z80 ./source/sound ./source/generic ./source/rs97 ./source/sound/crabemu_sn76489
VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CP   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CP   = $(notdir $(patsubst %.cpp, %.o, $(SRC_CP)))
OBJS     = $(OBJ_C) $(OBJ_CP)

# Rules to make executable
$(PRGNAME)$(EXESUFFIX): $(OBJS)  
	$(LD) $(CFLAGS) -o $(PRGNAME)$(EXESUFFIX) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_CP) : %.o : %.cpp
	$(CCP) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o
