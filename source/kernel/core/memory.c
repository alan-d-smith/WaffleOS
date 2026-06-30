/* Derived from: https://embeddedartistry.com/blog/2017/03/22/memset-memcpy-memcmp-and-memmove/
and: https://opensource.apple.com/source/xnu/xnu-2050.7.9/libsyscall/wrappers/memcpy.c

malloc(), calloc() and free() were based on a simplified implementation of a combination of:
- https://arjunsreedharan.org/post/148675821737/memory-allocators-101-write-a-simple-memory
- https://embeddedartistry.com/blog/2017/02/15/implementing-malloc-first-fit-free-list/
- logic from: https://pages.cs.wisc.edu/~remzi/OSTEP/vm-api.pdf
*/

#include "memory.h"

#include <stdint.h>
#include <stddef.h>

#include "stdio.h"
#include "../libs/string.h"
#include "log.h"

#define wsize sizeof(uint32_t)
#define wmask (wsize - 1)

static uint8_t heap[MEMORY_SIZE];
static MemoryBlock* head = NULL;

void merge_surrounding_free_blocks(MemoryBlock* block) {
    MemoryBlock* current = head;
    MemoryBlock* previous = NULL;

    while (current && current != block) {    // Loop until we find the block, or reach the end of the linked list
        previous = current;
        current = current->Next;
    }

    if (!current) {    // Block not in linked list
        return;
    }

    // merge with previous block if free
    if (previous && previous->IsFree) {
        previous->Size += sizeof(MemoryBlock) + block->Size;
        previous->Next = block->Next;
        current = previous;
    }

    // merge with all subsequent blocks if free
    while (current->Next && current->Next->IsFree) {
        current->Size += sizeof(MemoryBlock) + current->Next->Size;
        current->Next = current->Next->Next;
    }
}

// NOTE: I DID NOT WRITE THIS, ALL CREDIT GOES TO PHILLIP JOHNSTON
// From: https://embeddedartistry.com/blog/2017/03/22/memset-memcpy-memcmp-and-memmove/
void* memset(void* dst0, int value, size_t length) {
    unsigned char *s = dst0;
    uint32_t c = (unsigned char)value;
    size_t n = length;
    size_t k;

    /* Fill head and tail with minimal branching. Each
     * conditional ensures that all the subsequently used
     * offsets are well-defined and in the dst0 region. */

    if (!n) return dst0;
    s[0] = s[n-1] = c;
    if (n <= 2) return dst0;
    s[1] = s[n-2] = c;
    s[2] = s[n-3] = c;
    if (n <= 6) return dst0;
    s[3] = s[n-4] = c;
    if (n <= 8) return dst0;

    /* Advance pointer to align it at a 4-byte boundary,
     * and truncate n to a multiple of 4. The previous code
     * already took care of any head/tail that get cut off
     * by the alignment. */

    k = -(uintptr_t)s & 3;
    s += k;
    n -= k;
    n &= -4;
    n /= 4;

    uint32_t *ws = (uint32_t *)s;
    uint32_t wc = c & 0xFF;
    wc |= ((wc << 8) | (wc << 16) | (wc << 24));

    /* Pure C fallback with no aliasing violations. */
    for (; n; n--, ws++) *ws = wc;

    return dst0;
}

