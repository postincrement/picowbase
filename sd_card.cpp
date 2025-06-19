#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "sd_card.h"

// Global instance
SDCard sd_card;

// Constructor
SDCard::SDCard() {
    initialized = false;
    card_type = SD_TYPE_UNKNOWN;
    card_size = 0;
    first_fat_sector = 0;
    root_dir_sector = 0;
    data_sector = 0;
    sectors_per_cluster = 0;
    bytes_per_sector = 0;
}

// Private SPI functions
void SDCard::spi_init() {
    ::spi_init(SD_SPI_PORT, 400000);  // Start at 400kHz for initialization
    gpio_set_function(SD_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SD_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SD_SCK_PIN, GPIO_FUNC_SPI);
    gpio_init(SD_CS_PIN);
    gpio_set_dir(SD_CS_PIN, GPIO_OUT);
    gpio_put(SD_CS_PIN, 1);  // CS high (inactive)
}

void SDCard::cs_low() {
    gpio_put(SD_CS_PIN, 0);
    sleep_us(1);
}

void SDCard::cs_high() {
    sleep_us(1);
    gpio_put(SD_CS_PIN, 1);
}

uint8_t SDCard::spi_transfer(uint8_t data) {
    uint8_t received = 0;
    ::spi_write_read_blocking(SD_SPI_PORT, &data, &received, 1);
    return received;
}

void SDCard::spi_transfer_multiple(const uint8_t* data_out, uint8_t* data_in, size_t length) {
    ::spi_write_read_blocking(SD_SPI_PORT, data_out, data_in, length);
}

uint8_t SDCard::send_command(uint8_t cmd, uint32_t arg) {
    uint8_t command[6];
    command[0] = 0x40 | cmd;  // Command byte
    command[1] = (arg >> 24) & 0xFF;  // Argument (big endian)
    command[2] = (arg >> 16) & 0xFF;
    command[3] = (arg >> 8) & 0xFF;
    command[4] = arg & 0xFF;
    command[5] = 0x95;  // CRC (only needed for CMD0 and CMD8)
    
    cs_low();
    
    // Send command
    for (int i = 0; i < 6; i++) {
        spi_transfer(command[i]);
    }
    
    // Wait for response (up to 8 bytes)
    uint8_t response = 0xFF;
    for (int i = 0; i < 8; i++) {
        response = spi_transfer(0xFF);
        if ((response & 0x80) == 0) break;
    }
    
    cs_high();
    return response;
}

// Public interface
bool SDCard::read_block(uint32_t block_addr, uint8_t* buffer) {
    uint8_t response = send_command(CMD17, block_addr);
    if (response != 0) {
        return false;
    }
    
    // Wait for data token
    uint8_t token = 0xFF;
    for (int i = 0; i < 1000; i++) {
        token = spi_transfer(0xFF);
        if (token == 0xFE) break;
    }
    
    if (token != 0xFE) {
        return false;
    }
    
    // Read 512 bytes of data
    spi_transfer_multiple(NULL, buffer, 512);
    
    // Read CRC (ignore)
    spi_transfer(0xFF);
    spi_transfer(0xFF);
    
    return true;
}

bool SDCard::write_block(uint32_t block_addr, const uint8_t* buffer) {
    uint8_t response = send_command(CMD24, block_addr);
    if (response != 0) {
        return false;
    }
    
    // Send data token
    spi_transfer(0xFE);
    
    // Send 512 bytes of data
    spi_transfer_multiple(buffer, NULL, 512);
    
    // Send CRC (dummy)
    spi_transfer(0xFF);
    spi_transfer(0xFF);
    
    // Read data response
    uint8_t data_response = spi_transfer(0xFF);
    if ((data_response & 0x1F) != 0x05) {
        return false;
    }
    
    // Wait for write completion
    uint8_t busy = 0xFF;
    for (int i = 0; i < 1000; i++) {
        busy = spi_transfer(0xFF);
        if (busy == 0xFF) break;
    }
    
    return (busy == 0xFF);
}

