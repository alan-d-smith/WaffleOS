// using source: https://wiki.osdev.org/Floppy_Disk_Controller

#include "floppy.h"
#include "../core/x86.h"
#include "../core/stdio.h"
#include "../core/idt.h"
#include <stdbool.h>

#define SECTOR_SIZE 512

extern void irq6(void);

FloppyController Controller = {
    .DOR  = 0x3F2,
    .MSR  = 0x3F4,
    .FIFO = 0x3F5,
    .CCR  = 0x3F7
};

volatile bool ReceivedIRQ = false;

void lba_2_chs(uint32_t lba, uint16_t* cyl, uint16_t* head, uint16_t* sector)
{
    *cyl    = lba / (2 * FLOPPY_144_SECTORS_PER_TRACK);
    *head   = ((lba % (2 * FLOPPY_144_SECTORS_PER_TRACK)) / FLOPPY_144_SECTORS_PER_TRACK);
    *sector = ((lba % (2 * FLOPPY_144_SECTORS_PER_TRACK)) % FLOPPY_144_SECTORS_PER_TRACK + 1);
}

uint8_t read_data_byte(void) {
    // Wait until the controller is ready to send data.
    while (!(x86_inb(Controller.MSR) & 0x80));
    return x86_inb(Controller.FIFO);
}


void floppy_handler()
{
    ReceivedIRQ = true;
}

void send_command(uint8_t command) {
    // Wait until the controller is ready.
    while (!(x86_inb(Controller.MSR) & 0x80));

    x86_outb(Controller.FIFO, command);
}

void reset_floppy() {
    ReceivedIRQ = false;
    // Enter reset mode
    x86_outb(Controller.DOR, 0x00);
    // Exit reset mode: set motor enable and interrupt enable bits (0x0C)
    x86_outb(Controller.DOR, 0x0C);
    // Wait until IRQ is received
    while(!ReceivedIRQ);
    // Typically, four SENSE_INTERRUPT commands are issued after reset.
    for (int i = 0; i < 4; i++) {
        send_command(SENSE_INTERRUPT);
        read_data_byte();
        read_data_byte();
    }
    // Set data rate for a 1.44MB floppy (500Kbps)
    x86_outb(Controller.CCR, 0x00);
    // Configure drive parameters using SPECIFY command.
    send_command(SPECIFY);
    x86_outb(Controller.FIFO, STEPRATE_HEADUNLOAD);
    x86_outb(Controller.FIFO, HEADLOAD_NDMA);
}

void init_floppy(void) {
    printf("[I/O] Initializing Floppy...\r\n");
    register_interrupt_handler(38, (uint32_t)irq6); // IRQ6 remapped to vector 38
    enable_irq(6);
    printf("[I/O] Floppy initialized\r\n");
}

bool FLOPPY_ReadSectors(uint32_t lba, uint32_t sector_count, void* buffer) {
    uint8_t* buf = (uint8_t*)buffer;

    for (uint32_t i = 0; i < sector_count; i++) {
        uint16_t cyl, head, sector;
        lba_2_chs(lba + i, &cyl, &head, &sector);

        ReceivedIRQ = false;

        send_command(READ_DATA);
        // Byte 1: (drive << 2) | head; using drive 0 here.
        x86_outb(Controller.FIFO, (0 << 2) | head);
        x86_outb(Controller.FIFO, cyl);
        x86_outb(Controller.FIFO, head);
        x86_outb(Controller.FIFO, sector);
        x86_outb(Controller.FIFO, 2);  // 2 means 512 bytes per sector (128 << 2 == 512)
        x86_outb(Controller.FIFO, FLOPPY_144_SECTORS_PER_TRACK);  // Typically 18 sectors per track.
        x86_outb(Controller.FIFO, 27);  // Gap length.
        x86_outb(Controller.FIFO, 0xFF);  // Data length (unused).

        while (!ReceivedIRQ) {
            // TODO: add timeout
        }

        // Read 512 bytes (one sector) from the FIFO.
        for (int j = 0; j < SECTOR_SIZE; j++) {
            buf[i * SECTOR_SIZE + j] = read_data_byte();
        }
    }

    return true;
}