#include "terminal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "../../drivers/display.h"
#include "../../drivers/vga.h"
#include "../ui/graphics_context.h"
#include "../ui/window_manager.h"
#include "../../drivers/font.h"
#include "../../core/cmd_executor.h"
#include "../string.h"

#define TERMINAL_WIDTH   200
#define TERMINAL_HEIGHT  200

#define TERMINAL_BUFFER_SIZE 1024
#define TERMINAL_INPUT_MAX   128

#define MAX_CHARS_PER_LINE 24
#define FONT_HEIGHT 8

static Window *terminal_window = NULL;
static Canvas *terminal_canvas = NULL;

static char terminal_buffer[TERMINAL_BUFFER_SIZE];
static int terminal_buffer_index = 0;

static char input_line[TERMINAL_INPUT_MAX];
static int input_line_index = 0;

static char terminal_wrapped_cache[TERMINAL_BUFFER_SIZE * 2];
static int terminal_wrapped_cache_valid = 0;
static int terminal_visible_offset = 0;

/* Append a character to the terminal output buffer */
void terminal_putc(char c) {
    if (terminal_buffer_index < TERMINAL_BUFFER_SIZE - 1) {
        terminal_buffer[terminal_buffer_index++] = c;
        terminal_buffer[terminal_buffer_index] = '\0';
        terminal_wrapped_cache_valid = 0;
    }
}

/* Append text to the terminal output buffer */
void terminal_print(const char* text) {
    int len = strlen(text);
    if (terminal_buffer_index + len < TERMINAL_BUFFER_SIZE) {
        memcpy(terminal_buffer + terminal_buffer_index, text, len);
        terminal_buffer_index += len;
        terminal_buffer[terminal_buffer_index] = '\0';
    } else {
        int available = TERMINAL_BUFFER_SIZE - terminal_buffer_index - 1;
        if (available > 0) {
            memcpy(terminal_buffer + terminal_buffer_index, text, available);
            terminal_buffer_index += available;
            terminal_buffer[terminal_buffer_index] = '\0';
        }
    }
    terminal_wrapped_cache_valid = 0;
}

/* Clear the terminal output buffer */
void clear_terminal_buffer(void) {
    terminal_buffer_index = 0;
    terminal_buffer[0] = '\0';
    terminal_wrapped_cache_valid = 0;
}

/* Wrap text by inserting newlines when a line exceeds max_chars_per_line */
static void wrap_text(const char* input, char* output, size_t out_size, int max_chars_per_line) {
    int count = 0;
    size_t out_index = 0;
    for (size_t i = 0; input[i] != '\0' && out_index < out_size - 1; i++) {
        char c = input[i];
        output[out_index++] = c;
        if (c == '\n') {
            count = 0;
        } else {
            count++;
            if (count >= max_chars_per_line) {
                if (input[i+1] != '\n' && out_index < out_size - 1) {
                    output[out_index++] = '\n';
                }
                count = 0;
            }
        }
    }
    output[out_index] = '\0';
}

/* Update visible offset:
*/
static void update_visible_offset(const char* wrapped, int max_lines) {
    int total_lines = 0;
    for (size_t i = 0; wrapped[i] != '\0'; i++) {
        if (wrapped[i] == '\n')
            total_lines++;
    }
    if (total_lines < max_lines) {
        terminal_visible_offset = 0;
        return;
    }
    int lines_to_skip = total_lines - max_lines;
    int count = 0;
    size_t i = 0;
    while (wrapped[i] != '\0') {
        if (wrapped[i] == '\n') {
            count++;
            if (count == lines_to_skip) {
                terminal_visible_offset = i + 1;
                return;
            }
        }
        i++;
    }
    terminal_visible_offset = 0;
}

/* Terminal callback: Create (or bring to front) the terminal window */
void terminal_callback(UIWidget *widget, Window *parent) {
    if (terminal_window) {
        bring_to_front(terminal_window);
        return;
    }
    int win_x = (g_SCREEN_WIDTH - TERMINAL_WIDTH) / 2;
    int win_y = (g_SCREEN_HEIGHT - TERMINAL_HEIGHT) / 2;
    terminal_window = create_window(win_x, win_y, TERMINAL_WIDTH, TERMINAL_HEIGHT, LIGHT_GRAY_16, "Terminal");
    if (!terminal_window) {
        console_printf("> Failed to create terminal window\r\n");
        return;
    }
    int margin = 5;
    terminal_canvas = create_window_canvas(terminal_window, margin, TITLE_BAR_HEIGHT, TERMINAL_WIDTH - 2 * margin, TERMINAL_HEIGHT - margin - TITLE_BAR_HEIGHT);
    if (!terminal_canvas) {
        console_printf("> Failed to create terminal canvas\r\n");
        return;
    }
    clear_terminal_buffer();
    terminal_print("root $ ");
}

/* Update the terminal display
*/
void update_terminal(void) {
    if (!terminal_window || !terminal_canvas) return;

    GraphicsContext ctx = create_context(terminal_canvas->buffer,
                                         terminal_canvas->width,
                                         terminal_canvas->height,
                                         g_BYTES_PER_PIXEL);
    // Clear the canvas (fill with BLACK).
    for (int i = 0; i < ctx.width * ctx.height; i++) {
        ((uint8_t *)ctx.buffer)[i] = BLACK_16;
    }

    if (!terminal_wrapped_cache_valid) {
        wrap_text(terminal_buffer, terminal_wrapped_cache, sizeof(terminal_wrapped_cache), MAX_CHARS_PER_LINE);
        int max_lines = terminal_canvas->height / FONT_HEIGHT;
        update_visible_offset(terminal_wrapped_cache, max_lines);
        terminal_wrapped_cache_valid = 1;
    }

    // Draw only the visible portion from the wrapped cache.
    ctx_draw_string_styled(&ctx, 0, 0, terminal_wrapped_cache + terminal_visible_offset, WHITE_16, FontStyle_8x8_Basic);
}

/* Handle a key press in terminal mode.
*/
void terminal_handle_key(char c) {
    if (c == '\n') {
        input_line[input_line_index] = '\0';
        terminal_print("\r\n");
        if (input_line_index > 0) {
            execute_command(input_line);
        }
        terminal_print("root $ ");
        input_line_index = 0;
    } else if (c == '\b') {
        if (input_line_index > 0) {
            input_line_index--;
            if (terminal_buffer_index > 0) {
                terminal_buffer_index--;
                terminal_buffer[terminal_buffer_index] = '\0';
            }
        }
    } else {
        if (input_line_index < TERMINAL_INPUT_MAX - 1) {
            input_line[input_line_index++] = c;
            char str[2] = { c, '\0' };
            terminal_print(str);
        }
    }
    terminal_wrapped_cache_valid = 0;
}

/* Closes the terminal window. */
void terminal_close(void) {
    if (terminal_window) {
        destroy_window(terminal_window);
        terminal_window = NULL;
        terminal_canvas = NULL;
    }
}

void terminal_close_callback(UIWidget *widget, Window *parent) {
    terminal_close();
}
