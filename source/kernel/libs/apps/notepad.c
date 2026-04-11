#include "notepad.h"
// notepad.c
#include "../../drivers/keyboard.h"
#include "../../drivers/display.h"
#include <stdio.h>
#include "../string.h"
#include "../../core/memory.h"

void launch_notepad(void) {
    int win_width = 400;
    int win_height = 300;
    int win_x = (g_SCREEN_WIDTH - win_width) / 2;
    int win_y = (g_SCREEN_HEIGHT - win_height) / 2;

    Window *notepad_win = create_window(win_x, win_y, win_width, win_height, WHITE_16, "Notepad");
    if (!notepad_win) {
        return;
    }

    int canvas_x = 5;
    int canvas_y = 20;
    int canvas_width = win_width - 10;
    int canvas_height = win_height - 25;
    UIWidget *canvas = create_canvas(notepad_win, canvas_x, canvas_y, canvas_width, canvas_height);
    if (!canvas) {
        printf("[NOTEPAD] Failed to create canvas widget\r\n");
        return;
    }

    // Initialize a buffer to hold the text entered by the user.
    char text_buffer[1024];
    memset(text_buffer, 0, sizeof(text_buffer));
    int text_index = 0;

    // Clear the canvas initially (fill with white) and draw a simple instruction.
    canvas_clear(canvas, WHITE_16);
    draw_string(canvas->x + 2, canvas->y + 2, "Type here. Press ENTER for a new line.", BLACK_16);

    // Main loop: continuously read a line of input and update the canvas.
    while (1) {
        // Blocking call: wait for the user to type a line.
        char *line = read_line();
        int len = strlen(line);

        // Append the new line to our text buffer if there's room.
        if (text_index + len + 2 < sizeof(text_buffer)) {
            memcpy(&text_buffer[text_index], line, len);
            text_index += len;
            text_buffer[text_index++] = '\n';
            text_buffer[text_index] = '\0';
        } else {
            // Text buffer is full; you may wish to add scrolling or limit input.
        }

        // Clear the canvas before redrawing the updated text.
        canvas_clear(canvas, WHITE_16);

        // Draw each line from the text buffer onto the canvas.
        int line_y = canvas->y + 2;
        char *curr = text_buffer;
        char line_buf[128];
        while (*curr) {
            int i = 0;
            // Copy characters until a newline or end of string.
            while (*curr && *curr != '\n' && i < sizeof(line_buf) - 1) {
                line_buf[i++] = *curr;
                curr++;
            }
            line_buf[i] = '\0';
            if (*curr == '\n') {
                curr++;  // Skip the newline character.
            }
            draw_string(canvas->x + 2, line_y, line_buf, BLACK_16);
            line_y += 16;  // Advance to the next line (assuming 16-pixel line height).
            if (line_y > canvas->y + canvas->height - 16)
                break;  // Stop if we run out of vertical space.
        }
    }
}

void notepad_entry_callback(UIWidget *widget, Window *parent) {
    launch_notepad();
}
