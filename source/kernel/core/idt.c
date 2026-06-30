//
// Interrupt Descriptor Table
// Based on: https://wiki.osdev.org/Interrupt_Descriptor_Table
//
// Some code is from nanobye's IDT implementation:
// https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part8/src/kernel/arch/i686/idt.c
//

#include "idt.h"
#include "stdio.h"
#include "kernel.h"
#include "x86.h"
#include "gdt.h"

#include <utils/binary.h>
#include "log.h"

#define IDT_SIZE 256

// Start of code by nanobyte
typedef struct
{
    uint16_t BaseLow;            // offset bits 0..15
    uint16_t SegmentSelector;    // a code segment selector in GDT
    uint8_t Reserved;            // reserved/unused, set to 0
    uint8_t Flags;               // gate type, dpl, and p fields
    uint16_t BaseHigh;           // offset bits 16..31
} __attribute__((packed)) IDTEntry;

typedef struct
{
    uint16_t Limit;
    IDTEntry* Ptr;
} __attribute__((packed)) IDTDescriptor;

IDTEntry g_IDT[256];
IDTDescriptor g_IDTDescriptor = { sizeof(g_IDT) - 1, g_IDT };

void __attribute__((cdecl)) i686_IDT_Load(IDTDescriptor* idtDescriptor);

void i686_IDT_SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    g_IDT[interrupt].BaseLow = ((uint32_t)base) & 0xFFFF;
    g_IDT[interrupt].SegmentSelector = segmentDescriptor;
    g_IDT[interrupt].Reserved = 0;
    g_IDT[interrupt].Flags = flags;
    g_IDT[interrupt].BaseHigh = ((uint32_t)base >> 16) & 0xFFFF;
}

void i686_IDT_EnableGate(int interrupt)
{
    FLAG_SET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void i686_IDT_DisableGate(int interrupt)
{
    FLAG_UNSET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}
// End of code by nanobyte

void enable_interrupts(void) {
    __asm__ __volatile__("sti");
}

void disable_interrupts(void) {
    __asm__ __volatile__("cli");
}

void send_eoi(int irq) { // end of interrupt signal
    if (irq >= 8) {
        x86_outb(PIC2_CMD_PORT, PIC_EOI);  // Send EOI to slave PIC
    }
    x86_outb(PIC1_CMD_PORT, PIC_EOI);      // Always send EOI to master PIC
}

void init_idt(void) {
    log_info("IDT", "Initializing IDT...");

    for (int i = 0; i < IDT_SIZE; i++) {
        g_IDT[i].BaseLow = 0;
        g_IDT[i].SegmentSelector = 0;
        g_IDT[i].Reserved = 0;
        g_IDT[i].Flags = 0;
        g_IDT[i].BaseHigh = 0;
    }

    log_info("IDT", "IDT address: 0x%x", (uint32_t)g_IDTDescriptor.Ptr);

    i686_IDT_Load(&g_IDTDescriptor);

    log_ok("IDT", "Initialized IDT");

    disable_interrupts();
    init_pic();
}

#define ALL_IRQ_MASK 0xFF
void init_pic(void) { // remap PIC
    // Start the initialization sequence for both PICs
    x86_outb(PIC1_CMD_PORT, ICW1_INIT);
    x86_outb(PIC2_CMD_PORT, ICW1_INIT);
    // Set the offsets for the PICs
    x86_outb(PIC1_DATA_PORT, ICW2_MASTER_OFFSET);
    x86_outb(PIC2_DATA_PORT, ICW2_SLAVE_OFFSET);
    // Tell the PICs about their cascade
    x86_outb(PIC1_DATA_PORT, ICW3_MASTER);    // master
    x86_outb(PIC2_DATA_PORT, ICW3_SLAVE);    // slave
    // Set PICs to operate in 8086/88 mode
    x86_outb(PIC1_DATA_PORT, ICW4_8086_MODE);
    x86_outb(PIC2_DATA_PORT, ICW4_8086_MODE);

    // Mask all IRQs except IRQ2 (cascade)
    uint8_t master_mask = ALL_IRQ_MASK;
    FLAG_UNSET(master_mask, IRQ2_BIT); // This clears bit 2 (IRQ2) so that it is unmasked.
    x86_outb(PIC1_DATA_PORT, master_mask);

    // Leave the slave PIC fully masked until you explicitly enable needed IRQs.
    x86_outb(PIC2_DATA_PORT, ALL_IRQ_MASK);
}

void enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {    // master PIC (IRQ 0-7)
        port = PIC1_DATA_PORT;
    } else {          // slave PIC (IRQ 8-15)
        port = PIC2_DATA_PORT;
        irq -= 8;
    }

    value = x86_inb(port);
    // Clear the IRQ bit using bit mask
    FLAG_UNSET(value, (1 << irq));
    x86_outb(port, value);
}

void disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {    // master PIC (IRQ 0-7)
        port = PIC1_DATA_PORT;
    } else {          // slave PIC (IRQ 8-15)
        port = PIC2_DATA_PORT;
        irq -= 8;
    }

    value = x86_inb(port);
    value |= (1 << irq);   // set the bit to mask (disable) the IRQ
    x86_outb(port, value);
}

void register_interrupt_handler(uint8_t interrupt_number, uint32_t handler_address) {
    i686_IDT_SetGate(interrupt_number, (void*)handler_address, i686_GDT_CODE_SEGMENT,
                     IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_GATE_32BIT_INT);
    log_info("IDT", "Registered interrupt handler for IRQ %d", interrupt_number - 32);
}