void SDCard::spi_test() {
    printf("Testing SPI communication...\n");
    
    // Initialize SPI
    spi_init();
    
    // Test basic SPI transfer
    printf("Testing basic SPI transfer...\n");
    uint8_t test_data = 0x55;
    uint8_t received = spi_transfer(test_data);
    printf("Sent: 0x%02X, Received: 0x%02X\n", test_data, received);
    
    // Test CS control
    printf("Testing CS control...\n");
    cs_low();
    printf("CS set LOW\n");
    sleep_ms(100);
    cs_high();
    printf("CS set HIGH\n");
    
    // Test multiple transfers
    printf("Testing multiple transfers...\n");
    uint8_t test_pattern[] = {0xAA, 0x55, 0x00, 0xFF};
    uint8_t received_pattern[4];
    
    cs_low();
    for (int i = 0; i < 4; i++) {
        received_pattern[i] = spi_transfer(test_pattern[i]);
        printf("Sent: 0x%02X, Received: 0x%02X\n", test_pattern[i], received_pattern[i]);
    }
    cs_high();
    
    printf("SPI test completed\n");
}

bool SDCard::init() {
    printf("Initializing SD card...\n");
    printf("SPI Configuration: MOSI=%d, MISO=%d, SCK=%d, CS=%d\n", 
           SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN, SD_CS_PIN);
    
    spi_init();
    printf("SPI initialized at 400kHz\n");
    
    // Send 80 clock pulses with CS high
    printf("Sending 80 clock pulses...\n");
    cs_high();
    for (int i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }
    
    // Send CMD0 to reset card
    printf("Sending CMD0 (GO_IDLE_STATE)...\n");
    uint8_t response = send_command(CMD0, 0);
    printf("CMD0 response: 0x%02X\n", response);
    
    if (response != R1_IDLE_STATE) {
        printf("SD card not responding to CMD0 (response: 0x%02X)\n", response);
        printf("Expected response: 0x%02X (R1_IDLE_STATE)\n", R1_IDLE_STATE);
        printf("Possible issues:\n");
        printf("  1. Check wiring connections\n");
        printf("  2. Ensure SD card is powered with 3.3V\n");
        printf("  3. Verify SD card is properly inserted\n");
        printf("  4. Check for loose connections\n");
        return false;
    }
    
    printf("CMD0 successful, card is in idle state\n");
    
    // Send CMD8 to check voltage range
    printf("Sending CMD8 (SEND_IF_COND)...\n");
    response = send_command(CMD8, 0x1AA);
    printf("CMD8 response: 0x%02X\n", response);
    
    if (response == R1_IDLE_STATE) {
        // SD v2.0 card
        card_type = SD_TYPE_SD2;
        printf("SD v2.0 card detected\n");
    } else if (response == (R1_IDLE_STATE | R1_ILLEGAL_COMMAND)) {
        // SD v1.0 card
        card_type = SD_TYPE_SD1;
        printf("SD v1.0 card detected\n");
    } else {
        printf("Unknown SD card type (response: 0x%02X)\n", response);
        return false;
    }
    
    // Initialize card
    uint32_t timeout = 1000;
    while (timeout--) {
        if (card_type == SD_TYPE_SD2) {
            // Send ACMD41 for SD v2.0
            send_command(CMD55, 0);
            response = send_command(CMD41, 0x40000000);
        } else {
            // Send CMD1 for SD v1.0
            response = send_command(CMD1, 0);
        }
        
        if (response == 0) {
            break;
        }
        sleep_ms(10);
    }
    
    if (response != 0) {
        printf("SD card initialization failed\n");
        return false;
    }
    
    // Check if SDHC
    if (card_type == SD_TYPE_SD2) {
        response = send_command(CMD58, 0);
        if (response == 0) {
            uint8_t ocr[4];
            cs_low();
            for (int i = 0; i < 4; i++) {
                ocr[i] = spi_transfer(0xFF);
            }
            cs_high();
            
            if (ocr[0] & 0x40) {
                card_type = SD_TYPE_SDHC;
                printf("SDHC card detected\n");
            }
        }
    }
    
    // Set block size to 512 bytes (not needed for SDHC)
    if (card_type != SD_TYPE_SDHC) {
        response = send_command(CMD16, 512);
        if (response != 0) {
            printf("Failed to set block size\n");
            return false;
        }
    }
    
    // Increase SPI speed
    ::spi_set_baudrate(SD_SPI_PORT, 25000000);  // 25MHz
    
    printf("SD card initialized successfully\n");
    return true;
}

