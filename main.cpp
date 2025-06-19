#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/bootrom.h"
#include <string.h>
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "cyw43.h"
#include "lwip/init.h"
#include "pico/multicore.h"
#include <map>
#include <string>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "sd_card.h"

// Flash storage configuration
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define FLASH_CREDENTIALS_MAGIC 0x57494649  // "WIFI" in hex
#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64

// WiFi credentials structure
typedef struct {
    uint32_t magic;
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWORD_LEN];
    uint32_t checksum;
} wifi_credentials_t;

// Global variables
// Command buffer
#define MAX_CMD_LEN 128
char cmd_buffer[MAX_CMD_LEN];
int cmd_pos = 0;

// LED control
volatile bool led_blinking = false;
volatile uint32_t led_interval_ms = 500;  // Default 500ms interval

// WiFi state
bool wifi_initialized = false;

// SD Card state - now using the class instance
extern SDCard sd_card;

// Flash storage functions
uint32_t calculate_checksum(const wifi_credentials_t* creds) {
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)creds;
    for (size_t i = 0; i < sizeof(wifi_credentials_t) - sizeof(uint32_t); i++) {
        checksum += data[i];
    }
    return checksum;
}

bool save_wifi_credentials(const char* ssid, const char* password) {
    wifi_credentials_t creds;
    creds.magic = FLASH_CREDENTIALS_MAGIC;
    strncpy(creds.ssid, ssid, MAX_SSID_LEN - 1);
    creds.ssid[MAX_SSID_LEN - 1] = '\0';
    strncpy(creds.password, password, MAX_PASSWORD_LEN - 1);
    creds.password[MAX_PASSWORD_LEN - 1] = '\0';
    creds.checksum = calculate_checksum(&creds);
    
    // Disable interrupts during flash write
    uint32_t ints = save_and_disable_interrupts();
    
    // Erase the flash sector
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    
    // Write the credentials
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&creds, sizeof(wifi_credentials_t));
    
    // Restore interrupts
    restore_interrupts(ints);
    
    // Verify the write
    const wifi_credentials_t* stored = (const wifi_credentials_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    if (stored->magic == FLASH_CREDENTIALS_MAGIC && 
        stored->checksum == calculate_checksum(stored)) {
        return true;
    }
    
    return false;
}

bool load_wifi_credentials(char* ssid, char* password) {
    const wifi_credentials_t* stored = (const wifi_credentials_t*)(XIP_BASE + FLASH_TARGET_OFFSET);
    
    // Check magic number and checksum
    if (stored->magic != FLASH_CREDENTIALS_MAGIC) {
        return false;
    }
    
    if (stored->checksum != calculate_checksum(stored)) {
        return false;
    }
    
    // Copy credentials
    strncpy(ssid, stored->ssid, MAX_SSID_LEN);
    strncpy(password, stored->password, MAX_PASSWORD_LEN);
    
    return true;
}

bool clear_wifi_credentials() {
    // Disable interrupts during flash write
    uint32_t ints = save_and_disable_interrupts();
    
    // Erase the flash sector
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    
    // Restore interrupts
    restore_interrupts(ints);
    
    return true;
}

// Core 1 entry point
void core1_entry() {
    while (true) {
        if (led_blinking) {
            //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
        }
        sleep_ms(led_interval_ms);
    }
}

// Command handlers
void handle_help() {
    printf("\nAvailable commands:\n");
    printf("  help    - Show this help message\n");
    printf("  led on  - Turn LED on\n");
    printf("  led off - Turn LED off\n");
    printf("  led blink - Start LED blinking (500ms interval)\n");
    printf("  led blink <interval_ms> - Start LED blinking with custom interval\n");
    printf("  status  - Show system status\n");
    printf("  clear   - Clear screen\n");
    printf("  exit    - Enter bootloader mode for programming\n");
    printf("  ssid    - Scan for WiFi networks\n");
    printf("  wifi    - Connect using saved WiFi credentials\n");
    printf("  wifi <ssid> <password> - Connect with provided credentials\n");
    printf("  save <ssid> <password> - Save WiFi credentials to flash\n");
    printf("  load    - Load and connect using saved credentials\n");
    printf("  clear_creds - Clear saved WiFi credentials\n");
    printf("  sd_init - Initialize SD card and mount FAT32 filesystem\n");
    printf("  sd_ls   - List files and directories on SD card\n");
    printf("  sd_cat <file> - Display file contents from SD card\n");
    printf("  sd_write <file> <content> - Write text to file on SD card\n");
    printf("  sd_test - Test SPI communication with SD card\n");
    printf("  sd_format - Format SD card with FAT32 filesystem\n");
}

