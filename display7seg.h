/**
 * @file    display7seg.h
 * @brief   Donanim-bagimsiz 7-segment display kutuphanesi.
 *
 * Kullanim:
 *   1. DIGIT_COUNT, DISPLAY_COMMON_ANODE, DISPLAY_MIRROR_DIGITS makrolarini
 *      ihtiyacina gore ayarla.
 *   2. GPIO'lari kendi Pin_Init kodunda output yap.
 *   3. write_segments ve select_digit callback'lerini yaz, display7segInit'e ver.
 *   4. SysTick_Handler icinde 1 ms'de bir displayScan() cagir.
 *   5. print_display("...") ile yaz.
 *
 * Segment bit haritasi (callback'e gelen mask):
 *   bit0=A  bit1=B  bit2=C  bit3=D  bit4=E  bit5=F  bit6=G  bit7=DP(N)
 *   Bit set = segment mantiksal "ON". Polarite donusumu kullanici callback'inde.
 */
#ifndef DISPLAY7SEG_H
#define DISPLAY7SEG_H

#include <stdint.h>

// ============= Konfigurasyon =============
#define DIGIT_COUNT             5   // Toplam scan slot (text digit + LED slot)
#define DISPLAY_TEXT_DIGITS     4   // Ilk N slot text digit, kalanlar print_display tarafindan dokunulmaz
#define DISPLAY_COMMON_ANODE    0   // 1: common anode, 0: common cathode
#define DISPLAY_MIRROR_DIGITS   1   // 1: text digit sirasini ters cevir (LED slot etkilenmez)

// Ham yazi buffer'i (her digit icin DP olabilir + null term)
#define DISPLAY_BUFFER_SIZE     (DIGIT_COUNT * 2 + 1)

// Tum digit'leri kapatmak icin select_digit'e verilen sentinel
#define DISPLAY_DIGIT_NONE      0xFF

// Segment bit maskeleri (referans icin)
#define DISP_SEG_A   0x01
#define DISP_SEG_B   0x02
#define DISP_SEG_C   0x04
#define DISP_SEG_D   0x08
#define DISP_SEG_E   0x10
#define DISP_SEG_F   0x20
#define DISP_SEG_G   0x40
#define DISP_SEG_DP  0x80

// ============= Donanim soyutlamasi =============
typedef struct {
    /**
     * @brief 8 segmenti ayni anda surer.
     * @param segments Bit maskesi: bit0=A..bit7=DP. Bit set = segment ON (mantiksal).
     *                 Polarite donusumu (common anode invert) callback icinde yapilir.
     */
    void (*write_segments)(uint8_t segments);

    /**
     * @brief Aktif digit'i secer, digerlerini kapatir.
     * @param digit_index 0..DIGIT_COUNT-1: o digit'i etkinlestir.
     *                    DISPLAY_DIGIT_NONE (0xFF): tum digit'leri kapat.
     */
    void (*select_digit)(uint8_t digit_index);
} display_hw_t;

// ============= Genel arayuz =============

// Ham yazi buffer'i - debug/inceleme icin disa acildi
extern char g_display_data[DISPLAY_BUFFER_SIZE];

// ASCII -> segment bitmap lookup tablosu
extern const uint8_t display_ascii_table[128];

/**
 * @brief Library'i baslatir, donanim callback'lerini kaydeder.
 * @note  GPIO'lar bu fonksiyondan onceki Pin_Init'te output yapilmis olmalidir.
 */
void display7segInit(const display_hw_t *hw);

/**
 * @brief printf-style ekran yazimi. Ham gecirir, hizalama yapmaz.
 *        '.' karakteri onceki digit'in DP'sine merge edilir.
 *        Ornek: print_display("Err%d", val);  print_display("%2d.%1d", a, b);
 */
void print_display(const char *format, ...);
void print_display_float(float v, uint8_t decimals);
/**
 * @brief Tek bir digit'i tarar. SysTick_Handler icinde 1 ms periyotla cagrilir.
 *        Her cagrida bir sonraki digit'e gecer (multiplexing).
 */
void displayScan(void);

/**
 * @brief Ekstra LED slot'una (display_segments[DIGIT_COUNT-1]) direkt segment maskesi yazar.
 *        Her bit bir LED'i temsil eder: bit0 -> SEG_A, bit1 -> SEG_B, ..., bit7 -> SEG_DP.
 *        Donanimsal olarak bagli olmayan bit'ler hicbir etki yaratmaz.
 *        Ornek: displaySetLeds(0x05) -> SEG_A ve SEG_C hattindaki LED'ler yanar.
 */
void displaySetLeds(uint8_t bits);

#endif // DISPLAY7SEG_H