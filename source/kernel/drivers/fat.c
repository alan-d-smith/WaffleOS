// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/fat.c
#include "fat.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "../core/stdio.h"
#include "../core/memdefs.h"
#include "../libs/string.h"
#include "../libs/math.h"
#include "../core/memory.h"
#include "../core/cmd_executor.h"
#include "../drivers/disk.h"
#include "../drivers/floppy.h"

FAT_Filesystem current_filesystem;

FAT_Data* g_Data;
uint8_t* g_Fat = NULL;
uint32_t g_DataSectionLba;

bool FAT_ReadBootSector(DISK* disk)
{
    if (disk->is_floppy)
        return FLOPPY_ReadSectors(0, 1, g_Data->BS.BootSectorBytes);
    else
        return DISK_ReadSectors(disk, 0, 1, g_Data->BS.BootSectorBytes);
}

bool FAT_ReadFat(DISK* disk)
{
    if (disk->is_floppy)
        return FLOPPY_ReadSectors(g_Data->BS.BootSector.ReservedSectors,
                                   g_Data->BS.BootSector.SectorsPerFat,
                                   g_Fat);
    else
        return DISK_ReadSectors(disk,
                                g_Data->BS.BootSector.ReservedSectors,
                                g_Data->BS.BootSector.SectorsPerFat,
                                g_Fat);
}

bool FAT_Initialize(DISK* disk)
{
    g_Data = (FAT_Data*)MEMORY_FAT_ADDR;

    // read boot sector
    if (!FAT_ReadBootSector(disk))
    {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }

    // read FAT
    g_Fat = (uint8_t*)g_Data + sizeof(FAT_Data);
    uint32_t fatSize = g_Data->BS.BootSector.BytesPerSector * g_Data->BS.BootSector.SectorsPerFat;
    if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE)
    {
        printf("FAT: not enough memory to read FAT! Required %lu, only have %u\r\n", sizeof(FAT_Data) + fatSize, MEMORY_FAT_SIZE);
        return false;
    }

    if (!FAT_ReadFat(disk))
    {
        printf("FAT: read FAT failed\r\n");
        return false;
    }

    // open root directory file
    uint32_t rootDirLba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;

    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.IsDirectory = true;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Public.Size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    g_Data->RootDirectory.Opened = true;
    g_Data->RootDirectory.FirstCluster = rootDirLba;
    g_Data->RootDirectory.CurrentCluster = rootDirLba;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer))
    {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirSectors = (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
    g_DataSectionLba = rootDirLba + rootDirSectors;

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        g_Data->OpenedFiles[i].Opened = false;

    return true;
}

uint32_t FAT_ClusterToLba(uint32_t cluster)
{
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry)
{
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!g_Data->OpenedFiles[i].Opened)
            handle = i;
    }

    // out of handles
    if (handle < 0)
    {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    // setup vars
    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.Position = 0;
    fd->Public.Size = entry->FileSize;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (disk->is_floppy) {
        if (!FLOPPY_ReadSectors(FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer)) {
            console_printf("FAT: open entry failed (floppy) - read error cluster=%u lba=%u\n",
                   fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
            for (int i = 0; i < 11; i++)
                printf("%c", entry->Name[i]);
            printf("\n");
            return false;
        }
    } else {
        if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer)) {
            console_printf("FAT: open entry failed - read error cluster=%u lba=%u\n",
                   fd->CurrentCluster, FAT_ClusterToLba(fd->CurrentCluster));
            for (int i = 0; i < 11; i++)
                printf("%c", entry->Name[i]);
            printf("\n");
            return false;
        }
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT_NextCluster(uint32_t currentCluster)
{
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
}

uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut)
{
    // get file data
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE)
        ? &g_Data->RootDirectory
        : &g_Data->OpenedFiles[file->Handle];

    uint8_t* u8DataOut = (uint8_t*)dataOut;

    // don't read past the end of the file
    if (!fd->Public.IsDirectory || (fd->Public.IsDirectory && fd->Public.Size != 0))
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        // printf("leftInBuffer=%lu take=%lu\r\n", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take)
        {
            // Special handling for root directory
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE)
            {
                ++fd->CurrentCluster;

                // read next sector
                if (!DISK_ReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
            else
            {
                // calculate next cluster & sector to read
                if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster)
                {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
                }

                if (fd->CurrentCluster >= 0xFF8)
                {
                    // Mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                // read next sector
                if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
                {
                    printf("FAT: read error!\r\n");
                    break;
                }
            }
        }
    }

    return u8DataOut - (uint8_t*)dataOut;
}

bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry)
{
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File* file)
{
    if (file->Handle == ROOT_DIRECTORY_HANDLE)
    {
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else
    {
        g_Data->OpenedFiles[file->Handle].Opened = false;
    }
}

bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut)
{
    char fatName[12];
    FAT_DirectoryEntry entry;

    // convert from name to fat name
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != name + 11)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);
    }

    while (FAT_ReadEntry(disk, file, &entry))
    {
        if (memcmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }

    return false;
}

void FAT_MakeDirectory(char *name) {

}