void handle_led(const char* state) {
    if (strcmp(state, "on") == 0) {
        led_blinking = false;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        printf("LED turned ON\n");
    } else if (strcmp(state, "off") == 0) {
        led_blinking = false;
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        printf("LED turned OFF\n");
    } else if (strcmp(state, "blink") == 0) {
        led_blinking = true;
        printf("LED blinking started\n");
    } else if (strncmp(state, "blink", 5) == 0) {
        // Parse interval if provided (e.g., "blink 1000")
        int interval = 500;  // default 500ms
        sscanf(state + 5, "%d", &interval);
        if (interval > 0) {
            led_interval_ms = interval;
            led_blinking = true;
            printf("LED blinking started with %dms interval\n", interval);
        } else {
            printf("Invalid interval. Using default 500ms\n");
            led_blinking = true;
        }
    } else {
        printf("Invalid LED state. Use 'on', 'off', 'blink', or 'blink <interval_ms>'\n");
    }
}

void handle_status() {
    printf("\nSystem Status:\n");
    printf("  LED State: %s\n", cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN) ? "ON" : "OFF");
    
    // WiFi status
    int wifi_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    const char* status_str;
    switch (wifi_status) {
        case CYW43_LINK_JOIN:
            status_str = "Connected";
            break;
        case CYW43_LINK_UP:
            status_str = "Connected";
            break;
        case CYW43_LINK_DOWN:
            status_str = "Disconnected";
            break;
        case CYW43_LINK_FAIL:
            status_str = "Connection Failed";
            break;
        case CYW43_LINK_NONET:
            status_str = "No Network";
            break;
        case CYW43_LINK_BADAUTH:
            status_str = "Authentication Failed";
            break;
        default:
            status_str = "Unknown";
            break;
    }
    printf("  WiFi Status: %s (code: %d)\n", status_str, wifi_status);
    
    if (wifi_status == CYW43_LINK_UP || wifi_status == CYW43_LINK_JOIN) {
        // Get IP address
        struct netif *netif = netif_default;
        if (netif != NULL) {
            printf("  IP Address: %s\n", ip4addr_ntoa(&netif->ip_addr));
        }
        
        // Get signal strength
        int32_t rssi;
        if (cyw43_wifi_get_rssi(&cyw43_state, &rssi) == 0) {
            printf("  Signal Strength: %d dBm\n", rssi);
        }
        
        // Get BSSID (MAC address of the access point)
        uint8_t bssid[6];
        if (cyw43_wifi_get_bssid(&cyw43_state, bssid) == 0) {
            printf("  BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        }
    }
    
    // SD Card status
    printf("  SD Card: %s\n", sd_card.isInitialized() ? "Initialized" : "Not initialized");
    if (sd_card.isInitialized()) {
        printf("  SD Card Type: ");
        switch (sd_card.getType()) {
            case SD_TYPE_SD1: printf("SD v1.0"); break;
            case SD_TYPE_SD2: printf("SD v2.0"); break;
            case SD_TYPE_SDHC: printf("SDHC"); break;
            default: printf("Unknown"); break;
        }
        printf("\n");
        printf("  FAT32 Filesystem: Mounted\n");
        printf("  Sectors per cluster: %d\n", sd_card.getSectorsPerCluster());
    }
}

void handle_clear() {
    printf("\033[2J\033[H");  // ANSI escape sequence to clear screen
}

void handle_exit() {
    printf("\nEntering bootloader mode...\n");
    printf("Device will now appear as a USB mass storage device.\n");
    printf("You can now program it using picotool or drag-and-drop UF2 files.\n");
    sleep_ms(1000);  // Give time for the message to be sent
    reset_usb_boot(0, 0);  // Enter bootloader mode
}

// WiFi configuration

void handle_wifi(const char* ssid, const char* password) {
    char actual_ssid[MAX_SSID_LEN];
    char actual_password[MAX_PASSWORD_LEN];
    bool using_saved_creds = false;
    
    // If no arguments provided, try to load saved credentials
    if (strlen(ssid) == 0 || strlen(password) == 0) {
        if (load_wifi_credentials(actual_ssid, actual_password)) {
            printf("Using saved WiFi credentials for network: %s\n", actual_ssid);
            using_saved_creds = true;
        } else {
            printf("Error: No saved WiFi credentials found and no credentials provided\n");
            printf("Usage: wifi <ssid> <password> - Connect with provided credentials\n");
            printf("       wifi - Connect using saved credentials\n");
            printf("Use 'save <ssid> <password>' to store credentials first.\n");
            return;
        }
    } else {
        // Use provided credentials
        strncpy(actual_ssid, ssid, MAX_SSID_LEN - 1);
        actual_ssid[MAX_SSID_LEN - 1] = '\0';
        strncpy(actual_password, password, MAX_PASSWORD_LEN - 1);
        actual_password[MAX_PASSWORD_LEN - 1] = '\0';
    }
    
    printf("Connecting to WiFi network '%s'...\n", actual_ssid);
    
    // Disable power management
    //cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);
    //printf("Power management disabled\n");
    
    // Enable station mode
    cyw43_arch_enable_sta_mode();
    printf("Station mode enabled\n");
    
    // Try to connect with retries
    const int max_retries = 3;
    int retry_count = 0;
    int result = 0;
    
    while (retry_count < max_retries) {
        printf("Connection attempt %d of %d...\n", retry_count + 1, max_retries);
        
        // Try to connect
        result = cyw43_arch_wifi_connect_timeout_ms(actual_ssid, actual_password, CYW43_AUTH_WPA3_WPA2_AES_PSK, 30000);
        if (result == 0) {
            break;  // Connection successful
        }
        
        printf("Connection attempt failed (error: %d), retrying...\n", result);
        retry_count++;
        if (retry_count < max_retries) {
            sleep_ms(2000);  // Wait before retry
        }
    }
    
    if (result != 0) {
        printf("Failed to connect after %d attempts\n", max_retries);
        return;
    }
    
    // Wait for connection to stabilize
    printf("Waiting for connection to stabilize...\n");
    sleep_ms(2000);
    
    // Verify connection
    int wifi_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    printf("WiFi status code: %d\n", wifi_status);
    
    if (wifi_status == CYW43_LINK_UP) {
        printf("Successfully connected to WiFi\n");
        
        // Get and display IP address
        struct netif *netif = netif_default;
        if (netif != NULL) {
            printf("IP Address: %s\n", ip4addr_ntoa(&netif->ip_addr));
        } else {
            printf("Warning: Network interface not available\n");
        }
    } else {
        const char* status_str;
        switch (wifi_status) {
            case CYW43_LINK_JOIN:
                status_str = "Joined";
                break;
            case CYW43_LINK_DOWN:
                status_str = "Link Down";
                break;
            case CYW43_LINK_FAIL:
                status_str = "Link Failed";
                break;
            case CYW43_LINK_NONET:
                status_str = "No Network";
                break;
            case CYW43_LINK_BADAUTH:
                status_str = "Authentication Failed";
                break;
            default:
                status_str = "Unknown";
                break;
        }
        printf("Connection failed - WiFi status: %s (%d)\n", status_str, wifi_status);
    }
}

void handle_save(const char* ssid, const char* password) {
    if (strlen(ssid) == 0 || strlen(password) == 0) {
        printf("Error: Both SSID and password are required\n");
        printf("Usage: save <ssid> <password>\n");
        return;
    }
    
    printf("Saving WiFi credentials to flash...\n");
    if (save_wifi_credentials(ssid, password)) {
        printf("WiFi credentials saved successfully!\n");
        printf("Credentials will be used automatically on next boot.\n");
    } else {
        printf("Failed to save WiFi credentials to flash.\n");
    }
}

void handle_load() {
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWORD_LEN];
    
    printf("Loading saved WiFi credentials...\n");
    if (load_wifi_credentials(ssid, password)) {
        printf("Found saved credentials for network: %s\n", ssid);
        printf("Attempting to connect...\n");
        handle_wifi(ssid, password);
    } else {
        printf("No saved WiFi credentials found.\n");
        printf("Use 'save <ssid> <password>' to store credentials.\n");
    }
}

