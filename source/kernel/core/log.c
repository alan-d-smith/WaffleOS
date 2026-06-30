#include "log.h"
#include "cmd_executor.h"
#include "../drivers/vga.h"
#include <stdarg.h>
#include <stdint.h>

// Hash the subsystem tag into a stable colour and avoid red/yellow
static uint8_t tag_color(const char *tag) {
    uint32_t hash = 2166136261u;
    for (const char *p = tag; *p; ++p) {
        hash ^= (uint8_t)*p;
        hash *= 16777619u;
    }
    static const uint8_t palette[] = {
        BLUE_16, GREEN_16, CYAN_16, MAGENTA_16, BROWN_16,
        LIGHT_BLUE_16, LIGHT_GREEN_16, LIGHT_CYAN_16, LIGHT_MAGENTA_16, WHITE_16,
    };
    return palette[hash % (sizeof(palette) / sizeof(palette[0]))];
}

// Apply severity colour for message body
static void set_message_color(LogLevel level) {
    switch (level) {
        case LOG_OK:
            console_set_color(LIGHT_GREEN_16);
            break;
        case LOG_WARN:
            console_set_color(YELLOW_16);
            break;
        case LOG_ERROR:
            console_set_color(LIGHT_RED_16);
            break;
        case LOG_INFO:
        default:
            console_reset_color();
            break;
    }
}

void klog(LogLevel level, const char *tag, const char *fmt, ...) {
    console_reset_color();
    console_printf("[");
    console_set_color(tag_color(tag));
    console_printf("%s", tag);
    console_reset_color();
    console_printf("] ");

    set_message_color(level);

    va_list args;
    va_start(args, fmt);
    console_vprintf(fmt, args);
    va_end(args);

    console_reset_color();
    console_printf("\r\n");
}
