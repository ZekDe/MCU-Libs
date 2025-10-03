#include "flash_manager.h"
#include "SW2023.h"
#include <string.h>
#include <stdio.h>

// Magic number'lar, bu degerler degistirildiginde flash'a hiç yazmamis gibi davranir ve 0'dan baslar.
// fakat gerçekten silme ihtiyaci olursa erase fonksiyonu kullanilabilir
#define FLASH_MAGIC_STATIC      0x53544156  // "STAT"
#define FLASH_MAGIC_DYNAMIC     0x44594E46  // "DYNA"  
#define FLASH_MAGIC_SYSTEM      0x53595356  // "SYST"

#define DEBUG_FLASH_MANAGER

// Statik layout tanimi - runtime hesaplama yok!
static const category_layout_t flash_layout[PARAM_CAT_COUNT] = 
{
    [PARAM_CAT_STATIC]  = {0, STATIC_SECTOR_COUNT, 1},
    [PARAM_CAT_DYNAMIC] = {STATIC_SECTOR_COUNT, DYNAMIC_SECTOR_COUNT, DYNAMIC_WEAR_SETS},
    [PARAM_CAT_SYSTEM]  = {STATIC_SECTOR_COUNT + DYNAMIC_SECTOR_COUNT, SYSTEM_SECTOR_COUNT, 1}
};

// Statik work buffer - malloc yok!
static uint8_t work_buffer[MAX_CATEGORY_DATA_SIZE];

// Manager state
typedef struct {
    flash_config_t config;
    param_descriptor_t params[MAX_PARAMS];
    uint8_t param_count;
    uint8_t current_wear_set[PARAM_CAT_COUNT];
    uint32_t write_counts[PARAM_CAT_COUNT];
    uint32_t sequence_numbers[PARAM_CAT_COUNT];
    flash_error_callback_t error_callback;
    uint8_t initialized;
} flash_manager_t;

static flash_manager_t mgr = {0};

// Iç fonksiyon prototipleri
static uint32_t getAbsoluteSectorAddress(uint8_t sector_index);
static uint8_t getCategoryStartSector(param_category_t category, uint8_t wear_set);
static uint32_t getMagicNumber(param_category_t category);
static param_descriptor_t* findParam(uint32_t param_id);
static flash_result_t initSequenceNumbers(void);
static flash_result_t saveCategoryMultiSector(param_category_t category);
static flash_result_t loadCategoryMultiSector(param_category_t category);
static uint32_t calcCRC32(const uint8_t *data, uint32_t len);
static flash_result_t flashWrite(uint32_t addr, const void *data, uint32_t size);
static flash_result_t flashRead(uint32_t addr, void *data, uint32_t size);
static flash_result_t flashErase(uint32_t addr);
static flash_result_t verifyWrite(uint32_t addr, const void *data, uint32_t size);

/**
 * @brief Flash Manager'i baslatir
 */
flash_result_t flashManagerInit(const flash_config_t *config)
{
    if (mgr.initialized) {
        return FLASH_OK;
    }
    
    // Config kopyala
    if (config) {
        mgr.config = *config;
    } else {
        // Default config
        mgr.config.write_delay_ms = 30000;
        mgr.config.verification_enabled = 1;
    }
    
    // State'i sifirla
    mgr.param_count = 0;
    memset(mgr.current_wear_set, 0, sizeof(mgr.current_wear_set));
    memset(mgr.write_counts, 0, sizeof(mgr.write_counts));
    memset(mgr.sequence_numbers, 0, sizeof(mgr.sequence_numbers));
    mgr.error_callback = NULL;
    
    // FMC baslat
    SYS_UnlockReg();
    FMC_Open();
    FMC_Close();
    SYS_LockReg();
    
    mgr.initialized = 1;
    
    // Mevcut flash'tan sequence number'lari oku
    initSequenceNumbers();
    
    return FLASH_OK;
}

/**
 * @brief Sequence number'lari flash'tan oku
 */
