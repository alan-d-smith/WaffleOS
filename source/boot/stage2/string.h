// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/string.h
#pragma once
#include <stddef.h>

const char* strchr(const char* str, char chr);
char* strncpy(char* dst, const char* src, size_t n);
unsigned strlen(const char* str);