// Display driver
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include "vga.h"
#include "font.h"

extern bool g_text_mode;
extern uint32_t pitch;
extern uint16_t g_SCREEN_WIDTH;
extern uint16_t g_SCREEN_HEIGHT;
extern uint16_t g_BYTES_PER_PIXEL;

typedef struct {
    uint16_t ScreenWidth;
    uint16_t ScreenHeight;
    uint16_t BPP; // Bytes per pixel
} __attribute__((packed)) GraphicsMode;

// Table of graphics modes -------------------------------------------------
typedef enum {
    MODE_320x200x1, // 320x200 pixels, 1 bpp
} GraphicsModeEnum;

static uint8_t* registerInitTable[] = {
    g_320x200x256,    // 0 (MODE_320x200x4)
};

static const GraphicsMode graphicsModes[] = {
    // x, y, words per pixel
    {320, 200, 1},
};

// --------------------------------------------------------------------------

typedef struct {
    uint16_t b : 5;
    uint16_t g : 6;
    uint16_t r : 5;
} __attribute__((packed)) Pixel16;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
} __attribute__((packed)) Pixel24;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} __attribute__((packed)) Pixel32;

// Start of code from: https://wiki.osdev.org/VESA_Video_Modes
struct VbeInfoBlock {
    char     VbeSignature[4];         // == "VESA"
    uint16_t VbeVersion;              // == 0x0300 for VBE 3.0
    uint16_t OemStringPtr[2];         // isa vbeFarPtr
    uint8_t  Capabilities[4];
    uint16_t VideoModePtr[2];         // isa vbeFarPtr
    uint16_t TotalMemory;             // as # of 64KB blocks
    uint8_t  Reserved[492];
} __attribute__((packed));

typedef struct {
    uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
    uint8_t window_a;			// deprecated
    uint8_t window_b;			// deprecated
    uint16_t granularity;		// deprecated; used while calculating bank numbers
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
    uint16_t pitch;			// number of bytes per horizontal line
    uint16_t width;			// width in pixels
    uint16_t height;			// height in pixels
    uint8_t w_char;			// unused...
    uint8_t y_char;			// ...
    uint8_t planes;
    uint8_t bpp;			// bits per pixel in this mode
    uint8_t banks;			// deprecated; total number of banks in this mode
    uint8_t memory_model;
    uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
    uint8_t image_pages;
    uint8_t reserved0;

    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;

    uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
    uint8_t reserved1[206];
} __attribute__ ((packed)) VBEModeInfo;
// end of code from: https://wiki.osdev.org/VESA_Video_Modes

void enable_graphics_mode(uint16_t mode);  // 320x200x1 by default

uint8_t *display_get_framebuffer(); // get the framebuffer pointer
void set_drawing_target(uint8_t *target); // set the drawing target

void g_clrscr(uint32_t color); // clear screen in graphics mode
void g_clear_screen();

// Basic drawing
void put_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int width, int height, uint32_t color);
void draw_line(int x1, int y1, int x2, int y2, uint32_t color);

// Text rendering routines using an 8x16 bitmap font
void draw_char(int x, int y, char ch, uint32_t color);
void draw_string(int x, int y, const char *str, uint32_t color);
void draw_scaled_char(int x, int y, char ch, uint32_t color, float scale);
void draw_scaled_string(int x, int y, const char *str, uint32_t color, float scale);
void draw_char_styled(int x, int y, char ch, uint32_t color, FontStyle style);
void draw_string_styled(int x, int y, const char *str, uint32_t color, FontStyle style);
