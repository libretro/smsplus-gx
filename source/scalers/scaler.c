
#include "scaler.h"

/* Not*/

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

/* alekmaul's scaler taken from mame4all */
void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t* restrict src, uint16_t* restrict dst)
{
    uint32_t W,H,ix,iy,x,y;
    x=startx<<16;
    y=starty<<16;
    W=newwidth;
    H=newheight;
    ix=(viswidth<<16)/W;
    iy=(visheight<<16)/H;

    do 
    {
        uint16_t* restrict buffer_mem=&src[(y>>16)*pitchsrc];
        W=newwidth; x=startx<<16;
        do 
        {
            *dst++=buffer_mem[x>>16];
            x+=ix;
        } while (--W);
        dst+=pitchdest;
        y+=iy;
    } while (--H);
}

/*
    Upscale 160x144 -> 320x240
    Horizontal upscale:
        320/160=2  --  simple doubling of pixels
        [ab][cd] -> [aa][bb][cc][dd]
    Vertical upscale:
        Bresenham algo with simple interpolation
    Parameters:
        uint32_t *dst - pointer to 320x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer

    SMS has 256x192 screen, but GG uses 160x144 window centered at x=48, y=24
*/
void upscale_160x144_to_320x240(uint32_t* restrict dst, uint32_t* restrict src)
{
    uint32_t midh = 240 / 2;
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t i, j;

    for (i = 0; i < 240; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 320/8; j++)
        {
            uint32_t a, b, c, d, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]);
                cd = AVERAGE(cd, src[source+256/2+1]);
            }

            a = (ab & 0xFFFF) | (ab << 16);
            b = (ab & 0xFFFF0000) | (ab >> 16);
            c = (cd & 0xFFFF) | (cd << 16);
            d = (cd & 0xFFFF0000) | (cd >> 16);

            *dst++ = a;
            *dst++ = b;
            *dst++ = c;
            *dst++ = d;

            source += 2;

        }

        Eh += 144; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}

/*
    Upscale 256x192 -> 320x240

    Horizontal upscale:
        320/256=1.25  --  do some horizontal interpolation
        8p -> 10p
        4dw -> 5dw

        coarse interpolation:
        [ab][cd][ef][gh] -> [ab][(bc)c][de][f(fg)][gh]

        fine interpolation
        [ab][cd][ef][gh] -> [a(0.25a+0.75b)][(0.5b+0.5c)(0.75c+0.25d)][de][(0.25e+0.75f)(0.5f+0.5g)][(0.75g+0.25h)h]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 320x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
*/

void upscale_SMS_to_320x240(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height_scale)
{
    uint32_t midh = 240 / 2;
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t y, x;

    for (y = 0; y < 240; y++)
    {
        source = dh * 256 / 2;

        for (x = 0; x < 320/10; x++)
        {
            register uint32_t ab, cd, ef, gh;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
                ef = AVERAGE(ef, src[source + 256/2 + 2]) & 0xF7DEF7DE; // to prevent overflow
                gh = AVERAGE(gh, src[source + 256/2 + 3]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = ab;
            *dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + (cd << 16);
            *dst++  = (cd >> 16) + (ef << 16);
            *dst++  = (ef >> 16) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
            *dst++  = gh;

            source += 4;

        }
        Eh += height_scale; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}


/* Retro Arcade Mini scalers. Boo, ugly */

/* Game Gear*/

void upscale_160x144_to_320x272_for_480x272(uint32_t* restrict dst, uint32_t* restrict src)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t i, j;

    dst += (480-320)/4; // center correction for 480x272 mode

    for (i = 0; i < 272; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 320/8; j++)
        {
            uint32_t a, b, c, d, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

			/*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]);
                cd = AVERAGE(cd, src[source+256/2+1]);
            }*/

            a = (ab & 0xFFFF) | (ab << 16);
            b = (ab & 0xFFFF0000) | (ab >> 16);
            c = (cd & 0xFFFF) | (cd << 16);
            d = (cd & 0xFFFF0000) | (cd >> 16);

            *dst++ = a;
            *dst++ = b;
            *dst++ = c;
            *dst++ = d;

            source += 2;

        }
        dst += (480-320)/2; // pitch correction for 480x272 mode
        Eh += 144; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}


void upscale_160x144_to_480x272(uint32_t* restrict dst, uint32_t* restrict src)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t i, j;

    for (i = 0; i < 272; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 480/12; j++)
        {
            uint32_t a, b, c, d, e, f, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

			/*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]);
                cd = AVERAGE(cd, src[source+256/2+1]);
            }*/

            a = (ab & 0xFFFF) | (ab << 16);
            b = ab;
            c = (ab & 0xFFFF0000) | (ab >> 16);
            d = (cd & 0xFFFF) | (cd << 16);
            e = cd;
            f = (cd & 0xFFFF0000) | (cd >> 16);

            *dst++ = a;
            *dst++ = b;
            *dst++ = c;
            *dst++ = d;
            *dst++ = e;
            *dst++ = f;

            source += 2;

        }
        Eh += 144; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}

