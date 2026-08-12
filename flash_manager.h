#ifndef FLASH_MANAGER_H
#define FLASH_MANAGER_H

#include "stdint.h"
/*
* M031LC2AE (32 KB APROM) - DFBA = 0x7000 (CONFIG1)
* FLASH LAYOUT:
*   Sector 0-1: STATIC                (DFBA + 0x000 .. DFBA + 0x3FF)
*   Sector 2-3: DYNAMIC wear_set=0    (DFBA + 0x400 .. DFBA + 0x7FF)
*   Sector 4-5: DYNAMIC wear_set=1    (DFBA + 0x800 .. DFBA + 0xBFF)
*   Sector 6-7: SYSTEM                (DFBA + 0xC00 .. DFBA + 0xFFF)
* Toplam 4 KB kullanici alani. DYNAMIC icin 1 KB - 1 KB dongusel wear leveling.
* FLASH_USER_BASE runtime'da FMC_ReadDataFlashBaseAddr() ile alinir.

M24SE3AE için ->
Sector 0-1: STATIC (0x1F000-0x1F3FF)
Sector 2-3: DYNAMIC wear_set=0 (0x1F400-0x1F7FF)
Sector 4-5: DYNAMIC wear_set=1 (0x1F800-0x1FBFF)
Sector 6-7: SYSTEM (0x1FC00-0x1FFFF)
Toplam 4 Kb yazma alani vardir. DYNAMIC için 1 kb - 1kb döngüsel
*/
// Flash fiziksel ozellikleri
#define FLASH_END_ADDR          0x20000    // 128KB son adres
#define FLASH_SECTOR_SIZE       0x200      // 512 bytes per sector
#define FLASH_USER_SIZE         0x1000     // 4KB toplam alan
#define FLASH_USER_BASE          (FLASH_END_ADDR - FLASH_USER_SIZE)

// Maksimum desteklenen veri boyutu (2 sekt�r)
#define MAX_CATEGORY_DATA_SIZE  0x400      // 1024 bytes

// Statik flash layout - degismez!
#define STATIC_SECTOR_COUNT     2   // 1KB (2x512)
#define DYNAMIC_SECTOR_COUNT    4   // 2KB (4x512)
#define SYSTEM_SECTOR_COUNT     2   // 1KB (2x512)
#define DYNAMIC_WEAR_SETS      2   // 2 set alternating (her set 1KB)

// Maksimum parametre sayisi
#define MAX_PARAMS              32

// Saglik uyarisi icin yazma sayisi siniri (M031 flash 100K cycle endurance)
#define FLASH_HEALTH_THRESHOLD  100000U

// Sentinel for "no next sector" in chained sector header
#define FLASH_NEXT_SECTOR_NONE  0xFFFFFFFFU

// Header yapisi
typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t write_count;
    uint32_t data_size;     // Bu sekt�rdeki veri boyutu
    uint32_t next_sector;   // Sonraki sekt�r index (0xFF = yok)
    uint32_t layout_id;     // Kategori parametre semasi imzasi (id+size CRC32)
    uint32_t crc32;
} flash_header_t;

#define HEADER_SIZE 		sizeof(flash_header_t)
#define SECTOR_DATA_SIZE 	(FLASH_SECTOR_SIZE - HEADER_SIZE)

// Kategori layout bilgisi
typedef struct {
    uint8_t start_sector;   // Baslangi� sekt�r index
    uint8_t sector_count;   // Toplam sekt�r sayisi
    uint8_t wear_sets;      // Wear leveling set sayisi
} category_layout_t;

// Parametre kategorileri
typedef enum {
    PARAM_CAT_STATIC = 0,
    PARAM_CAT_DYNAMIC,
    PARAM_CAT_SYSTEM,
    PARAM_CAT_COUNT
} param_category_t;

// Flash durumlari
typedef enum {
    FLASH_OK = 0,
    FLASH_ERROR_WRITE,
    FLASH_ERROR_READ,
    FLASH_ERROR_ERASE,
    FLASH_ERROR_VERIFY,
    FLASH_ERROR_INVALID_PARAM,
    FLASH_ERROR_NO_SPACE,
    FLASH_ERROR_CORRUPT,
} flash_result_t;

// Parametre yapisi
typedef struct {
    uint32_t id;                    // Parametre ID
    param_category_t category;      // Kategori
    uint32_t size;                  // Byte cinsinden boyut
    void *ram_ptr;                  // RAM'deki adres
    uint8_t dirty;                  // Degisti mi flag'i
    uint32_t last_change_time;      // Son degisiklik zamani
} param_descriptor_t;

// Flash config
typedef struct {
    uint32_t write_delay_ms;        // Yazma gecikmesi (ms)
    uint8_t verification_enabled;   // Yazma dogrulamasi
} flash_config_t;

// API Fonksiyonlari
flash_result_t flashManagerInit(const flash_config_t *config);
flash_result_t flashManagerRegisterParam(const param_descriptor_t *param);
flash_result_t flashManagerUpdate(uint32_t current_time);
flash_result_t flashManagerSaveAll(void);
flash_result_t flashManagerLoadAll(void);
flash_result_t flashManagerLoadCategory(param_category_t category);
flash_result_t flashManagerMarkDirty(uint32_t param_id);
flash_result_t flashManagerForceWrite(uint32_t param_id);

// Utility Fonksiyonlari
uint32_t flashManagerGetWriteCount(param_category_t category);
uint8_t flashManagerIsDirty(uint32_t param_id);
flash_result_t flashManagerGetHealth(void);
flash_result_t flashManagerGetValidSectors(uint8_t *valid_sectors);
void flashManagerPrintUsageReport(void);

// Flash temizleme
void flashManagerEraseAllUserFlash(void);
void flashManagerEraseSectorsBy(const flash_config_t *config);

// Error callback
typedef void (*flash_error_callback_t)(flash_result_t error, uint32_t param_id);
void flashManagerSetErrorCallback(flash_error_callback_t callback);

#endif // FLASH_MANAGER_H
