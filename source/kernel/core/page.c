#include "page.h"
#include "../core/memory.h"   // For malloc(), memset(), etc.
#include "../core/x86.h"      // For x86_load_cr3(), x86_load_cr0(), x86_store_cr0()
#include "../core/stdio.h"    // For printf()

page_directory_t *kernel_page_directory = 0;

/* A simple aligned allocation function for paging.
This is a very basic implementation: it allocates extra bytes and returns a pointer aligned to the specified boundary.
WARNING: free() is not supported here! */
static void* kmalloc_aligned(size_t size, size_t alignment) {
    // Allocate extra memory to allow for alignment adjustment.
    void *p = malloc(size + alignment - 1);
    if (!p) return 0;
    uintptr_t addr = (uintptr_t)p;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned_addr;
}

void init_paging(void) {
    // Allocate the page directory aligned to 4KB.
    kernel_page_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    if (!kernel_page_directory) {
        printf("[PAGING] Failed to allocate page directory\r\n");
        return;
    }
    memset(kernel_page_directory, 0, sizeof(page_directory_t));

    // Allocate one page table for the first 4MB.
    page_table_t *first_page_table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
    if (!first_page_table) {
        printf("[PAGING] Failed to allocate page table\r\n");
        return;
    }
    memset(first_page_table, 0, sizeof(page_table_t));

    for (int i = 0; i < PAGE_ENTRIES; i++) {
        first_page_table->entries[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
    }

    kernel_page_directory->entries[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW;

    enable_paging(kernel_page_directory);
}

void enable_paging(page_directory_t *dir) {
    x86_store_cr3((uint32_t)dir);

    // Enable paging by setting the PG bit (bit 31) in CR0.
    uint32_t cr0 = x86_load_cr0();
    cr0 |= 0x80000000;  // set paging bit.
    x86_store_cr0(cr0);

    printf("[PAGING] Paging enabled.\r\n");
}
