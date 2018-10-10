#include <mygba.h>

MULTIBOOT

#include "aa.h"
#include "ga.h"

#define SOUND

// EXPORTED VARIABLES ----------------------------------------------------

// MODULE VARIABLES ------------------------------------------------------

#define MAPBLOCKS (512)
MODULE unsigned short /* DATA_IN_EWRAM */ ArcadiaMap[MAPBLOCKS];
#define TILES 4096 // in words
MODULE unsigned short /* DATA_IN_EWRAM */ ArcadiaTiles[TILES];
MODULE UBYTE hamsprite[4],
             SpriteImagery[4][32];
MODULE ULONG paused  = FALSE;
MODULE int   whichgame;
// MODULE unsigned short* videoBuffer = (unsigned short*)0x6000000;
MODULE volatile unsigned short* ScanlineCounter = (volatile unsigned short*)0x4000006;

// IMPORTED VARIABLES ----------------------------------------------------

IMPORT UBYTE /* DATA_IN_EWRAM */ memory[MEMSIZE];
IMPORT int                       overcalc, // must be signed!
                                 slice;
IMPORT UBYTE                     psl,
                                 psu,
                                 r[7];
IMPORT UWORD                     iar,
                                 ras[8];

// MODULE ARRAYS ---------------------------------------------------------

MODULE FLAG   autofire = FALSE;
MODULE int    cpl,
              x,
              y,
              xx,
              yy;
MODULE FLAG   skip[4];
MODULE UBYTE  t,
              oldimagery,
              oldpitch = 0;
MODULE SLONG  spritex[4],
              spritey[4],
              row,
              column,
              whichsprite,
              imagedata;
MODULE ULONG  frames = 0;
MODULE const unsigned short SpritePalette[3][9] = { { // Super Bug 1
0x0000, // transparent
0x7FFF, // white  (%0,BBBBB,GGGGG,RRRRR)
0x03FF, // yellow (%0,00000,11111,11111)
0x7FE0, // cyan   (%0,11111,11111,00000)
0x03E0, // green  (%0,00000,11111,00000)
0x7C1F, // purple (%0,11111,00000,11111)
0x001F, // red    (%0,00000,00000,11111)
0x7C00, // blue   (%0,11111,00000,00000)
0x0000  // black  (%0,00000,00000,00000)
}, { // Super Bug 2
0x0000, // transparent
0x7FFF, // white  (%0,BBBBB,GGGGG,RRRRR)
0x03FF, // yellow (%0,00000,11111,11111)
0x7FE0, // cyan   (%0,11111,11111,00000)
0x03E0, // green  (%0,00000,11111,00000)
0x7C1F, // purple (%0,11111,00000,11111)
0x001F, // red    (%0,00000,00000,11111)
0x7C00, // blue   (%0,11111,00000,00000)
0x0000  // black  (%0,00000,00000,00000)
}, { // Capture
0x0000, // transparent
0x0000, // black  (%0,00000,00000,00000)
0x7C00, // blue   (%0,11111,00000,00000)
0x001F, // red    (%0,00000,00000,11111)
0x7C1F, // purple (%0,11111,00000,11111)
0x03E0, // green  (%0,00000,11111,00000)
0x7FE0, // cyan   (%0,11111,11111,00000)
0x03FF, // yellow (%0,00000,11111,11111)
0x7FFF  // white  (%0,BBBBB,GGGGG,RRRRR)
} };

MODULE UBYTE SpriteImagery[4][32]; /* = { {
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87
}, {
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87
}, {
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87
}, {
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87,
0x21, 0x43, 0x65, 0x87
} }; */

// MODULE FUNCTIONS ------------------------------------------------------

MODULE void drawsprites(void);
MODULE int sprite_collision(int first, int second);
MODULE __inline void collidesprites(void);
MODULE void DMAFastCopy(void* source, void* dest, unsigned int count, unsigned int mode);
MODULE void updatetiles(void);
MODULE void checkreset(void);

// CODE ------------------------------------------------------------------

