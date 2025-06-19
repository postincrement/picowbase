#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/spi.h"

// SD Card types
#define SD_TYPE_UNKNOWN 0
#define SD_TYPE_SD1     1
#define SD_TYPE_SD2     2
#define SD_TYPE_SDHC    3

// FAT32 structures
typedef struct {
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
} __attribute__((packed)) fat32_boot_sector_t;

class SDCard {
private:
    // SD Card configuration
    static spi_inst_t* SD_SPI_PORT;  // spi1
    static const int SD_MOSI_PIN = 11;
    static const int SD_MISO_PIN = 12;
    static const int SD_SCK_PIN = 13;
    static const int SD_CS_PIN = 14;

    // SD Card commands
    static const uint8_t CMD0 = 0;     // GO_IDLE_STATE
    static const uint8_t CMD1 = 1;     // SEND_OP_COND
    static const uint8_t CMD8 = 8;     // SEND_IF_COND
    static const uint8_t CMD9 = 9;     // SEND_CSD
    static const uint8_t CMD10 = 10;   // SEND_CID
    static const uint8_t CMD12 = 12;   // STOP_TRANSMISSION
    static const uint8_t CMD16 = 16;   // SET_BLOCKLEN
    static const uint8_t CMD17 = 17;   // READ_SINGLE_BLOCK
    static const uint8_t CMD18 = 18;   // READ_MULTIPLE_BLOCK
    static const uint8_t CMD23 = 23;   // SET_BLOCK_COUNT
    static const uint8_t CMD24 = 24;   // WRITE_BLOCK
    static const uint8_t CMD25 = 25;   // WRITE_MULTIPLE_BLOCK
    static const uint8_t CMD41 = 41;   // SEND_OP_COND (ACMD)
    static const uint8_t CMD55 = 55;   // APP_CMD
    static const uint8_t CMD58 = 58;   // READ_OCR

    // SD Card response types
    static const uint8_t R1_IDLE_STATE = 0x01;
    static const uint8_t R1_ERASE_RESET = 0x02;
    static const uint8_t R1_ILLEGAL_COMMAND = 0x04;
    static const uint8_t R1_COM_CRC_ERROR = 0x08;
    static const uint8_t R1_ERASE_SEQUENCE_ERROR = 0x10;
    static const uint8_t R1_ADDRESS_ERROR = 0x20;
    static const uint8_t R1_PARAMETER_ERROR = 0x40;

    // Private SPI functions
    void spi_init();
    void cs_low();
    void cs_high();
    uint8_t spi_transfer(uint8_t data);
    void spi_transfer_multiple(const uint8_t* data_out, uint8_t* data_in, size_t length);
    uint8_t send_command(uint8_t cmd, uint32_t arg);

public:
    // SD Card state
    bool initialized;
    uint8_t card_type;
    uint32_t card_size;
    fat32_boot_sector_t boot_sector;
    uint32_t first_fat_sector;
    uint32_t root_dir_sector;
    uint32_t data_sector;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_sector;

    // Constructor
    SDCard();

    // Public interface
    bool init();
    bool read_block(uint32_t block_addr, uint8_t* buffer);
    bool write_block(uint32_t block_addr, const uint8_t* buffer);
    bool parse_boot_sector();
    bool format();
    void spi_test();
    
    // Getter methods
    bool isInitialized() const { return initialized; }
    uint8_t getType() const { return card_type; }
    uint32_t getSize() const { return card_size; }
    uint32_t getFirstFatSector() const { return first_fat_sector; }
    uint32_t getFat32RootDirSector() const { return root_dir_sector; }
    uint32_t getDataSector() const { return data_sector; }
    uint32_t getSectorsPerCluster() const { return sectors_per_cluster; }
    uint32_t getBytesPerSector() const { return bytes_per_sector; }
};

// Global instance
extern SDCard sd_card;

#endif // SD_CARD_H 