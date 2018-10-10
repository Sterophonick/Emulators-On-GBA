#include "aa.h"

#define gosub          pushras(); branch();
#define BRANCHCODE     (opcode & 3)
#define CCFIELD        ((psl & PSL_CC) >> 6)
#define ONE_BYTE       iar = (iar + 1) & PMSK;
#define TWO_BYTES      iar = (iar + 2) & PMSK;
#define THREE_BYTES    iar = (iar + 3) & PMSK;
#define SIX_CYCLES     cycles += 6;
#define NINE_CYCLES    cycles += 9;
#define TWELVE_CYCLES  cycles += 12;
#define FIFTEEN_CYCLES cycles += 15;
#define OPERAND        (memory[iar + 1]) // doesn't wrap properly
#define GET_RR         rr = (opcode & 3); if (rr && (psl & PSL_RS)) rr += 3;

EXPORT UBYTE table_sz[256] =
{   1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, //   0.. 15 (LOD)
    1, 1 ,1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, //  16.. 31 (SPSW/RETC/BCTR/BCTA)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, //  32.. 47 (EOR)
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, //  48.. 63 (REDC/RETE/BSTR/BSTA)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, //  64.. 79 (HALT/AND)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, //  80.. 95 (RRR/REDE/BRNR/BRNA)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, //  96..111 (IOR)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, // 112..127 (REDD/CPSW/PPSW/BSNR/BSNA)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, // 128..143 (ADD)
    1, 1 ,1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, // 144..159 (LPSW/DAR/BCFR/ZBRR/BCFA/BXA)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, // 160..175 (SUB)
    1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, // 176..191 (WRTC/TPSW/BSFR/ZBSR/BSFA/BSXA)
    1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, // 192..207 (NOP/STR)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, // 208..223 (RRL/WRTE/BIRR/BIRA)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, // 224..239 (COM)
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3  // 240..255 (WRTD/TMI/BDRR/BDRA)
};

EXPORT int table_clockperiods[256] =
{   6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, //   0..15  (LOD)
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9, //  16..31
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, //  32..47  (EOR)
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9, //  48..63
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, //  64..79
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9, //  80..95  (RRR/REDE/BRNR/BRNA)
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, //  96..111 (IOR)
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9, // 112..127
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, // 128..143 (ADD)
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9, // 144..159
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, // 160..175 (SUB)
    6, 6, 6, 6, 9, 9, 6, 6, 9, 9, 9, 9,  9,  9,  9,  9, // 176..191
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, // 192..207
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9, // 208..223 (RRL/WRTE/BIRR/BIRA)
    6, 6, 6, 6, 6, 6, 6, 6, 9, 9, 9, 9, 12, 12, 12, 12, // 224..239 (COM)
    6, 6, 6, 6, 9, 9, 9, 9, 9, 9, 9, 9,  9,  9,  9,  9  // 240..255 (WRTD/TMI/BDRR/BDRA)
};
// Indirect addressing instructions require 15 or 18 cycles (6 more than usual).

/* condition code changes for a byte
   Apparently this table is wrong as regards the OVF bit.
   We do our own OVF updating afterwards to ensure it is correct. */
MODULE const UBYTE ccc[0x200] =
{   0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
    0x04,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x44,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,
    0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84
};

/***************************************************************
 * handy table to build PC relative offsets
 * from HR (holding register)
 ***************************************************************/
MODULE const int S2650_relative[0x100] =
{     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    -64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
    -48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
    -32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
    -16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
     32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    -64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,
    -48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,
    -32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,
    -16,-15,-14,-13,-12,-11,-10, -9, -8, -7, -6, -5, -4, -3, -2, -1
};

// IMPORTED VARIABLES-----------------------------------------------------

// MODULE VARIABLES-------------------------------------------------------

MODULE SBYTE  ovf;
MODULE SWORD  res;
MODULE UBYTE  operand,
              before,
              c;

// EXPORTED VARIABLES-----------------------------------------------------

EXPORT UBYTE  /* DATA_IN_EWRAM */ memory[MEMSIZE];
EXPORT UBYTE  psu,
              psl,
              opcode,
              rr, // register (0..6)
              r[7]; // register is a reserved word
EXPORT UWORD  iar,
              ras[8];
EXPORT ULONG  cycles,
              ea,
              elapsed;
EXPORT int    interrupt,
              overcalc, // must be signed!
              slice;

MODULE __inline void set_cc(UBYTE n);
MODULE __inline void pushras(void);
MODULE __inline void pullras(void);
MODULE __inline void branch(void);
MODULE void compare(UBYTE first, UBYTE second);
MODULE UBYTE add(UBYTE dest, UBYTE source);
MODULE UBYTE subtract(UBYTE dest, UBYTE source);
MODULE void ABS_EA(void);
MODULE void BRA_EA(void);
MODULE void REL_EA(void);
MODULE void ZERO_EA(void);
MODULE __inline UBYTE cpuread(int address);
MODULE __inline void cpuwrite(int address, UBYTE data);

