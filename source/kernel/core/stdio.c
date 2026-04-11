#include "stdio.h"
#include "x86.h"
#include "../libs/string.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <float.h>

// Nanobyte's code starts here
const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;
const uint8_t DEFAULT_COLOR = 0x7;

uint8_t* g_ScreenBuffer = (uint8_t*)0xB8000;
int g_ScreenX = 0, g_ScreenY = 0;

int get_g_screenx()
{
    return g_ScreenX;
}

int get_g_screeny()
{
    return g_ScreenY;
}

void putchr(int x, int y, char c)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

void putcolor(int x, int y, uint8_t color)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

char getchr(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

uint8_t getcolor(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

void setcursor(int x, int y)
{
    int pos = y * SCREEN_WIDTH + x;

    x86_outb(0x3D4, 0x0F);
    x86_outb(0x3D5, (uint8_t)(pos & 0xFF));
    x86_outb(0x3D4, 0x0E);
    x86_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clrscr()
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenX = 0;
    g_ScreenY = 0;
    setcursor(g_ScreenX, g_ScreenY);
}

void scrollback(int lines)
{
    for (int y = lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }

    for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }

    g_ScreenY -= lines;
}

void putc(char c)
{
    switch (c)
    {
        case '\n':
            g_ScreenX = 0;
            g_ScreenY++;
            break;

        case '\t':
            for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
                putc(' ');
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        case '\b':
            if (g_ScreenX > 0)
            {
                g_ScreenX--;
                putchr(g_ScreenX, g_ScreenY, ' ');
            }
            break;

        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= SCREEN_WIDTH)
    {
        g_ScreenY++;
        g_ScreenX = 0;
    }
    if (g_ScreenY >= SCREEN_HEIGHT)
        scrollback(1);

    setcursor(g_ScreenX, g_ScreenY);
}

void puts(const char* str)
{
    while(*str)
    {
        putc(*str);
        str++;
    }
}

const char g_HexChars[] = "0123456789abcdef";

void printf_unsigned(unsigned long long number, int radix)
{
    char buffer[32];
    int pos = 0;

    // convert number to ASCII
    do
    {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // print number in reverse order
    while (--pos >= 0)
        putc(buffer[pos]);
}

void printf_signed(long long number, int radix)
{
    if (number < 0)
    {
        putc('-');
        printf_unsigned(-number, radix);
    }
    else printf_unsigned(number, radix);
}

void printf_float(double number) {
    // negative numbers
    if (number < 0) {
        putc('-');
        number = -number;
    }

    // extract integer part
    int int_part = (int)number;
    double frac_part = number - int_part;
    printf_unsigned(int_part, 10);
    putc('.');

    // extract fractional part
    for (int i = 0; i < 6; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        putc('0' + digit);
        frac_part -= digit;
    }
}

#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4


static void buffer_putc(char *buffer, size_t size, int *pos, char c) {
    if (*pos < (int)size - 1) {
        buffer[*pos] = c;
    }
    (*pos)++;
}

static void buffer_print_unsigned(char *buffer, size_t size, int *pos, unsigned long long number, int radix) {
    char temp[32];
    int tpos = 0;
    do {
        unsigned long long rem = number % radix;
        number /= radix;
        temp[tpos++] = g_HexChars[rem];
    } while (number > 0);

    while (tpos-- > 0) { // print in reverse
        buffer_putc(buffer, size, pos, temp[tpos]);
    }
}

static void buffer_print_signed(char *buffer, size_t size, int *pos, long long number, int radix) {
    if (number < 0) {
        buffer_putc(buffer, size, pos, '-');
        buffer_print_unsigned(buffer, size, pos, (unsigned long long)(-number), radix);
    } else {
        buffer_print_unsigned(buffer, size, pos, (unsigned long long)number, radix);
    }
}

// vsnprintf: formats a string and writes it into str (up to size-1 characters), then null terminates it
int vsnprintf(char *str, size_t size, const char *fmt, va_list args) {
    int pos = 0;
    int state = PRINTF_STATE_NORMAL;
    int length_modifier = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    bool number = false;

    while (*fmt) {
        switch (state) {
            case PRINTF_STATE_NORMAL:
                if (*fmt == '%') {
                    state = PRINTF_STATE_LENGTH;
                } else {
                    buffer_putc(str, size, &pos, *fmt);
                }
                break;

            case PRINTF_STATE_LENGTH:
                if (*fmt == 'h') {
                    length_modifier = PRINTF_LENGTH_SHORT;
                    state = PRINTF_STATE_LENGTH_SHORT;
                } else if (*fmt == 'l') {
                    length_modifier = PRINTF_LENGTH_LONG;
                    state = PRINTF_STATE_LENGTH_LONG;
                } else {
                    state = PRINTF_STATE_SPEC;
                    continue; // Reprocess same character
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h') {
                    length_modifier = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                } else {
                    state = PRINTF_STATE_SPEC;
                    continue;
                }
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l') {
                    length_modifier = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                } else {
                    state = PRINTF_STATE_SPEC;
                    continue;
                }
                break;

            case PRINTF_STATE_SPEC:
                switch (*fmt) {
                    case 'c': {
                        int c = va_arg(args, int);
                        buffer_putc(str, size, &pos, (char)c);
                        break;
                    }
                    case 's': {
                        const char* s = va_arg(args, const char*);
                        if (s == NULL) s = "(null)";
                        while (*s) {
                            buffer_putc(str, size, &pos, *s++);
                        }
                        break;
                    }
                    case '%':
                        buffer_putc(str, size, &pos, '%');
                        break;
                    case 'd':
                    case 'i':
                        radix = 10;
                        sign = true;
                        number = true;
                        break;
                    case 'u':
                        radix = 10;
                        sign = false;
                        number = true;
                        break;
                    case 'x':
                    case 'X':
                    case 'p':
                        radix = 16;
                        sign = false;
                        number = true;
                        break;
                    case 'o':
                        radix = 8;
                        sign = false;
                        number = true;
                        break;
                    default:
                        // If the specifier is unrecognized, just output it.
                        buffer_putc(str, size, &pos, *fmt);
                        break;
                }
                if (number) {
                    if (sign) {
                        switch (length_modifier) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:
                                buffer_print_signed(str, size, &pos, va_arg(args, int), radix);
                                break;
                            case PRINTF_LENGTH_LONG:
                                buffer_print_signed(str, size, &pos, va_arg(args, long), radix);
                                break;
                            case PRINTF_LENGTH_LONG_LONG:
                                buffer_print_signed(str, size, &pos, va_arg(args, long long), radix);
                                break;
                        }
                    } else {
                        switch (length_modifier) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:
                                buffer_print_unsigned(str, size, &pos, va_arg(args, unsigned int), radix);
                                break;
                            case PRINTF_LENGTH_LONG:
                                buffer_print_unsigned(str, size, &pos, va_arg(args, unsigned long), radix);
                                break;
                            case PRINTF_LENGTH_LONG_LONG:
                                buffer_print_unsigned(str, size, &pos, va_arg(args, unsigned long long), radix);
                                break;
                        }
                    }
                    state = PRINTF_STATE_NORMAL;
                    length_modifier = PRINTF_LENGTH_DEFAULT;
                    radix = 10;
                    sign = false;
                    number = false;
                }
                break;
        }
        fmt++;
    }
    if (pos < (int)size)
        str[pos] = '\0';
    else if (size > 0)
        str[size - 1] = '\0';
    return pos;
}

