// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/fat.h

// And based on: https://wiki.osdev.org/FAT
// and: https://wiki.osdev.org/GPT
// and: https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "../core/memory.h"
#include "../drivers/disk.h"

#define SECTOR_SIZE 512

#define MAX_PATH_LENGTH 256
#define MAX_FILE_HANDLES 10

#define GPT_PROTECTIVE_MBR 0xEE
#define UEFI_SYSTEM_PARTITION 0xEF

#define MAX_PATH_SIZE           256
#define ROOT_DIRECTORY_HANDLE   -1
#define FAT_CACHE_SIZE          5

typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;          // serial number, value doesn't matter
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemId[8];
} __attribute__((packed)) FAT_BootSector;

typedef struct
{
    int Handle;
    bool IsDirectory;
    uint32_t Position;
    uint32_t Size;
} FAT_File;

typedef struct {
    uint8_t  Jump[3];
    uint8_t  OEM_ID[8];
    uint16_t BytesPerSector;
    uint8_t  SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t  FATCount;
    uint16_t RootEntries;
    uint16_t TotalSectors;
    uint8_t  MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t SectorCountLarge;    // set if more than 65535 sectors
} __attribute__((packed)) FAT_BPB_Bits;

typedef struct {
    uint8_t  DriveNumber;
    uint8_t  Reserved;
    uint8_t  Signature;           // MUST BE 0x28 or 0x29
    uint32_t VolumeID;            // Serial number of the volume
    uint8_t  VolumeLabel[11];
    uint8_t  SystemID[8];         // File system type (Don't trust)
    uint8_t  BootCode[448];
    uint16_t BootSignature;       // MUST BE 0xAA55
} __attribute__((packed)) FAT12_EBPB_Bits;

typedef struct {
    uint8_t  DriveNumber;
    uint8_t  Reserved;
    uint8_t  Signature;           // MUST BE 0x28 or 0x29
    uint32_t VolumeID;            // Serial number of the volume
    uint8_t  VolumeLabel[11];
    uint8_t  SystemID[8];         // File system type (Don't trust)
    uint8_t  BootCode[448];
    uint16_t BootSignature;       // MUST BE 0xAA55
} __attribute__((packed)) FAT16_EBPB_Bits;

typedef struct {
    uint32_t SectorsPerFat;
    uint16_t Flags;
    uint16_t Version;
    uint32_t RootCluster;
    uint16_t FSInfo;
    uint16_t BackupBootSector;
    uint8_t  Reserved[12];
    uint8_t  DriveNumber;
    uint8_t  Reserved1;
    uint8_t  Signature;           // MUST BE 0x28 or 0x29
    uint32_t VolumeID;            // Serial number of the volume
    uint8_t  VolumeLabel[11];
    uint8_t  SystemID[8];         // File system type (Don't trust)
    uint8_t  BootCode[420];
    uint16_t BootSignature;       // MUST BE 0xAA55
} __attribute__((packed)) FAT32_EBPB_Bits;

typedef struct
{
    uint8_t Buffer[SECTOR_SIZE];
    FAT_File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;

} FAT_FileData;

enum FAT_Attributes
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

typedef struct
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;          // serial number, value doesn't matter
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemId[8];
} __attribute__((packed)) FAT12_BootSector;

typedef struct
{
    union
    {
        FAT12_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES];

} FAT_Data;

typedef enum {
    FAT12,
    FAT16,
    FAT32,
    ExFAT,
    Unknown
} FATType;

typedef struct {
    uint8_t  Name[8];
    uint8_t  Extension[3];
    uint8_t  Attributes;            // READ_ONLY=0x01 HIDDEN=0x02 SYSTEM=0x04 VOLUME_ID=0x08 DIRECTORY=0x10 ARCHIVE=0x20 LFN=READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID
    uint8_t  Reserved;
    uint8_t  CreationTimeHundredths;
    uint16_t CreationTime;          // 5/6/5 bits for hour/minute/second - Multiply seconds by 2
    uint16_t CreationDate;          // 7/4/5 bits for year/month/day
    uint16_t LastAccessDate;        // 7/4/5 bits for year/month/day
    uint16_t FirstClusterHigh;      // FAT32 only (0 for FAT12/16)
    uint16_t LastWriteTime;         // 5/6/5 bits for hour/minute/second - Multiply seconds by 2
    uint16_t LastWriteDate;         // 7/4/5 bits for year/month/day
    uint16_t FirstClusterLow;
    uint32_t FileSize;
} __attribute__((packed)) FAT_DirectoryEntry;

