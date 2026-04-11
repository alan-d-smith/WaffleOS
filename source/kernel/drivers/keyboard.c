// A basic keyboard driver for reading input from the user
// Using information from: https://wiki.osdev.org/PS/2_Keyboard

#include <stdint.h>
#include <stddef.h>

#include "keyboard.h"
#include "display.h"
#include "../core/stdio.h"
#include "../core/x86.h"
#include "../core/idt.h"
#include "../core/kernel.h"
#include "../core/cmd_executor.h"
#include "../libs/apps/terminal.h"

extern void irq1(void);

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define BACKSPACE_SCANCODE 0x0E
#define ENTER_SCANCODE 0x1C
#define MAX_INPUT_LENGTH 256

char input_buffer[MAX_INPUT_LENGTH];
uint32_t buffer_index = 0;
volatile int line_ready = 0;  // line ready to return flag
volatile int min_col = 0;

// US keyboard layout mapping
const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

void keyboard_handler(void) {
    uint8_t status;
    uint8_t scancode;
    char ascii;

    status = x86_inb(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        scancode = x86_inb(KEYBOARD_DATA_PORT);

        // Only process key press events (ignore key releases)
        if (!(scancode & 0x80)) {
            if (scancode == ENTER_SCANCODE) {
                if (!g_text_mode) {
                    terminal_handle_key('\n'); // Forward newline to terminal window
                } else {
                    console_putc('\r');
                    console_putc('\n');
                    input_buffer[buffer_index] = '\0';
                    line_ready = 1;  // Line is ready
                }
                send_eoi(1);
                return;
            } else if (scancode == BACKSPACE_SCANCODE) {
                if (!g_text_mode) {
                    terminal_handle_key('\b');
                } else {
                    if (buffer_index > 0) {
                        buffer_index--;
                        input_buffer[buffer_index] = '\0';
                        console_putc('\b');
                    }
                }
            } else if (buffer_index < MAX_INPUT_LENGTH - 1) {
                ascii = scancode_to_ascii[scancode];
                if (ascii != 0) {
                    if (!g_text_mode) {
                        terminal_handle_key(ascii);
                    } else {
                        input_buffer[buffer_index++] = ascii;
                        console_putc(ascii);
                    }
                }
            }
        }
    }

    send_eoi(1);
}

void init_keyboard(void) {
    printf("[I/O] Initializing keyboard...\r\n");

    register_interrupt_handler(33, (uint32_t)irq1);
    enable_irq(1);

    printf("[I/O] Keyboard initialized\r\n");
}


void reset_keyboard(void) {
    buffer_index = 0;
    line_ready = 0;
    min_col = get_g_screenx();
}


char* read_line(void) {
    reset_keyboard();
    while (!line_ready) {
        __asm__("hlt");  // halt CPU until next interrupt
    }
    return input_buffer;
}

char* read_line_nonblocking(void) {
    return line_ready ? input_buffer : NULL;
}