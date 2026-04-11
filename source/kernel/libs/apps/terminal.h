#pragma once

#include "../ui/ui_widget.h"
#include "../ui/window_manager.h"

void terminal_callback(UIWidget *widget, Window *parent);
void update_terminal(void);
void terminal_close(void);
void terminal_putc(char c);
void terminal_print(const char* text);
void terminal_close_callback(UIWidget *widget, Window *parent);
void terminal_handle_key(char c);
void clear_terminal_buffer(void);
