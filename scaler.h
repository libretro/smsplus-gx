
#ifndef _SCALER_H
#define _SCALER_H

#include <stdint.h>

void upscale_160x144_to_320x240(uint32_t *dst, uint32_t *src);
void upscale_160x144_to_320x240_for_400x240(uint32_t *dst, uint32_t *src);
void upscale_160x144_to_400x240(uint32_t *dst, uint32_t *src);
void upscale_160x144_to_320x272_for_480x272(uint32_t *dst, uint32_t *src);
void upscale_160x144_to_480x272(uint32_t *dst, uint32_t *src);

void upscale_256x192_to_320x240(uint32_t *dst, uint32_t *src);
//void upscale_256x192_to_304x240_for_400x240(uint32_t *dst, uint32_t *src);
void upscale_256x192_to_384x240_for_400x240(uint32_t *dst, uint32_t *src);
//void upscale_256x192_to_400x240(uint32_t *dst, uint32_t *src);
void upscale_256x192_to_384x272_for_480x272(uint32_t *dst, uint32_t *src);
void upscale_256x192_to_480x272(uint32_t *dst, uint32_t *src);

void upscale_256x192_to_320x480(uint32_t *dst, uint32_t *src);
void upscale_160x144_to_320x480(uint32_t *dst, uint32_t *src);

#endif
