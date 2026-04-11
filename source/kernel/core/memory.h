#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MEMORY_SIZE 0x100000 // in bytes
#define BLOCK_SIZE 16

typedef struct MemoryBlock {
    size_t Size;
    uint8_t IsFree;
    struct MemoryBlock* Next;
} MemoryBlock;

#ifndef __BASIC_DATA__
    #define __BASIC_DATA__
    typedef void *LIN_ADDR;
    typedef unsigned long  DWORD;
    typedef unsigned short WORD;
    typedef unsigned char  BYTE;
    typedef unsigned long  TIME;
    typedef unsigned long  SYS_FLAGS;
    #define MAX_DWORD	0xFFFFFFFF
    #define MAX_WORD	0xFFFF
    #define MAX_BYTE	0xFF
#endif

void init_memory();
void *memset(void* dst0, int value, size_t length);
void *memcpy(void* dst0, const void* src0, size_t length);
void *lmemcpy(LIN_ADDR t, LIN_ADDR f, size_t n);
int memcmp(const void* ptr1, const void* ptr2, uint16_t num);
void *memmove(void * dest,const void * src, size_t n);
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void free(void* ptr);

void *memchr(const void * cs,int c,size_t count);
void *memscan(void * addr, int c, size_t size);
void *__memset_generic(void * s, char c,size_t count);
void *__constant_c_memset(void * s, unsigned long c, size_t count);
void *__constant_c_and_count_memset(void * s, unsigned long pattern, size_t count);

#define __constant_count_memset(s,c,count) __memset_generic((s),(c),(count))