int main(void)
{   ham_Init();
    ham_SetBgMode(0);
    REG_BG0CNT  = BG_CBB(0) | BG_SBB(31) | _BG_16C | _BG_REG_S32;
    REG_DISPCNT = _DCNT_MODE0 | DCNT_BG0_ON | OBJ_ENABLE | OBJ_MAP_1D;

    hamsprite[0] = ham_CreateObj(&SpriteImagery[0],0,0,OBJ_MODE_NORMAL,0,0,0,0,0,0,0,240,160);
    hamsprite[1] = ham_CreateObj(&SpriteImagery[1],0,0,OBJ_MODE_NORMAL,0,0,0,0,0,0,0,240,160);
    hamsprite[2] = ham_CreateObj(&SpriteImagery[2],0,0,OBJ_MODE_NORMAL,0,0,0,0,0,0,0,240,160);
    hamsprite[3] = ham_CreateObj(&SpriteImagery[3],0,0,OBJ_MODE_NORMAL,0,0,0,0,0,0,0,240,160);

#ifdef SOUND
    REG_SNDSTAT   = SSTAT_ON;
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR2_ON | SDMG_NOISE_ON, 7);
    REG_SNDDSCNT  = SDS_DMG100;
/* bits 15..12: volume     (15)
   bit  11:     direction  (1)
   bots 10..8:  step time  (0)
   bits 7..6:   duty cycle
   bits 5..0:   length */
    REG_SND2CNT   = 0xF080;
    REG_SND2FREQ  = 0;
    REG_SND4CNT   = 0xF080;
    REG_SND4FREQ  = 0;
#endif

    a_setmemmap();

    for (;;)
    {   if (paused)
        {   a_emuinput();
        } else
        {   uvi();
        }
        if (whichgame != CAPTURE)
        {   while (*ScanlineCounter < 160) { ; } // ie. WaitVBlank();
        }
        ham_CopyObjToOAM();
}   }

/* SOUND4CNT_H aka REG_SND4FREQ:
15:    reset                         1
14:    =0: loop, =1: play once       0
13..8: unused?                       0
 7..4: pre-scaler divider            10 (%1010)
 3:    =0: 15 stages, =1: 7 stages   0
 2..0: clock divider                 3  (%011)
SOUND4CNT_L aka REG_SND4CNT:
same as for channel 1 */

EXPORT void playsound(void)
{
#ifndef SOUND
    return;
#endif

    if
    (    ((memory[A_PITCH] ) &   127)
     &&  ((memory[A_VOLUME]) &     7)
    )
    {   switch ((memory[A_VOLUME]) & 0x18)
        {
        case 0x08: // tone only
            REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR2_ON                , memory[A_VOLUME] & 7);
            REG_SND2FREQ  = SFREQ_RESET | (2048 - (((memory[A_PITCH] & 127) + 1) * 17));
        acase 0x10: // noise only
            REG_SNDDMGCNT = SDMG_BUILD_LR(               SDMG_NOISE_ON, memory[A_VOLUME] & 7);
            REG_SND4FREQ  = 0x80A3;
/* Authentic noise frequency for PITCH of $60 is 81.17526Hz.
   Our approximation is 85.3' Hz. */
        acase 0x18: // both
            REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR2_ON | SDMG_NOISE_ON, memory[A_VOLUME] & 7);
            REG_SND2FREQ  = SFREQ_RESET | (2048 - (((memory[A_PITCH] & 127) + 1) * 17));
            REG_SND4FREQ  = 0x80A3;
        acase 0x00: // neither
            REG_SNDDMGCNT = SDMG_BUILD_LR(0                           , 0                   );
    }   }
    else
    {    REG_SNDDMGCNT    = SDMG_BUILD_LR(0                           , 0                   );
}   }