bool SDCard::parse_boot_sector() {
    uint8_t buffer[512];
    
    // Read boot sector (sector 0)
    if (!read_block(0, buffer)) {
        printf("Failed to read boot sector\n");
        return false;
    }
    
    // Copy to structure
    memcpy(&boot_sector, buffer, sizeof(fat32_boot_sector_t));
    
    // Check for FAT32 signature
    if (boot_sector.BPB_BytsPerSec != 512) {
        printf("Unsupported sector size: %d\n", boot_sector.BPB_BytsPerSec);
        return false;
    }
    
    if (strncmp((char*)boot_sector.BS_FilSysType, "FAT32", 5) != 0) {
        printf("Not a FAT32 filesystem\n");
        return false;
    }
    
    // Calculate important values
    bytes_per_sector = boot_sector.BPB_BytsPerSec;
    sectors_per_cluster = boot_sector.BPB_SecPerClus;
    first_fat_sector = boot_sector.BPB_RsvdSecCnt;
    root_dir_sector = first_fat_sector + 
                     (boot_sector.BPB_NumFATs * boot_sector.BPB_FATSz32);
    data_sector = root_dir_sector + 
                 ((boot_sector.BPB_RootEntCnt * 32) / bytes_per_sector);
    
    printf("FAT32 filesystem detected\n");
    printf("  Sectors per cluster: %d\n", sectors_per_cluster);
    printf("  Bytes per sector: %d\n", bytes_per_sector);
    printf("  First FAT sector: %d\n", first_fat_sector);
    printf("  Root directory sector: %d\n", root_dir_sector);
    printf("  Data sector: %d\n", data_sector);
    
    return true;
}