void handle_clear_creds() {
    printf("Clearing saved WiFi credentials...\n");
    if (clear_wifi_credentials()) {
        printf("WiFi credentials cleared successfully.\n");
    } else {
        printf("Failed to clear WiFi credentials.\n");
    }
}

void handle_sd_init() {
    if (sd_card.isInitialized()) {
        printf("SD card already initialized\n");
        return;
    }
    
    if (sd_card.init() && sd_card.parse_boot_sector()) {
        printf("SD card and FAT32 filesystem ready\n");
    } else {
        printf("Failed to initialize SD card\n");
    }
}

void handle_sd_ls() {
    if (!sd_card.isInitialized()) {
        printf("SD card not initialized. Use 'sd_init' first.\n");
        return;
    }
    
    printf("Listing root directory...\n");
    
    // Read root directory sector
    uint8_t buffer[512];
    if (!sd_card.read_block(sd_card.getFat32RootDirSector(), buffer)) {
        printf("Failed to read root directory\n");
        return;
    }
    
    // Parse directory entries
    for (int i = 0; i < 512; i += 32) {
        uint8_t* entry = &buffer[i];
        
        // Check if entry is valid
        if (entry[0] == 0x00) {
            break;  // End of directory
        }
        if (entry[0] == 0xE5) {
            continue;  // Deleted file
        }
        if ((entry[11] & 0x10) == 0x10) {
            // Directory
            printf("DIR  ");
        } else {
            // File
            printf("FILE ");
        }
        
        // Print filename (8 characters)
        for (int j = 0; j < 8; j++) {
            if (entry[j] != ' ') {
                printf("%c", entry[j]);
            }
        }
        
        // Print extension (3 characters)
        if (entry[8] != ' ') {
            printf(".");
            for (int j = 8; j < 11; j++) {
                if (entry[j] != ' ') {
                    printf("%c", entry[j]);
                }
            }
        }
        
        // Print file size
        uint32_t file_size = (entry[31] << 24) | (entry[30] << 16) | (entry[29] << 8) | entry[28];
        printf("  %d bytes\n", file_size);
    }
}

