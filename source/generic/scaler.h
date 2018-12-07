
#ifndef _SCALER_H
#define _SCALER_H

#include <stdint.h>

void upscale_160x144_to_320x240(uint32_t *dst, uint32_t *src);
void upscale_160x144_to_320x480(uint32_t *dst, uint32_t *src);

void upscale_SMS_to_320x240(uint32_t *dst, uint32_t *src, uint32_t height_scale);
void upscale_SMS_to_320x480(uint32_t *dst, uint32_t *src, uint32_t height_scale);
void upscale_SMS_to_800x480(uint32_t *dst, uint32_t *src, uint32_t height_scale);

void upscale_160x144_to_800x480(uint32_t *dst, uint32_t *src);
//void upscale_256x192_to_800x480(uint32_t *dst, uint32_t *src, uint32_t height_scale);

void upscale_160x144_to_320x272_for_800x480(uint32_t *dst, uint32_t *src);

void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t *src, uint16_t *dst);



#endif
