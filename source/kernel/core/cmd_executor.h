#pragma once

#include <stddef.h>
#include <stdint.h>

typedef void (*command_func_t)(int argc, char **argv);

typedef struct {
    const char* name;     // Command name
    command_func_t func;  // Function to execute
    const char* help;     // Help string for the command
} Command;

void command_executor_init(void);
void register_command(const char* name, command_func_t func, const char* help);
void execute_command(const char* input);
void print_command_list(void);

void console_putc(char c);
void console_printf(const char* fmt, ...);