void upscale_256xXXX_to_384x272_for_480x272(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t y, x;

    dst += (480 - 384) / 4;

    for (y = 0; y < 272; y++)
    {
        source = dh * 256 / 2;

        for (x = 0; x < 384/6; x++)
        {
            register uint32_t ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            /*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
            }*/

            *dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
            *dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
            *dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

            source += 2;

        }
        dst += (480 - 384) / 2; 
        Eh += height; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}

void upscale_256xXXX_to_480x272(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t y, x;

    for (y = 0; y < 272; y++)
    {
        source = dh * 256 / 2;

        for (x = 0; x < 480/30; x++)
        {
            register uint32_t ab, cd, ef, gh, ij, kl, mn, op;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;
            ij = src[source + 4] & 0xF7DEF7DE;
            kl = src[source + 5] & 0xF7DEF7DE;
            mn = src[source + 6] & 0xF7DEF7DE;
            op = src[source + 7] & 0xF7DEF7DE;

            /*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE;
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE;
                ef = AVERAGE(ef, src[source + 256/2 + 2]) & 0xF7DEF7DE;
                gh = AVERAGE(gh, src[source + 256/2 + 3]) & 0xF7DEF7DE;
                ij = AVERAGE(ij, src[source + 256/2 + 4]) & 0xF7DEF7DE;
                kl = AVERAGE(kl, src[source + 256/2 + 5]) & 0xF7DEF7DE;
                mn = AVERAGE(mn, src[source + 256/2 + 6]) & 0xF7DEF7DE;
                op = AVERAGE(op, src[source + 256/2 + 7]) & 0xF7DEF7DE;
            }*/

            *dst++ = (ab & 0xFFFF) + (ab << 16);            // [aa]
            *dst++ = (ab >> 16) + (ab & 0xFFFF0000);        // [bb]
            *dst++ = (cd & 0xFFFF) + (cd << 16);            // [cc]
            *dst++ = (cd >> 16) + (((cd & 0xF7DE0000) >> 1) + ((ef & 0xF7DE) << 15)); // [d(de)]
            *dst++ = ef;                                    // [ef]
            *dst++ = (ef >> 16) + (gh << 16);               // [fg]
            *dst++ = gh;                                    // [gh]
            *dst++ = (gh >> 16) + (ij << 16);               // [hi]
            *dst++ = ij;                                    // [ij]
            *dst++ = (ij >> 16) + (kl << 16);               // [jk]
            *dst++ = kl;                                    // [kl]
            *dst++ = (((kl & 0xF7DE0000) >> 17) + ((mn & 0xF7DE) >> 1)) + (mn << 16); // [(lm)m]
            *dst++ = (mn >> 16) + (mn & 0xFFFF0000);        // [nn]
            *dst++ = (op & 0xFFFF) + (op << 16);            // [oo]
            *dst++ = (op >> 16) + (op & 0xFFFF0000);        // [pp]

            source += 8;
        }
        Eh += height; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}

/* PAP Gameta II scalers */
/* Same as Arcade Mini ones but height is higher */

void upscale_160x144_to_320x320_for_480x320(uint32_t* restrict dst, uint32_t* restrict src)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t i, j;

    dst += (480-320)/4; // center correction for 480x320 mode

    for (i = 0; i < 320; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 320/8; j++)
        {
            uint32_t a, b, c, d, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

			/*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]);
                cd = AVERAGE(cd, src[source+256/2+1]);
            }*/

            a = (ab & 0xFFFF) | (ab << 16);
            b = (ab & 0xFFFF0000) | (ab >> 16);
            c = (cd & 0xFFFF) | (cd << 16);
            d = (cd & 0xFFFF0000) | (cd >> 16);

            *dst++ = a;
            *dst++ = b;
            *dst++ = c;
            *dst++ = d;

            source += 2;

        }
        dst += (480-320)/2; // pitch correction for 480x320 mode
        Eh += 144; if(Eh >= 320) { Eh -= 320; dh++; }
    }
}