FAT_File* FAT_Open(DISK* disk, const char* path)
{
    char name[MAX_PATH_SIZE];

    // ignore leading slash
    if (path[0] == '/')
        path++;

    FAT_File* current = &g_Data->RootDirectory.Public;

    while (*path) {
        // extract next file name from path
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim != NULL)
        {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }

        // find directory entry in current directory
        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry))
        {
            FAT_Close(current);

            // check if directory
            if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0)
            {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_OpenEntry(disk, &entry);
        }
        else
        {
            FAT_Close(current);

            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }

    return current;
}

void print_guid(const uint8_t *guid) {
    for (int i = 0; i < 16; i++) {
        if (guid[i] < 16)
            printf("0");
        printf("%x", guid[i]);

        if (i == 3 || i == 5 || i == 7 || i == 9)
            printf("-");
    }
}


void parse_guid_partition_table(uint8_t* gpt_sector) {
}

void init_filesystem(void) {
    uint8_t boot_sector[SECTOR_SIZE];
    if (!read_sectors(0, 1, boot_sector)) {    // read boot sector into buffer
        printf("[FS] Unable to read boot sector\r\n");
        return;
    }

    if (boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        printf("[FS] Invalid boot sector signature\r\n");
        return;
    }
}

void parse_gpt(void) { // TODO: implement
    return;
}

void parse_mbr(LegacyMBR* mbr) { // TODO: implement
    return;
}

void init_fat_partition(uint32_t partition_start_lba, uint8_t* boot_sector) {
    // parse BIOS parameter block
    FAT_BPB_Bits* BPB = (FAT_BPB_Bits*)boot_sector;

    uint32_t total_sectors = (BPB->TotalSectors == 0) ? BPB->SectorCountLarge : BPB->TotalSectors;
	uint32_t fat_size = (BPB->SectorsPerFat == 0) ? ((FAT32_EBPB_Bits*)(boot_sector + 36))->SectorsPerFat : BPB->SectorsPerFat;
	uint32_t root_dir_sectors = ((BPB->RootEntries * 32) + (BPB->BytesPerSector - 1)) / BPB->BytesPerSector;
	uint32_t data_sectors = total_sectors - (BPB->ReservedSectors + (BPB->FATCount * fat_size) + root_dir_sectors);

	if (BPB->SectorsPerCluster == 0) {
        printf("[FAT] Error: SectorsPerCluster is 0\r\n");
        return;
    }

    uint32_t total_clusters = data_sectors / BPB->SectorsPerCluster;

    current_filesystem.type = get_fat_type(total_clusters, BPB->BytesPerSector);

    // store filesystem information depending on type
    printf("[FAT] Filesystem type: ");
    switch (current_filesystem.type) {
        case FAT12:
            printf("FAT12\r\n");
            current_filesystem.FAT12_FS.BPB = *BPB;
            memcpy(&current_filesystem.FAT12_FS.EBPB, boot_sector + 36, sizeof(FAT12_EBPB_Bits));
            current_filesystem.FAT12_FS.FirstFATSector = BPB->ReservedSectors;
            current_filesystem.FAT12_FS.RootDirSector = BPB->ReservedSectors + (BPB->FATCount * BPB->SectorsPerFat);
            current_filesystem.FAT12_FS.FirstDataSector = current_filesystem.FAT12_FS.RootDirSector + root_dir_sectors;
            current_filesystem.FAT12_FS.RootDirSectors = root_dir_sectors;
            break;
        case FAT16:
            printf("FAT16\r\n");
            current_filesystem.FAT16_FS.BPB = *BPB;
            memcpy(&current_filesystem.FAT16_FS.EBPB, boot_sector + 36, sizeof(FAT16_EBPB_Bits));
            current_filesystem.FAT16_FS.FirstFATSector = BPB->ReservedSectors;
            current_filesystem.FAT16_FS.RootDirSector = BPB->ReservedSectors + (BPB->FATCount * BPB->SectorsPerFat);
            current_filesystem.FAT16_FS.FirstDataSector = current_filesystem.FAT16_FS.RootDirSector + root_dir_sectors;
            current_filesystem.FAT16_FS.RootDirSectors = root_dir_sectors;
            break;
        case FAT32:
            printf("FAT32\r\n");
            current_filesystem.FAT32_FS.BPB = *BPB;
            memcpy(&current_filesystem.FAT32_FS.EBPB, boot_sector + 36, sizeof(FAT32_EBPB_Bits));
            current_filesystem.FAT32_FS.FirstFATSector = BPB->ReservedSectors;
            current_filesystem.FAT32_FS.RootDirSector = ((FAT32_EBPB_Bits*)(boot_sector + 36))->RootCluster;
            current_filesystem.FAT32_FS.FirstDataSector = BPB->ReservedSectors + (BPB->FATCount * fat_size);
            current_filesystem.FAT32_FS.RootDirSectors = 0; // FAT32 has no fixed root dir
            break;
        default:
            printf("Unknown/Unsupported FAT type\r\n");
            return;
    }

    printf("[FAT] FAT filesystem initialized\r\n");
}

char* get_current_path() {
    return "/";
}

FATType get_fat_type(uint32_t total_clusters, uint16_t bytes_per_sector) {
    // ExFAT currently not supported
    if (total_clusters < 4085) {
        return FAT12;
    } else if (total_clusters < 65525) {
        return FAT16;
    } else {
        return FAT32;
    }
}
