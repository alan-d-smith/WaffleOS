//
// Based on: https://wiki.osdev.org/Programmable_Interval_Timer
//

#include "../timers/timer.h"
#include "../core/idt.h"
#include "../core/stdio.h"
#include "../core/x86.h"
#include "log.h"

#define PIT_FREQUENCY 1193182
#define TARGET_FREQUENCY 100     // Hz
#define PIT_COMMAND 0x36
#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND_PORT 0x43

extern void irq0(void);

// 64 bit tick count
static volatile uint32_t milliseconds_low = 0;
static volatile uint32_t milliseconds_high = 0;

void init_pit() {
    log_info("PIT", "Initializing Programmable Interval Timer...");

    uint32_t divisor = PIT_FREQUENCY / TARGET_FREQUENCY;

    x86_outb(PIT_COMMAND_PORT, PIT_COMMAND);
    x86_outb(PIT_CHANNEL_0, (uint8_t)(divisor & 0xFF));
    x86_outb(PIT_CHANNEL_0, (uint8_t)((divisor >> 8) & 0xFF));

    register_interrupt_handler(32, (uint32_t)irq0); // IRQ0 is interrupt 32

    enable_irq(0); // Enable IRQ0

    log_ok("PIT", "Programmable Interval Timer initialized and running at %dHz", TARGET_FREQUENCY);
}

void timer_handler(void) {
    milliseconds_low+=10;
    if (milliseconds_low == 0) {  // Overflow occurred
        milliseconds_high++;
    }

    send_eoi(0);
}

void sleep_millis(uint32_t duration) {
    uint32_t start_low = milliseconds_low;
    uint32_t start_high = milliseconds_high;

    uint32_t end_low = start_low + duration;
    uint32_t end_high = start_high + (end_low < start_low ? 1 : 0);

    while (milliseconds_high < end_high || ((milliseconds_high == end_high) && (milliseconds_low < end_low))) {
        __asm__ volatile("hlt");
    }
}

void sleep(uint32_t duration) {
    sleep_millis(duration * 1000);
}

void get_milliseconds(uint32_t* low, uint32_t* high) {
    disable_interrupts();

    *low = milliseconds_low;
    *high = milliseconds_high;

    enable_interrupts();
}

uint32_t get_seconds(void) {
    uint32_t low, high;
    get_milliseconds(&low, &high);

    uint32_t seconds = high * 4294967 + low / 1000;

    // 37/125 is 0.296 : used for added precision
    seconds += (high * 37) / 125;
    return seconds;
}

void get_systime_string(char* buffer) {
    uint32_t seconds = get_seconds();
    uint32_t minutes = (seconds % 3600) / 60;;
    uint32_t hours = seconds / 3600;
    seconds %= 60;

    // ascii manipulation
    buffer[0] = '0' + hours / 10;
    buffer[1] = '0' + hours % 10;
    buffer[2] = ':';
    buffer[3] = '0' + minutes / 10;
    buffer[4] = '0' + minutes % 10;
    buffer[5] = ':';
    buffer[6] = '0' + seconds / 10;
    buffer[7] = '0' + seconds % 10;
    buffer[8] = '\0';
}

void get_systime_millis(uint32_t* high, uint32_t* low) {
    disable_interrupts();

    if (low) *low = milliseconds_low;
    if (high) *high = milliseconds_high;

    enable_interrupts();
}

void print_systime(void) {    // print time since system startup
    char buffer[9];
    get_systime_string(buffer);
    printf("%s", buffer);
}