void upscale_160x144_to_480x320(uint32_t* restrict dst, uint32_t* restrict src)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t i, j;

    for (i = 0; i < 320; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 480/12; j++)
        {
            uint32_t a, b, c, d, e, f, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

			/*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]);
                cd = AVERAGE(cd, src[source+256/2+1]);
            }*/

            a = (ab & 0xFFFF) | (ab << 16);
            b = ab;
            c = (ab & 0xFFFF0000) | (ab >> 16);
            d = (cd & 0xFFFF) | (cd << 16);
            e = cd;
            f = (cd & 0xFFFF0000) | (cd >> 16);

            *dst++ = a;
            *dst++ = b;
            *dst++ = c;
            *dst++ = d;
            *dst++ = e;
            *dst++ = f;

            source += 2;

        }
        Eh += 144; if(Eh >= 320) { Eh -= 320; dh++; }
    }
}

void upscale_256xXXX_to_384x320_for_480x320(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t y, x;

    dst += (480 - 384) / 4;

    for (y = 0; y < 320; y++)
    {
        source = dh * 256 / 2;

        for (x = 0; x < 384/6; x++)
        {
            register uint32_t ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            /*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
            }*/

            *dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
            *dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
            *dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

            source += 2;

        }
        dst += (480 - 384) / 2; 
        Eh += height; if(Eh >= 320) { Eh -= 320; dh++; }
    }
}

void upscale_256xXXX_to_480x320(uint32_t* restrict dst, uint32_t* restrict src, uint32_t height)
{
    uint32_t Eh = 0;
    uint32_t source = 0;
    uint32_t dh = 0;
    uint32_t y, x;

    for (y = 0; y < 320; y++)
    {
        source = dh * 256 / 2;

        for (x = 0; x < 480/30; x++)
        {
            register uint32_t ab, cd, ef, gh, ij, kl, mn, op;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;
            ef = src[source + 2] & 0xF7DEF7DE;
            gh = src[source + 3] & 0xF7DEF7DE;
            ij = src[source + 4] & 0xF7DEF7DE;
            kl = src[source + 5] & 0xF7DEF7DE;
            mn = src[source + 6] & 0xF7DEF7DE;
            op = src[source + 7] & 0xF7DEF7DE;

            /*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE;
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE;
                ef = AVERAGE(ef, src[source + 256/2 + 2]) & 0xF7DEF7DE;
                gh = AVERAGE(gh, src[source + 256/2 + 3]) & 0xF7DEF7DE;
                ij = AVERAGE(ij, src[source + 256/2 + 4]) & 0xF7DEF7DE;
                kl = AVERAGE(kl, src[source + 256/2 + 5]) & 0xF7DEF7DE;
                mn = AVERAGE(mn, src[source + 256/2 + 6]) & 0xF7DEF7DE;
                op = AVERAGE(op, src[source + 256/2 + 7]) & 0xF7DEF7DE;
            }*/

            *dst++ = (ab & 0xFFFF) + (ab << 16);            // [aa]
            *dst++ = (ab >> 16) + (ab & 0xFFFF0000);        // [bb]
            *dst++ = (cd & 0xFFFF) + (cd << 16);            // [cc]
            *dst++ = (cd >> 16) + (((cd & 0xF7DE0000) >> 1) + ((ef & 0xF7DE) << 15)); // [d(de)]
            *dst++ = ef;                                    // [ef]
            *dst++ = (ef >> 16) + (gh << 16);               // [fg]
            *dst++ = gh;                                    // [gh]
            *dst++ = (gh >> 16) + (ij << 16);               // [hi]
            *dst++ = ij;                                    // [ij]
            *dst++ = (ij >> 16) + (kl << 16);               // [jk]
            *dst++ = kl;                                    // [kl]
            *dst++ = (((kl & 0xF7DE0000) >> 17) + ((mn & 0xF7DE) >> 1)) + (mn << 16); // [(lm)m]
            *dst++ = (mn >> 16) + (mn & 0xFFFF0000);        // [nn]
            *dst++ = (op & 0xFFFF) + (op << 16);            // [oo]
            *dst++ = (op >> 16) + (op & 0xFFFF0000);        // [pp]

            source += 8;
        }
        Eh += height; if(Eh >= 320) { Eh -= 320; dh++; }
    }
}