// NOTE: I DID NOT WRITE THIS, ALL CREDIT GOES TO APPLE INC.
// From: https://opensource.apple.com/source/xnu/xnu-2050.7.9/libsyscall/wrappers/memcpy.c
void* memcpy(void* dst0, const void* src0, size_t length) {
    char* dst = dst0;
    const char* src = src0;
    size_t t;

    if (length == 0 || dst == src)		/* nothing to do */
        goto done;

    /*
     * Macros: loop-t-times; and loop-t-times, t>0
     */
#define	TLOOP(s) if (t) TLOOP1(s)
#define	TLOOP1(s) do { s; } while (--t)

    if ((uint32_t)dst < (uint32_t)src) {
        /*
         * Copy forward.
         */
        t = (uintptr_t)src;	/* only need low bits */
        if ((t | (uintptr_t)dst) & wmask) {
            /*
             * Try to align operands.  This cannot be done
             * unless the low bits match.
             */
            if ((t ^ (uintptr_t)dst) & wmask || length < wsize)
                t = length;
            else
                t = wsize - (t & wmask);
            length -= t;
            TLOOP1(*dst++ = *src++);
        }
        /*
         * Copy whole words, then mop up any trailing bytes.
         */
        t = length / wsize;
        TLOOP(*(uint32_t*)dst = *(uint32_t*)src; src += wsize; dst += wsize);
        t = length & wmask;
        TLOOP(*dst++ = *src++);
    } else {
        /*
         * Copy backwards.  Otherwise essentially the same.
         * Alignment works as before, except that it takes
         * (t&wmask) bytes to align, not wsize-(t&wmask).
         */
        src += length;
        dst += length;
        t = (uintptr_t)src;
        if ((t | (uintptr_t)dst) & wmask) {
            if ((t ^ (uintptr_t)dst) & wmask || length <= wsize)
                t = length;
            else
                t &= wmask;
            length -= t;
            TLOOP1(*--dst = *--src);
        }
        t = length / wsize;
        TLOOP(src -= wsize; dst -= wsize; *(uint32_t*)dst = *(uint32_t*)src);
        t = length & wmask;
        TLOOP(*--dst = *--src);
    }
    done:
        return (dst0);
}

