
#include "scaler.h"

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

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
void upscale_160x144_to_320x240(uint32_t *dst, uint32_t *src)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int i, j;

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

void upscale_160x144_to_320x480(uint32_t *dst, uint32_t *src)
{
    int midh = 480 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int i, j;

    for (i = 0; i < 480; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 320/8; j++)
        {
            uint32_t a, b, c, d, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

           /* if(Eh >= midh) {
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

        Eh += 144; if(Eh >= 480) { Eh -= 480; dh++; }
    }
}


/*
    Upscale 160x144 -> 320x240 for 400x240 mode
    Horizontal upscale:
        320/160=2  --  simple doubling of pixels
        [ab][cd] -> [aa][bb][cc][dd]
    Vertical upscale:
        Bresenham algo with simple interpolation
    Parameters:
        uint32_t *dst - pointer to 400x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction

    SMS has 256x192 screen, but GG uses 160x144 window centered at x=48, y=24
*/
void upscale_160x144_to_320x240_for_400x240(uint32_t *dst, uint32_t *src)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int i, j;

    dst += (400-320)/4; // center correction for 400x240 mode

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
        dst += (400-320)/2;
        Eh += 144; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}



/*
    Upscale 160x144 -> 400x240
    Horizontal upscale:
        400/160=2.5  --  do some horizontal interpolation
        [ab][cd] -> [aa][(ab)b][bc][c(cd)][dd]
    Vertical upscale:
        Bresenham algo with simple interpolation
    Parameters:
        uint32_t *dst - pointer to 400x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer

    SMS has 256x192 screen, but GG uses 160x144 window centered at x=48, y=24
*/
void upscale_160x144_to_400x240(uint32_t *dst, uint32_t *src)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int i, j;

    for (i = 0; i < 240; i++)
    {
        source = dh * 256 / 2;

        for (j = 0; j < 400/10; j++)
        {
            uint32_t a, b, c, d, e, ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source+256/2+1]) & 0xF7DEF7DE; // to prevent overflow
            }

            a = (ab & 0xFFFF) | (ab << 16);
            b = (((ab & 0xFFFF) >> 1) + ((ab & 0xFFFF0000) >> 17)) | (ab & 0xFFFF0000);
            c = (ab >> 16) | (cd << 16);
            d = (cd & 0xFFFF) | (((cd & 0xFFFF) << 15) + ((cd & 0xFFFF0000) >> 1));
            e = (cd >> 16) | (cd & 0xFFFF0000);

            *dst++ = a;
            *dst++ = b;
            *dst++ = c;
            *dst++ = d;
            *dst++ = e;

            source += 2;

        }
        Eh += 144; if(Eh >= 240) { Eh -= 240; dh++; } // 144 - real gb y size
    }
}

/*
    Upscale 160x144 -> 320x272 for 480x272 mode
    Horizontal upscale:
        320/160=2  --  simple doubling of pixels
        [ab][cd] -> [aa][bb][cc][dd]
    Vertical upscale:
        Bresenham algo with simple interpolation
    Parameters:
        uint32_t *dst - pointer to 480x272x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction

    SMS has 256x192 screen, but GG uses 160x144 window centered at x=48, y=24
*/
void upscale_160x144_to_320x272_for_480x272(uint32_t *dst, uint32_t *src)
{
    int midh = 272 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int i, j;

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
        dst += (480-320)/2; // pitch correction for 480x272 mode
        Eh += 144; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}

/*
    Upscale 160x144 -> 480x272
    Horizontal upscale
        480/160=3  --  simple tripling of pixels
        [ab][cd] -> [aa][ab][bb][cc][cd][dd]
    Vertical upscale:
        Bresenham algo with simple interpolation
    Parameters:
        uint32_t *dst - pointer to 480x272x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer

    SMS has 256x192 screen, but GG uses 160x144 window centered at x=48, y=24
*/
void upscale_160x144_to_480x272(uint32_t *dst, uint32_t *src)
{
    int midh = 272 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int i, j;

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

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source+256/2]);
                cd = AVERAGE(cd, src[source+256/2+1]);
            }

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

void upscale_256x192_to_320x480(uint32_t *dst, uint32_t *src)
{
    int midh = 480 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

    for (y = 0; y < 480; y++)
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

            /*if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
                ef = AVERAGE(ef, src[source + 256/2 + 2]) & 0xF7DEF7DE; // to prevent overflow
                gh = AVERAGE(gh, src[source + 256/2 + 3]) & 0xF7DEF7DE; // to prevent overflow
            }*/

            *dst++ = ab;
            *dst++  = ((ab >> 17) + ((cd & 0xFFFF) >> 1)) + (cd << 16);
            *dst++  = (cd >> 16) + (ef << 16);
            *dst++  = (ef >> 16) + (((ef & 0xFFFF0000) >> 1) + ((gh & 0xFFFF) << 15));
            *dst++  = gh;

            source += 4;

        }
        /* 190 rather than 192 because we're removing the black bars */
        Eh += 190; if(Eh >= 480) { Eh -= 480; dh++; }
    }
}

