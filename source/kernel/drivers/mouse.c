// A minimal implementation of a PS/2 mouse driver
// Using information from: https://wiki.osdev.org/PS/2_Mouse
// Adapted from SANiK's mouse driver: https://forum.osdev.org/viewtopic.php?t=10247

#include "mouse.h"
#include "../core/idt.h"
#include "../core/x86.h"
#include "../core/stdio.h"
#include "../core/kernel.h"
#include "../libs/math.h"
#include "display.h"

// global mouse state
volatile MouseState mouse_state = { .x = 160, .y = 100, .buttons = 0 };

// Vars to accumulate the 3 byte mouse packet
static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[3];
static uint8_t mouse_id = 0x00;

extern void irq12(void);

static inline void mouse_wait(uint8_t a_type)
{
    uint32_t _time_out = 100000;
    if(a_type == 0) {
        // Wait for data to be available
        while(_time_out--) { // Data
            if((x86_inb(MOUSE_STATUS_PORT) & 1) == 1)
            {
            	return;
          	}
        }
        return;
    } else {
        // Wait for the input buffer to be empty
        while(_time_out--) { // Signal
            if((x86_inb(MOUSE_STATUS_PORT) & 2) == 0)
            {
            	return;
            }
        }
        return;
    }
}

static inline void mouse_write(uint8_t a_write)
{
    mouse_wait(1);
    // Tell the controller we're sending a command to the mouse
    x86_outb(MOUSE_STATUS_PORT, 0xD4);
    mouse_wait(1);
    x86_outb(MOUSE_DATA_PORT, a_write);
}

uint8_t mouse_read()
{
    mouse_wait(0);
    return x86_inb(MOUSE_DATA_PORT);
}

void mouse_handler(void)
{
    uint8_t data = x86_inb(MOUSE_DATA_PORT);

    // If we're at the beginning of a packet, verify that the packet is aligned.
    if (mouse_cycle == 0 && !(data & 0x08)) {
        // Packet is misaligned. Discard byte
        return;
    }

    mouse_bytes[mouse_cycle] = data;
    mouse_cycle++;

    if (mouse_cycle == 3) {
        // Process complete packet:
        mouse_state.buttons = mouse_bytes[0] & 0x07;
        int8_t x_move = (int8_t)mouse_bytes[1];
        int8_t y_move = (int8_t)mouse_bytes[2];

        mouse_state.x = clamp(mouse_state.x + x_move, 0, g_SCREEN_WIDTH - 1);
        mouse_state.y = clamp(mouse_state.y - y_move, 0, g_SCREEN_HEIGHT - 1);

        mouse_cycle = 0;
    }

    send_eoi(12);
}

int init_mouse(void)
{
    uint8_t status;

    // Enable the auxiliary (mouse) device.
    mouse_wait(1);
    x86_outb(MOUSE_STATUS_PORT, PS2_CMD_ENABLE_MOUSE);

    // Enable the interrupts for the mouse.
    mouse_wait(1);
    x86_outb(MOUSE_STATUS_PORT, PS2_CMD_READ_CONFIG);
    mouse_wait(0);
    status = (x86_inb(MOUSE_DATA_PORT) | 2);
    mouse_wait(1);
    x86_outb(MOUSE_STATUS_PORT, PS2_CMD_WRITE_CONFIG);
    mouse_wait(1);
    x86_outb(MOUSE_DATA_PORT, status);

    // Reset the mouse before setting defaults.
    mouse_write(MOUSE_CMD_RESET);
    uint8_t ack = mouse_read();
    if (ack != MOUSE_ACK) {
        printf("[MOUSE] Mouse reset failed (ACK not received).\r\n");
        return -1;
    }

    uint8_t self_test = mouse_read();
    if (self_test != MOUSE_SELF_TEST_PASS) {
        printf("[MOUSE] Mouse self-test failed.\r\n");
        return -1;
    }

    // Set the mouse to its default settings.
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    mouse_id = mouse_read();

    uint8_t defaults_ack = mouse_read();
    if (defaults_ack != MOUSE_ACK) {
    	printf("[MOUSE] Failed to set defaults. Received: 0x%x\r\n", defaults_ack);
        return -1;
    }

    // Enable data reporting.
    mouse_write(MOUSE_CMD_ENABLE);
    if (mouse_read() != MOUSE_ACK) {
        printf("[MOUSE] Failed to enable data reporting.\r\n");
        return -1;
    }

    // Register the mouse interrupt handler.
    // IRQ 12 is remapped to interrupt number 44
    register_interrupt_handler(32 + 12, (uint32_t)irq12);
    enable_irq(12);

    printf("[MOUSE] Mouse driver installed.\r\n");
    return 0; // Success
}