int memcmp(const void* ptr1, const void* ptr2, uint16_t num)
{
    const uint8_t* u8Ptr1 = (const uint8_t *)ptr1;
    const uint8_t* u8Ptr2 = (const uint8_t *)ptr2;

    for (uint16_t i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;

    return 0;
}

// Start of code by Luca Abeni & Gerardo Lamastra
void *lmemcpy(LIN_ADDR t, LIN_ADDR f, size_t n)
{
    void *p1 = (void *)t;
    void *p2 = (void *)f;
    return memcpy(p1, p2, n);
}

void *memmove(void * dest,const void * src, size_t n)
{
    int d0, d1, d2;
    if (dest<src)
        __asm__ __volatile__(
            "cld\n\t"
            "rep\n\t"
            "movsb"
            : "=&c" (d0), "=&S" (d1), "=&D" (d2)
            :"0" (n),"1" (src),"2" (dest)
            : "memory");
    else
        __asm__ __volatile__(
            "std\n\t"
            "rep\n\t"
            "movsb\n\t"
            "cld"
            : "=&c" (d0), "=&S" (d1), "=&D" (d2)
            :"0" (n),
             "1" (n-1+(const char *)src),
             "2" (n-1+(char *)dest)
            :"memory");
    return dest;
}

void * memchr(const void * cs,int c,size_t count)
{
    int d0;
    register void * __res;
    if (!count)
        return NULL;
    __asm__ __volatile__(
        "cld\n\t"
        "repne\n\t"
        "scasb\n\t"
        "je 1f\n\t"
        "movl $1,%0\n"
        "1:\tdecl %0"
        :"=D" (__res), "=&c" (d0) : "a" (c),"0" (cs),"1" (count));
    return __res;
}

void * __memset_generic(void * s, char c,size_t count)
{
    int d0, d1;
    __asm__ __volatile__(
        "cld\n\t"
        "rep\n\t"
        "stosb"
        : "=&c" (d0), "=&D" (d1)
        :"a" (c),"1" (s),"0" (count)
        :"memory");
    return s;
}

/*
 * memset(x,0,y) is a reasonably common thing to do, so we want to fill
 * things 32 bits at a time even when we don't know the size of the
 * area at compile-time..
 */
void * __constant_c_memset(void * s, unsigned long c, size_t count)
{
    int d0, d1;
    __asm__ __volatile__(
        "cld\n\t"
        "rep ; stosl\n\t"
        "testb $2,%b3\n\t"
        "je 1f\n\t"
        "stosw\n"
        "1:\ttestb $1,%b3\n\t"
        "je 2f\n\t"
        "stosb\n"
        "2:"
        : "=&c" (d0), "=&D" (d1)
        :"a" (c), "q" (count), "0" (count/4), "1" ((long) s)
        :"memory");
    return (s);
}

/*
 * This looks horribly ugly, but the compiler can optimize it totally,
 * as we by now know that both pattern and count is constant..
 */
void * __constant_c_and_count_memset(void * s, unsigned long pattern, size_t count)
{
    switch (count) {
    case 0:
        return s;
    case 1:
        *(unsigned char *)s = pattern;
        return s;
    case 2:
        *(unsigned short *)s = pattern;
        return s;
    case 3:
        *(unsigned short *)s = pattern;
        *(2+(unsigned char *)s) = pattern;
        return s;
    case 4:
        *(unsigned long *)s = pattern;
        return s;
    }
#define COMMON(x) \
    __asm__  __volatile__("cld\n\t" \
    "rep ; stosl" \
    x \
    : "=&c" (d0), "=&D" (d1) \
    : "a" (pattern),"0" (count/4),"1" ((long) s) \
    : "memory")
    {
        int d0, d1;
        switch (count % 4) {
        case 0: COMMON(""); return s;
        case 1: COMMON("\n\tstosb"); return s;
        case 2: COMMON("\n\tstosw"); return s;
        default: COMMON("\n\tstosw\n\tstosb"); return s;
        }
    }

#undef COMMON
}

void * memscan(void * addr, int c, size_t size)
{
    if (!size)
        return addr;
    __asm__ volatile ("cld\n\t"
        "repnz; scasb\n\t"
        "jnz 1f\n\t"
        "dec %%edi\n\t"
    "1:"
        : "=D" (addr), "=c" (size)
        : "0" (addr), "1" (size), "a" (c)
    );
    return addr;
}
// End of code by Luca Abeni & Gerardo Lamastra

void* malloc(size_t size) {
    MemoryBlock *current, *previous;
    void* result;

    if (!head) {
        init_memory();
    }

    current = head;
    while (current) {
        if (current->IsFree && current->Size >= size) {    // If the block is free and large enough
            if (current->Size > size + sizeof(MemoryBlock) + BLOCK_SIZE) {    // If the block is larger than the requested size
                MemoryBlock* new_block = (MemoryBlock*)((uint8_t*)current + sizeof(MemoryBlock) + size);
                new_block->Size = current->Size - size - sizeof(MemoryBlock);
                new_block->IsFree = true;
                new_block->Next = current->Next;
                current->Size = size;
                current->Next = new_block;
            }
            current->IsFree = false;
            result = (void*)(current + 1);
            return result;
        }
        previous = current;
        current = current->Next;
    }

    return NULL;
}

void* calloc(size_t num_items, size_t size) {
    size_t total_size = num_items * size;
    void* result = malloc(total_size);
    if (result) {
        memset(result, 0, total_size);
    }
    return result;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    MemoryBlock* block = (MemoryBlock*)ptr - 1;
    block->IsFree = true;

    merge_surrounding_free_blocks(block);
}

// for debug only! (inclusive, not inclusive)
void memdump_array(uint8_t* address, int start, int end) {
    for (int i = start; i < end; i += 16) {
        // print offset
        printf("%x: ", i);

        // Print 16 bytes on this line
        for (int j = i; j < i + 16 && j < end; j++) {
            // Manually add a leading zero if the value is less than 16
            if (address[j] < 16) {
                printf("0%x ", address[j]);
            } else {
                printf("%x ", address[j]);
            }
        }
        printf("\r\n");
    }
}

void init_memory() {
    head = (MemoryBlock*)heap;
    head->Size = MEMORY_SIZE - sizeof(MemoryBlock);
    head->IsFree = true;
    head->Next = NULL;
    log_ok("MEMORY", "Heap initialised");
}