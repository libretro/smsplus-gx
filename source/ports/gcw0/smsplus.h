#ifndef SMSPLUS_H
#define SMSPLUS_H

#define HOST_WIDTH_RESOLUTION 320
#define HOST_HEIGHT_RESOLUTION 240

#define VIDEO_WIDTH_SMS 256
#define VIDEO_HEIGHT_SMS 192
#define VIDEO_WIDTH_GG 160
#define VIDEO_HEIGHT_GG 144

/* Input defines for custom remapping */
#define CONFIG_BUTTON_UP 0
#define CONFIG_BUTTON_DOWN 1
#define CONFIG_BUTTON_LEFT 2
#define CONFIG_BUTTON_RIGHT 3
#define CONFIG_BUTTON_BUTTON1 4
#define CONFIG_BUTTON_BUTTON2 5
#define CONFIG_BUTTON_START 6

/* Colecovision specific */
#define CONFIG_BUTTON_DOLLARS 7
#define CONFIG_BUTTON_ASTERISK 8
#define CONFIG_BUTTON_ONE 10
#define CONFIG_BUTTON_TWO 11
#define CONFIG_BUTTON_THREE 12
#define CONFIG_BUTTON_FOUR 13
#define CONFIG_BUTTON_FIVE 14
#define CONFIG_BUTTON_SIX 15
#define CONFIG_BUTTON_SEVEN 16
#define CONFIG_BUTTON_EIGHT 17
#define CONFIG_BUTTON_NINE 18

/* End of Defines for input remapping */


typedef struct {
	char gamename[256];
	char sramdir[256];
	char sramfile[256];
	char stdir[256];
	char scrdir[256];
	char biosdir[256];
} gamedata_t;

void smsp_state(uint8_t slot_number, uint8_t mode);

#define SOUND_FREQUENCY 44100
#define SOUND_SAMPLES_SIZE 1024

#endif
