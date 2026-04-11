// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/stdio.h
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void clrscr();
void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);