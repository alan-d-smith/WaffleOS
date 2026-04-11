#include "gui.h"
#include "../drivers/display.h"
#include "../drivers/mouse.h"
#include "icons.h"
#include "cursor.h"
#include "../timers/timer.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "../core/memory.h"

#include "ui/window_manager.h"
#include "ui/events.h"

#include "apps/cpu_visualiser.h"
#include "apps/scheduling_visualiser.h"
#include "apps/paging_visualiser.h"
#include "apps/terminal.h"

static uint8_t *back_buffer = NULL;
static uint8_t *overlay_buffer = NULL;
static uint8_t previous_buttons = 0;

void update_mouse_click(void) {
    // If the left button was pressed in the previous frame and is now released:
    if ((previous_buttons & 0x01) && !(mouse_state.buttons & 0x01)) {
        process_mouse_click(mouse_state.x, mouse_state.y);
    }
    previous_buttons = mouse_state.buttons;
}

// GUI rendering Functions ---------------------------------------------------------------------------------------------

void init_gui(void) {
    back_buffer = (uint8_t *)malloc(g_SCREEN_WIDTH * g_SCREEN_HEIGHT * g_BYTES_PER_PIXEL);
    if (!back_buffer) {
        return;
    }

    init_taskbar();
    terminal_callback(NULL, NULL); // launch terminal on startup
}

static void clear_back_buffer(uint32_t color) {
    if (g_BYTES_PER_PIXEL == 1) {
        memset(back_buffer, (uint8_t)color, g_SCREEN_WIDTH * g_SCREEN_HEIGHT);
    }
}

static void flip_buffer(void) {
    memcpy((void*)display_get_framebuffer(), back_buffer, g_SCREEN_WIDTH * g_SCREEN_HEIGHT * g_BYTES_PER_PIXEL);
}

// Draw the GUI to the back buffer.
static void draw_gui_to_buffer(void) {
    set_drawing_target(back_buffer);

    draw_rect(0, 0, g_SCREEN_WIDTH - 1, g_SCREEN_HEIGHT - 1, LIGHT_BLUE_16); // Background
    draw_rect(0, 0, g_SCREEN_WIDTH - 1, 20, BLUE_16);                         // Top bar
    draw_string(3, 3, "Welcome to WaffleOS!", YELLOW_16);

    update_drag();
    update_mouse_click();

    draw_all_windows();
    draw_cursor();

    // Reset drawing target to the default framebuffer
    set_drawing_target(NULL);
}

// Main render function that uses double buffering.
void render_gui(void) {
    draw_gui_to_buffer();

    update_cpu_visualiser();
    update_scheduling_visualiser();
    update_paging_visualiser();
    update_terminal();

    flip_buffer();    // Blit back buffer to actual framebuffer
}