bool SDCard::format() {
    printf("WARNING: This will erase ALL data on the SD card!\n");
    printf("Are you sure you want to continue? (type 'yes' to confirm): ");
    
    // Read confirmation
    char confirm[10];
    int pos = 0;
    while (pos < 9) {
        int c = getchar_timeout_us(1000000); // 1 second timeout
        if (c == PICO_ERROR_TIMEOUT) {
            printf("\nFormat cancelled (timeout)\n");
            return false;
        }
        if (c == '\r' || c == '\n') {
            break;
        }
        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                printf("\b \b");
            }
        } else {
            confirm[pos++] = c;
            printf("%c", c);
        }
    }
    confirm[pos] = '\0';
    printf("\n");
    
    if (strcmp(confirm, "yes") != 0) {
        printf("Format cancelled\n");
        return false;
    }
    
    printf("Starting SD card format...\n");
    
    // Initialize SD card if not already done
    if (!initialized) {
        if (!init()) {
            printf("Failed to initialize SD card for formatting\n");
            return false;
        }
    }
    
    printf("Creating FAT32 filesystem...\n");
    
    // Create a basic FAT32 boot sector
    fat32_boot_sector_t new_boot_sector = {0};
    
    // Boot sector signature
    new_boot_sector.BS_jmpBoot[0] = 0xEB;
    new_boot_sector.BS_jmpBoot[1] = 0x58;
    new_boot_sector.BS_jmpBoot[2] = 0x90;
    
    // OEM name
    strncpy((char*)new_boot_sector.BS_OEMName, "PICO   ", 8);
    
    // BPB (BIOS Parameter Block)
    new_boot_sector.BPB_BytsPerSec = 512;           // 512 bytes per sector
    new_boot_sector.BPB_SecPerClus = 8;             // 8 sectors per cluster
    new_boot_sector.BPB_RsvdSecCnt = 32;            // 32 reserved sectors
    new_boot_sector.BPB_NumFATs = 2;                // 2 FATs
    new_boot_sector.BPB_RootEntCnt = 0;             // 0 for FAT32
    new_boot_sector.BPB_TotSec16 = 0;               // 0 for FAT32
    new_boot_sector.BPB_Media = 0xF8;               // Fixed disk
    new_boot_sector.BPB_FATSz16 = 0;                // 0 for FAT32
    new_boot_sector.BPB_SecPerTrk = 63;             // Sectors per track
    new_boot_sector.BPB_NumHeads = 255;             // Number of heads
    new_boot_sector.BPB_HiddSec = 0;                // Hidden sectors
    new_boot_sector.BPB_TotSec32 = 0;               // Will be calculated
    new_boot_sector.BPB_FATSz32 = 0;                // Will be calculated
    new_boot_sector.BPB_ExtFlags = 0;               // Extended flags
    new_boot_sector.BPB_FSVer = 0;                  // Filesystem version
    new_boot_sector.BPB_RootClus = 2;               // Root cluster
    new_boot_sector.BPB_FSInfo = 1;                 // FSInfo sector
    new_boot_sector.BPB_BkBootSec = 6;              // Backup boot sector
    new_boot_sector.BS_DrvNum = 0x80;               // Drive number
    new_boot_sector.BS_Reserved1 = 0;               // Reserved
    new_boot_sector.BS_BootSig = 0x29;              // Boot signature
    new_boot_sector.BS_VolID = 0x12345678;          // Volume ID
    strncpy((char*)new_boot_sector.BS_VolLab, "PICO_SD_CARD", 11); // Volume label
    strncpy((char*)new_boot_sector.BS_FilSysType, "FAT32   ", 8);  // Filesystem type
    
    // Calculate filesystem parameters based on card size
    // For now, use a reasonable default for a 4GB card
    uint32_t total_sectors = 8192; // 4MB for testing
    new_boot_sector.BPB_TotSec32 = total_sectors;
    
    // Calculate FAT size
    uint32_t data_sectors = total_sectors - 32; // Reserved sectors
    uint32_t clusters = data_sectors / 8;       // 8 sectors per cluster
    new_boot_sector.BPB_FATSz32 = (clusters * 4 + 511) / 512; // 4 bytes per cluster entry
    
    printf("Writing boot sector...\n");
    
    // Write boot sector to sector 0
    uint8_t boot_sector_buffer[512];
    memcpy(boot_sector_buffer, &new_boot_sector, sizeof(fat32_boot_sector_t));
    
    // Add boot sector signature
    boot_sector_buffer[510] = 0x55;
    boot_sector_buffer[511] = 0xAA;
    
    if (!write_block(0, boot_sector_buffer)) {
        printf("Failed to write boot sector\n");
        return false;
    }
    
    printf("Writing backup boot sector...\n");
    
    // Write backup boot sector to sector 6
    if (!write_block(6, boot_sector_buffer)) {
        printf("Failed to write backup boot sector\n");
        return false;
    }
    
    printf("Creating FAT tables...\n");
    
    // Create FAT1 (sector 32)
    uint8_t fat_buffer[512] = {0};
    fat_buffer[0] = 0xF8; // Media descriptor
    fat_buffer[1] = 0xFF;
    fat_buffer[2] = 0xFF;
    fat_buffer[3] = 0x0F; // End of cluster chain marker
    
    // Mark cluster 2 (root directory) as end of chain
    fat_buffer[8] = 0xFF;
    fat_buffer[9] = 0xFF;
    fat_buffer[10] = 0xFF;
    fat_buffer[11] = 0x0F;
    
    if (!write_block(32, fat_buffer)) {
        printf("Failed to write FAT1\n");
        return false;
    }
    
    // Create FAT2 (copy of FAT1)
    if (!write_block(32 + new_boot_sector.BPB_FATSz32, fat_buffer)) {
        printf("Failed to write FAT2\n");
        return false;
    }
    
    printf("Creating root directory...\n");
    
    // Create empty root directory (sector 64)
    uint8_t root_dir_buffer[512] = {0};
    if (!write_block(64, root_dir_buffer)) {
        printf("Failed to write root directory\n");
        return false;
    }
    
    printf("Format completed successfully!\n");
    printf("SD card is now formatted with FAT32 filesystem\n");
    
    // Re-parse the boot sector to update our internal state
    if (parse_boot_sector()) {
        initialized = true;
        printf("Filesystem mounted successfully\n");
        return true;
    } else {
        printf("Warning: Filesystem created but could not be mounted\n");
        return false;
    }
} 