EXPORT void uvi(void)
{   psu &= ~(PSU_S);
    memory[A_CHARLINE] =
    memory[A_P1PADDLE] =       // "The potentiometer is set to $FF
    memory[A_P2PADDLE] = 0xFF; // on the trailing edge of VRST."

    slice = (cpl * 207) - overcalc;
    cpu(); // rasters 0..206
    for (row = 0; row < 13; row++)
    {   for (column = 0; column < 16; column++)
        {   ArcadiaMap[((row + 3) * 32) + 7 + column] = memory[0x1800 + (row * 16) + column];
    }   }
    a_emuinput();
    slice = (cpl *  33) - overcalc;
    cpu(); // rasters 207..239
    psu |= PSU_S;
    slice = (cpl *  22) - overcalc;
    cpu(); // rasters 240..261
    // rastline is 0 (262), Sense bit is set, CHARLINE is $FD

    // cache sprite variables
    spritex[0] =               memory[A_SPRITE0X] - 27;
    spritey[0] = (SWORD) 256 - memory[A_SPRITE0Y];
    spritex[1] =               memory[A_SPRITE1X] - 27;
    spritey[1] = (SWORD) 256 - memory[A_SPRITE1Y];
    spritex[2] =               memory[A_SPRITE2X] - 27;
    spritey[2] = (SWORD) 256 - memory[A_SPRITE2Y];
    spritex[3] =               memory[A_SPRITE3X] - 27;
    spritey[3] = (SWORD) 256 - memory[A_SPRITE3Y];
    if (whichgame == CAPTURE)
    {   spritey[0] += 6;
        spritey[1] += 6;
        spritey[2] += 6;
        spritey[3] += 6;
    }
    for (whichsprite = 0; whichsprite < 4; whichsprite++)
    {   // check sprite position
        if (memory[A_SPRITE0X + (2 * whichsprite)] > 227)
        {   // "If the HC is set to >227, the object is effectively
            // removed from the video field."
            skip[whichsprite] = TRUE;
        } else
        {   skip[whichsprite] = FALSE;
    }   }
    memory[A_BGCOLLIDE] = 0xFF;

    if (memory[0x19BF] != oldimagery)
    {   oldimagery = memory[0x19BF];
        updatetiles();
    }

    drawsprites();
    if (whichgame != CAPTURE)
    {   collidesprites(); // must not be done at SENSELINE, eg. ROBOTKIL, ESCAPE
    }
    psu &= ~(PSU_S); // clear Sense bit. Necessary, eg. CIRCUS
    memory[A_CHARLINE] = 0xFF;

    // Load map into SBB 31
    DMAFastCopy(ArcadiaMap, &se_mem[31][0], MAPBLOCKS, DMA_16NOW);

    if (memory[A_PITCH] != oldpitch)
    {   oldpitch = memory[A_PITCH];
        playsound();
    }

    checkreset();
}

MODULE __inline void collidesprites(void)
{   /* This doesn't use screen[], thus it is safe at any point in the frame. */

    t = 0xFF;
    if (sprite_collision(0, 1))
    {   t &=  ~1; // AND with %11111110 (clear bit 0)
    }
    if (sprite_collision(0, 2))
    {   t &=  ~2; // AND with %11111101 (clear bit 1)
    }
    if (sprite_collision(0, 3))
    {   t &=  ~4; // AND with %11111011 (clear bit 2)
    }
    if (sprite_collision(1, 2))
    {   t &=  ~8; // AND with %11110111 (clear bit 3)
    }
    if (sprite_collision(1, 3))
    {   t &= ~16; // AND with %11101111 (clear bit 4)
    }
    if (sprite_collision(2, 3))
    {   t &= ~32; // AND with %11011111 (clear bit 5)
    }
    memory[A_SPRITECOLLIDE] = t;
}

