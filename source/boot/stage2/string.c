// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/string.c
#include "string.h"
#include <stdint.h>
#include <stddef.h>

const char* strchr(const char* str, char chr)
{
    if (str == NULL)
        return NULL;

    while (*str)
    {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char* strncpy(char* dst, const char* src, size_t n)
{
    char* origDst = dst;

    if (dst == NULL || n == 0)
        return dst;

    if (src == NULL)
    {
        *dst = '\0';
        return dst;
    }

    while (*src && n > 1)
    {
        *dst = *src;
        ++src;
        ++dst;
        --n;
    }

    *dst = '\0';
    return origDst;
}

unsigned strlen(const char* str)
{
    unsigned len = 0;
    while (*str)
    {
        ++len;
        ++str;
    }

    return len;
}