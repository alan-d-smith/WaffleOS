#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "window_manager.h"
#include "graphics_context.h"
#include "../../drivers/display.h"
#include "../../core/memory.h"
#include "../cursor.h"
#include "../string.h"

#include "../apps/terminal.h"
#include "../apps/cpu_visualiser.h"
#include "../apps/scheduling_visualiser.h"
#include "../apps/paging_visualiser.h"

static void dummy_app_callback(UIWidget *widget, Window *parent) {
    return;
}

static void default_close_callback(UIWidget *widget, Window *parent) {
    destroy_window(parent);
}

static void start_button_callback(UIWidget *widget, Window *parent) {
    int num_buttons = 5;
    int button_width = 120;
    int button_height = 20;
    int padding = 5;

    int new_window_width = button_width + 2*padding;
    int new_window_height = 130;
    int new_window_x = 0;
    int new_window_y = g_SCREEN_HEIGHT - 20 - new_window_height;

    // Create the new window with a light gray background and "Start Menu" title.
    Window *start_menu = create_window(new_window_x, new_window_y, new_window_width, new_window_height, LIGHT_GRAY_16, "Start Menu");
    if (!start_menu) {
        return;
    }
    start_menu->has_title_bar = false; // make not draggable

    create_button(start_menu, padding, padding+8, button_width, button_height, "Terminal", BLUE_16, WHITE_16, terminal_callback);
    create_button(start_menu, padding, padding*2 + button_height+8, button_width, button_height, "CPU Visualiser", BLUE_16, WHITE_16, cpu_visualiser_callback);
    create_button(start_menu, padding, padding*3 + button_height*2+8, button_width, button_height, "Scheduling", BLUE_16, WHITE_16, scheduling_visualiser_callback);
    create_button(start_menu, padding, padding*4 + button_height*3+8, button_width, button_height, "Paging", BLUE_16, WHITE_16, paging_visualiser_callback);
}

static Window* windows[MAX_WINDOWS];
static int window_count = 0;
Window *g_taskbar_window = NULL;  // Global pointer for taskbar.

int get_window_count(void) { return window_count; }
Window* get_window(int index) { return windows[index]; }

void init_taskbar(void) {
    g_taskbar_window = create_window(0, 180, g_SCREEN_WIDTH, 20, BLUE_16, "Taskbar");
    if (g_taskbar_window) {
        g_taskbar_window->has_title_bar = false; // Remove title bar for the taskbar.
        create_button(g_taskbar_window, 5, 2, 50, 16, "Start", BLUE_16, WHITE_16, start_button_callback);
    }
}

Window* create_window(int x, int y, int width, int height, uint32_t bg_color, const char *title) {
    if (window_count >= MAX_WINDOWS) {
        return NULL;  // No more windows available.
    }
    Window *win = (Window*)malloc(sizeof(Window));
    if (!win)
        return NULL;

    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->bg_color = bg_color;
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = '\0';
    win->is_active = true;
    win->has_title_bar = true;    // Default window has a title bar.
    win->widget_count = 0;
    win->canvas = NULL;           // No canvas by default.

    windows[window_count++] = win;

    if (win->has_title_bar) {
        int btn_size = TITLE_BAR_HEIGHT - 4;
        int btn_x = win->width - btn_size - 4;
        int btn_y = 2;

        // Use a specialized close callback for the Terminal window.
        WidgetCallback close_cb = default_close_callback;
        if (strcmp(win->title, "Terminal") == 0) {
            close_cb = terminal_close_callback;
        }

        UIWidget *btn = create_button(win, btn_x, btn_y, btn_size, btn_size, "x", RED_16, WHITE_16, close_cb);
        if (btn) {
    		btn->text_margin_y = 1;
    		btn->text_margin_y = -2;
		}
    }

    return win;
}

