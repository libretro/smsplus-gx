/*
    pygame - Python Game Library
    Copyright (C) 2000-2001  Pete Shinners
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301  USA
    Pete Shinners
    pete@shinners.org
*/

/*
   This implements the AdvanceMAME Scale2x feature found on this page,
   http://advancemame.sourceforge.net/scale2x.html
   It is an incredibly simple and powerful image doubling routine that does
   an astonishing job of doubling game graphic data while interpolating out
   the jaggies. Congrats to the AdvanceMAME team, I'm very impressed and
   surprised with this code!
*/

/* https://github.com/CTPUG/pygame_cffi/blob/master/cffi_builders/lib/scale2x.c */
#include <SDL/SDL.h>
#include "scale2x.h"

#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))

/* This requires a destination surface already setup to be twice as large as the source.
 * Only for 16-bits */

void scale2x(uint16_t* restrict srcpixels, uint16_t* restrict dstpixels, const int32_t srcpitch, const int32_t dstpitch, const int32_t width, const int32_t height)
{
    int32_t looph, loopw;
    uint8_t* restrict srcpix = (uint8_t* restrict)srcpixels;
    uint8_t* restrict dstpix = (uint8_t* restrict)dstpixels;
	uint16_t E0, E1, E2, E3, B, D, E, F, H;
	for(looph = 0; looph < height; ++looph)
	{
		for(loopw = 0; loopw < width; ++loopw)
		{
			B = *(uint16_t* restrict)(srcpix + (MAX(0,looph-1)*srcpitch) + (2*loopw));
			D = *(uint16_t* restrict)(srcpix + (looph*srcpitch) + (2*MAX(0,loopw-1)));
			E = *(uint16_t* restrict)(srcpix + (looph*srcpitch) + (2*loopw));
			F = *(uint16_t* restrict)(srcpix + (looph*srcpitch) + (2*MIN(width-1,loopw+1)));
			H = *(uint16_t* restrict)(srcpix + (MIN(height-1,looph+1)*srcpitch) + (2*loopw));

			E0 = D == B && B != F && D != H ? D : E;
			E1 = B == F && B != D && F != H ? F : E;
			E2 = D == H && D != B && H != F ? D : E;
			E3 = H == F && D != H && B != F ? F : E;

			*(uint16_t* restrict)(dstpix + looph*2*dstpitch + loopw*2*2) = E0;
			*(uint16_t* restrict)(dstpix + looph*2*dstpitch + (loopw*2+1)*2) = E1;
			*(uint16_t* restrict)(dstpix + (looph*2+1)*dstpitch + loopw*2*2) = E2;
			*(uint16_t* restrict)(dstpix + (looph*2+1)*dstpitch + (loopw*2+1)*2) = E3;
		}
	}
}
