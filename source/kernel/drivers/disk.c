//
// Created by Alan on 12/07/2024.
// ATA PIO mode driver based on : https://wiki.osdev.org/ATA_PIO_Mode
//
#include <stdint.h>
#include <stddef.h>

#include "disk.h"
#include "../core/x86.h"
#include "../core/stdio.h"
#include "../timers/timer.h"

#define ATA_PRIMARY_DATA_PORT 0x1F0
#define ATA_PRIMARY_ERROR_PORT 0x1F1
#define ATA_PRIMARY_SECTOR_COUNT_PORT 0x1F2
#define ATA_PRIMARY_LBA_LOW_PORT 0x1F3
#define ATA_PRIMARY_LBA_MID_PORT 0x1F4
#define ATA_PRIMARY_LBA_HIGH_PORT 0x1F5
#define ATA_PRIMARY_DRIVE_PORT 0x1F6
#define ATA_PRIMARY_COMMAND_PORT 0x1F7
#define ATA_PRIMARY_STATUS_PORT 0x1F7

#define ATA_PRIMARY_ALT_STATUS_PORT 0x3F6
#define ATA_SECONDARY_ALT_STATUS_PORT 0x376

#define ATA_READ_SECTORS_CMD 0x20
#define ATA_WRITE_SECTORS_CMD 0x30

#define ATA_READ_MULTIPLE_CMD 0xC4
#define ATA_WRITE_MULTIPLE_CMD 0xC5

#define ATA_IDENTIFY_DEVICE_CMD 0xEC
#define ATA_FLUSH_CACHE_CMD 0xE7

#define ATA_SECONDARY_DATA_PORT 0x170
#define ATA_SECONDARY_ERROR_PORT 0x171

#define ATA_MASTER_DRIVE 0xA0
#define ATA_SLAVE_DRIVE 0xB0

// ATA status
#define ATA_STATUS_BSY 0x80		// Busy
#define ATA_STATUS_DRDY 0x40	// Device ready
#define ATA_STATUS_DRQ 0x08		// Data request
#define ATA_STATUS_ERR 0x01		// Error
#define ATA_STATUS_DF 0x20		// Device fault

#define ATA_FLOATING_BUS 0xFF

bool x86_Disk_GetDriveParams(uint8_t drive,
                                                    uint8_t* driveTypeOut,
                                                    uint16_t* cylindersOut,
                                                    uint16_t* sectorsOut,
                                                    uint16_t* headsOut) {
    return false; // TODO: implement
}

bool x86_Disk_Reset(uint8_t drive) {
    return false; // TODO: implement
}

bool x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t sector, uint16_t head, uint8_t count, void* lowerDataOut) {
    return false; // TODO: implement
}

static uint8_t last_selected_device = ATA_FLOATING_BUS;  // initial value is invalid - No drive

// Start of code from https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/disk.c
bool DISK_Initialize(DISK* disk, uint8_t driveNumber)
{
    uint8_t driveType;
    uint16_t cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(disk->id, &driveType, &cylinders, &sectors, &heads))
        return false;

    disk->id = driveNumber;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->sectors = sectors;

    return true;
}

void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylinderOut, uint16_t* sectorOut, uint16_t* headOut)
{
    // sector = (LBA % sectors per track + 1)
    *sectorOut = lba % disk->sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinderOut = (lba / disk->sectors) / disk->heads;

    // head = (LBA / sectors per track) % heads
    *headOut = (lba / disk->sectors) % disk->heads;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* dataOut)
{
    uint16_t cylinder, sector, head;

    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for (int i = 0; i < 3; i++)
    {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, dataOut))
            return true;

        x86_Disk_Reset(disk->id);
    }

    return false;
}
// End of code by Nanobyte

uint8_t read_status(void) {
    return x86_inb(ATA_PRIMARY_ALT_STATUS_PORT);
}

char* read_status_str() {
    uint8_t status = read_status();
    if (status & ATA_STATUS_BSY) {
        return "BUSY";
    } else if (status & ATA_STATUS_DRDY) {
        return "READY";
    } else if (status & ATA_STATUS_DRQ) {
        return "DATA REQUEST";
    } else if (status & ATA_STATUS_ERR) {
        return "ERROR";
    } else if (status & ATA_STATUS_DF) {
        return "DEVICE FAULT";
    } else {
        return "UNKNOWN";
    }
}