void handle_sd_cat(const char* filename) {
    if (!sd_card.isInitialized()) {
        printf("SD card not initialized. Use 'sd_init' first.\n");
        return;
    }
    
    if (strlen(filename) == 0) {
        printf("Error: Filename required\n");
        printf("Usage: sd_cat <filename>\n");
        return;
    }
    
    printf("Reading file: %s\n", filename);
    
    // For now, just read the first sector of the root directory
    // In a full implementation, you'd need to parse the FAT chain
    uint8_t buffer[512];
    if (!sd_card.read_block(sd_card.getFat32RootDirSector(), buffer)) {
        printf("Failed to read directory\n");
        return;
    }
    
    // Simple file search (this is a basic implementation)
    bool found = false;
    for (int i = 0; i < 512; i += 32) {
        uint8_t* entry = &buffer[i];
        
        if (entry[0] == 0x00) break;
        if (entry[0] == 0xE5) continue;
        
        // Extract filename
        char entry_name[13];
        int name_pos = 0;
        
        // Copy main part of filename
        for (int j = 0; j < 8; j++) {
            if (entry[j] != ' ') {
                entry_name[name_pos++] = entry[j];
            }
        }
        
        // Add extension if present
        if (entry[8] != ' ') {
            entry_name[name_pos++] = '.';
            for (int j = 8; j < 11; j++) {
                if (entry[j] != ' ') {
                    entry_name[name_pos++] = entry[j];
                }
            }
        }
        entry_name[name_pos] = '\0';
        
        if (strcmp(entry_name, filename) == 0) {
            found = true;
            printf("File found! (Basic implementation - would need FAT parsing for full content)\n");
            break;
        }
    }
    
    if (!found) {
        printf("File not found: %s\n", filename);
    }
}