void destroy_window(Window *win) {
    if (!win)
        return;
    int index = -1;
    for (int i = 0; i < window_count; i++) {
        if (windows[i] == win) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return;

    // Free canvas if it exists.
    if (win->canvas) {
        if (win->canvas->buffer)
            free(win->canvas->buffer);
        free(win->canvas);
    }
    free(win);
    for (int i = index; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    window_count--;
}

void move_window(Window *win, int new_x, int new_y) {
    if (win) {
        win->x = new_x;
        win->y = new_y;
    }
}

void resize_window(Window *win, int new_width, int new_height) {
    if (win) {
        win->width = new_width;
        win->height = new_height;
    }
}

void bring_to_front(Window *win) {
    if (!win)
        return;
    int index = -1;
    for (int i = 0; i < window_count; i++) {
        if (windows[i] == win) {
            index = i;
            break;
        }
    }
    if (index == -1 || index == window_count - 1)
        return;

    for (int i = index; i < window_count - 1; i++) {
        windows[i] = windows[i + 1];
    }
    windows[window_count - 1] = win;
}

Canvas* create_window_canvas(Window *win, int x, int y, int width, int height) {
    if (!win) return NULL;
    Canvas *c = (Canvas*)malloc(sizeof(Canvas));
    if (!c) return NULL;
    c->x = x;
    c->y = y;
    c->width = width;
    c->height = height;
    c->buffer = (uint8_t*)malloc(width * height * g_BYTES_PER_PIXEL);
    if (!c->buffer) {
        free(c);
        return NULL;
    }
    // Initialize canvas with a default color (optional).
    memset(c->buffer, 0, width * height * g_BYTES_PER_PIXEL);
    win->canvas = c;
    return c;
}

void clear_window_canvas(Window *win, uint32_t color) {
    if (!win || !win->canvas) return;
    Canvas *c = win->canvas;

    if (g_BYTES_PER_PIXEL == 1) {
        memset(c->buffer, (uint8_t)color, c->width * c->height);
    }
}

void draw_window_canvas(Window *win) {
    if (!win || !win->canvas) return;
    Canvas *c = win->canvas;
    // draw the canvas buffer onto the back buffer,
    // mapping canvas (i,j) to window pixel (win->x + c->x + i, win->y + c->y + j).
    for (int j = 0; j < c->height; j++) {
        for (int i = 0; i < c->width; i++) {
            int offset;
            uint32_t color = 0;
            if (g_BYTES_PER_PIXEL == 1) {
                offset = j * c->width + i;
                color = c->buffer[offset];
            } else if (g_BYTES_PER_PIXEL == 2) {
                offset = j * c->width + i;
                color = ((uint16_t*)c->buffer)[offset];
            } else if (g_BYTES_PER_PIXEL == 4) {
                offset = j * c->width + i;
                color = ((uint32_t*)c->buffer)[offset];
            }
            put_pixel(win->x + c->x + i, win->y + c->y + j, color);
        }
    }
}

static void draw_window_content(Window *win) {
    draw_rect(win->x, win->y, win->width, win->height, win->bg_color);

    for (int i = 0; i < win->width; i++) {
        put_pixel(win->x + i, win->y, WHITE_16);
        put_pixel(win->x + i, win->y + win->height - 1, WHITE_16);
    }
    for (int j = 0; j < win->height; j++) {
        put_pixel(win->x, win->y + j, WHITE_16);
        put_pixel(win->x + win->width - 1, win->y + j, WHITE_16);
    }

    if (win->has_title_bar) {
        draw_rect(win->x, win->y, win->width, TITLE_BAR_HEIGHT, BLUE_16);
        draw_string_styled(win->x + 4, win->y + 4, win->title, WHITE_16, FontStyle_8x8_Basic);
    }

    if (win->canvas) {
        draw_window_canvas(win);
    }

    for (int i = 0; i < win->widget_count; i++) {
        if (win->widgets[i]) {
            draw_widget(win->widgets[i], win);
        }
    }
}

void draw_all_windows(void) {
    for (int i = 0; i < window_count; i++) {
        if (windows[i]->is_active) {
            draw_window_content(windows[i]);
        }
    }
}

void update_taskbar(void) {
    // TODO: Implement
}