static flash_result_t initSequenceNumbers(void)
{
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; cat++) {
        param_category_t category = (param_category_t)cat;
        const category_layout_t *layout = &flash_layout[category];
        uint32_t max_sequence = 0;
        uint32_t max_write_count = 0;
        uint8_t best_wear_set = 0;
        
        // Tüm wear set'leri kontrol et
        for (uint8_t ws = 0; ws < layout->wear_sets; ws++) {
            uint8_t sector = getCategoryStartSector(category, ws);
            uint32_t addr = getAbsoluteSectorAddress(sector);
            flash_header_t header;
            
            if (flashRead(addr, &header, sizeof(header)) == FLASH_OK) {
                if (header.magic == getMagicNumber(category) && 
                    header.sequence > max_sequence) {
                    max_sequence = header.sequence;
                    max_write_count = header.write_count;
                    best_wear_set = ws;
                }
            }
        }
        
        mgr.sequence_numbers[category] = max_sequence;
        mgr.write_counts[category] = max_write_count;
        mgr.current_wear_set[category] = best_wear_set;
		
#ifdef DEBUG_FLASH_MANAGER
        printf("Init: Cat %d, seq=%u, writes=%u, wear_set=%u\n", 
               cat, max_sequence, max_write_count, best_wear_set);
#endif
    }
    
    return FLASH_OK;
}

/**
 * @brief Parametre kaydet
 */
flash_result_t flashManagerRegisterParam(const param_descriptor_t *param)
{
    if (!mgr.initialized || !param || mgr.param_count >= MAX_PARAMS) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    // Ayni ID var mi kontrol et
    if (findParam(param->id)) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    // Parametreyi kaydet
    mgr.params[mgr.param_count] = *param;
    mgr.params[mgr.param_count].dirty = 0;
    mgr.params[mgr.param_count].last_change_time = 0;
    mgr.params[mgr.param_count].sector_index = 0;
    mgr.param_count++;
    
    return FLASH_OK;
}

/**
 * @brief Flash manager güncelleme - main loop'ta çagrilmali
 */
flash_result_t flashManagerUpdate(uint32_t current_time)
{
    if (!mgr.initialized) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    flash_result_t result = FLASH_OK;
    uint8_t needs_save = 0;
    uint32_t oldest_change_time = UINT32_MAX;
    
    // TÜM dirty parametrelerin zamanini güncelle
    for (uint8_t i = 0; i < mgr.param_count; i++) 
    {
        param_descriptor_t *param = &mgr.params[i];
        
        if (param->dirty && param->category == PARAM_CAT_DYNAMIC) 
        {
            // Ilk kez dirty yapilmissa zamani kaydet
            if (param->last_change_time == 0) {
                param->last_change_time = current_time;
            }
            
            // En eski degisiklik zamanini bul
            if (param->last_change_time < oldest_change_time) {
                oldest_change_time = param->last_change_time;
            }
            needs_save = 1;
        }
    }
    
    // Timeout kontrolü
    if (needs_save && (current_time - oldest_change_time) >= mgr.config.write_delay_ms) 
    {
        result = saveCategoryMultiSector(PARAM_CAT_DYNAMIC);
        if (result != FLASH_OK && mgr.error_callback) {
            mgr.error_callback(result, 0);
        }
    }
    
    return result;
}

/**
 * @brief Multi-sector save - MALLOC'SUZ
 */
