
/*
    Copyright (c) 2002, 2003 Gregory Montoir

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "sdlsms.h"
#include "saves.h"
#include "shared.h"


static t_config cfg;


static int parse_args(int argc, char **argv)
{
  int i;

  /* default virtual console emulation settings */
  sms.use_fm = 1;
  sms.country = TYPE_OVERSEAS;


  strcpy(cfg.game_name, argv[1]);

  for(i = 2; i < argc; ++i) {
    if(strcasecmp(argv[i], "--psg") == 0)
      sms.use_fm = 0;
    else if(strcasecmp(argv[i], "--eu") == 0)
      sms.country = TYPE_DOMESTIC;
    else if(strcasecmp(argv[i], "--usesram") == 0) {
      cfg.usesram = 1;
      sms.save = 1;
    }
    else if(strcasecmp(argv[i], "--nosound") == 0)
      cfg.nosound = 1;
    else if(strcasecmp(argv[i], "--joystick") == 0)
      cfg.joystick = 1;
    else 
      printf("WARNING: unknown option '%s'.\n", argv[i]);
  }
  return 1;
}


int main(int argc, char **argv)
{
  printf("%s (Build date %s)\n", SMSSDL_TITLE, __DATE__);
  printf("(C) Charles Mac Donald in 1998, 1999, 2000\n");
  printf("SDL Version by Gregory Montoir (cyx@frenchkiss.net)\n");
  printf("\n");

  if(argc < 2) {
    int i;
    printf("Usage: %s <filename.<SMS|GG>> [--options]\n", argv[0]);
    printf("Options:\n");
    printf(" --psh           \t PSG sound only.\n");
    printf(" --eu        	\t set the machine type as OVERSEAS instead of DOMESTIC.\n");
    printf(" --usesram      \t load/save SRAM contents before starting/exiting.\n");
    printf(" --fskip <n>    \t specify the number of frames to skip.\n");
    printf(" --fullspeed    \t do not limit to 60 frames per second.\n");
    printf(" --fullscreen   \t start in fullscreen mode.\n");
    printf(" --joystick     \t use joystick.\n");
    printf(" --nosound      \t disable sound.\n");
    printf(" --filter <mode>\t render using a filter: ");
    return 1;
  }


  memset(&cfg, 0, sizeof(cfg));
  if(!parse_args(argc, argv))
	  return 0;

  if(sdlsms_init(&cfg)) {
    sdlsms_emulate();
    sdlsms_shutdown();
  }

  return 0;
}