void wait_not_busy(void) {
    while (read_status() & ATA_STATUS_BSY);
}

void wait_ready(void) {
    while (!(read_status() & ATA_STATUS_DRDY));
}

int identify_device(ATA_IDENTIFY_DEVICE_DATA* device_info) {
    select_device(ATA_MASTER_DRIVE);
    wait_not_busy();

    // Identify cmd params
    x86_outb(ATA_PRIMARY_SECTOR_COUNT_PORT, 0);
    x86_outb(ATA_PRIMARY_LBA_LOW_PORT, 0);
    x86_outb(ATA_PRIMARY_LBA_MID_PORT, 0);
    x86_outb(ATA_PRIMARY_LBA_HIGH_PORT, 0);

    x86_outb(ATA_PRIMARY_COMMAND_PORT, ATA_IDENTIFY_DEVICE_CMD);

    uint8_t status = read_status();
    if (status == 0) {
        printf("[DISK] No device detected\r\n");
        return 0;
    }

    wait_not_busy();

    if (x86_inb(ATA_PRIMARY_LBA_MID_PORT) != 0 || x86_inb(ATA_PRIMARY_LBA_HIGH_PORT) != 0) {
        printf("[DISK] Not an ATA device\r\n");
        return 0;
    }

    // Wait until data is ready
    uint32_t start_time_low, current_time_low;
    get_systime_millis(&start_time_low, NULL);
    uint32_t timeout_duration = 5000; // 5s timeout
    while(1) {
        get_systime_millis(&current_time_low, NULL);
        status = read_status();

        if (status & ATA_STATUS_ERR) {
            printf("[DISK] Error during device identification\r\n");
            return 0;
        }
        if (status & ATA_STATUS_DRQ) break;

        // Timeout
        if (current_time_low - start_time_low > timeout_duration) {
            printf("[DISK] Timeout during device identification\r\n");
            return 0;
        }
    }

    // Read data from IDENTIFY (data from IDENTIFY is in the form of 512 bytes - 256 words)
    uint16_t* data = (uint16_t*)device_info;
    for (int i = 0; i < sizeof(ATA_IDENTIFY_DEVICE_DATA) / 2; i++) {
        data[i] = x86_inw(ATA_PRIMARY_DATA_PORT);
    }

    // Convert to strings
    for (int i = 0; i < 20; i+=2) {
        uint8_t tmp = device_info->SerialNumber[i];
        device_info->SerialNumber[i] = device_info->SerialNumber[i+1];
        device_info->SerialNumber[i+1] = tmp;
    }
    for (int i = 0; i < 8; i+=2) {
        uint8_t tmp = device_info->FirmwareRevision[i];
        device_info->FirmwareRevision[i] = device_info->FirmwareRevision[i+1];
        device_info->FirmwareRevision[i+1] = tmp;
    }
    for (int i = 0; i < 40; i+=2) {
        uint8_t tmp = device_info->ModelNumber[i];
        device_info->ModelNumber[i] = device_info->ModelNumber[i+1];
        device_info->ModelNumber[i+1] = tmp;
    }

    return 1;
}

void select_device(uint8_t device) {
    if (device != last_selected_device) {
        x86_outb(ATA_PRIMARY_DRIVE_PORT, device);
        last_selected_device = device;

        // Read the status register 15 times after device selection (waits 400ns)
        for (int i = 0; i < 15; i++) {
            x86_inb(ATA_PRIMARY_ALT_STATUS_PORT);
        }
    }
}

int detect_device(void) {
    select_device(ATA_MASTER_DRIVE);
    wait_not_busy();

    uint8_t status = read_status();
    uint8_t lba_mid = x86_inb(ATA_PRIMARY_LBA_MID_PORT);
    uint8_t lba_high = x86_inb(ATA_PRIMARY_LBA_HIGH_PORT);
    if (status == ATA_FLOATING_BUS && lba_mid == ATA_FLOATING_BUS && lba_high == ATA_FLOATING_BUS) {    // No drive
        return 0;
    }

    return 1;    // Drive detected
}

