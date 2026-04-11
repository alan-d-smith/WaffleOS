// Display driver
#include "display.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "../libs/string.h"
#include "../core/x86.h"
#include "../core/memory.h"
#include "font.h" // contains basic font definitions

bool g_text_mode = true;

uint16_t g_SCREEN_WIDTH = 80;  // Default value, overridden at runtime
uint16_t g_SCREEN_HEIGHT = 25; // Default value, overridden at runtime
uint32_t pitch = 80;
uint16_t g_BYTES_PER_PIXEL = 1;

// Global variables ----------------------------------------------------------------------------------------------------
static volatile uint8_t *framebuffer = (volatile uint8_t *)0;
static volatile uint8_t *current_target = NULL;

// Helper functions ----------------------------------------------------------------------------------------------------

uint8_t *display_get_framebuffer() {
    return (uint8_t *)framebuffer;
}

void set_drawing_target(uint8_t *target) {
    current_target = target;
}

static inline volatile uint8_t* get_drawing_target() {
    return current_target ? current_target : framebuffer;
}

// ---------------------------------------------------------------------------------------------------------------------

void enable_graphics_mode(uint16_t mode) {
    g_text_mode = false;

    framebuffer = (volatile uint8_t*)VGA_ADDRESS;

    if (mode > MODE_320x200x1 || mode < MODE_320x200x1) {
        mode = MODE_320x200x1; // revert to default if out of range
    }

    // Update screen variables
    GraphicsMode selectedMode = graphicsModes[mode];
    g_SCREEN_WIDTH = selectedMode.ScreenWidth;
    g_SCREEN_HEIGHT = selectedMode.ScreenHeight;
    g_BYTES_PER_PIXEL = selectedMode.BPP;  // BPP is in bytes already

    pitch = g_SCREEN_WIDTH;

    // write registers for specific graphics mode
    write_regs(registerInitTable[mode]);
}

// Clear the screen to the specified color.
void g_clrscr(uint32_t color) {
    if (g_text_mode) {
        clrscr();
        return;
    }
    for (int y = 0; y < g_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < g_SCREEN_WIDTH; x++) {
            put_pixel(x, y, color);
        }
    }
}

void g_clear_screen() {
    g_clrscr(BLACK_16);
}

// Put a single pixel at the specified (x, y) position.
void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= g_SCREEN_WIDTH || y < 0 || y >= g_SCREEN_HEIGHT)
        return;

    // Use current_target if set; otherwise, fallback to the framebuffer.
    volatile uint8_t *target = current_target ? current_target : framebuffer;

    if (target) {
        uint32_t offset = y * pitch + x * g_BYTES_PER_PIXEL;
        switch (g_BYTES_PER_PIXEL) {
        case 1:
            ((volatile uint8_t*)target)[offset] = (uint8_t)color;
            break;
        case 2:
            *(volatile uint16_t*)(target + offset) = (uint16_t)color;
            break;
        case 4:
            *(volatile uint32_t*)(target + offset) = color;
            break;
        default:
            break;
        }
    } else {
        uint32_t offset = y * g_SCREEN_WIDTH + x;
        ((volatile uint8_t*)VGA_ADDRESS)[offset] = (uint8_t)color;
    }
}

// Draw a filled rectangle at (x, y) with width and height, in the specified color.
void draw_rect(int x, int y, int width, int height, uint32_t color) {
    if (width <= 0 || height <= 0)
        return;
    // Simple clipping
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > g_SCREEN_WIDTH)  width = g_SCREEN_WIDTH - x;
    if (y + height > g_SCREEN_HEIGHT) height = g_SCREEN_HEIGHT - y;
    if (width <= 0 || height <= 0)
        return;

    // Use the current drawing target.
    volatile uint8_t *target = get_drawing_target();

    for (int j = 0; j < height; j++) {
        uint32_t offset = (y + j) * pitch + x * g_BYTES_PER_PIXEL;
        volatile uint8_t *row_ptr = target + offset;

        if (g_BYTES_PER_PIXEL == 1) {
            for (int i = 0; i < width; i++) {
                row_ptr[i] = (uint8_t)color;
            }
        } else if (g_BYTES_PER_PIXEL == 2) {
            volatile uint16_t *pixel_ptr = (volatile uint16_t*)row_ptr;
            for (int i = 0; i < width; i++) {
                pixel_ptr[i] = (uint16_t)color;
            }
        } else if (g_BYTES_PER_PIXEL == 4) {
            volatile uint32_t *pixel_ptr = (volatile uint32_t*)row_ptr;
            for (int i = 0; i < width; i++) {
                pixel_ptr[i] = color;
            }
        } else {
            // Fallback: use VGA_ADDRESS if all else fails.
            uint32_t offset = y * g_SCREEN_WIDTH + x;
            ((volatile uint8_t*)VGA_ADDRESS)[offset] = (uint8_t)color;
        }
    }
}

