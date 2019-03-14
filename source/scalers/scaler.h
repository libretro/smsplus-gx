#ifndef SCALER_H
#define SCALER_H

#include <stdint.h>

extern void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t* restrict src, uint16_t* restrict dst);
extern void upscale_160x144_to_320x240(uint32_t* restrict dst, uint32_t* restrict src);
extern void upscale_SMS_to_320x240(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height_scale);
extern void upscale_160x144_to_320x272_for_480x272(uint32_t* restrict dst, uint32_t* restrict src);
extern void upscale_160x144_to_480x272(uint32_t* restrict dst, uint32_t* restrict src);
extern void upscale_256xXXX_to_384x272_for_480x272(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height);
extern void upscale_256xXXX_to_480x272(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height);


#endif
