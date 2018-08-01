#include <stdio.h>
#include <SDL/SDL.h>
#include <stdint.h>
#include "shared.h"
#include "sdlsms.h"

SDL_Surface *font = NULL;
SDL_Surface *bigfontred = NULL;
SDL_Surface *bigfontwhite = NULL;

extern t_sdl_video sdl_video;

uint16_t gfx_font_width(SDL_Surface* inFont, char* inString) 
{
    if((inFont == NULL) || (inString == NULL))
        return 0;
    uintptr_t i, tempCur, tempMax;
    for(i = 0, tempCur = 0, tempMax = 0; inString[i] != '\0'; i++) {
        if(inString[i] == '\t')
            tempCur += 4;
        else if((inString[i] == '\r') || (inString[i] == '\n'))
            tempCur = 0;
        else
            tempCur++;
        if(tempCur > tempMax) tempMax = tempCur;
    }
    tempMax *= (inFont->w >> 4);
    return tempMax;
}

uint16_t gfx_font_height(SDL_Surface* inFont) 
{
    if(inFont == NULL)
        return 0;
    return (inFont->h >> 4);
}

void gfx_font_print(SDL_Surface* dest,int16_t inX, int16_t inY, SDL_Surface* inFont, char* inString) 
{
    if((inFont == NULL) || (inString == NULL))
        return;

    uint16_t* tempBuffer = dest->pixels;
    uint16_t* tempFont = inFont->pixels;
    uint8_t*  tempChar;
    int16_t   tempX = inX;
    int16_t   tempY = inY;
    uintptr_t i, j, x, y;

    SDL_LockSurface(dest);
    SDL_LockSurface(inFont);

    for(tempChar = (uint8_t*)inString; *tempChar != '\0'; tempChar++) 
    {
		switch(*tempChar)
		{
			case ' ':
				tempX += (inFont->w >> 4);
			break;
			case '\t':
				tempX += ((inFont->w >> 4) << 2);
			break;
			case '\r':
				tempX = inX;
			break;
			case '\n':
				tempX = inX;
				tempY += (inFont->h >> 4);
			break;
		}

        for(j = ((*tempChar >> 4) * (inFont->h >> 4)), y = tempY; (j < (((*tempChar >> 4) + 1) * (inFont->h >> 4))) && (y < dest->h); j++, y++) 
        {
            for(i = ((*tempChar & 0x0F) * (inFont->w >> 4)), x = tempX; (i < (((*tempChar & 0x0F) + 1) * (inFont->w >> 4))) && (x < dest->w); i++, x++)
            {
                tempBuffer[(y * dest->w) + x] |= tempFont[(j * inFont->w) + i];
            }
        }
        tempX += (inFont->w >> 4);
    }
    SDL_UnlockSurface(dest);
    SDL_UnlockSurface(inFont);
}

void gfx_font_print_char(SDL_Surface* dest,int16_t inX, int16_t inY, SDL_Surface* inFont, char inChar) 
{
    char tempStr[2] = 
    { 
		inChar, '\0' 
	};
    gfx_font_print(dest,inX, inY, inFont, tempStr);
}

void gfx_font_print_center(SDL_Surface* dest,int16_t inY, SDL_Surface* inFont, char* inString) 
{
    int16_t tempX = (dest->w - gfx_font_width(inFont, inString)) >> 1;
    gfx_font_print(dest,tempX, inY, inFont, inString);
}

void gfx_font_print_fromright(SDL_Surface* dest,int16_t inX, int16_t inY, SDL_Surface* inFont, char* inString) 
{
    int16_t tempX = inX - gfx_font_width(inFont, inString);
    gfx_font_print(dest,tempX, inY, inFont, inString);
}

SDL_Surface* gfx_tex_load_tga_from_array(uint8_t* buffer) 
{
    if(buffer == NULL)
        return NULL;

    uint8_t  tga_ident_size;
    uint8_t  tga_color_map_type;
    uint8_t  tga_image_type;
    uint16_t tga_color_map_start;
    uint16_t tga_color_map_length;
    uint8_t  tga_color_map_bpp;
    uint16_t tga_origin_x;
    uint16_t tga_origin_y;
    uint16_t tga_width;
    uint16_t tga_height;
    uint8_t  tga_bpp;
    uint8_t  tga_descriptor;

    tga_ident_size = buffer[0];
    tga_color_map_type = buffer[1];
    tga_image_type = buffer[2];
    tga_color_map_start = buffer[3] + (buffer[4] << 8);
    tga_color_map_length = buffer[5] + (buffer[6] << 8);
    tga_color_map_bpp = buffer[7];
    tga_origin_x = buffer[8] + (buffer[9] << 8);
    tga_origin_y = buffer[10] + (buffer[11] << 8);
    tga_width = buffer[12] + (buffer[13] << 8);
    tga_height = buffer[14] + (buffer[15] << 8);
    tga_bpp = buffer[16];
    tga_descriptor = buffer[17];

    uint32_t bufIndex = 18;

    SDL_Surface* tempTexture = SDL_CreateRGBSurface(SDL_SWSURFACE, tga_width, tga_height, 16, sdl_video.surf_screen->format->Rmask, sdl_video.surf_screen->format->Gmask, sdl_video.surf_screen->format->Bmask, sdl_video.surf_screen->format->Amask);
    if(tempTexture == NULL) 
    {
        return NULL;
    }

    int upsideDown = (tga_descriptor & 0x20) > 0;

    uintptr_t i;
    uintptr_t iNew;
    uint8_t tempColor[3];
    SDL_LockSurface(tempTexture);
    uint16_t* tempTexPtr = tempTexture->pixels;
    for(i = 0; i < (tga_width * tga_height); i++) {
        tempColor[2] = buffer[bufIndex + 0];
        tempColor[1] = buffer[bufIndex + 1];
        tempColor[0] = buffer[bufIndex + 2];
        bufIndex += 3;

        if (upsideDown)
            iNew = i;
        else
            iNew = (tga_height - 1 - (i / tga_width)) * tga_width + i % tga_width;

        tempTexPtr[iNew] = SDL_MapRGB(sdl_video.surf_screen->format,tempColor[0], tempColor[1], tempColor[2]);
    }
    SDL_UnlockSurface(tempTexture);
    return tempTexture;
}