static flash_result_t saveCategoryMultiSector(param_category_t category)
{
    const category_layout_t *layout = &flash_layout[category];
    
    // 1. Toplam veri boyutunu hesapla
    uint32_t total_data_size = 0;
    for (uint8_t i = 0; i < mgr.param_count; i++) {
        if (mgr.params[i].category == category) {
            total_data_size += mgr.params[i].size;
        }
    }
    
    if (total_data_size == 0) {
        return FLASH_OK; // Bu kategoride parametre yok
    }
    
    // Buffer siniri kontrolü
    if (total_data_size > MAX_CATEGORY_DATA_SIZE) {
        return FLASH_ERROR_NO_SPACE;
    }
    
    // 2. Kaç sektör gerekli?
    uint32_t sectors_needed = 0;
    uint32_t remaining = total_data_size;
    while (remaining > 0) {
        sectors_needed++;
        remaining = (remaining > SECTOR_DATA_SIZE) ? 
                   (remaining - SECTOR_DATA_SIZE) : 0;
    }
    
    // Wear leveling için maksimum sektör sayisi
    uint8_t max_sectors = (category == PARAM_CAT_DYNAMIC) ?
                         (layout->sector_count / layout->wear_sets) :
                         layout->sector_count;
    
    if (sectors_needed > max_sectors) {
        return FLASH_ERROR_NO_SPACE;
    }
    
    // 3. Wear set seçimi
    uint8_t wear_set = mgr.current_wear_set[category];
    if (category == PARAM_CAT_DYNAMIC) {
        wear_set = (wear_set + 1) % layout->wear_sets;
        mgr.current_wear_set[category] = wear_set;
    }
    
    // 4. Parametreleri buffer'a topla
    memset(work_buffer, 0xFF, sizeof(work_buffer));
    uint32_t buf_offset = 0;
    
    for (uint8_t i = 0; i < mgr.param_count; i++) 
	{
        if (mgr.params[i].category == category) 
		{
            memcpy(work_buffer + buf_offset, mgr.params[i].ram_ptr, mgr.params[i].size);
            mgr.params[i].sector_index = buf_offset / SECTOR_DATA_SIZE;
            buf_offset += mgr.params[i].size;
        }
    }
    
    // 5. Sektörlere yaz
    uint8_t start_sector = getCategoryStartSector(category, wear_set);
    uint32_t data_written = 0;
    
    mgr.sequence_numbers[category]++;
    mgr.write_counts[category]++;
    
    for (uint8_t sector = 0; sector < sectors_needed; sector++) {
        uint32_t addr = getAbsoluteSectorAddress(start_sector + sector);
        
        // Sektörü sil
        if (flashErase(addr) != FLASH_OK) {
            mgr.sequence_numbers[category]--;
            mgr.write_counts[category]--;
            return FLASH_ERROR_ERASE;
        }
        
        // Header hazirla
        flash_header_t header;
        header.magic = getMagicNumber(category);
        header.sequence = mgr.sequence_numbers[category];
        header.write_count = mgr.write_counts[category];
        header.next_sector = (sector < sectors_needed - 1) ? 
                            (start_sector + sector + 1) : 0xFF;
        
        // Bu sektöre yazilacak veri miktari
        uint32_t chunk_size = (total_data_size - data_written > SECTOR_DATA_SIZE) ?
                             SECTOR_DATA_SIZE : (total_data_size - data_written);
        header.data_size = chunk_size;
        header.crc32 = calcCRC32(work_buffer + data_written, chunk_size);
        
        // Header'i yaz
        if (flashWrite(addr, &header, sizeof(header)) != FLASH_OK) {
            mgr.sequence_numbers[category]--;
            mgr.write_counts[category]--;
            return FLASH_ERROR_WRITE;
        }
        
        // Veriyi yaz
        if (flashWrite(addr + sizeof(header), 
                      work_buffer + data_written, 
                      chunk_size) != FLASH_OK) {
            mgr.sequence_numbers[category]--;
            mgr.write_counts[category]--;
            return FLASH_ERROR_WRITE;
        }
        
        // Dogrulama
        if (mgr.config.verification_enabled) 
		{
            if (verifyWrite(addr + sizeof(header), work_buffer + data_written, chunk_size) != FLASH_OK) 
			{
                mgr.sequence_numbers[category]--;
                mgr.write_counts[category]--;
                return FLASH_ERROR_VERIFY;
            }
        }
        
        data_written += chunk_size;
#ifdef DEBUG_FLASH_MANAGER
        printf("SAVE: Sector %d, addr 0x%X, %u bytes\n", 
               start_sector + sector, addr, chunk_size);
#endif
    }
    
    // 6. Dirty flag'leri temizle
    for (uint8_t i = 0; i < mgr.param_count; i++) {
        if (mgr.params[i].category == category) {
            mgr.params[i].dirty = 0;
            mgr.params[i].last_change_time = 0;
        }
    }
    
    return FLASH_OK;
}

