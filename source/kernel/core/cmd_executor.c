#include "cmd_executor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "../libs/string.h"
#include "stdio.h"
#include "memory.h"
#include "../drivers/display.h"
#include "../drivers/vga.h"
#include "../libs/apps/terminal.h"

#include "base_cmds/basic.h"
#include "base_cmds/file_cmds.h"

#define MAX_COMMANDS 32
#define MAX_TOKENS   16

#define CONSOLE_TEXT_DEFAULT  LIGHT_GRAY_16 // VGA text default
#define CONSOLE_GUI_DEFAULT   WHITE_16      // GUI terminal default

static Command command_list[MAX_COMMANDS];
static int command_count = 0;

// Maps ANSI SGR foreground codes 30-37 to VGA/palette indices.
static const uint8_t ansi_fg_to_vga[8] = {
    BLACK_16, RED_16, GREEN_16, BROWN_16, BLUE_16, MAGENTA_16, CYAN_16, LIGHT_GRAY_16
};

void console_set_color(uint8_t color) {
    set_text_color(color);
    terminal_set_color(color);
}

void console_reset_color(void) {
    set_text_color(CONSOLE_TEXT_DEFAULT);
    terminal_set_color(CONSOLE_GUI_DEFAULT);
}

// Apply a single ANSI "Select Graphic Rendition" parameter.
static void apply_sgr(int n) {
    if (n == 0 || n == 39) {
        console_reset_color();
    } else if (n >= 30 && n <= 37) {
        console_set_color(ansi_fg_to_vga[n - 30]);
    } else if (n >= 90 && n <= 97) {  // bright foreground
        console_set_color((uint8_t)(ansi_fg_to_vga[n - 90] + 8));
    }
    // TODO: add other codes (bold, background, ...)
}

// Minimal ANSI escape parser state for "ESC [ <params> m" sequences.
static int ansi_state = 0; // 0 = normal, 1 = saw ESC, 2 = inside CSI
static int ansi_param = 0;

static void console_emit(char c) {
    if (g_text_mode) {
        putc(c);
    } else {
        terminal_putc(c);
    }
}

void console_putc(char c) {
    switch (ansi_state) {
        case 0:
            if (c == 0x1B) ansi_state = 1;   // ESC
            else console_emit(c);
            break;
        case 1:
            if (c == '[') { ansi_state = 2; ansi_param = 0; }
            else { ansi_state = 0; console_emit(c); }
            break;
        case 2:
            if (c >= '0' && c <= '9') {
                ansi_param = ansi_param * 10 + (c - '0');
            } else if (c == ';') {
                apply_sgr(ansi_param);
                ansi_param = 0;
            } else if (c == 'm') {
                apply_sgr(ansi_param);
                ansi_state = 0;
            } else {
                ansi_state = 0; // unsupported sequence so abort
            }
            break;
    }
}

void console_printf(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (const char* p = buffer; *p; ++p) {
        console_putc(*p);
    }
}

void command_executor_init(void) {
    command_count = 0;
    register_basics();
    register_file_commands();
}

void register_command(const char* name, command_func_t func, const char* help) {
    if (command_count < MAX_COMMANDS) {
        command_list[command_count].name = name;
        command_list[command_count].func = func;
        command_list[command_count].help = help;
        command_count++;
    } else {
        console_printf("Error: Command table full!\r\n");
    }
}

// Tokenises a command str
static int tokenize(char* input, char** argv, int max_tokens) {
    int argc = 0;
    char* token = input;
    while (*token && argc < max_tokens) {
        while (*token == ' ') token++;
        if (!*token)
            break;
        argv[argc++] = token;
        while (*token && *token != ' ') token++;
        if (*token) {
            *token = '\0';  // Null-terminate token
            token++;
        }
    }
    return argc;
}

// Execute a command given the input str
void execute_command(const char* input) {
    char buffer[256];
    strncpy(buffer, input, 255);
    buffer[255] = '\0';

    char* argv[MAX_TOKENS];
    int argc = tokenize(buffer, argv, MAX_TOKENS);
    if (argc == 0) {
        return;
    }
    for (int i = 0; i < command_count; i++) {
        if (strcmp(argv[0], command_list[i].name) == 0) {
            command_list[i].func(argc, argv);
            console_reset_color();
            return;
        }
    }
    console_printf("\033[91mUnrecognised command: %s\033[0m\r\n", argv[0]);
}

// Print the list of available commands
void print_command_list(void) {
    console_printf("\033[96mAvailable commands:\033[0m\r\n");
    for (int i = 0; i < command_count; i++) {
        console_printf("  \033[93m%s\033[0m: %s\r\n", command_list[i].name, command_list[i].help);
    }
}