void handle_sd_write(const char* filename, const char* content) {
    if (!sd_card.isInitialized()) {
        printf("SD card not initialized. Use 'sd_init' first.\n");
        return;
    }
    
    if (strlen(filename) == 0 || strlen(content) == 0) {
        printf("Error: Both filename and content are required\n");
        printf("Usage: sd_write <filename> <content>\n");
        return;
    }
    
    printf("Writing to file: %s\n", filename);
    printf("Content: %s\n", content);
    printf("(Note: Full file writing requires FAT32 directory and FAT chain management)\n");
}

void handle_sd_format() {
    if (sd_card.format()) {
        printf("SD card formatted successfully!\n");
        printf("You can now use 'sd_ls' to verify the filesystem.\n");
    } else {
        printf("SD card formatting failed.\n");
    }
}

// WiFi scan callback for ssid command
static int ssid_scan_callback(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        std::map<std::string, int> & ssid_map = *(std::map<std::string, int> *)env;   
        std::string ssid((char *)result->ssid, (int)result->ssid_len);

        if (ssid_map.find(ssid) == ssid_map.end()) {
            ssid_map.insert(std::pair<std::string, int>(ssid, result->rssi));

            const char *auth_mode_str;
            switch (result->auth_mode) {
                case CYW43_AUTH_OPEN: auth_mode_str = "Open"; break;
                case CYW43_AUTH_WPA_TKIP_PSK: auth_mode_str = "WPA"; break;
                case CYW43_AUTH_WPA2_AES_PSK: auth_mode_str = "WPA2"; break;
                case CYW43_AUTH_WPA2_MIXED_PSK: auth_mode_str = "WPA2 Mixed"; break;
                case CYW43_AUTH_WPA3_SAE_AES_PSK: auth_mode_str = "WPA3"; break;
                case CYW43_AUTH_WPA3_WPA2_AES_PSK: auth_mode_str = "WPA2/WPA3"; break;
                default: auth_mode_str = "Unknown"; break;
            }

            printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %s (%u)\n",
                ssid.c_str(), result->rssi, result->channel,
                result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
                auth_mode_str, result->auth_mode);
        }
    } 
    return 0;
}


void handle_ssid() {
    printf("Scanning for WiFi networks...\n");

    printf("scan started\n");

    absolute_time_t scan_time = make_timeout_time_ms(10000);

    bool scanning = false;

    std::map<std::string, int> ssid_map;

    // scan for 10 seconds
    while (!time_reached(scan_time)) {
        if (!scanning) {
            cyw43_wifi_scan_options_t scan_options = {0};
            int err = cyw43_wifi_scan(&cyw43_state, &scan_options, &ssid_map, ssid_scan_callback);
            if (err != 0) {
                printf("error: cyw43_wifi_scan failed with code %d\n", err);
                return;
            }
        }
        else if (!cyw43_wifi_scan_active(&cyw43_state)) {
            scanning = false;
        }
        sleep_ms(100);
    }
}