void setup_drive_read_write(uint8_t device, uint32_t lba, uint8_t sector_count) {
    select_device(ATA_MASTER_DRIVE);
    wait_not_busy();
    wait_ready();

    uint8_t drive_byte = (device == ATA_SLAVE_DRIVE) ? 0xF0 : 0xE0; // default to master drive (0xE0)

    // from "28 bit PIO" section of "https://wiki.osdev.org/ATA_PIO_Mode"
    x86_outb(ATA_PRIMARY_DRIVE_PORT, drive_byte | ((lba >> 24) & 0x0F));
    x86_outb(ATA_PRIMARY_SECTOR_COUNT_PORT, sector_count);
    x86_outb(ATA_PRIMARY_LBA_LOW_PORT, (uint8_t)(lba));
    x86_outb(ATA_PRIMARY_LBA_MID_PORT, (uint8_t)(lba >> 8));
    x86_outb(ATA_PRIMARY_LBA_HIGH_PORT, (uint8_t)(lba >> 16));
}

int read_sectors(uint32_t lba, uint8_t sector_count, uint8_t* buffer) {
    setup_drive_read_write(ATA_MASTER_DRIVE, lba, sector_count);
    x86_outb(ATA_PRIMARY_COMMAND_PORT, ATA_READ_SECTORS_CMD);

    // read data
	for (int sector = 0; sector < sector_count; sector++) {
        wait_not_busy();
        wait_ready();

        uint8_t status = read_status();
        if (status & ATA_STATUS_ERR) {
            printf("[DISK] Error reading sector\r\n");
            return 0;
        }

        for (int i = 0; i < 256; i++) {	// 256 words (512 bytes) per sector
            uint16_t data = x86_inw(ATA_PRIMARY_DATA_PORT);
            buffer[i * 2] = (uint8_t)data;
            buffer[i * 2 + 1] = (uint8_t)(data >> 8);
        }

        buffer += 512;	// goto next sector (512 byte offset)
    }

    return 1;
}

int write_sectors(uint32_t lba, uint8_t sector_count, const uint8_t* buffer) {
    setup_drive_read_write(ATA_MASTER_DRIVE, lba, sector_count);
    x86_outb(ATA_PRIMARY_COMMAND_PORT, ATA_WRITE_SECTORS_CMD);

    // write data
    const uint16_t* buf = (const uint16_t*)buffer;
    for (int sector = 0; sector < sector_count; sector++) {
        wait_not_busy();
        wait_ready();

        // Write 512 bytes for each sector
        for (int i = 0; i < 256; i++) {
            x86_outw(ATA_PRIMARY_DATA_PORT, buf[i]);
        }
        buf += 256;

        x86_outb(ATA_PRIMARY_COMMAND_PORT, ATA_FLUSH_CACHE_CMD);
        wait_not_busy();
    }

    return 1;
}

// TODO: Correct the following functions to not overwrite last char, and instead use malloc when implemented or a buffer
char* get_device_model_number(ATA_IDENTIFY_DEVICE_DATA* device_info) {
    char* model = device_info->ModelNumber;
	model[39] = '\0';
    return model;
}

char* get_device_serial_number(ATA_IDENTIFY_DEVICE_DATA* device_info) {
	char* serial = device_info->SerialNumber;
	serial[19] = '\0';
    return serial;
}

char* get_device_firmware_revision(ATA_IDENTIFY_DEVICE_DATA* device_info) {
    char* firmware = device_info->FirmwareRevision;
	firmware[7] = '\0';
	return firmware;
}

void print_ata_device_info(ATA_IDENTIFY_DEVICE_DATA* device_info_ptr) {
    printf("Model:\t%s\r\nSerial:\t%s\r\nFirmware:\t%s\r\n",
        get_device_model_number(device_info_ptr),
        get_device_serial_number(device_info_ptr),
        get_device_firmware_revision(device_info_ptr));
}

void init_disk(void) {
    printf("[DISK] Initializing disk...\r\n");

    if (!detect_device()) {
        printf("[DISK] No drive detected\r\n");
        return;
    }

	printf("[DISK] Disk status: %s\r\n", read_status_str());

    ATA_IDENTIFY_DEVICE_DATA device_info;
    if (identify_device(&device_info)) {
        printf("[DISK] Device Information:\r\n");
        print_ata_device_info(&device_info);
        printf("[DISK] ATA disk initialized\r\n");
    } else {
        printf("[DISK] Failed to initialize ATA disk\r\n");
    }
}