/**
 * @brief Multi-sector load - MALLOC'SUZ
 * 
 * NEDEN WORK_BUFFER KULLANIYORUZ?
 * 1. Veriler birden fazla sektöre yayilmis olabilir (600 byte veri = 2 sektör)
 * 2. Her sektörde header var, veri parçalanmis durumda
 * 3. CRC kontrolü için önce tüm veriyi okumamiz gerekiyor
 * 4. Parametreler sirali saklaniyor ama farkli sektörlerde olabilir
 * 
 * Eski versiyonda tek sektör varsayimi vardi, yeni versiyonda multi-sector
 * destegi oldugu için önce tüm veriyi toplamak zorundayiz.
 */
static flash_result_t loadCategoryMultiSector(param_category_t category)
{
    const category_layout_t *layout = &flash_layout[category];
    flash_header_t header;
    
    // 1. En güncel wear set'i bul
    uint32_t max_sequence = 0;
    uint8_t best_wear_set = 0;
    
    for (uint8_t ws = 0; ws < layout->wear_sets; ws++) 
	{
        uint8_t sector = getCategoryStartSector(category, ws);
        uint32_t addr = getAbsoluteSectorAddress(sector);
        
        if (flashRead(addr, &header, sizeof(header)) == FLASH_OK) 
		{
            if (header.magic == getMagicNumber(category) && header.sequence > max_sequence) 
			{
                max_sequence = header.sequence;
                best_wear_set = ws;
            }
        }
    }
    
    if (max_sequence == 0) {
        return FLASH_ERROR_READ;
    }
    
    // 2. Buffer'i temizle
    memset(work_buffer, 0, sizeof(work_buffer));
    
    // 3. Seçilen wear set'ten veriyi oku (birden fazla sektör olabilir!)
    uint8_t start_sector = getCategoryStartSector(category, best_wear_set);
    uint8_t current_sector = start_sector;
    uint32_t data_read = 0;
    
    while (current_sector != 0xFF && data_read < MAX_CATEGORY_DATA_SIZE) 
	{
        uint32_t addr = getAbsoluteSectorAddress(current_sector);
        
        // Header oku
        if (flashRead(addr, &header, sizeof(header)) != FLASH_OK) {
            return FLASH_ERROR_READ;
        }
        
        // Magic kontrol
        if (header.magic != getMagicNumber(category)) {
            return FLASH_ERROR_CORRUPT;
        }
        
        // Veri boyutu kontrolü
        if (data_read + header.data_size > MAX_CATEGORY_DATA_SIZE) {
            return FLASH_ERROR_NO_SPACE;
        }
        
        // Veriyi work_buffer'a oku (header'dan sonraki kisim)
        if (flashRead(addr + sizeof(header), work_buffer + data_read, header.data_size) != FLASH_OK) 
		{
            return FLASH_ERROR_READ;
        }
        
        // CRC kontrolü, todo: donanimsal CRC eklenecek
        uint32_t calc_crc = calcCRC32(work_buffer + data_read, header.data_size);
		
        if (calc_crc != header.crc32) 
		{
#ifdef DEBUG_FLASH_MANAGER
            printf("CRC Error: Expected 0x%X, Got 0x%X\n", 
                   header.crc32, calc_crc);
#endif
            return FLASH_ERROR_CORRUPT;
        }
        
        data_read += header.data_size;
		
#ifdef DEBUG_FLASH_MANAGER
        printf("LOAD: Sector %d, addr 0x%X, %u bytes\n", 
               current_sector, addr, header.data_size);
#endif
        // Sonraki sektöre geç (eger varsa)
        if (header.next_sector == 0xFF) break;
        current_sector = header.next_sector;
    }
    
    // 4. Work buffer'dan RAM'e kopyala (tüm veri toplandiktan sonra)
    uint32_t offset = 0;
    for (uint8_t i = 0; i < mgr.param_count; i++) 
	{
        if (mgr.params[i].category == category) 
		{
            if (offset + mgr.params[i].size > data_read) 
			{
                return FLASH_ERROR_CORRUPT;
            }
            
            memcpy(mgr.params[i].ram_ptr, work_buffer + offset, mgr.params[i].size);
            offset += mgr.params[i].size;
        }
    }
    
    // State güncelle
    mgr.write_counts[category] = header.write_count;
    mgr.sequence_numbers[category] = header.sequence;
    mgr.current_wear_set[category] = best_wear_set;
    
    return FLASH_OK;
}

