#include "vga.h"
#include <stdio.h>
#include <x86.h>

// Begin copied code
// source: http://files.osdev.org/mirrors/geezer/osd/graphics/modes.c

unsigned char g_320x200x256[] =
{
	/* MISC */
	0x63,
/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x0E,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00,	0x00
};

unsigned char g_640x480x16[] =
{
	/* MISC */
	0xE3,
/* SEQ */
	0x03, 0x01, 0x08, 0x00, 0x06,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};

void write_regs(unsigned char *regs)
{
	unsigned i;

/* write MISCELLANEOUS reg */
	x86_outb(VGA_MISC_WRITE, *regs);
	regs++;
/* write SEQUENCER regs */
	for(i = 0; i < VGA_NUM_SEQ_REGS; i++)
	{
		x86_outb(VGA_SEQ_INDEX, i);
		x86_outb(VGA_SEQ_DATA, *regs);
		regs++;
	}
/* unlock CRTC registers */
	x86_outb(VGA_CRTC_INDEX, 0x03);
	x86_outb(VGA_CRTC_DATA, x86_inb(VGA_CRTC_DATA) | 0x80);
	x86_outb(VGA_CRTC_INDEX, 0x11);
	x86_outb(VGA_CRTC_DATA, x86_inb(VGA_CRTC_DATA) & ~0x80);
/* make sure they remain unlocked */
	regs[0x03] |= 0x80;
	regs[0x11] &= ~0x80;
/* write CRTC regs */
	for(i = 0; i < VGA_NUM_CRTC_REGS; i++)
	{
		x86_outb(VGA_CRTC_INDEX, i);
		x86_outb(VGA_CRTC_DATA, *regs);
		regs++;
	}
/* write GRAPHICS CONTROLLER regs */
	for(i = 0; i < VGA_NUM_GC_REGS; i++)
	{
		x86_outb(VGA_GC_INDEX, i);
		x86_outb(VGA_GC_DATA, *regs);
		regs++;
	}
/* write ATTRIBUTE CONTROLLER regs */
	for(i = 0; i < VGA_NUM_AC_REGS; i++)
	{
		(void)x86_inb(VGA_INSTAT_READ);
		x86_outb(VGA_AC_INDEX, i);
		x86_outb(VGA_AC_WRITE, *regs);
		regs++;
	}
/* lock 16-color palette and unblank display */
	(void)x86_inb(VGA_INSTAT_READ);
	x86_outb(VGA_AC_INDEX, 0x20);
}
// end copied code

void vga_set_palette_entry(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	x86_outb(VGA_DAC_WRITE_INDEX, index);
	x86_outb(VGA_DAC_DATA, r & 0x3F);
	x86_outb(VGA_DAC_DATA, g & 0x3F);
	x86_outb(VGA_DAC_DATA, b & 0x3F);
}

void vga_load_256_palette(void)
{
	// 0-15: standard 16 VGA colours
	static const uint32_t base16[16] = {
		BLACK_256, BLUE_256, GREEN_256, CYAN_256,
		RED_256, MAGENTA_256, BROWN_256, LIGHTGRAY_256,
		DARKGRAY_256, LIGHTBLUE_256, LIGHTGREEN_256, LIGHTCYAN_256,
		LIGHTRED_256, LIGHTMAGENTA_256, YELLOW_256, WHITE_256
	};
	for (int i = 0; i < 16; i++) {
		uint32_t c = base16[i];
		vga_set_palette_entry((unsigned char)i,
		                      (unsigned char)(((c >> 16) & 0xFF) >> 2),
		                      (unsigned char)(((c >> 8) & 0xFF) >> 2),
		                      (unsigned char)((c & 0xFF) >> 2));
	}

	// 16-231: 6x6x6 RGB colour cube (216 cols)
	int idx = 16;
	for (int r = 0; r < 6; r++)
		for (int g = 0; g < 6; g++)
			for (int b = 0; b < 6; b++)
				vga_set_palette_entry((unsigned char)idx++,
				                      (unsigned char)(r * 63 / 5),
				                      (unsigned char)(g * 63 / 5),
				                      (unsigned char)(b * 63 / 5));

	// 232-255: a 24-step grayscale ramp.
	for (int i = 0; i < 24; i++) {
		unsigned char v = (unsigned char)(i * 63 / 23);
		vga_set_palette_entry((unsigned char)(232 + i), v, v, v);
	}
}

// Register dump for standard 80x25 colour text mode (mode 0x03).
// source: http://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
unsigned char g_80x25_text[] =
{
/* MISC */
	0x67,
/* SEQ */
	0x03, 0x00, 0x03, 0x00, 0x02,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F,
	0x00, 0x4F, 0x0D, 0x0E, 0x00, 0x00, 0x00, 0x50,
	0x9C, 0x0E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x0C, 0x00, 0x0F, 0x08, 0x00,
};

// Load an 8x16 font into VGA plane 2 so text mode can render glyphs again
// source: https://wiki.osdev.org/VGA_Fonts
void vga_write_font(const uint8_t font[128][16])
{
	// save the registers we are about to repurpose
	unsigned char seq2, seq4, gc4, gc5, gc6;
	x86_outb(VGA_SEQ_INDEX, 2); seq2 = x86_inb(VGA_SEQ_DATA);
	x86_outb(VGA_SEQ_INDEX, 4); seq4 = x86_inb(VGA_SEQ_DATA);
	x86_outb(VGA_GC_INDEX, 4);  gc4  = x86_inb(VGA_GC_DATA);
	x86_outb(VGA_GC_INDEX, 5);  gc5  = x86_inb(VGA_GC_DATA);
	x86_outb(VGA_GC_INDEX, 6);  gc6  = x86_inb(VGA_GC_DATA);

	// set up linear access to plane 2 at 0xA0000
	x86_outb(VGA_SEQ_INDEX, 2); x86_outb(VGA_SEQ_DATA, 0x04);
	x86_outb(VGA_SEQ_INDEX, 4); x86_outb(VGA_SEQ_DATA, 0x06);
	x86_outb(VGA_GC_INDEX, 4);  x86_outb(VGA_GC_DATA, 0x02);
	x86_outb(VGA_GC_INDEX, 5);  x86_outb(VGA_GC_DATA, 0x00);
	x86_outb(VGA_GC_INDEX, 6);  x86_outb(VGA_GC_DATA, 0x00);

	volatile unsigned char* plane = (volatile unsigned char*)0xA0000;
	for (int c = 0; c < 128; c++)
		for (int row = 0; row < 16; row++)
			plane[c * 32 + row] = font[c][row];

	// restore the saved registers
	x86_outb(VGA_SEQ_INDEX, 2); x86_outb(VGA_SEQ_DATA, seq2);
	x86_outb(VGA_SEQ_INDEX, 4); x86_outb(VGA_SEQ_DATA, seq4);
	x86_outb(VGA_GC_INDEX, 4);  x86_outb(VGA_GC_DATA, gc4);
	x86_outb(VGA_GC_INDEX, 5);  x86_outb(VGA_GC_DATA, gc5);
	x86_outb(VGA_GC_INDEX, 6);  x86_outb(VGA_GC_DATA, gc6);
}
