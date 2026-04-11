#include "events.h"
#include "../../drivers/mouse.h"           // For the mouse_state struct.
#include "../../drivers/display.h"           // For drawing pixels
#include "window_manager.h"  // For get_window_count() and get_window().
#include "ui_widget.h"        // For UIWidget definition.
#include <stddef.h>

// Global variables to manage drag-and-drop state:
static Window *dragged_window = NULL;
static int drag_offset_x = 0;
static int drag_offset_y = 0;

// Process a mouse click event at absolute coordinates (click_x, click_y).
void process_mouse_click(int click_x, int click_y) {
    int count = get_window_count();
    for (int i = count - 1; i >= 0; i--) {
        Window *win = get_window(i);
        if (!win || !win->is_active)
            continue;
        if (click_x >= win->x && click_x < win->x + win->width &&
            click_y >= win->y && click_y < win->y + win->height) {
            // Check each widget in this window.
            for (int j = 0; j < win->widget_count; j++) {
                UIWidget *widget = win->widgets[j];
                if (!widget)
                    continue;
                int abs_x = win->x + widget->x;
                int abs_y = win->y + widget->y;
                if (click_x >= abs_x && click_x < abs_x + widget->width &&
                    click_y >= abs_y && click_y < abs_y + widget->height) {
                    if (widget->on_click) {
                        widget->on_click(widget, win);
                        return; // Process only the topmost widget.
                    }
                }
            }
        }
    }
}

// Called every frame
void update_drag(void) {
    int title_bar_height = 16;

    // Check if left mouse button is pressed (bit0 in buttons).
    if (mouse_state.buttons & 0x01) {
        if (dragged_window == NULL) {
            int count = get_window_count();
            for (int i = count - 1; i >= 0; i--) {
                Window *win = get_window(i);
                if (!win || !win->is_active)
                    continue;
                if (!win->has_title_bar)
                    continue;
                if (mouse_state.x >= win->x && mouse_state.x < win->x + win->width &&
                    mouse_state.y >= win->y && mouse_state.y < win->y + title_bar_height) {
                    // Begin drag
                    dragged_window = win;
                    drag_offset_x = mouse_state.x - win->x;
                    drag_offset_y = mouse_state.y - win->y;
                    break;
                }
            }
        } else {
            // Update dragged window position
            dragged_window->x = mouse_state.x - drag_offset_x;
            dragged_window->y = mouse_state.y - drag_offset_y;
        }
    } else {
        // stop dragging
        dragged_window = NULL;
    }
}
