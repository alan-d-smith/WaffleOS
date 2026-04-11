#pragma once

#include <stdint.h>
#include <float.h>
#include <stdbool.h>

// Font bitmaps
extern const uint8_t font8x16[128][16];
extern const uint8_t font8x8_basic[128][8];
extern const uint8_t font8x8_block[32][8];

// Font style structure
typedef struct {
    float Scale;
    uint32_t CharacterWidth;
    uint32_t CharacterHeight;
    const uint8_t *Reference;  // Pointer to the font data
} FontStyle;

extern FontStyle FontStyle_8x16;
extern FontStyle FontStyle_8x8_Basic;
extern FontStyle FontStyle_8x8_Block;