MODULE void drawsprites(void)
{   PERSIST int sprcolour[4]; // PERSISTent for speed

    sprcolour[0] = 7 - ((memory[A_SPRITES01CTRL] & 0x38) >> 3); // %00111000 -> %00000111
    sprcolour[1] = 7 - ( memory[A_SPRITES01CTRL] & 0x07      ); // %00000111 -> %00000111
    sprcolour[2] = 7 - ((memory[A_SPRITES23CTRL] & 0x38) >> 3); // %00111000 -> %00000111
    sprcolour[3] = 7 - ( memory[A_SPRITES23CTRL] & 0x07      ); // %00000111 -> %00000111

    for (whichsprite = 0; whichsprite < 4; whichsprite++)
    {   x = spritex[whichsprite];
        y = spritey[whichsprite];
        if (memory[A_SPRITE0Y + (whichsprite << 1)] == 0)
        // Capture can put the sprite at any X-position, so we don't check X-position
        {   ham_SetObjX(hamsprite[whichsprite], 240);
            ham_SetObjY(hamsprite[whichsprite], 160);
            continue;
        } else
        {   ham_SetObjX(hamsprite[whichsprite], x + 40);
            ham_SetObjY(hamsprite[whichsprite], (y / 2) + 12);
            SpriteImagery[whichsprite][(yy * 4) + (xx / 2)] &= 0x0F;
        }
        
        for (yy = 0; yy < 8; yy++) // for each y-line of the sprite
        {   y = spritey[whichsprite] + yy;
            imagedata = memory[0x1980 + (whichsprite * 8) + yy];
            SpriteImagery[whichsprite][(yy * 4)    ] =
            SpriteImagery[whichsprite][(yy * 4) + 1] =
            SpriteImagery[whichsprite][(yy * 4) + 2] =
            SpriteImagery[whichsprite][(yy * 4) + 3] = 0x0;

            for (xx = 0; xx < 8; xx++)
            {   if (imagedata & (0x80 >> xx)) // if this x-bit is set
                {   x = spritex[whichsprite] + xx;
                    if (xx & 1)
                    {   SpriteImagery[whichsprite][(yy * 4) + (xx / 2)] |= (sprcolour[whichsprite] + 1) << 4;
                    } else
                    {   SpriteImagery[whichsprite][(yy * 4) + (xx / 2)] |= (sprcolour[whichsprite] + 1);
                    }
        }   }   }

        ham_UpdateObjGfx(hamsprite[whichsprite], &SpriteImagery[whichsprite]);
}   }

EXPORT void a_emuinput(void)
{   PERSIST FLAG fresh_b  = TRUE,
                 fresh_r  = TRUE;

    if (KEY_DOWN_NOW(KEY_R)) // pause
    {   if (fresh_r)
        {    if (paused)
             {   paused = FALSE;
                 playsound();
                 ArcadiaMap[32 + 10] =
                 ArcadiaMap[32 + 12] =
                 ArcadiaMap[32 + 13] =
                 ArcadiaMap[32 + 14] =
                 ArcadiaMap[32 + 15] =
                 ArcadiaMap[32 + 16] =
                 ArcadiaMap[32 + 17] =
                 ArcadiaMap[32 + 19] = 0;
             } else
             {   paused = TRUE;
                 REG_SNDDMGCNT = SDMG_BUILD_LR(0, 0);
                 if (memory[A_BGCOLOUR] & 0x8)
                 {   ArcadiaMap[32 + 10] =
                     ArcadiaMap[32 + 19] = 0x03; // #
                     ArcadiaMap[32 + 12] = 0x29; // P
                     ArcadiaMap[32 + 13] = 0x1A; // A
                     ArcadiaMap[32 + 14] = 0x2E; // U
                     ArcadiaMap[32 + 15] = 0x2C; // S
                     ArcadiaMap[32 + 16] = 0x1E; // E
                     ArcadiaMap[32 + 17] = 0x1D; // D
                 } else
                 {   ArcadiaMap[32 + 10] =
                     ArcadiaMap[32 + 19] = 0xC3; // #
                     ArcadiaMap[32 + 12] = 0xE9; // P
                     ArcadiaMap[32 + 13] = 0xDA; // A
                     ArcadiaMap[32 + 14] = 0xEE; // U
                     ArcadiaMap[32 + 15] = 0xEC; // S
                     ArcadiaMap[32 + 16] = 0xDE; // E
                     ArcadiaMap[32 + 17] = 0xDD; // D
        }    }   }
        fresh_r = FALSE;
    } else
    {   fresh_r = TRUE;
    }

    if (paused)
    {   checkreset();
        return;
    }
    
    t = 0;
    if (KEY_DOWN_NOW(KEY_START))
    {   t |= 1;
    }
    if (KEY_DOWN_NOW(KEY_SELECT))
    {   t |= 2;
    }
    memory[A_CONSOLE] = t;

    if (KEY_DOWN_NOW(KEY_B)) // pause
    {   if (fresh_b)
        {    if (autofire)
             {   autofire = FALSE;
             } elif (whichgame != CAPTURE)
             {   autofire = TRUE;
        }    }
        fresh_b = FALSE;
    } else
    {   fresh_b = TRUE;
    }

    if (autofire && whichgame != CAPTURE)
    {   if (frames & 1)
        {   memory[A_P1MIDDLEKEYS] = 0;
        } else
        {   memory[A_P1MIDDLEKEYS] = 8;
    }   }
    else
    {   if (KEY_DOWN_NOW(KEY_A) || (whichgame == CAPTURE && KEY_DOWN_NOW(KEY_B)))
        {   memory[A_P1MIDDLEKEYS] =
            memory[A_P2MIDDLEKEYS] = 8;
            // videoBuffer[((150) * 240) + 238] = 0x7FFF;
        } else
        {   memory[A_P1MIDDLEKEYS] =
            memory[A_P2MIDDLEKEYS] = 0;
            // videoBuffer[((150) * 240) + 238] = 0;
    }   }

    if (whichgame != CAPTURE)
    {   if (memory[A_BGCOLOUR] & 0x40) // paddle interpolation bit
        {   if (KEY_DOWN_NOW(KEY_LEFT))
            {   memory[A_P1PADDLE] = 1;
            } elif (KEY_DOWN_NOW(KEY_RIGHT))
            {   memory[A_P1PADDLE] = 254;
            } else
            {   memory[A_P1PADDLE] = 112;
        }   }
        else
        {   if (KEY_DOWN_NOW(KEY_UP))
            {   memory[A_P1PADDLE] = 1;
            } elif (KEY_DOWN_NOW(KEY_DOWN))
            {   memory[A_P1PADDLE] = 254;
            } else
            {   memory[A_P1PADDLE] = 112;
    }   }   }
    else
    {   if (KEY_DOWN_NOW(KEY_LEFT )) memory[A_P1LEFTKEYS  ] = memory[A_P2LEFTKEYS ] = 2; else memory[A_P1LEFTKEYS ] = memory[A_P2LEFTKEYS ] = 0;
        if (KEY_DOWN_NOW(KEY_RIGHT)) memory[A_P1RIGHTKEYS ] = memory[A_P2RIGHTKEYS] = 2; else memory[A_P1RIGHTKEYS] = memory[A_P2RIGHTKEYS] = 0;
        if (KEY_DOWN_NOW(KEY_UP   )) { memory[A_P1MIDDLEKEYS] |= 4; memory[A_P2MIDDLEKEYS] |= 4; }
        if (KEY_DOWN_NOW(KEY_DOWN )) { memory[A_P1MIDDLEKEYS] |= 1; memory[A_P2MIDDLEKEYS] |= 1; }
}   }

