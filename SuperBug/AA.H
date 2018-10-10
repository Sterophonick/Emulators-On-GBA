// #define GBA

#define AUTO               auto   /* automatic variables */
#define TRANSIENT          auto   /* automatic variables */
#define MODULE             static /* external static (file-scope) */
#define PERSIST            static /* internal static (function-scope) */
#define DISCARD            (void) /* discarded return values */
#define elif               else if
#define acase              break; case
#define adefault           break; default
#define EXPORT
#define IMPORT             extern
#define UNUSED

typedef unsigned char      UBYTE;
typedef unsigned char      TEXT;
typedef signed char        FLAG;    /*  8-bit signed quantity (replaces BOOL) */
typedef signed char        SBYTE;   /*  8-bit signed quantity (replaces Amiga BYTE) */
typedef unsigned short     UWORD;
typedef signed short       SWORD;   /* 16-bit signed quantity (replaces Amiga WORD) */
typedef long               SHUGE;
typedef long               LONG;
typedef unsigned long      UHUGE;
typedef unsigned long      ULONG;
typedef signed long        SLONG;   /* 32-bit signed quantity (same as LONG) */
typedef unsigned char*     STRPTR;
typedef void*              APTR;

#define DATA_IN_EWRAM      __attribute__((section(".ewram")))

#define KILOBYTE           1024
#define MEMSIZE            (8 * KILOBYTE)

#define A_OFFSET_UPPERSCREEN 0x1800
#define A_OFFSET_LOWERSCREEN 0x1A00
#define A_OFFSET_SPRITES     0x1980
#define A_OFFSET_UDCS        0x19A0

#define PMSK    0x1fff          /* mask page offset */
#define PLEN    0x2000          /* page length */
#define PAGE    0x6000          /* mask page */
#define AMSK    0x7fff          /* mask address range */

#define PSU_S            128
#define PSU_F             64
#define PSU_II            32
// bits 4..3 are unused and unusable
#define PSU_SP             7 // %00000111
#define PSU_WRITABLE    0x67 // %01100111
#define PSU_READONLY    0x98 // %10011000
#define PSL_CC 192 // $11000000
#define PSL_IDC 32
#define PSL_RS  16
#define PSL_WC   8
#define PSL_OVF  4
#define PSL_COM  2
#define PSL_C    1

// cpu.c
EXPORT void cpu(void);

// arcadia.c
EXPORT void uvi(void);
EXPORT void a_emuinput(void);
EXPORT void a_setmemmap(void);
EXPORT void romtiles(void);

// gba.c
EXPORT void playsound(void);