typedef struct {
    uint8_t  SequenceNumber;
    uint16_t Name1[5];
    uint8_t  Attributes;
    uint8_t  Type;
    uint8_t  Checksum;
    uint16_t Name2[6];
    uint16_t ZERO;
    uint16_t Name3[2];
} __attribute__((packed)) FAT_ShortFilenameEntry;

typedef struct {
    FAT_BPB_Bits BPB;
    FAT12_EBPB_Bits EBPB;
    uint32_t FirstFATSector;
    uint32_t FirstDataSector;
    uint32_t RootDirSector;
    uint32_t RootDirSectors;
} __attribute__((packed)) FAT12_Filesystem;

typedef struct {
    FAT_BPB_Bits BPB;
    FAT16_EBPB_Bits EBPB;
    uint32_t FirstFATSector;
    uint32_t FirstDataSector;
    uint32_t RootDirSector;
    uint32_t RootDirSectors;
} __attribute__((packed)) FAT16_Filesystem;

typedef struct {
    FAT_BPB_Bits BPB;
    FAT32_EBPB_Bits EBPB;
    uint32_t FirstFATSector;
    uint32_t FirstDataSector;
    uint32_t RootDirSector;
    uint32_t RootDirSectors;
} __attribute__((packed)) FAT32_Filesystem;

typedef struct {
    FATType type;
    union {
        FAT12_Filesystem FAT12_FS;
        FAT16_Filesystem FAT16_FS;
        FAT32_Filesystem FAT32_FS;
    };
} __attribute__((packed)) FAT_Filesystem;

// Using information from: https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html#legacy-mbr-partition-record
typedef struct {
    uint8_t  BootIndicator;
    uint8_t  StartingCHS[3];
	/* OS Types (partition types)
    0xEF (i.e., UEFI System Partition) defines a UEFI system partition.

    0xEE (i.e., GPT Protective) is used by a protective MBR (see 5.2.2) to define a fake partition covering the entire disk.
	*/
    uint8_t OSType;
    uint8_t EndingCHS[3];
    uint32_t StartingLBA;
    uint32_t SizeInLBA;
} __attribute__((packed)) LegacyMBRPartitionRecord;

// Using information from: https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html
typedef struct {
    uint8_t  BootCode[424];
	uint32_t UniqueMBRDiskSignature;
	uint16_t Unknown;
	LegacyMBRPartitionRecord PartitionRecord[4];
	uint16_t BootSignature;       // MUST BE 0xAA55
} __attribute__((packed)) LegacyMBR;

typedef struct {
    uint8_t RequiredPartition : 1;
    uint8_t NoBlockIOProtocol : 1;
    uint8_t LegacyBIOSBootable : 1;
    uint64_t Reserved : 45;
    uint16_t Reserved2;
} __attribute__((packed)) GPTPartitionEntryAttributes;

typedef struct {
    uint8_t PartitionTypeGUID[16];
    uint8_t UniquePartitionGUID[16];
    uint64_t StartingLBA;
    uint64_t EndingLBA;
    GPTPartitionEntryAttributes Attributes;
    uint16_t PartitionName[36];		// Null terminated string (UTF-16) - Human readable partition name
} __attribute__((packed)) GPTPartitionEntry;

typedef struct {
    uint8_t  Signature[8];
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t HeaderCRC32;
    uint32_t Reserved;		// Must be 0
    uint64_t MyLBA;			// LBA containing this structure
    uint64_t AlternateLBA;
    uint64_t FirstUsableLBA;
    uint64_t LastUsableLBA;
    uint8_t  DiskGUID[16];
    uint64_t PartitionEntryLBA;
    uint32_t NumberOfPartitionEntries;
    uint32_t SizeOfPartitionEntry;
    uint32_t PartitionEntryArrayCRC32;
} __attribute__((packed)) GPTHeader;

// Declare the FAT filesystem structure.
extern FAT_Filesystem current_filesystem;

// Make the FAT data pointer available externally.
extern FAT_Data* g_Data;
extern uint8_t* g_Fat;
extern uint32_t g_DataSectionLba;

bool FAT_Initialize(DISK* disk);
FAT_File * FAT_Open(DISK* disk, const char* path);
uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut);
bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_File* file);
void FAT_MakeDirectory(char *name);
char* get_current_path();

void init_filesystem(void);
void parse_gpt();
void parse_mbr(LegacyMBR* mbr);
FATType get_fat_type(uint32_t total_clusters, uint16_t bytes_per_sector);