void upscale_256x192_to_320x240(uint32_t *dst, uint32_t *src)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

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
        Eh += 190; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}

/*
    Upscale 256x192 -> 384x240 (for 400x240)

    Horizontal interpolation
        384/256=1.5
        4p -> 6p
        2dw -> 3dw

        for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
        [ab][cd] => [a(ab)][bc][(cd)d]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 400x240x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction is made
*/

void upscale_256x192_to_384x240_for_400x240(uint32_t *dst, uint32_t *src)
{
    int midh = 240 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

    dst += (400 - 384) / 4;

    for (y = 0; y < 240; y++)
    {
        source = dh * 256 / 2;

        for (x = 0; x < 384/6; x++)
        {
            register uint32_t ab, cd;

            __builtin_prefetch(dst + 4, 1);
            __builtin_prefetch(src + source + 4, 0);

            ab = src[source] & 0xF7DEF7DE;
            cd = src[source + 1] & 0xF7DEF7DE;

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
            *dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
            *dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

            source += 2;

        }
        dst += (400 - 384) / 2; 
        Eh += 192; if(Eh >= 240) { Eh -= 240; dh++; }
    }
}

/*
    Upscale 256x192 -> 384x272 (for 480x240)

    Horizontal interpolation
        384/256=1.5
        4p -> 6p
        2dw -> 3dw

        for each line: 4 pixels => 6 pixels (*1.5) (64 blocks)
        [ab][cd] => [a(ab)][bc][(cd)d]

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 480x272x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
        pitch correction is made
*/

void upscale_256x192_to_384x272_for_480x272(uint32_t *dst, uint32_t *src)
{
    int midh = 272 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

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

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE; // to prevent overflow
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE; // to prevent overflow
            }

            *dst++ = (ab & 0xFFFF) + AVERAGEHI(ab);
            *dst++ = (ab >> 16) + ((cd & 0xFFFF) << 16);
            *dst++ = (cd & 0xFFFF0000) + AVERAGELO(cd);

            source += 2;

        }
        dst += (480 - 384) / 2; 
        Eh += 192; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}
/*
    Upscale 256x192 -> 480x272

    Horizontal interpolation
        480/256=1.875
        16p -> 30p
        8dw -> 15dw

        For each line 16 pixels => 30 pixels (*1.875) (32 blocks)
        Coarse:
            [ab][cd][ef][gh][ij][kl][mn][op]
                =>
            [aa][bb][cc][d(de)][ef][fg][gh][hi][ij][jk][kl][(lm)m][nn][oo][pp]
        Fine:
            ab` = a, (0.875a + 0.125b)
            cd` = b, (0.75b  + 0.25c)
            ef` = c, (0.625c + 0.375d)
            gh` = d, (0.5d   + 0.5e)
            ij` = e, (0.375e + 0.625f)
            kl` = f, (0.25f  + 0.75g)
            mn` = g, (0.125g + 0.875h)
            op` = h, i
            qr` = (0.875i + 0.125j), j
            st` = (0.75j  + 0.25k),  k
            uv` = (0.625k + 0.375l), l
            wx` = (0.5l   + 0.5m),   m
            yz` = (0.375m + 0.625n), n
            12` = (0.25n  + 0.75o),  o
            34` = (0.125o + 0.875p), p

    Vertical upscale:
        Bresenham algo with simple interpolation

    Parameters:
        uint32_t *dst - pointer to 480x272x16bpp buffer
        uint32_t *src - pointer to 256x192x16bpp buffer
*/

void upscale_256x192_to_480x272(uint32_t *dst, uint32_t *src)
{
    int midh = 272 / 2;
    int Eh = 0;
    int source = 0;
    int dh = 0;
    int y, x;

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

            if(Eh >= midh) {
                ab = AVERAGE(ab, src[source + 256/2]) & 0xF7DEF7DE;
                cd = AVERAGE(cd, src[source + 256/2 + 1]) & 0xF7DEF7DE;
                ef = AVERAGE(ef, src[source + 256/2 + 2]) & 0xF7DEF7DE;
                gh = AVERAGE(gh, src[source + 256/2 + 3]) & 0xF7DEF7DE;
                ij = AVERAGE(ij, src[source + 256/2 + 4]) & 0xF7DEF7DE;
                kl = AVERAGE(kl, src[source + 256/2 + 5]) & 0xF7DEF7DE;
                mn = AVERAGE(mn, src[source + 256/2 + 6]) & 0xF7DEF7DE;
                op = AVERAGE(op, src[source + 256/2 + 7]) & 0xF7DEF7DE;
            }

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
        Eh += 192; if(Eh >= 272) { Eh -= 272; dh++; }
    }
}
