#include <stddef.h>
#include <stdint.h>
#include "../ui/ui_widget.h"
#include "../ui/window_manager.h"
#include "../drivers/display.h"
#include "../core/memory.h"
#include "../core/stdio.h"
#include "../libs/string.h"
#include "../../drivers/vga.h"   // Use only the 16-color palette
#include "../ui/graphics_context.h"    // Context drawing functions

// For a 320x200 screen, use a small window.
#define CPU_VIS_WIDTH   200
#define CPU_VIS_HEIGHT  120

static const int BLOCK_HEIGHT      = 10;
static const int MAX_STACK_BLOCKS  = 10;

// Process structure.
typedef struct {
    char     name[16];
    int      blocks;
    uint32_t color;
} FakeProcess;

#define MAX_PROCESSES  6
static FakeProcess g_processes[MAX_PROCESSES];
static int g_processCount = 0;   // Number of processes on the stack.

// Simple LCG random generator.
static uint32_t g_rand_seed = 123456789;
static uint32_t lcg_rand(void) {
    g_rand_seed = (1103515245U * g_rand_seed + 12345U);
    return g_rand_seed;
}
static int rand_in_range(int min_val, int max_val) {
    if (min_val > max_val) return min_val;
    uint32_t r = lcg_rand();
    return (int)(r % (max_val - min_val + 1)) + min_val;
}

// Global visualiser window and its canvas.
static Window *cpu_vis_window = NULL;
static Canvas *cpu_vis_canvas = NULL;

// Sample process names.
static const char *g_name_options[] = {
    "Terminal", "CPU Sched", "Paging", "UI Render"
};
static const int g_name_options_count = sizeof(g_name_options) / sizeof(g_name_options[0]);

// Process color options (using only 16-color macros).
static uint32_t g_color_options[] = {
    BROWN_16,
    LIGHT_BLUE_16,
    LIGHT_GREEN_16,
    LIGHT_CYAN_16,
    LIGHT_RED_16,
    LIGHT_MAGENTA_16,
    YELLOW_16,
    WHITE_16
};
static const int g_color_options_count = sizeof(g_color_options) / sizeof(g_color_options[0]);

// Stack management functions
static int total_used_blocks(void) {
    int total = 0;
    for (int i = 0; i < g_processCount; i++) {
        total += g_processes[i].blocks;
    }
    return total;
}

static void push_process(void) {
    if (g_processCount >= MAX_PROCESSES) return;
    int needed = rand_in_range(1, 3);
    if (total_used_blocks() + needed > MAX_STACK_BLOCKS) return;

    FakeProcess proc;
    const char *name_choice = g_name_options[rand_in_range(0, g_name_options_count - 1)];
    strncpy(proc.name, name_choice, sizeof(proc.name) - 1);
    proc.name[sizeof(proc.name) - 1] = '\0';
    proc.blocks = needed;
    proc.color = g_color_options[rand_in_range(0, g_color_options_count - 1)];

    g_processes[g_processCount++] = proc;
}

static void pop_process(void) {
    if (g_processCount > 0)
        g_processCount--;
}

// Drawing the stack using the graphics context
static void draw_stack(void) {
    if (!cpu_vis_canvas) return;

    GraphicsContext ctx = create_context(cpu_vis_canvas->buffer,
                                         cpu_vis_canvas->width,
                                         cpu_vis_canvas->height,
                                         g_BYTES_PER_PIXEL);

    for (int i = 0; i < ctx.width * ctx.height; i++) {
        ((uint8_t *)ctx.buffer)[i] = DARK_GRAY_16;
    }

    const char *title = "CPU Stack";
    int title_w = (int)(8 * strlen(title)); // assuming 8px per character
    int title_x = (ctx.width - title_w) / 2;
    ctx_draw_string(&ctx, title_x, 2, title, WHITE_16);

    // Define the stack drawing area.
    int spacing = 1;      // space between blocks
    int top_of_stack_y = ctx.height - 2;  // bottom margin

    // Draw each process from top (last pushed) downward.
    for (int i = g_processCount - 1; i >= 0; i--) {
        FakeProcess *p = &g_processes[i];
        int block_pixel_height = p->blocks * BLOCK_HEIGHT;
        int x = 5;
        int y = top_of_stack_y - block_pixel_height;
        int w = ctx.width - 10;  // margins of 5 pixels on each side

        // Draw the process block.
        ctx_draw_rect(&ctx, x, y, w, block_pixel_height - spacing, p->color);

        // Label the block if there's enough height.
        if (block_pixel_height >= 10) {
            int name_w = (int)(8 * strlen(p->name));
            int name_x = x + (w - name_w) / 2;
            int name_y = y + (block_pixel_height / 2) - 4;
            ctx_draw_string(&ctx, name_x, name_y, p->name, BLACK_16);
        }

        top_of_stack_y = y - spacing;
    }

    // Draw white "stack walls" around the stack region.
    ctx_draw_line(&ctx, 5, 0, 5, ctx.height - 1, WHITE_16);       // left wall
    ctx_draw_line(&ctx, ctx.width - 5, 0, ctx.width - 5, ctx.height - 1, WHITE_16); // right wall
    ctx_draw_line(&ctx, 5, ctx.height - 1, ctx.width - 5, ctx.height - 1, WHITE_16); // bottom wall
}

// Public functions
void cpu_visualiser_callback(UIWidget *widget, Window *parent) {
    if (cpu_vis_window) {
        bring_to_front(cpu_vis_window);
        return;
    }

    int win_x = (g_SCREEN_WIDTH - CPU_VIS_WIDTH) / 2;
    int win_y = (g_SCREEN_HEIGHT - CPU_VIS_HEIGHT) / 2;

    // Create the visualiser window with a LIGHT_GRAY_16 background.
    cpu_vis_window = create_window(win_x, win_y, CPU_VIS_WIDTH, CPU_VIS_HEIGHT, LIGHT_GRAY_16, "CPU Visualiser");
    if (!cpu_vis_window) {
        printf("[CPU VIS] Failed to create CPU Visualiser window\r\n");
        return;
    }

    int margin = 5;
    // Create a canvas for this window using the new canvas system.
    cpu_vis_canvas = create_window_canvas(cpu_vis_window, margin, margin, CPU_VIS_WIDTH - 2 * margin, CPU_VIS_HEIGHT - 2 * margin);
    if (!cpu_vis_canvas) {
        printf("[CPU VIS] Failed to create canvas for visualiser\r\n");
        return;
    }

    printf("[CPU VIS] CPU Visualiser started (dynamic stack)\r\n");
}

void update_cpu_visualiser(void) {
    if (!cpu_vis_window || !cpu_vis_canvas) return;

    int r = rand_in_range(0, 99);
    if (g_processCount > 0 && r < 3) {
        pop_process();
    } else {
        r = rand_in_range(0, 99);
        if (r < 3) {
            push_process();
        }
    }

    draw_stack();
}

void close_cpu_visualiser(void) {
    if (cpu_vis_window) {
        destroy_window(cpu_vis_window);
        cpu_vis_window = NULL;
        cpu_vis_canvas = NULL;
        g_processCount = 0;
    }
}
