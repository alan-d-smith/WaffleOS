#include "ui_widget.h"
#include "../../drivers/display.h"
#include "../../core/memory.h"
#include <stddef.h>
#include "../string.h"

UIWidget* create_button(Window *win, int x, int y, int width, int height, const char *text,
                          uint32_t bg_color, uint32_t text_color, WidgetCallback callback) {
    if (!win || win->widget_count >= MAX_WIDGETS_PER_WINDOW)
        return NULL;
    UIWidget *widget = (UIWidget*)malloc(sizeof(UIWidget));
    if (!widget)
        return NULL;
    widget->type = UI_BUTTON;
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;
    strncpy(widget->text, text, sizeof(widget->text) - 1);
    widget->text[sizeof(widget->text) - 1] = '\0';
    widget->bg_color = bg_color;
    widget->text_color = text_color;
    widget->on_click = callback;
    widget->user_data = NULL;
    widget->canvas_buffer = NULL;
    widget->text_margin_x = 2;  // Default margin
    widget->text_margin_y = 0;  // Default margin
    win->widgets[win->widget_count++] = widget;
    return widget;
}

UIWidget* create_button_with_data(Window *win, int x, int y, int width, int height, const char *text,
                                  uint32_t bg_color, uint32_t text_color, WidgetCallback callback, void *user_data) {
    UIWidget *widget = create_button(win, x, y, width, height, text, bg_color, text_color, callback);
    if (widget) {
        widget->user_data = user_data;
    }
    return widget;
}

UIWidget* create_label(Window *win, int x, int y, int width, int height, const char *text, uint32_t text_color) {
    if (!win || win->widget_count >= MAX_WIDGETS_PER_WINDOW)
        return NULL;
    UIWidget *widget = (UIWidget*)malloc(sizeof(UIWidget));
    if (!widget)
        return NULL;
    widget->type = UI_LABEL;
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;
    strncpy(widget->text, text, sizeof(widget->text) - 1);
    widget->text[sizeof(widget->text) - 1] = '\0';
    widget->bg_color = 0;
    widget->text_color = text_color;
    widget->on_click = NULL;
    widget->user_data = NULL;
    widget->canvas_buffer = NULL;
    widget->text_margin_x = 2;  // Default margin
    widget->text_margin_y = 0;  // Default margin
    win->widgets[win->widget_count++] = widget;
    return widget;
}

UIWidget* create_canvas(Window *win, int x, int y, int width, int height) {
    if (!win || win->widget_count >= MAX_WIDGETS_PER_WINDOW)
        return NULL;
    UIWidget *widget = (UIWidget*)malloc(sizeof(UIWidget));
    if (!widget)
        return NULL;
    widget->type = UI_CANVAS;
    widget->x = x;
    widget->y = y;
    widget->width = width;
    widget->height = height;
    widget->text[0] = '\0';
    widget->bg_color = 0;
    widget->text_color = 0;
    widget->on_click = NULL;
    widget->user_data = NULL;
    // Allocate the canvas buffer (assuming 32-bit pixels).
    widget->canvas_buffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if (!widget->canvas_buffer) {
        free(widget);
        return NULL;
    }
    // Initialize the canvas (fill with BLACK_16, defined in display.h).
    for (int i = 0; i < width * height; i++) {
        widget->canvas_buffer[i] = BLACK_16;
    }
    win->widgets[win->widget_count++] = widget;
    return widget;
}

void canvas_clear(UIWidget *canvas, uint32_t color) {
    if (!canvas || canvas->type != UI_CANVAS || !canvas->canvas_buffer)
        return;
    for (int i = 0; i < canvas->width * canvas->height; i++) {
        canvas->canvas_buffer[i] = color;
    }
}

void canvas_draw_pixel(UIWidget *canvas, int x, int y, uint32_t color) {
    if (!canvas || canvas->type != UI_CANVAS || !canvas->canvas_buffer)
        return;
    if (x < 0 || y < 0 || x >= canvas->width || y >= canvas->height)
        return;
    canvas->canvas_buffer[y * canvas->width + x] = color;
}

void draw_widget(UIWidget *widget, Window *win) {
    if (!widget || !win)
        return;
    int abs_x = win->x + widget->x;
    int abs_y = win->y + widget->y;
    switch (widget->type) {
        case UI_BUTTON:
            // Draw button background.
                draw_rect(abs_x, abs_y, widget->width, widget->height, widget->bg_color);
        // Draw button border.
        for (int i = 0; i < widget->width; i++) {
            put_pixel(abs_x + i, abs_y, WHITE_16);
            put_pixel(abs_x + i, abs_y + widget->height - 1, WHITE_16);
        }
        for (int j = 0; j < widget->height; j++) {
            put_pixel(abs_x, abs_y + j, WHITE_16);
            put_pixel(abs_x + widget->width - 1, abs_y + j, WHITE_16);
        }
        // Draw button text using the text margin.
        draw_string(abs_x + widget->text_margin_x, abs_y + widget->text_margin_y, widget->text, widget->text_color);
        break;
        case UI_LABEL:
            // Draw label text.
                draw_string(abs_x + widget->text_margin_x, abs_y + widget->text_margin_y, widget->text, widget->text_color);
        break;
        case UI_CANVAS:
            // For the canvas, iterate over its pixel buffer.
                if (widget->canvas_buffer) {
                    for (int j = 0; j < widget->height; j++) {
                        for (int i = 0; i < widget->width; i++) {
                            uint32_t color = widget->canvas_buffer[j * widget->width + i];
                            put_pixel(abs_x + i, abs_y + j, color);
                        }
                    }
                }
        break;
        default:
            break;
    }
}