/**
 * @brief Tüm kategorileri kaydet
 */
flash_result_t flashManagerSaveAll(void)
{
    if (!mgr.initialized) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    flash_result_t result;
    
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; cat++) {
        result = saveCategoryMultiSector((param_category_t)cat);
        if (result != FLASH_OK) {
            return result;
        }
    }
    
    return FLASH_OK;
}

/**
 * @brief Tüm kategorileri yükle
 */
flash_result_t flashManagerLoadAll(void)
{
    if (!mgr.initialized) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    flash_result_t result;
    
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; cat++) {
        result = loadCategoryMultiSector((param_category_t)cat);
        if (result != FLASH_OK) {
#ifdef DEBUG_FLASH_MANAGER
            printf("Load failed for category %d: %d\n", cat, result);
#endif
        }
    }
    
    return FLASH_OK;
}

/**
 * @brief Parametreyi dirty olarak isaretle
 */
flash_result_t flashManagerMarkDirty(uint32_t param_id)
{
    param_descriptor_t *param = findParam(param_id);
    if (!param) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    param->dirty = 1;
    param->last_change_time = 0;
    
    return FLASH_OK;
}

/**
 * @brief Parametreyi zorla kaydet
 */
flash_result_t flashManagerForceWrite(uint32_t param_id)
{
    param_descriptor_t *param = findParam(param_id);
    if (!param) {
        return FLASH_ERROR_INVALID_PARAM;
    }
    
    return saveCategoryMultiSector(param->category);
}

/**
 * @brief Varsayilan degerleri ayarla
 */

flash_result_t flashManagerSetDefaults(param_category_t category)
{
    for (uint8_t i = 0; i < mgr.param_count; i++) 
	{
        param_descriptor_t *param = &mgr.params[i];
        if (param->category == category)
		{
            if (param->size <= sizeof(uint32_t)) 
			{
                // Küçük degerler için default_value kullan
                memcpy(param->ram_ptr, &param->default_value, param->size);
            } 
			else 
			{
                // Array'ler için sifirla
                memset(param->ram_ptr, 0, param->size);
            }
            param->dirty = 1;
        }
    }
    return FLASH_OK;
}

/**
 * @brief Geçerli sektörleri kontrol et
 */
flash_result_t flashManagerGetValidSectors(uint8_t *valid_sectors)
{
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; cat++) {
        param_category_t category = (param_category_t)cat;
        const category_layout_t *layout = &flash_layout[category];
        uint8_t valid_found = 0;
        
        for (uint8_t ws = 0; ws < layout->wear_sets; ws++) {
            uint8_t sector = getCategoryStartSector(category, ws);
            uint32_t addr = getAbsoluteSectorAddress(sector);
            flash_header_t header;
            
            if (flashRead(addr, &header, sizeof(header)) == FLASH_OK &&
                header.magic == getMagicNumber(category)) {
                valid_found = 1;
                break;
            }
        }
        
        valid_sectors[cat] = valid_found;
    }
    
    return FLASH_OK;
}

/**
 * @brief Yazma sayisini döndür
 */
