#pragma once

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE       4096
#define PAGE_ENTRIES    1024

#define PAGE_PRESENT    0x1
#define PAGE_RW         0x2
#define PAGE_USER       0x4

typedef uint32_t page_entry_t;

// Page table: array of 1024 32-bit entries.
typedef struct {
    page_entry_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// Page directory: array of 1024 32-bit entries.
// 4KB ALIGNED
typedef struct {
    page_entry_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

extern page_directory_t *kernel_page_directory;

void init_paging(void);
void enable_paging(page_directory_t *dir);