EXPORT void cpu(void)
{   if (slice <= 0) // this can happen! remember some instructions take up to 18 cycles!
    {   overcalc = -slice;
        return;
    }

    do
    {   opcode = memory[iar];

        switch (opcode)
        {
        case 0:                                                 // LODZ r0
            // hopefully this is the same behaviour as on a genuine console
            set_cc(r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 1:                                                // LODZ r1
        case 2:                                                 // LODZ r2
        case 3:                                                 // LODZ r3
            GET_RR;
            r[0] = r[rr];
            set_cc(r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 4:                                                // LODI,r0
            r[0] = OPERAND;
            set_cc(r[0]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 5:                                                // LODI,r1
        case 6:                                                 // LODI,r2
        case 7:                                                 // LODI,r3
            GET_RR;
            r[rr] = OPERAND;
            set_cc(r[rr]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 8:                                                // LODR,r0
            REL_EA();
            r[0] = cpuread((int) ea);
            set_cc(r[0]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 9:                                                // LODR,r1
        case 10:                                                // LODR,r2
        case 11:                                                // LODR,r3
            REL_EA();
            GET_RR;
            r[rr] = cpuread((int) ea);
            set_cc(r[rr]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 12:                                               // LODA,r0
        case 13:                                                // LODA,r1
        case 14:                                                // LODA,r2
        case 15:                                                // LODA,r3
            ABS_EA(); // calls GET_RR for us
            r[rr] = cpuread((int) ea);
            set_cc(r[rr]);
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 18:                                               // SPSU
            /* if (machine == PIPBUG)
            {   if (rand() % 2)
                {   psu |= PSU_S;
                } else
                {   psu &= ~(PSU_S);
            }   } */
            r[0] = psu;
            set_cc(psu);
            ONE_BYTE;
            SIX_CYCLES;
        acase 19:                                               // SPSL
            r[0] = psl;
            set_cc(psl);
            ONE_BYTE;
            SIX_CYCLES;
        acase 20:                                               // RETC,eq
        case 21:                                                // RETC,gt
        case 22:                                                // RETC,lt
            if (BRANCHCODE == CCFIELD)
            {   pullras();
            } else
            {   ONE_BYTE;
            }
            NINE_CYCLES;
        acase 23:                                               // RETC,un
            pullras();
            NINE_CYCLES;
        acase 24:                                               // BCTR,eq
        case 25:                                                // BCTR,gt
        case 26:                                                // BCTR,lt
            if (BRANCHCODE == CCFIELD)
            {   REL_EA();
                branch();
            } else
            {   TWO_BYTES;
            }
            NINE_CYCLES;
        acase 27:                                               // BCTR,un
            REL_EA();
            branch();
            NINE_CYCLES;
        acase 28:                                               // BCTA,eq
        case 29:                                                // BCTA,gt
        case 30:                                                // BCTA,lt
            if (BRANCHCODE == CCFIELD)
            {   BRA_EA();
                branch();
            } else
            {   THREE_BYTES;
            }
            NINE_CYCLES;
        acase 31:                                               // BCTA,un
            BRA_EA();
            branch();
            NINE_CYCLES;
        acase 32:                                               // EORZ r0
            r[0] = 0;
            psl &= ~(PSL_CC); // clear CC bits of PSL (ie. CC = "eq")
            ONE_BYTE;
            SIX_CYCLES;
        acase 33:                                               // EORZ r1
        case 34:                                                // EORZ r2
        case 35:                                                // EORZ r3
            GET_RR;
            r[0] ^= r[rr];
            set_cc(r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 36:                                               // EORI,r0
            r[0] ^= OPERAND;
            set_cc(r[0]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 37:                                               // EORI,r1
        case 38:                                                // EORI,r2
        case 39:                                                // EORI,r3
            GET_RR;
            r[rr] ^= OPERAND;
            set_cc(r[rr]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 40:                                               // EORR,r0
            REL_EA();
            r[0] ^= cpuread((int) ea);
            set_cc(r[0]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 41:                                               // EORR,r1
        case 42:                                                // EORR,r2
        case 43:                                                // EORR,r3
            REL_EA();
            GET_RR;
            r[rr] ^= cpuread((int) ea);
            set_cc(r[rr]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 44:                                               // EORA,r0
        case 45:                                                // EORA,r1
        case 46:                                                // EORA,r2
        case 47:                                                // EORA,r3
            ABS_EA(); // calls GET_RR for us
            r[rr] ^= cpuread((int) ea);
            set_cc(r[rr]);
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 52:                                               // RETE,eq
        case 53:                                                // RETE,gt
        case 54:                                                // RETE,lt
            if (BRANCHCODE == CCFIELD)
            {   psu &= ~(PSU_II); // clear II bit (PSU &= %11011111)
                pullras();
            } else
            {   ONE_BYTE;
            }
            NINE_CYCLES;
        acase 55:                                               // RETE,un
            psu &= ~(PSU_II); // clear II bit (PSU &= %11011111)
            pullras();
            NINE_CYCLES;
        acase 56:                                               // BSTR,eq
        case 57:                                                // BSTR,gt
        case 58:                                                // BSTR,lt
            if (BRANCHCODE == CCFIELD)
            {   REL_EA();
                gosub;
            } else
            {   TWO_BYTES;
            }
            NINE_CYCLES;
        acase 59:                                               // BSTR,un
            REL_EA();
            gosub;
            NINE_CYCLES;
        acase 60:                                               // BSTA,eq
        case 61:                                                // BSTA,gt
        case 62:                                                // BSTA,lt
        case 63:                                                // BSTA,un
            if (opcode == 63 || BRANCHCODE == CCFIELD)
            {   BRA_EA();
                gosub;
            } else
            {   THREE_BYTES;
            }
            NINE_CYCLES;
        acase 64:                                               // HALT
            /* Strictly speaking, we should maybe halt the emulation
               entirely, instead of just repeatedly executing
               the HALT instruction (since instruction fetching is
               halted). But this way is easier, because
               interrupts may still occur. */
            // ZERO_BYTES;
            SIX_CYCLES;
        acase 65:                                               // ANDZ r1
        case 66:                                                // ANDZ r2
        case 67:                                                // ANDZ r3
            GET_RR;
            r[0] &= r[rr];
            set_cc(r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 68:                                               // ANDI,r0
            r[0] &= OPERAND;
            set_cc(r[0]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 69:                                               // ANDI,r1
        case 70:                                                // ANDI,r2
        case 71:                                                // ANDI,r3
            GET_RR;
            r[rr] &= OPERAND;
            set_cc(r[rr]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 72:                                               // ANDR,r0
            REL_EA();
            r[0] &= cpuread((int) ea);
            set_cc(r[0]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 73:                                               // ANDR,r1
        case 74:                                                // ANDR,r2
        case 75:                                                // ANDR,r3
            REL_EA();
            GET_RR;
            r[rr] &= cpuread((int) ea);
            set_cc(r[rr]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 76:                                               // ANDA,r0
        case 77:                                                // ANDA,r1
        case 78:                                                // ANDA,r2
        case 79:                                                // ANDA,r3
            ABS_EA(); // calls GET_RR for us
            r[rr] &= cpuread((int) ea);
            set_cc(r[rr]);
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 80:                                               // RRR,r0
        case 81:                                                // RRR,r1
        case 82:                                                // RRR,r2
        case 83:                                                // RRR,r3
            GET_RR;
            before = r[rr];
            if (psl & PSL_WC)
            {   c = psl & PSL_C;
                psl &= ~(PSL_C + PSL_IDC);
                r[rr] = (before >> 1) | (c << 7);
                psl |= (before & PSL_C) + (r[rr] & PSL_IDC);
            } else
            {   r[rr] = (before >> 1) | (before << 7);
            }
            psl = (psl & ~(PSL_OVF | PSL_CC)) | ccc[r[rr] + (((r[rr] ^ before) << 1) & 256)];
            if ((before & 0x80) != (r[rr] & 0x80))
            {   psl |= PSL_OVF;
            } else
            {   psl &= ~(PSL_OVF);
            }
            // set_cc() not necessary
            ONE_BYTE;
            SIX_CYCLES;
        acase 88:                                               // BRNR,r0
        case 89:                                                // BRNR,r1
        case 90:                                                // BRNR,r2
        case 91:                                                // BRNR,r3
            GET_RR;
            if (r[rr])
            {   REL_EA();
                branch();
                NINE_CYCLES;
            } else
            {   TWO_BYTES;
                NINE_CYCLES;
            }
        acase 92:                                               // BRNA,r0
        case 93:                                                // BRNA,r1
        case 94:                                                // BRNA,r2
        case 95:                                                // BRNA,r3
            GET_RR;
            if (r[rr])
            {   BRA_EA();
                branch();
                NINE_CYCLES;
            } else
            {   THREE_BYTES;
                NINE_CYCLES;
            }
        acase 96:                                               // IORZ r0
            // r[0] |= r[0]; is not needed
            set_cc(r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 97:                                               // IORZ r1
        case 98:                                                // IORZ r2
        case 99:                                                // IORZ r3
            GET_RR;
            r[0] |= r[rr];
            set_cc(r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 100:                                              // IORI,r0
            r[0] |= OPERAND;
            set_cc(r[0]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 101:                                              // IORI,r1
        case 102:                                               // IORI,r2
        case 103:                                               // IORI,r3
            GET_RR;
            r[rr] |= OPERAND;
            set_cc(r[rr]);
            TWO_BYTES;
            SIX_CYCLES;
        acase 104:                                              // IORR,r0
            REL_EA();
            r[0] |= cpuread((int) ea);
            set_cc(r[0]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 105:                                              // IORR,r1
        case 106:                                               // IORR,r2
        case 107:                                               // IORR,r3
            REL_EA();
            GET_RR;
            r[rr] |= cpuread((int) ea);
            set_cc(r[rr]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 108:                                              // IORA,r0
        case 109:                                               // IORA,r1
        case 110:                                               // IORA,r2
        case 111:                                               // IORA,r3
            ABS_EA(); // calls GET_RR for us
            r[rr] |= cpuread((int) ea);
            set_cc(r[rr]);
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 116:                                              // CPSU
            psu &= (~(OPERAND & PSU_WRITABLE));
            TWO_BYTES;
            NINE_CYCLES;
        acase 117:                                              // CPSL
            psl &= (~OPERAND);
            TWO_BYTES;
            NINE_CYCLES;
        acase 118:                                              // PPSU
            psu |= (OPERAND & PSU_WRITABLE);
            TWO_BYTES;
            NINE_CYCLES;
        acase 119:                                              // PPSL
            psl |= OPERAND;
            TWO_BYTES;
            NINE_CYCLES;
        acase 120:                                              // BSNR,r0
            if (r[0])
            {   REL_EA();
                gosub;
            } else
            {   TWO_BYTES;
            }
            NINE_CYCLES;
        acase 121:                                              // BSNR,r1
        case 122:                                               // BSNR,r2
        case 123:                                               // BSNR,r3
            GET_RR;
            if (r[rr])
            {   REL_EA();
                gosub;
            } else
            {   TWO_BYTES;
            }
            NINE_CYCLES;
        acase 124:                                              // BSNA,r0
            if (r[0])
            {   BRA_EA();
                gosub;
            } else
            {   THREE_BYTES;
            }
            NINE_CYCLES;
        acase 125:                                              // BSNA,r1
        case 126:                                               // BSNA,r2
        case 127:                                               // BSNA,r3
            GET_RR;
            if (r[rr])
            {   BRA_EA();
                gosub;
            } else
            {   THREE_BYTES;
            }
            NINE_CYCLES;
        acase 128:                                              // ADDZ r0
            r[0] = add(r[0], r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 129:                                              // ADDZ r1
        case 130:                                               // ADDZ r2
        case 131:                                               // ADDZ r3
            GET_RR;
            r[0] = add(r[0], r[rr]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 132:                                              // ADDI,r0
            r[0] = add(r[0], OPERAND);
            TWO_BYTES;
            SIX_CYCLES;
        acase 133:                                              // ADDI,r1
        case 134:                                               // ADDI,r2
        case 135:                                               // ADDI,r3
            GET_RR;
            r[rr] = add(r[rr], OPERAND);
            TWO_BYTES;
            SIX_CYCLES;
        acase 136:                                              // ADDR,r0
            REL_EA();
            r[0] = add(r[0], cpuread((int) ea));
            TWO_BYTES;
            NINE_CYCLES;
        acase 137:                                              // ADDR,r1
        case 138:                                               // ADDR,r2
        case 139:                                               // ADDR,r3
            REL_EA();
            GET_RR;
            r[rr] = add(r[rr], cpuread((int) ea));
            TWO_BYTES;
            NINE_CYCLES;
        acase 140:                                              // ADDA,r0
        case 141:                                               // ADDA,r1
        case 142:                                               // ADDA,r2
        case 143:                                               // ADDA,r3
            ABS_EA(); // calls GET_RR for us
            r[rr] = add(r[rr], cpuread((int) ea));
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 146:                                              // LPSU
            psu &= PSU_READONLY;
            psu |= (r[0] & PSU_WRITABLE);
            ONE_BYTE;
            SIX_CYCLES;
        acase 147:                                              // LPSL
            psl = r[0];
            ONE_BYTE;
            SIX_CYCLES;
        acase 148:                                              // DAR  r0
            if ((psl & PSL_C) == 0)
            {   // add ten to the tens digit (the high nybble)
                r[0] += 0xA0; // 160. overflow OK. %1010,0000
            }
            if ((psl & PSL_IDC) == 0)
            {   // OR the high nybble with the (low nybble + 10)
                r[0] = (r[0] & 0xF0) | ((r[0] + 0x0A) & 0x0F);
            }
            set_cc(r[0]);
            ONE_BYTE;
            NINE_CYCLES;
        acase 149:                                              // DAR  r1
        case 150:                                               // DAR  r2
        case 151:                                               // DAR  r3
            GET_RR;
            if ((psl & PSL_C) == 0)
            {   // add ten to the tens digit (the high nybble)
                r[rr] += 0xA0; // 160. overflow OK. %1010,0000
            }
            if ((psl & PSL_IDC) == 0)
            {   // OR the high nybble with the (low nybble + 10)
                r[rr] = (r[rr] & 0xF0) | ((r[rr] + 0x0A) & 0x0F);
            }
            set_cc(r[rr]);
            ONE_BYTE;
            NINE_CYCLES;
        acase 152:                                              // BCFR,eq
        case 153:                                               // BCFR,gt
        case 154:                                               // BCFR,lt
            if (BRANCHCODE != CCFIELD)
            {   REL_EA();
                branch();
            } else
            {   TWO_BYTES;
            }
            NINE_CYCLES;
        acase 155:                                              // ZBRR
            ZERO_EA();
            branch();
            NINE_CYCLES;
        acase 156:                                              // BCFA,eq
        case 157:                                               // BCFA,gt
        case 158:                                               // BCFA,lt
            if (BRANCHCODE != CCFIELD)
            {   BRA_EA();
                branch();
            } else
            {   THREE_BYTES;
            }
            NINE_CYCLES;
        acase 159:                                              // BXA,r3
            BRA_EA();
            if (psl & PSL_RS)
            {   ea = (ea + r[6]) & AMSK;
            } else
            {   ea = (ea + r[3]) & AMSK;
            }
            branch();
            NINE_CYCLES;
        acase 160:                                              // SUBZ r0
            r[0] = subtract(r[0], r[0]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 161:                                              // SUBZ r1
        case 162:                                               // SUBZ r2
        case 163:                                               // SUBZ r3
            GET_RR;
            r[0] = subtract(r[0], r[rr]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 164:                                              // SUBI,r0
            r[0] = subtract(r[0], OPERAND);
            TWO_BYTES;
            SIX_CYCLES;
        acase 165:                                              // SUBI,r1
        case 166:                                               // SUBI,r2
        case 167:                                               // SUBI,r3
            GET_RR;
            r[rr] = subtract(r[rr], OPERAND);
            TWO_BYTES;
            SIX_CYCLES;
        acase 168:                                              // SUBR,r0
            REL_EA();
            r[0] = subtract(r[0], cpuread((int) ea));
            TWO_BYTES;
            NINE_CYCLES;
        acase 169:                                              // SUBR,r1
        case 170:                                               // SUBR,r2
        case 171:                                               // SUBR,r3
            REL_EA();
            GET_RR;
            r[rr] = subtract(r[rr], cpuread((int) ea));
            TWO_BYTES;
            NINE_CYCLES;
        acase 172:                                              // SUBA,r0
        case 173:                                               // SUBA,r1
        case 174:                                               // SUBA,r2
        case 175:                                               // SUBA,r3
            ABS_EA(); // this calls GET_RR for us
            r[rr] = subtract(r[rr], cpuread((int) ea));
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 180:                                              // TPSU
            psl &= 0x3F; // set CC to %00xxxxxx ("eq")
            if ((psu & OPERAND) < OPERAND)
            {   psl |= 0x80; // set CC to %10xxxxxx ("lt")
            }
            TWO_BYTES;
            NINE_CYCLES;
        acase 181:                                              // TPSL
            psl &= 0x3F; // set CC to %00xxxxxx ("eq")
            if ((psl & OPERAND) < OPERAND)
            {   psl |= 0x80; // set CC to %10xxxxxx ("lt")
            }
            TWO_BYTES;
            NINE_CYCLES;
        acase 184:                                              // BSFR,eq
        case 185:                                               // BSFR,gt
        case 186:                                               // BSFR,lt
            if (BRANCHCODE != CCFIELD)
            {   REL_EA();
                gosub;
            } else
            {   TWO_BYTES;
            }
            NINE_CYCLES;
        acase 187:                                              // ZBSR
            ZERO_EA();
            // we are making an assumption that the USE monitor switches to the
            // main register bank when these functions are called; this assumption is
            // probably not true.
            gosub;
            NINE_CYCLES;
        acase 188:                                              // BSFA,eq
        case 189:                                               // BSFA,gt
        case 190:                                               // BSFA,lt
            if (BRANCHCODE != CCFIELD)
            {   BRA_EA();
                gosub;
            } else
            {   THREE_BYTES;
            }
            NINE_CYCLES;
        acase 191:                                              // BSXA,r3
            BRA_EA();
            if (psl & PSL_RS)
            {   ea = (ea + r[6]) & AMSK;
            } else
            {   ea = (ea + r[3]) & AMSK;
            }
            gosub;
            NINE_CYCLES;
        acase 192:                                              // NOP
            ONE_BYTE;
            SIX_CYCLES;
        acase 193:                                              // STRZ r1
        case 194:                                               // STRZ r2
        case 195:                                               // STRZ r3
            GET_RR;
            r[rr] = r[0];
            set_cc(r[rr]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 200:                                              // STRR,r0
            REL_EA();
            cpuwrite((int) ea, r[0]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 201:                                              // STRR,r1
        case 202:                                               // STRR,r2
        case 203:                                               // STRR,r3
            REL_EA();
            GET_RR;
            cpuwrite((int) ea, r[rr]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 204:                                              // STRA,r0
        case 205:                                               // STRA,r1
        case 206:                                               // STRA,r2
        case 207:                                               // STRA,r3
            ABS_EA(); // this calls GET_RR for us
            cpuwrite((int) ea, r[rr]);
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 208:                                              // RRL,r0
        case 209:                                               // RRL,r1
        case 210:                                               // RRL,r2
        case 211:                                               // RRL,r3
            GET_RR;
            before = r[rr];
            if (psl & PSL_WC)
            {   c = psl & PSL_C;
                psl &= ~(PSL_C + PSL_IDC);
                r[rr] = (before << 1) | c;
                psl |= (before >> 7) + (r[rr] & PSL_IDC);
            } else
            {   r[rr] = (before << 1) | (before >> 7);
            }
            psl = (psl & ~(PSL_OVF | PSL_CC)) | ccc[r[rr] + (((r[rr] ^ before) << 1) & 256)];
            if ((before & 0x80) != (r[rr] & 0x80))
            {   psl |= PSL_OVF;
            } else
            {   psl &= ~(PSL_OVF);
            }
            // set_cc() not necessary
            ONE_BYTE;
            SIX_CYCLES;
        acase 216:                                              // BIRR,r0
        case 217:                                               // BIRR,r1
        case 218:                                               // BIRR,r2
        case 219:                                               // BIRR,r3
            GET_RR;
            if (++r[rr]) // overflow OK
            {   REL_EA();
                branch();
                NINE_CYCLES;
            } else
            {   TWO_BYTES;
                NINE_CYCLES;
            }
        acase 220:                                              // BIRA,r0
        case 221:                                               // BIRA,r1
        case 222:                                               // BIRA,r2
        case 223:                                               // BIRA,r3
            GET_RR;
            if (++r[rr]) // overflow OK
            {   BRA_EA();
                branch();
                NINE_CYCLES;
            } else
            {   THREE_BYTES;
                NINE_CYCLES;
            }
        acase 224:                                              // COMZ r0
            psl &= ~(PSL_CC); // clear CC bits of PSL
            ONE_BYTE;
            SIX_CYCLES;
        acase 225:                                              // COMZ r1
        case 226:                                               // COMZ r2
        case 227:                                               // COMZ r3
            GET_RR;
            compare(r[0], r[rr]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 228:                                              // COMI,r0
            compare(r[0], OPERAND);
            TWO_BYTES;
            SIX_CYCLES;
        acase 229:                                              // COMI,r1
        case 230:                                               // COMI,r2
        case 231:                                               // COMI,r3
            GET_RR;
            compare(r[rr], OPERAND);
            TWO_BYTES;
            SIX_CYCLES;
        acase 232:                                              // COMR,r0
            REL_EA();
            compare(r[0], cpuread((int) ea));
            TWO_BYTES;
            NINE_CYCLES;
        acase 233:                                              // COMR,r1
        case 234:                                               // COMR,r2
        case 235:                                               // COMR,r3
            REL_EA();
            GET_RR;
            compare(r[rr], cpuread((int) ea));
            TWO_BYTES;
            NINE_CYCLES;
        acase 236:                                              // COMA,r0
        case 237:                                               // COMA,r1
        case 238:                                               // COMA,r2
        case 239:                                               // COMA,r3
            ABS_EA(); // this calls GET_RR for us
            compare(r[rr], cpuread((int) ea));
            THREE_BYTES;
            TWELVE_CYCLES;
        acase 244:                                              // TMI,r0
            psl &= 0x3F; // set CC to %00 ("eq")
            if ((r[0] & OPERAND) < OPERAND)
            {   psl |= 0x80; // set CC to %10 ("lt")
            }
            TWO_BYTES;
            NINE_CYCLES;
        acase 245:                                              // TMI,r1
        case 246:                                               // TMI,r2
        case 247:                                               // TMI,r3
            GET_RR;
            psl &= 0x3F; // set CC to %00 ("eq")
            if ((r[rr] & OPERAND) < OPERAND)
            {   psl |= 0x80; // set CC to %10 ("lt")
            }
            TWO_BYTES;
            NINE_CYCLES;
        acase 248:                                              // BDRR,r0
        case 249:                                               // BDRR,r1
        case 250:                                               // BDRR,r2
        case 251:                                               // BDRR,r3
            GET_RR;
            if (--r[rr]) // underflow OK
            {   REL_EA();
                branch();
                NINE_CYCLES;
            } else
            {   TWO_BYTES;
                NINE_CYCLES;
            }
        acase 252:                                              // BDRA,r0
        case 253:                                               // BDRA,r1
        case 254:                                               // BDRA,r2
        case 255:                                               // BDRA,r3
            GET_RR;
            if (--r[rr]) // underflow OK
            {   BRA_EA();
                branch();
                NINE_CYCLES;
            } else
            {   THREE_BYTES;
                NINE_CYCLES;
            }
        acase 48:                                               // REDC r0
        case 49:                                                // REDC r1
        case 50:                                                // REDC r2
        case 51:                                                // REDC r3
            GET_RR;
            set_cc(r[rr]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 84:                                               // REDE,r0
        case 85:                                                // REDE,r1
        case 86:                                                // REDE,r2
        case 87:                                                // REDE,r3
            GET_RR;
            set_cc(r[rr]);
            TWO_BYTES;
            NINE_CYCLES;
        acase 112:                                              // REDD r0
        case 113:                                               // REDD r1
        case 114:                                               // REDD r2
        case 115:                                               // REDD r3
            GET_RR;
            set_cc(r[rr]);
            ONE_BYTE;
            SIX_CYCLES;
        acase 176:                                              // WRTC r0
        case 177:                                               // WRTC r1
        case 178:                                               // WRTC r2
        case 179:                                               // WRTC r3
            GET_RR;
            ONE_BYTE;
            SIX_CYCLES;
        acase 212:                                              // WRTE r0
        case 213:                                               // WRTE r1
        case 214:                                               // WRTE r2
        case 215:                                               // WRTE r3
            GET_RR;
            TWO_BYTES;
            NINE_CYCLES;
        acase 240:                                              // WRTD r0
        case 241:                                               // WRTD r1
        case 242:                                               // WRTD r2
        case 243:                                               // WRTD r3
            GET_RR;
            ONE_BYTE;
            SIX_CYCLES;
        acase 0x10:
        case 0x11:
        case 0x90:
        case 0x91:
        case 0xB6:
        case 0xB7:
        case 0xC4:
        case 0xC5:
        case 0xC6:
        case 0xC7:
            // rarely used "do nothing" opcodes
            // These are all currently defined as 1 byte and 6 cycles
            iar = (iar + table_sz[opcode]) & PMSK;
            cycles += table_clockperiods[opcode];
        }
    } while (cycles < (ULONG) slice);

    overcalc = cycles - slice;
    elapsed += cycles;
    cycles = 0; // needed
}

// ** Update condition code reg **
MODULE __inline void set_cc(UBYTE n) // set CC
{   psl &= ~(PSL_CC); // clear CC bits of PSL

    if (n >= 128)
    {   psl |= 0x80; // CC = %10
    } elif (n > 0)
    {   psl |= 0x40; // CC = %01
}   }

// ** Push Return Address Stack **
MODULE __inline void pushras(void)
{   // store address
    ras[psu & PSU_SP] = iar + table_sz[opcode];

    // increment SP
    if ((psu & PSU_SP) == 7) // overflow
    {   psu &= ~(PSU_SP); // set SP to 0
    } else
    {   psu++;
}   }

// ** Pull Return Address Stack **
MODULE __inline void pullras(void)
{   // decrement SP
    if ((psu & PSU_SP) == 0) // underflow
    {   psu |= PSU_SP; // set SP to 7
    } else psu--;

    // retrieve address
    ea   = ras[psu & PSU_SP];
    iar  = (UWORD) ea;
}

MODULE __inline void branch(void)
{   iar  = (UWORD) ea;
    if (operand & 0x80)
    {   cycles += 6;
}   }

MODULE void compare(UBYTE first, UBYTE second)
{   int d;

    psl &= 0x3F; // clear CC
    if (psl & PSL_COM) // unsigned compare
    {   d = (UBYTE) first - (UBYTE) second;
    } else
    {   d = (SBYTE) first - (SBYTE) second;
    }
    if (d < 0 )
    {   psl |= 0x80; // %10 = lt
    } elif (d > 0)
    {   psl |= 0x40; // %01 = gt
}   }

// Add source to destination
// Add (if WC==0) or add with carry (if WC==1)
MODULE UBYTE add(UBYTE dest, UBYTE source)
{   before = dest;

    if (source <= 127 && dest <= 127)
    {   ovf = 1;
    } elif (source >= 128 && dest >= 128)
    {   ovf = -1;
    } else ovf = 0;

    res = dest + source + ((psl >> 3) & psl & PSL_C); // Apparently.

    psl &= ~(PSL_C | PSL_OVF | PSL_IDC);
    if (res & 0x100)
    {   psl |= PSL_C;
    }
    dest = res & 0xFF;
    if ((dest & 15) < (before & 15))
    {   psl |= PSL_IDC;
    }

    psl = (psl & ~(PSL_OVF | PSL_CC)) | ccc[dest + (((dest ^ before) << 1) & 256)];

    if (ovf == 0)
    {   psl &= ~(PSL_OVF);
    } elif (ovf == 1)
    {   if (dest >= 128)
        {   psl |= PSL_OVF;
        } else
        {   psl &= ~(PSL_OVF);
    }   }
    elif (ovf == -1)
    {   if (dest <= 127)
        {   psl |= PSL_OVF;
        } else
        {   psl &= ~(PSL_OVF);
    }   }
    /* OVF is set if both numbers were positive (<= 127) and the answer is negative (>= 128).
               or if both numbers were negative (>= 128) and the answer is positive (<= 127).
    Otherwise, it is cleared. */

    // set_cc() not necessary

    return((UBYTE) res);
}

// Subtract source from destination
// Subtract with borrow if WC flag of PSL is set
MODULE UBYTE subtract(UBYTE dest, UBYTE source)
{   before = dest;

    if (source <= 127 && dest <= 127)
    {   ovf = 1;
    } elif (source >= 128 && dest >= 128)
    {   ovf = -1;
    } else ovf = 0;

    res = dest - source - ((psl >> 3) & (psl ^ PSL_C) & PSL_C);
    psl &= ~(PSL_C | PSL_OVF | PSL_IDC);
    if ((res & 0x100) == 0)
    {   psl |= PSL_C;
    }
    dest = res & 0xFF;
    if ((dest & 15) <= (before & 15))
    {   psl |= PSL_IDC;
    }

    psl = (psl & ~(PSL_OVF | PSL_CC)) | ccc[dest + (((dest ^ before) << 1) & 256)];

    if (ovf == 0)
    {   psl &= ~(PSL_OVF);
    } elif (ovf == 1)
    {   if (dest >= 128)
        {   psl |= PSL_OVF;
        } else
        {   psl &= ~(PSL_OVF);
    }   }
    elif (ovf == -1)
    {   if (dest <= 127)
        {   psl |= PSL_OVF;
        } else
        {   psl &= ~(PSL_OVF);
    }   }
    /* OVF is set if both numbers were positive (<= 127) and the answer is negative (>= 128).
               or if both numbers were negative (>= 128) and the answer is positive (<= 127).
    Otherwise, it is cleared. */

    // set_cc() not necessary

    return((UBYTE) res);
}

MODULE void ABS_EA(void)
{   operand = memory[iar + 1];
    ea = ((operand << 8) + memory[iar + 2]) & PMSK;
    GET_RR;

    if (operand & 0x80) /* indirect addressing? */
    {   ea = (memory[ea] << 8) + memory[ea + 1];
        SIX_CYCLES;
    }

    /* check indexed addressing modes */
    switch (operand & 0x60)
    {
    case 0x20: /* auto increment indexed */
        r[rr]++;
        ea += r[rr];
        rr = 0; /* absolute addressing reg is R0 */
    acase 0x40: /* auto decrement indexed */
        r[rr]--;
        ea += r[rr];
        rr = 0; /* absolute addressing reg is R0 */
    acase 0x60: /* indexed */
        ea += r[rr];
        rr = 0; /* absolute addressing reg is R0 */
}   }

MODULE void BRA_EA(void)
{   operand = memory[iar + 1];
    ea = ((operand << 8) + memory[iar + 2]) & AMSK;

    if (operand & 0x80) // indirect addressing?
    {   ea = (memory[ea] << 8) + memory[ea + 1];
        SIX_CYCLES;
}   }

MODULE void REL_EA(void)
{   operand = memory[iar + 1];
    ea = (iar + 2 + S2650_relative[operand]) & PMSK;

    if (operand & 0x80) /* indirect bit set? */
    {   ea = (memory[ea] << 8) + memory[ea + 1];
        SIX_CYCLES;
}   }

MODULE void ZERO_EA(void)
{   operand = memory[iar + 1];
    ea = S2650_relative[operand] & PMSK;
    
    if (operand & 0x80) // indirect bit set?
    {   ea = (memory[ea] << 8) + memory[ea + 1];
        SIX_CYCLES;
}   }

MODULE __inline UBYTE cpuread(int address)
{   return memory[address];
}

MODULE __inline void cpuwrite(int address, UBYTE data)
{   memory[address] = data;
}