uint32_t flashManagerGetWriteCount(param_category_t category)
{
    if (category >= PARAM_CAT_COUNT) {
        return 0;
    }
    return mgr.write_counts[category];
}

/**
 * @brief Parametre dirty mi?
 */
uint8_t flashManagerIsDirty(uint32_t param_id)
{
    param_descriptor_t *param = findParam(param_id);
    return param ? param->dirty : 0;
}

/**
 * @brief Flash saglik durumu
 */
flash_result_t flashManagerGetHealth(void)
{
    for (uint8_t i = 0; i < PARAM_CAT_COUNT; i++) {
        if (mgr.write_counts[i] > 8000) {
            return FLASH_ERROR_WRITE;
        }
    }
    return FLASH_OK;
}

/**
 * @brief Kullanim raporu
 */
#ifdef DEBUG_FLASH_MANAGER
void flashManagerPrintUsageReport(void)
{
    printf("\n=== FLASH USAGE REPORT ===\n");
    printf("Layout:\n");
    printf("  STATIC:  %d sectors at 0x%X\n", 
           STATIC_SECTOR_COUNT, getAbsoluteSectorAddress(0));
    printf("  DYNAMIC: %d sectors at 0x%X (wear sets: %d)\n", 
           DYNAMIC_SECTOR_COUNT, 
           getAbsoluteSectorAddress(STATIC_SECTOR_COUNT),
           DYNAMIC_WEAR_SETS);
    printf("  SYSTEM:  %d sectors at 0x%X\n", 
           SYSTEM_SECTOR_COUNT,
           getAbsoluteSectorAddress(STATIC_SECTOR_COUNT + DYNAMIC_SECTOR_COUNT));
    
    printf("\nWrite Counts:\n");
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; cat++) {
        const char* names[] = {"STATIC", "DYNAMIC", "SYSTEM"};
        printf("  %s: %u writes, seq=%u, wear_set=%u\n", 
               names[cat], 
               mgr.write_counts[cat],
               mgr.sequence_numbers[cat],
               mgr.current_wear_set[cat]);
    }
    
    uint32_t total_used = 0;
    for (uint8_t i = 0; i < mgr.param_count; i++) {
        total_used += mgr.params[i].size;
    }
    printf("\nData: %u/%u bytes (%.1f%%)\n", 
           total_used, MAX_CATEGORY_DATA_SIZE,
           (total_used * 100.0) / MAX_CATEGORY_DATA_SIZE);
    
    printf("==========================\n");
}
#endif
/**
 * @brief Tüm kullanici flash'ini sil
 */
void flashManagerEraseAllUserFlash(void)
{
    SYS_UnlockReg();
    FMC_Open();
	uint32_t current_addr = FLASH_USER_BASE;
    while (current_addr < FLASH_END_ADDR) {
        FMC_Erase(current_addr);
        current_addr += FLASH_SECTOR_SIZE;
    }
    
    FMC_Close();
    SYS_LockReg();
    
    // State'i sifirla
    memset(mgr.write_counts, 0, sizeof(mgr.write_counts));
    memset(mgr.sequence_numbers, 0, sizeof(mgr.sequence_numbers));
    memset(mgr.current_wear_set, 0, sizeof(mgr.current_wear_set));
}

/**
 * @brief Config'e göre sektörleri sil
 */
void flashManagerEraseSectorsBy(const flash_config_t *config)
{
    if (!config) return;
    
    SYS_UnlockReg();
    FMC_Open();
    
    for (uint8_t cat = 0; cat < PARAM_CAT_COUNT; cat++) {
        const category_layout_t *layout = &flash_layout[cat];
        for (uint8_t sector = 0; sector < layout->sector_count; sector++) {
            uint32_t addr = getAbsoluteSectorAddress(layout->start_sector + sector);
            FMC_Erase(addr);
        }
    }
    
    FMC_Close();
    SYS_LockReg();
}

/**
 * @brief Error callback ayarla
 */
