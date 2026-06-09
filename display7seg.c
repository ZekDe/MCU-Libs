/**
 * @file    display7seg.c
 * @brief   Donanim-bagimsiz 7-segment display kutuphanesi implementasyonu.
 * @author Emrah Duatepe
 */
#include "display7seg.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ============= ASCII -> segment lookup =============
// Bit haritasi: bit0=A bit1=B bit2=C bit3=D bit4=E bit5=F bit6=G bit7=DP
// '*' karakteri derece sembolu olarak kullanilir (ust kare: A+B+F+G)
const uint8_t display_ascii_table[128] =
{
    [0]   = 0x00,  // null/empty slot
    [' '] = 0x00,  // space
    ['*'] = 0x63,  // ° (derece) - A+B+F+G
    ['-'] = 0x40,  //
    ['.'] = 0x80,  //
    ['='] = 0x48,  // D+G
    ['['] = 0x39,  // A+D+E+F (C ile ayni)
    [']'] = 0x0F,  // A+B+C+D
    ['_'] = 0x08,  // D
    ['0'] = 0x3F,
    ['1'] = 0x06,
    ['2'] = 0x5B,
    ['3'] = 0x4F,
    ['4'] = 0x66,
    ['5'] = 0x6D,
    ['6'] = 0x7D,
    ['7'] = 0x07,
    ['8'] = 0x7F,
    ['9'] = 0x6F,
    ['A'] = 0x77,
    ['C'] = 0x39,
    ['E'] = 0x79,
    ['F'] = 0x71,
    ['H'] = 0x76,
    ['I'] = 0x30,  // E+F (1 ile karismasin diye)
    ['J'] = 0x1E,
    ['L'] = 0x38,
    ['N'] = 0x37,  // A+B+C+E+F
    ['P'] = 0x73,
    ['S'] = 0x6D,  // 5 ile ayni
    ['U'] = 0x3E,
    ['b'] = 0x7C,
    ['c'] = 0x58,
    ['d'] = 0x5E,
    ['n'] = 0x54,
    ['o'] = 0x5C,
    ['q'] = 0x67,
    ['r'] = 0x50,
    ['t'] = 0x78,
    ['u'] = 0x1C,
    ['y'] = 0x6E,
};

// ============= Dahili durum =============
char g_display_data[DISPLAY_BUFFER_SIZE] = {0};

static uint8_t  display_segments[DIGIT_COUNT] = {0};   // pre-rendered
static uint8_t  scan_index = 0;
static const display_hw_t *hw_ptr = 0;

// ============= Render: ham buffer -> segment dizisi =============
// Sadece ilk DISPLAY_TEXT_DIGITS slot text icin kullanilir.
// Kalan slot'lar (LED slot vb.) displaySetLeds gibi ayri API'ler tarafindan yonetilir; renderToSegments dokunmaz.
static void renderToSegments(void)
{
    uint8_t segs[DISPLAY_TEXT_DIGITS] = {0};
    uint8_t digit = 0;
    int last_filled = -1;

    for (uint16_t i = 0; i < DISPLAY_BUFFER_SIZE && digit < DISPLAY_TEXT_DIGITS; i++)
    {
        char c = g_display_data[i];
        if (c == 0) break;

        if (c == '.' && last_filled >= 0) {
            segs[last_filled] |= DISP_SEG_DP;
            continue;
        }

        uint8_t bitmap = ((uint8_t)c < 128) ? display_ascii_table[(uint8_t)c] : 0x00;
        segs[digit] = bitmap;
        last_filled = digit;
        digit++;
    }

    for (; digit < DISPLAY_TEXT_DIGITS; digit++) {
        segs[digit] = 0x00;
    }

    for (uint8_t i = 0; i < DISPLAY_TEXT_DIGITS; i++) {
        display_segments[i] = segs[i];
    }
}

// ============= Genel API =============
void display7segInit(const display_hw_t *hw)
{
    hw_ptr = hw;
    scan_index = 0;
    memset(g_display_data, 0, sizeof(g_display_data));
    memset(display_segments, 0, sizeof(display_segments));

    if (hw_ptr && hw_ptr->select_digit) {
        hw_ptr->select_digit(DISPLAY_DIGIT_NONE);
    }
    if (hw_ptr && hw_ptr->write_segments) {
        hw_ptr->write_segments(0x00);
    }
}

void print_display(const char *format, ...)
{
    va_list va;
    va_start(va, format);
    vsnprintf(g_display_data, DISPLAY_BUFFER_SIZE, format, va);
    va_end(va);
    renderToSegments();
}

void print_display_float(float v, uint8_t decimals)
{
    if (decimals == 1) {
        int ip = (int)v;
        int fp = (int)((v - ip) * 10);
        if (fp < 0) fp = -fp;
        print_display("%3d.%d", ip, fp);
    } else {  // 2 decimal
        int ip = (int)v;
        int fp = (int)((v - ip) * 100);
        if (fp < 0) fp = -fp;
        print_display("%d.%02d", ip, fp);
    }
}

void displayScan(void)
{
    if (!hw_ptr || !hw_ptr->write_segments || !hw_ptr->select_digit) {
        return;
    }

    // Ghosting onleme: once tum digit'leri kapat
    hw_ptr->select_digit(DISPLAY_DIGIT_NONE);

    // Mirror sadece text digit'lere uygulanir; ekstra slot'lar (LED) sabit index'te kalir
    uint8_t physical;
#if DISPLAY_MIRROR_DIGITS
    if (scan_index < DISPLAY_TEXT_DIGITS) {
        physical = (DISPLAY_TEXT_DIGITS - 1) - scan_index;
    } else {
        physical = scan_index;
    }
#else
    physical = scan_index;
#endif

    // Segmentleri yerlestir, sonra digit'i etkinlestir
    hw_ptr->write_segments(display_segments[scan_index]);
    hw_ptr->select_digit(physical);

    scan_index++;
    if (scan_index >= DIGIT_COUNT) {
        scan_index = 0;
    }
}

void displaySetLeds(uint8_t bits)
{
    display_segments[DIGIT_COUNT - 1] = bits;
}