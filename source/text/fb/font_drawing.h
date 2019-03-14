#ifndef FONT_DRAWING_H
#define FONT_DRAWING_H

#include <stdint.h>
#include <string.h>

extern uint16_t* restrict backbuffer;
void print_string(const char *s,const uint16_t fg_color, const uint16_t bg_color, int32_t x, int32_t y);

#endif