// Process a complete command
void process_command() {
    if (cmd_pos == 0) return;  // Empty command
    
    cmd_buffer[cmd_pos] = '\0';  // Null terminate the command
    printf("\n");  // New line after command
    
    // Parse command
    char cmd[32] = {0};
    char arg[32] = {0};
    char subarg[64] = {0};
    
    if (strncmp(cmd_buffer, "wifi", 4) == 0 || strncmp(cmd_buffer, "save", 4) == 0 || strncmp(cmd_buffer, "sd_write", 8) == 0) {
        sscanf(cmd_buffer, "%31s %31s %63s", cmd, arg, subarg);
    } else {
        sscanf(cmd_buffer, "%31s %31s", cmd, arg);
    }
    
    // Execute command
    if (strcmp(cmd, "help") == 0) {
        handle_help();
    } else if (strcmp(cmd, "led") == 0) {
        handle_led(arg);
    } else if (strcmp(cmd, "status") == 0) {
        handle_status();
    } else if (strcmp(cmd, "clear") == 0) {
        handle_clear();
    } else if (strcmp(cmd, "exit") == 0) {
        handle_exit();
    } else if (strcmp(cmd, "wifi") == 0) {
        handle_wifi(arg, subarg);
    } else if (strcmp(cmd, "ssid") == 0) {
        handle_ssid();
    } else if (strcmp(cmd, "save") == 0) {
        handle_save(arg, subarg);
    } else if (strcmp(cmd, "load") == 0) {
        handle_load();
    } else if (strcmp(cmd, "clear_creds") == 0) {
        handle_clear_creds();
    } else if (strcmp(cmd, "sd_init") == 0) {
        handle_sd_init();
    } else if (strcmp(cmd, "sd_ls") == 0) {
        handle_sd_ls();
    } else if (strcmp(cmd, "sd_cat") == 0) {
        handle_sd_cat(arg);
    } else if (strcmp(cmd, "sd_write") == 0) {
        handle_sd_write(arg, subarg);
    } else if (strcmp(cmd, "sd_format") == 0) {
        handle_sd_format();
    } else if (strcmp(cmd, "sd_test") == 0) {
        sd_card.spi_test();
    } else {
        printf("Unknown command. Type 'help' for available commands.\n");
    }
    
    // Reset command buffer
    cmd_pos = 0;
    printf("\n> ");  // Show prompt
}

int main() {
    // Initialize stdio
    stdio_init_all();
    
    
    // Initialize WiFi
    //if (cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRALIA)) {
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43\n");
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    
    // Enable LED
    for (int i = 0; i < 20; i++) {
        // Toggle the LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !cyw43_arch_gpio_get(CYW43_WL_GPIO_LED_PIN));
        sleep_ms(250);
    }
    
    // Disable power management
    cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);
    
    // Enable station mode
    cyw43_arch_enable_sta_mode();
    
    // Try to connect using saved credentials
    char saved_ssid[MAX_SSID_LEN];
    char saved_password[MAX_PASSWORD_LEN];
    if (load_wifi_credentials(saved_ssid, saved_password)) {
        printf("Found saved WiFi credentials. Attempting to connect...\n");
        printf("Connecting to: %s\n", saved_ssid);
        
        // Try to connect with retries
        const int max_retries = 3;
        int retry_count = 0;
        int result = 0;
        
        while (retry_count < max_retries) {
            printf("Connection attempt %d of %d...\n", retry_count + 1, max_retries);
            
            result = cyw43_arch_wifi_connect_timeout_ms(saved_ssid, saved_password, CYW43_AUTH_WPA3_WPA2_AES_PSK, 30000);
            if (result == 0) {
                break;  // Connection successful
            }
            
            printf("Connection attempt failed (error: %d), retrying...\n", result);
            retry_count++;
            if (retry_count < max_retries) {
                sleep_ms(2000);  // Wait before retry
            }
        }
        
        if (result == 0) {
            printf("Successfully connected to saved WiFi network!\n");
            struct netif *netif = netif_default;
            if (netif != NULL) {
                printf("IP Address: %s\n", ip4addr_ntoa(&netif->ip_addr));
            }
        } else {
            printf("Failed to connect to saved WiFi network after %d attempts\n", max_retries);
        }
    } else {
        printf("No saved WiFi credentials found.\n");
    }
    
    printf("\nPico W WiFi CLI\n");
    printf("Type 'help' for available commands\n\n");
    printf("> ");  // Show prompt
    
    // Main loop
    while (true) {
        // Check for incoming characters
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            // Handle backspace
            if (c == '\b' || c == 127) {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    printf("\b \b");  // Erase character
                }
            }
            // Handle enter
            else if (c == '\r' || c == '\n') {
                process_command();
            }
            // Handle regular character
            else if (cmd_pos < MAX_CMD_LEN - 1) {
                cmd_buffer[cmd_pos++] = c;
                printf("%c", c);  // Echo character
            }
        }
        
        // Small delay to prevent busy waiting
        sleep_ms(10);
    }
} 