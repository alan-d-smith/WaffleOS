#pragma once

#include <stdint.h>

#define VGA_ADDRESS 0xA0000

// 16 colour palette
#define BLACK_16        0x0  // Black
#define BLUE_16         0x1  // Blue
#define GREEN_16        0x2  // Green
#define CYAN_16         0x3  // Cyan
#define RED_16          0x4  // Red
#define MAGENTA_16      0x5  // Magenta
#define BROWN_16        0x6  // Brown (Dark Yellow)
#define LIGHT_GRAY_16   0x7  // Light Gray (Default UI color)
#define DARK_GRAY_16    0x8  // Dark Gray
#define LIGHT_BLUE_16   0x9  // Light Blue
#define LIGHT_GREEN_16  0xA  // Light Green
#define LIGHT_CYAN_16   0xB  // Light Cyan
#define LIGHT_RED_16    0xC  // Light Red
#define LIGHT_MAGENTA_16 0xD // Light Magenta (Pink)
#define YELLOW_16       0xE  // Yellow
#define WHITE_16        0xF  // White

// 256 colour palette
#define BLACK_256       0x000000  // RGB(0, 0, 0)
#define BLUE_256        0x0000AA  // RGB(0, 0, 170)
#define GREEN_256       0x00AA00  // RGB(0, 170, 0)
#define CYAN_256        0x00AAAA  // RGB(0, 170, 170)
#define RED_256         0xAA0000  // RGB(170, 0, 0)
#define MAGENTA_256     0xAA00AA  // RGB(170, 0, 170)
#define BROWN_256       0xAA5500  // RGB(170, 85, 0)
#define LIGHTGRAY_256   0xAAAAAA  // RGB(170, 170, 170)

#define DARKGRAY_256    0x555555  // RGB(85, 85, 85)
#define LIGHTBLUE_256   0x5555FF  // RGB(85, 85, 255)
#define LIGHTGREEN_256  0x55FF55  // RGB(85, 255, 85)
#define LIGHTCYAN_256   0x55FFFF  // RGB(85, 255, 255)
#define LIGHTRED_256    0xFF5555  // RGB(255, 85, 85)
#define LIGHTMAGENTA_256 0xFF55FF // RGB(255, 85, 255)
#define YELLOW_256      0xFFFF55  // RGB(255, 255, 85)
#define WHITE_256       0xFFFFFF  // RGB(255, 255, 255)

// Copied code from source: https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
#define	VGA_AC_INDEX		0x3C0
#define	VGA_AC_WRITE		0x3C0
#define	VGA_AC_READ		    0x3C1
#define	VGA_INSTAT_READ		0x3DA
#define	VGA_MISC_WRITE		0x3C2
#define	VGA_MISC_READ		0x3CC

/*			COLOR emulation		MONO emulation */
#define VGA_CRTC_INDEX		0x3D4		/* 0x3B4 */
#define VGA_CRTC_DATA		0x3D5		/* 0x3B5 */
#define VGA_GC_INDEX 		0x3CE
#define VGA_GC_DATA 		0x3CF
#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5

#define	VGA_NUM_AC_REGS		21
#define	VGA_NUM_CRTC_REGS	25
#define	VGA_NUM_GC_REGS		9
#define	VGA_NUM_SEQ_REGS	5

extern unsigned char g_320x200x256[];
extern unsigned char g_640x480x16[];

void write_regs(unsigned char *regs);
// end copied code

// VGA DAC (palette) ports
#define VGA_DAC_WRITE_INDEX  0x3C8
#define VGA_DAC_DATA         0x3C9

// Set a single palette entry. r/g/b are 6-bit (0-63).
void vga_set_palette_entry(unsigned char index, unsigned char r, unsigned char g, unsigned char b);

// Load a full 256-colour palette
void vga_load_256_palette(void);

// Standard 80x25 text mode register dump
extern unsigned char g_80x25_text[];
void vga_write_font(const uint8_t font[128][16]);