// COPIED CODE FROM Nanobyte (https://github.com/nanobyte-dev/nanobyte_os/blob/videos/part7/src/kernel/stdio.c) STARTS HERE
// Adjustments:
// Added support for floating point numbers
void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false;
    bool number = false;

    while (*fmt)
    {
        switch (state)
        {
            case PRINTF_STATE_NORMAL:
                switch (*fmt)
                {
                    case '%':   state = PRINTF_STATE_LENGTH;
                                break;
                    default:    putc(*fmt);
                                break;
                }
                break;

            case PRINTF_STATE_LENGTH:
                switch (*fmt)
                {
                    case 'h':   length = PRINTF_LENGTH_SHORT;
                                state = PRINTF_STATE_LENGTH_SHORT;
                                break;
                    case 'l':   length = PRINTF_LENGTH_LONG;
                                state = PRINTF_STATE_LENGTH_LONG;
                                break;
                    default:    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h')
                {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l')
                {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state = PRINTF_STATE_SPEC;
                }
                else goto PRINTF_STATE_SPEC_;
                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt)
                {
                    case 'c':   putc((char)va_arg(args, int));
                                break;

                    case 's':
                                puts(va_arg(args, const char*));
                                break;

                    case '%':   putc('%');
                                break;

                    case 'd':
                    case 'i':   radix = 10; sign = true; number = true;
                                break;

                    case 'u':   radix = 10; sign = false; number = true;
                                break;

                    case 'X':
                    case 'x':
                    case 'p':   radix = 16; sign = false; number = true;
                                break;

                    case 'o':   radix = 8; sign = false; number = true;
                                break;

                    case 'f': {
                                double f = va_arg(args, double);
                                printf_float(f);
                                break;
                    }

                    // ignore invalid spec
                    default:    break;
                }

                if (number)
                {
                    if (sign)
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     printf_signed(va_arg(args, int), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG:        printf_signed(va_arg(args, long), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_signed(va_arg(args, long long), radix);
                                                        break;
                        }
                    }
                    else
                    {
                        switch (length)
                        {
                        case PRINTF_LENGTH_SHORT_SHORT:
                        case PRINTF_LENGTH_SHORT:
                        case PRINTF_LENGTH_DEFAULT:     printf_unsigned(va_arg(args, unsigned int), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG:        printf_unsigned(va_arg(args, unsigned  long), radix);
                                                        break;

                        case PRINTF_LENGTH_LONG_LONG:   printf_unsigned(va_arg(args, unsigned  long long), radix);
                                                        break;
                        }
                    }
                }

                // reset state
                state = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix = 10;
                sign = false;
                number = false;
                break;
        }

        fmt++;
    }

    va_end(args);
}

void print_buffer(const char* msg, const void* buffer, uint32_t count)
{
    const uint8_t* u8Buffer = (const uint8_t*)buffer;

    puts(msg);
    for (uint16_t i = 0; i < count; i++)
    {
        putc(g_HexChars[u8Buffer[i] >> 4]);
        putc(g_HexChars[u8Buffer[i] & 0xF]);
    }
    puts("\n");
}

// COPIED CODE ENDS HERE

int snprintf(char* str, size_t size, const char* format, ...) {
    if (size == 0) return 0;

    size_t written = 0;
    size_t max_size = size - 1;  // Reserve space for null terminator
    char* current = str;
    const char* fmt = format;

    va_list args;
    va_start(args, format);

    while (*fmt && written < max_size) {
        if (*fmt != '%') {
            *current++ = *fmt++;
            written++;
        } else {
            fmt++;
            switch (*fmt) {
            case 's': {
                    const char* s = va_arg(args, const char*);
                    size_t len = strlen(s);
                    if (written + len > max_size) {
                        len = max_size - written;
                    }
                    memcpy(current, s, len);
                    current += len;
                    written += len;
                    break;
            }
            case 'd': {
                    int d = va_arg(args, int);
                    char num_str[20];
                    int num_len;
                    int_to_str(d, num_str, &num_len);
                    if (written + num_len > max_size) {
                        num_len = max_size - written;
                    }
                    memcpy(current, num_str, num_len);
                    current += num_len;
                    written += num_len;
                    break;
            }
            default:
                *current++ = *fmt;
                written++;
                break;
            }
            fmt++;
        }
    }

    *current = '\0';
    va_end(args);

    return (int)written;
}