MODULE int sprite_collision(int first, int second)
{   if
    (   skip[first]
     || skip[second]
    )
    {   return FALSE;
    }

    if (second == 3) // bullet
    {   if
        (   spritex[first] <= spritex[second] - 5
         || spritex[first] >= spritex[second] + 5
         || spritey[first] <= spritey[second] - 10
         || spritey[first] >= spritey[second] + 10
        )
        {   return FALSE;
        } else
        {   return TRUE;
    }   }
    else
    {   if
        (   spritex[first] <= spritex[second] - 8
         || spritex[first] >= spritex[second] + 8
         || spritey[first] <= spritey[second] - 16
         || spritey[first] >= spritey[second] + 16
        )
        {   return FALSE;
        } else
        {   return TRUE;
}   }   }

EXPORT void a_setmemmap(void)
{   int i;

    switch (whichgame)
    {
    case SUPERBUG1:
        for (i = 0; i <= 0xFFF; i++)
        {   memory[i] = a_superbug1a[i];
        }
        /* for (i = 0x1000; i <= 0x17FF; i++)
        {   memory[i] = 0;
        } */
        for (i = 0x0; i <= 0x2FF; i++)
        {   memory[0x1800 + i] = a_superbug1b[i];
        }
        /* for (i = 0x1B00; i <= 0x1FFF; i++)
        {   memory[i] = 0;
        } */
        cpl = 13;
        
        psu = 0x62;
        psl = 0x02;
        iar = 0x688;
        ras[0] = 0x172;
        ras[1] = 0x986;
        r[0] = 0x10;
        r[1] = 0;
        r[2] = 0x4B;
        r[3] = 1;

        BGPaletteMem[7] = 0x7FFF; // white  (%0,BBBBB,GGGGG,RRRRR)
        BGPaletteMem[6] = 0x03FF; // yellow (%0,00000,11111,11111)
        BGPaletteMem[5] = 0x7FE0; // cyan   (%0,11111,11111,00000)
        BGPaletteMem[4] = 0x03E0; // green  (%0,00000,11111,00000)
        BGPaletteMem[3] = 0x7C1F; // purple (%0,11111,00000,11111)
        BGPaletteMem[2] = 0x001F; // red    (%0,00000,00000,11111)
        BGPaletteMem[1] = 0x7C00; // blue   (%0,11111,00000,00000)
        BGPaletteMem[0] = 0x0000; // black  (%0,00000,00000,00000)
    acase SUPERBUG2:
        for (i = 0; i <= 0xFFF; i++)
        {   memory[i] = a_superbug2a[i];
        }
        /* for (i = 0x1000; i <= 0x17FF; i++)
        {   memory[i] = 0;
        } */
        for (i = 0x0; i <= 0x2FF; i++)
        {   memory[0x1800 + i] = a_superbug2b[i];
        }
        /* for (i = 0x1B00; i <= 0x1FFF; i++)
        {   memory[i] = 0;
        } */
        cpl = 13;
        
        psu = 0x62;
        psl = 0x02;
        iar = 0x688;
        ras[0] = 0x172;
        ras[1] = 0x986;
        r[0] = 0x10;
        r[1] = 0;
        r[2] = 0x4B;
        r[3] = 1;
        
        BGPaletteMem[7] = 0x7FFF; // white  (%0,BBBBB,GGGGG,RRRRR)
        BGPaletteMem[6] = 0x03FF; // yellow (%0,00000,11111,11111)
        BGPaletteMem[5] = 0x7FE0; // cyan   (%0,11111,11111,00000)
        BGPaletteMem[4] = 0x03E0; // green  (%0,00000,11111,00000)
        BGPaletteMem[3] = 0x7C1F; // purple (%0,11111,00000,11111)
        BGPaletteMem[2] = 0x001F; // red    (%0,00000,00000,11111)
        BGPaletteMem[1] = 0x7C00; // blue   (%0,11111,00000,00000)
        BGPaletteMem[0] = 0x0000; // black  (%0,00000,00000,00000)
    acase CAPTURE:
        for (i = 0; i <= 0x7FF; i++)
        {   memory[i] = a_capture[i];
        }
        for (i = 0x800; i <= 0x1FFF; i++)
        {   memory[i] = 0;
        }
        iar = 0;
        cpl = 53;
        
        BGPaletteMem[0] = 0x7FFF; // white  (%0,BBBBB,GGGGG,RRRRR)
        BGPaletteMem[1] = 0x03FF; // yellow (%0,00000,11111,11111)
        BGPaletteMem[2] = 0x7FE0; // cyan   (%0,11111,11111,00000)
        BGPaletteMem[3] = 0x03E0; // green  (%0,00000,11111,00000)
        BGPaletteMem[4] = 0x7C1F; // purple (%0,11111,00000,11111)
        BGPaletteMem[5] = 0x001F; // red    (%0,00000,00000,11111)
        BGPaletteMem[6] = 0x7C00; // blue   (%0,11111,00000,00000)
        BGPaletteMem[7] = 0x0000; // black  (%0,00000,00000,00000)
    }

    ham_LoadObjPal(&SpritePalette[whichgame], 9);
    oldimagery = 0xFF;
    paused = FALSE;
    ArcadiaMap[32 + 10] =
    ArcadiaMap[32 + 12] =
    ArcadiaMap[32 + 13] =
    ArcadiaMap[32 + 14] =
    ArcadiaMap[32 + 15] =
    ArcadiaMap[32 + 16] =
    ArcadiaMap[32 + 17] =
    ArcadiaMap[32 + 19] = 0;
}

