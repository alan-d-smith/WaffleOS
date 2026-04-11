#pragma once

#include <stdint.h>
#include "window_manager.h"

#define MAX_WIDGETS_PER_WINDOW 10

typedef struct Window Window;

typedef enum {
    UI_BUTTON,
    UI_LABEL,
    UI_CANVAS
} UIWidgetType;

typedef void (*WidgetCallback)(struct UIWidget *widget, Window *parent);

typedef struct UIWidget {
    UIWidgetType type;
    int x, y;                   // Position relative to parent.
    int width, height;
    char text[128];
    uint32_t bg_color;
    uint32_t text_color;
    int text_margin_x;
    int text_margin_y;
    WidgetCallback on_click;
    void *user_data;            // Custom data pointer
    uint32_t *canvas_buffer;    // Drawing within the window
} UIWidget;

UIWidget* create_button(Window *win, int x, int y, int width, int height, const char *text,
                          uint32_t bg_color, uint32_t text_color, WidgetCallback callback);
UIWidget* create_button_with_data(Window *win, int x, int y, int width, int height, const char *text,
                                  uint32_t bg_color, uint32_t text_color, WidgetCallback callback, void *user_data);
UIWidget* create_label(Window *win, int x, int y, int width, int height, const char *text, uint32_t text_color);
UIWidget* create_canvas(Window *win, int x, int y, int width, int height);

void canvas_clear(UIWidget *canvas, uint32_t color);
void canvas_draw_pixel(UIWidget *canvas, int x, int y, uint32_t color);

void draw_widget(UIWidget *widget, Window *win);
