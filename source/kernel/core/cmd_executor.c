#include "cmd_executor.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "../libs/string.h"
#include "stdio.h"
#include "memory.h"
#include "../drivers/display.h"
#include "../libs/apps/terminal.h"

#include "base_cmds/basic.h"
#include "base_cmds/file_cmds.h"

#define MAX_COMMANDS 32
#define MAX_TOKENS   16

static Command command_list[MAX_COMMANDS];
static int command_count = 0;

void console_putc(char c) {
    if (g_text_mode) {
        putc(c);
    } else {
        terminal_putc(c); // add to terminal buffer
    }
}

void console_printf(const char* fmt, ...) { // TODO: make more efficient
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (g_text_mode) {
        printf("%s", buffer);
    } else {
        terminal_print(buffer);
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
            return;
        }
    }
    console_printf("Unrecognised command: %s\r\n", argv[0]);
}

// Print the list of available commands
void print_command_list(void) {
    console_printf("Available commands:\r\n");
    for (int i = 0; i < command_count; i++) {
        console_printf("  %s: %s\r\n", command_list[i].name, command_list[i].help);
    }
}
