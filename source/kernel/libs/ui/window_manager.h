#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ui_widget.h"

#define MAX_WINDOWS 10
#define TITLE_BAR_HEIGHT 16

// Forward declaration for UIWidget.
typedef struct UIWidget UIWidget;

// New Canvas type for custom drawing.
typedef struct Canvas {
    uint8_t *buffer;  // Pixel buffer for the canvas.
    int x, y;         // Position relative to the window.
    int width, height;
} Canvas;

typedef struct Window {
    int x, y, width, height;
    uint32_t bg_color;
    char title[128];
    bool is_active;
    bool has_title_bar;
    UIWidget* widgets[10];
    int widget_count;
    Canvas *canvas;   // Optional canvas for custom drawing.
} Window;

int get_window_count(void);
Window* get_window(int index);

void init_taskbar(void);
Window* create_window(int x, int y, int width, int height, uint32_t bg_color, const char *title);
void destroy_window(Window *win);
void move_window(Window *win, int new_x, int new_y);
void resize_window(Window *win, int new_width, int new_height);
void bring_to_front(Window *win);
void draw_all_windows(void);
void update_taskbar(void);

// New functions for canvas management.
Canvas* create_window_canvas(Window *win, int x, int y, int width, int height);
void clear_window_canvas(Window *win, uint32_t color);
void draw_window_canvas(Window *win);
