// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/kernel/stdio.h

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <memory.h>

// Color Table for VGA text mode
#define VGA_BLACK 0x0
#define VGA_BLUE 0x1
#define VGA_GREEN 0x2
#define VGA_CYAN 0x3
#define VGA_RED 0x4
#define VGA_MAGENTA 0x5
#define VGA_BROWN 0x6
#define VGA_LIGHT_GRAY 0x7
#define VGA_DARK_GRAY 0x8
#define VGA_LIGHT_BLUE 0x9
#define VGA_LIGHT_GREEN 0xa
#define VGA_LIGHT_CYAN 0xb
#define VGA_LIGHT_RED 0xc
#define VGA_LIGHT_MAGENTA 0xd
#define VGA_YELLOW 0xe
#define VGA_WHITE 0xf

#define WHITE_ON_BLACK 0x0f
#define WHITE_ON_BLUE 0x1f

int get_g_screenx();
int get_g_screeny();

void clrscr();
void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
int vsnprintf(char *str, size_t size, const char *fmt, va_list args);
int snprintf(char* str, size_t size, const char* format, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);