MODULE void updatetiles(void)
{   PERSIST int fgc1, fgc2, fgc3, fgc4,
                i,
                y; // all PERSISTent for speed
    
    for (i = 0; i < 64 * 16; i++)
    {   ArcadiaTiles[i             ] =
        ArcadiaTiles[i + ( 64 * 16)] =
        ArcadiaTiles[i + (128 * 16)] =
        ArcadiaTiles[i + (192 * 16)] = 0x0000;
    }

    fgc1 = ((memory[A_BGCOLOUR] >> 3) &  1); // 0 or 1
    fgc2 = ((0x40) >> 5)  // %11000000 -> %00000110 (0, 2, 4 or 6)
         + ((memory[A_BGCOLOUR] >> 3) &  1); // 0 or 1
    fgc3 = ((0x80) >> 5)  // %11000000 -> %00000110 (0, 2, 4 or 6)
         + ((memory[A_BGCOLOUR] >> 3) &  1); // 0 or 1
    fgc4 = ((0xC0) >> 5)  // %11000000 -> %00000110 (0, 2, 4 or 6)
         + ((memory[A_BGCOLOUR] >> 3) &  1); // 0 or 1

    for (i = 0; i < 56; i++)
    {   for (y = 0; y < 8; y++)
        {   t = pix[i][y];
            if (t & 128)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4;
            }
            if (t & 64)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1 << 4;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2 << 4;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3 << 4;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4 << 4;
            }
            if (t & 32)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1 << 8;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2 << 8;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3 << 8;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4 << 8;
            }
            if (t & 16)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1 << 12;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2 << 12;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3 << 12;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4 << 12;
            }
            if (t & 8)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4;
            }
            if (t & 4)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1 << 4;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2 << 4;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3 << 4;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4 << 4;
            }
            if (t & 2)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1 << 8;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2 << 8;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3 << 8;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4 << 8;
            }
            if (t & 1)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1 << 12;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2 << 12;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3 << 12;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4 << 12;
    }   }   }

    for (i = 56; i < 64; i++)
    {   for (y = 0; y < 8; y++)
        {   t = memory[A_OFFSET_SPRITES + (8 * (i - 56)) + y];
            if (t & 128)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4;
            }
            if (t & 64)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1 << 4;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2 << 4;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3 << 4;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4 << 4;
            }
            if (t & 32)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1 << 8;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2 << 8;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3 << 8;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4 << 8;
            }
            if (t & 16)
            {   ArcadiaTiles[( i        * 16) + (y * 2)]     |= fgc1 << 12;
                ArcadiaTiles[((i +  64) * 16) + (y * 2)]     |= fgc2 << 12;
                ArcadiaTiles[((i + 128) * 16) + (y * 2)]     |= fgc3 << 12;
                ArcadiaTiles[((i + 192) * 16) + (y * 2)]     |= fgc4 << 12;
            }
            if (t & 8)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4;
            }
            if (t & 4)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1 << 4;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2 << 4;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3 << 4;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4 << 4;
            }
            if (t & 2)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1 << 8;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2 << 8;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3 << 8;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4 << 8;
            }
            if (t & 1)
            {   ArcadiaTiles[( i        * 16) + (y * 2) + 1] |= fgc1 << 12;
                ArcadiaTiles[((i +  64) * 16) + (y * 2) + 1] |= fgc2 << 12;
                ArcadiaTiles[((i + 128) * 16) + (y * 2) + 1] |= fgc3 << 12;
                ArcadiaTiles[((i + 192) * 16) + (y * 2) + 1] |= fgc4 << 12;
    }   }   }

    DMAFastCopy(&ArcadiaTiles[0], &tile_mem[0][0], TILES, DMA_16NOW);
}

MODULE void DMAFastCopy(void* source, void* dest, unsigned int count, unsigned int mode)
{   if (mode == DMA_16NOW || mode == DMA_32NOW)
    {   REG_DMA3SAD = (unsigned int) source;
        REG_DMA3DAD = (unsigned int) dest;
        REG_DMA3CNT = count | mode;
}   }

MODULE void checkreset(void)
{   PERSIST FLAG fresh_l = TRUE;

    if (KEY_DOWN_NOW(KEY_L))
    {   if (fresh_l)
        {   if (whichgame == SUPERBUG1)
            {   whichgame = SUPERBUG2;
            } elif (whichgame == SUPERBUG2)
            {   whichgame = CAPTURE;
            } else
            {   // assert(whichgame == CAPTURE);
                whichgame = SUPERBUG1;
            }
            a_setmemmap();
        }
        fresh_l = FALSE;
    } else
    {   fresh_l = TRUE;
}   }

