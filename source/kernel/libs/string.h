// using some definitions
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "datastructures.h"

#define STR(x) #x
#define XSTR(x) STR(x)

// from: https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/bootloader/stage2/string.h
unsigned strlen(const char* str);

int isalnum(int c);
int strcmp(const char* str1, const char* str2);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* str1, const char* str2);
char* strchr(const char* str, int character);
char* strpbrk(const char* str, const char* char_set);
SinglyLinkedList* strsplit(char* src, char delimiter);
int int_to_hex_str(uint32_t value, char* buffer);
void int_to_str(int num, char* str, int* len);

bool islower(char chr);
char toupper(char chr);