void flashManagerSetErrorCallback(flash_error_callback_t callback)
{
    mgr.error_callback = callback;
}

// ========== Helper Fonksiyonlar ==========

static uint32_t getAbsoluteSectorAddress(uint8_t sector_index)
{
    return FLASH_USER_BASE + (sector_index * FLASH_SECTOR_SIZE);
}

static uint8_t getCategoryStartSector(param_category_t category, uint8_t wear_set)
{
    const category_layout_t *layout = &flash_layout[category];
    
    if (category == PARAM_CAT_DYNAMIC && layout->wear_sets > 1) {
        uint8_t sectors_per_set = layout->sector_count / layout->wear_sets;
        return layout->start_sector + (wear_set * sectors_per_set);
    }
    
    return layout->start_sector;
}

static uint32_t getMagicNumber(param_category_t category)
{
    switch (category) {
        case PARAM_CAT_STATIC:  return FLASH_MAGIC_STATIC;
        case PARAM_CAT_DYNAMIC: return FLASH_MAGIC_DYNAMIC;
        case PARAM_CAT_SYSTEM:  return FLASH_MAGIC_SYSTEM;
        default: return 0xFFFFFFFF;
    }
}

static param_descriptor_t* findParam(uint32_t param_id)
{
    for (uint8_t i = 0; i < mgr.param_count; i++) {
        if (mgr.params[i].id == param_id) {
            return &mgr.params[i];
        }
    }
    return NULL;
}

/**
 * @brief CRC32 hesaplama
 */
static uint32_t calcCRC32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

/**
 * @brief Flash yazma dogrulama
 */
static flash_result_t verifyWrite(uint32_t addr, const void *data, uint32_t size)
{
    static uint8_t verify_buffer[64];
    
    for (uint32_t offset = 0; offset < size; offset += sizeof(verify_buffer)) {
        uint32_t chunk_size = (size - offset > sizeof(verify_buffer)) ? 
                             sizeof(verify_buffer) : (size - offset);
        
        if (flashRead(addr + offset, verify_buffer, chunk_size) != FLASH_OK) {
            return FLASH_ERROR_READ;
        }
        
        if (memcmp(verify_buffer, (const uint8_t*)data + offset, chunk_size) != 0) {
            return FLASH_ERROR_VERIFY;
        }
    }
    
    return FLASH_OK;
}

/**
 * @brief Flash yazma
 */
static flash_result_t flashWrite(uint32_t addr, const void *data, uint32_t size)
{
    __disable_irq();
    SYS_UnlockReg();
    FMC_Open();
    
    const uint32_t *src = (const uint32_t*)data;
    flash_result_t result = FLASH_OK;
    
    for (uint32_t i = 0; i < size; i += 4) {
        FMC_Write(addr + i, *src);
        if(g_FMC_i32ErrCode != eFMC_ERRCODE_SUCCESS) {
            result = FLASH_ERROR_WRITE;
            break;
        }
        src++;
    }
    
    FMC_Close();
    SYS_LockReg();
    __enable_irq();
    
    return result;
}

/**
 * @brief Flash okuma
 */
static flash_result_t flashRead(uint32_t addr, void *data, uint32_t size)
{
    __disable_irq();
    SYS_UnlockReg();
    FMC_Open();
    
    uint32_t *dst = (uint32_t*)data;
    
    for (uint32_t i = 0; i < size; i += 4) {
        *dst = FMC_Read(addr + i);
        dst++;
    }
    
    FMC_Close();
    SYS_LockReg();
    __enable_irq();
    
    return FLASH_OK;
}

/**
 * @brief Sektör silme
 */
static flash_result_t flashErase(uint32_t addr)
{
    __disable_irq();
    SYS_UnlockReg();
    FMC_Open();
    
    flash_result_t result = FLASH_OK;
    if (FMC_Erase(addr) != 0) {
        result = FLASH_ERROR_ERASE;
    }
    
    FMC_Close();
    SYS_LockReg();
    __enable_irq();
    
    return result;
}

