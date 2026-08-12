/**
* @Author: Emrah Duatepe
*/

#include "lcdlib_ex.h"
#include "M254SE3AE.h"
#include "lcdzone.h"

static const uint8_t ascii_table[128] =
{
    // 0x00-0x1F kontrol karakterleri = bos
    [32] = 0x00, // ' ' (bosluk)

    [45] = 0x40, // '-' (g)
    [46] = 0x80, // '.' (dp)

    // Sayilar
    ['0'] = 0x3F, // abcdef
    ['1'] = 0x06, // bc
    ['2'] = 0x5B, // abdeg
    ['3'] = 0x4F, // abcdg
    ['4'] = 0x66, // bcfg
    ['5'] = 0x6D, // acdfg
    ['6'] = 0x7D, // acdefg
    ['7'] = 0x07, // abc
    ['8'] = 0x7F, // abcdefg
    ['9'] = 0x6F, // abcdfg

    // Büyük harfler
    ['A'] = 0x77, // abcefg
    ['B'] = 0x7C, // cdefg (7seg’de B ˜ b)
    ['C'] = 0x39, // adef
    ['D'] = 0x5E, // bcdeg
    ['E'] = 0x79, // adefg
    ['F'] = 0x71, // aefg
    ['G'] = 0x3D, // acdef
    ['H'] = 0x76, // bcefg
    ['I'] = 0x30, // ef
    ['J'] = 0x1E, // bcde
    ['K'] = 0x75, // abefg (yaklasik K)
    ['L'] = 0x38, // def
    ['N'] = 0x37, // abcefg (yaklasik N)
    ['O'] = 0x3F, // abcdef
    ['P'] = 0x73, // abefg
    ['Q'] = 0x67, // abcfg (˜ q)
    ['R'] = 0x33, // abefg (˜ r)
    ['S'] = 0x6D, // acdfg
    ['T'] = 0x78, // defg
    ['U'] = 0x3E, // bcdef
    ['Y'] = 0x6E, // bcdfg
    ['Z'] = 0x5B, // abdeg (˜ 2)

    // Küçük harfler
    ['a'] = 0x5F, // abcdg (˜ a)
    ['b'] = 0x7C, // cdefg
    ['c'] = 0x58, // deg
    ['d'] = 0x5E, // bcdeg
    ['e'] = 0x7B, // abdegf (˜ E)
    ['f'] = 0x71, // aefg
    ['g'] = 0x6F, // abcdfg (˜ 9)
    ['h'] = 0x74, // cdefg
    ['i'] = 0x10, // e
    ['j'] = 0x0E, // bc d
    ['l'] = 0x30, // ef
    ['n'] = 0x54, // ceg
    ['o'] = 0x5C, // cdeg
    ['p'] = 0x73, // abefg
    ['q'] = 0x67, // abcfg
    ['r'] = 0x50, // eg
    ['s'] = 0x6D, // acdfg
    ['t'] = 0x78, // defg
    ['u'] = 0x1C, // cde
	['v'] = 0x1C, // cde
    ['y'] = 0x6E, // bcdfg

    ['_'] = 0x08, // d
};

void lcdPutChar(uint32_t u32Zone, char ch)
{
    uint8_t mask = ascii_table[(uint8_t)ch];

    // Bu digitin mapping tablosunu bul
    const unsigned char (*digitData)[2] = 
        (const unsigned char (*)[2])g_LCDZoneInfo[u32Zone].pu8GetLCDComSeg;

    uint32_t segnum = g_LCDZoneInfo[u32Zone].u8GetLCDComSegNum;

    for (uint32_t seg = 0; seg < segnum; seg++)
    {
        int com = digitData[seg][0];
        int s   = digitData[seg][1];

        int on = (mask >> seg) & 0x01;
        LCD_SetPixel(com, s, on);
    }
}

void lcdPutString(uint32_t startZone, const char *str)
{
    for (uint32_t i = 0; i < strlen(str); i++)
    {
        lcdPutChar(startZone + i, str[i]);
    }
}



void lcdSetSymbol(uint32_t symbol, uint32_t onOff)
{
    uint32_t com, seg;

    com = (symbol & 0xF);
    seg = ((symbol & 0xFF0) >> 4);

    if (onOff)
        LCD_SetPixel(com, seg, 1); /* Turn on display */
    else
        LCD_SetPixel(com, seg, 0); /* Turn off display */
}