// draw_line: Draw a line from (x1, y1) to (x2, y2) in the given color using Bresenham's algorithm.
void draw_line(int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = (x2 > x1 ? x2 - x1 : x1 - x2);
    int dy = (y2 > y1 ? y2 - y1 : y1 - y2);
    int sx = (x1 < x2 ? 1 : -1);
    int sy = (y1 < y2 ? 1 : -1);
    int err = dx - dy;

    while (1) {
        put_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

// Draw a single character at (x, y) using the 8x16 font.
// NOTE: This function assumes that the character is in the ASCII range 0-127, and does NOT support scaling
void draw_char(int x, int y, char ch, uint32_t color) {
    uint8_t c = (uint8_t)ch;  // Convert to unsigned value.
    for (int row = 0; row < 16; row++) {
        uint8_t bits = font8x16[c][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

// Draw a null-terminated string starting at (x, y).
// NOTE: This function assumes that the character is in the ASCII range 0-127, and does NOT support scaling
void draw_string(int x, int y, const char *str, uint32_t color) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 16;  // Move down by the font height
            x = orig_x;    // Reset to the left margin
        } else {
            draw_char(x, y, *str, color);
            x += 8;   // Advance by font width
        }
        str++;
    }
}

// Scaling--------------------------------------------------------------------------------------------------------------
// These functions are slightly modified from the above ones in order to support scaling of the font.

/**
* Draw a single scaled character at (x, y) using nearest neighbor sampling
* This supports both upscaling (scale > 1) and downscaling (scale < 1).
*/
void draw_scaled_char(int x, int y, char ch, uint32_t color, float scale) {
    uint8_t c = (uint8_t)ch;

    // Compute the output height if using the 16-pixel tall font.
    int scaled_height16 = (int)ceil(16.0f * scale);

    // Decide which font to use: if the height is less than 12, switch to 8x8.
    bool use_font8x8 = scaled_height16 < 12;
    int font_height = use_font8x8 ? 8 : 16;

    // Compute scaled dimensions based on the selected font.
    int scaled_width = (int)ceil(8.0f * scale);
    int scaled_height = (int)ceil(font_height * scale);

    for (int row = 0; row < scaled_height; row++) {
        // Map output y coordinate to source using center sampling.
        int src_y = (int)(((float)row + 0.5f) / scale);
        if (src_y >= font_height) src_y = font_height - 1;  // Safety clamp

        uint8_t bits;
        if (use_font8x8) {
            bits = font8x8_basic[c][src_y];
        } else {
            bits = font8x16[c][src_y];
        }

        for (int col = 0; col < scaled_width; col++) {
            // Map output x coordinate to source using center sampling.
            int src_x = (int)(((float)col + 0.5f) / scale);
            if (src_x >= 8) src_x = 7;  // Safety clamp

            if (bits & (0x80 >> src_x)) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

/**
* Draw a null-terminated string starting at (x, y) with scaling
*/
void draw_scaled_string(int x, int y, const char *str, uint32_t color, float scale) {
    int orig_x = x;

    while (*str) {
        if (*str == '\n') {
            y += (int)(16.0f * scale);
            x = orig_x;
        } else {
            draw_scaled_char(x, y, *str, color, scale);
            x += (int)(8.0f * scale);
        }
        str++;
    }
}

void draw_char_styled(int x, int y, char ch, uint32_t color, FontStyle style) {
    int scaled_width = (int)ceil(style.CharacterWidth * style.Scale);
    int scaled_height = (int)ceil(style.CharacterHeight * style.Scale);

    for (int row = 0; row < scaled_height; row++) {
        float src_y_f = ((float)row + 0.5f) / style.Scale;
        int src_y = (int)src_y_f;
        if (src_y >= (int)style.CharacterHeight)
            src_y = style.CharacterHeight - 1;

        for (int col = 0; col < scaled_width; col++) {
            float src_x_f = ((float)col + 0.5f) / style.Scale;
            int src_x = (int)src_x_f;
            if (src_x >= (int)style.CharacterWidth)
                src_x = style.CharacterWidth - 1;

            // For the 8x8 basic font style, invert horizontally
            if (style.Reference == (const uint8_t*)font8x8_basic) {
                src_x = style.CharacterWidth - 1 - src_x;
            }

            // Compute offset into the flattened font data.
            int char_index = (int)ch;  // ASCII 0-127.
            int offset = char_index * style.CharacterHeight + src_y;
            uint8_t row_data = ((const uint8_t*)style.Reference)[offset];

            // Create a mask for the current column.
            uint8_t mask = 1 << (style.CharacterWidth - 1 - src_x);
            if (row_data & mask) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

/**
 * Draws a null-terminated string starting at (x, y) using the given FontStyle.
 */
void draw_string_styled(int x, int y, const char *str, uint32_t color, FontStyle style) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += (int)ceil(style.CharacterHeight * style.Scale);
            x = orig_x;
        } else {
            draw_char_styled(x, y, *str, color, style);
            x += (int)ceil(style.CharacterWidth * style.Scale);
        }
        str++;
    }
}
