// Kernel x86 helper functions
// source: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/kernel/x86.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

uint8_t __attribute__((cdecl)) x86_inb(uint16_t port);
void __attribute__((cdecl)) x86_outb(uint16_t port, uint8_t value);
uint16_t __attribute__((cdecl)) x86_inw(uint16_t port);
void __attribute__((cdecl)) x86_outw(uint16_t port, uint16_t value);

void __attribute__((cdecl)) i686_Panic();
void __attribute__((cdecl)) crash_me();

uint32_t __attribute__((cdecl)) x86_load_cr3(void);
void __attribute__((cdecl)) x86_store_cr3(uint32_t cr3);
uint32_t __attribute__((cdecl)) x86_load_cr0(void);
void __attribute__((cdecl)) x86_store_cr0(uint32_t cr0);