// paging_visualiser.c
#include "paging_visualiser.h"
#include <stddef.h>
#include <stdint.h>
#include "../ui/ui_widget.h"
#include "../ui/window_manager.h"
#include "../drivers/display.h"
#include "../core/memory.h"
#include "../core/stdio.h"
#include "../libs/string.h"
#include "../../drivers/vga.h"          // For 16-color macros
#include "../ui/graphics_context.h"     // For context-based drawing
#include "../../drivers/font.h"         // For FontStyle if needed

// Use a smaller window: 240x160
#define PAGING_VIS_WIDTH   240
#define PAGING_VIS_HEIGHT  160

static Window *paging_vis_window = NULL;
static Canvas *paging_vis_canvas = NULL;

struct FrameInfo {
    const char* label;
    uint32_t color;
};

static void draw_paging_visualiser(void);

// Callback for the "Memory Visualiser" button.
void paging_visualiser_callback(UIWidget *widget, Window *parent) {
    if (paging_vis_window) {
        bring_to_front(paging_vis_window);
        return;
    }
    // Center the window.
    int win_x = (g_SCREEN_WIDTH - PAGING_VIS_WIDTH) / 2;
    int win_y = (g_SCREEN_HEIGHT - PAGING_VIS_HEIGHT) / 2;

    paging_vis_window = create_window(win_x, win_y, PAGING_VIS_WIDTH, PAGING_VIS_HEIGHT, LIGHT_GRAY_16, "Memory");
    if (!paging_vis_window) {
        printf("[MEM VIS] Failed to create Memory Visualiser window\r\n");
        return;
    }

    int margin = 0;
    paging_vis_canvas = create_window_canvas(paging_vis_window, margin, margin,
                                             PAGING_VIS_WIDTH - 2*margin,
                                             PAGING_VIS_HEIGHT - 2*margin);
    if (!paging_vis_canvas) {
        printf("[MEM VIS] Failed to create canvas for Memory\r\n");
        return;
    }

    printf("[MEM VIS] Memory Visualiser started\r\n");
}

// Main update function
void update_paging_visualiser(void) {
    if (!paging_vis_window || !paging_vis_canvas) return;
    draw_paging_visualiser();
}

void close_memory_visualiser(void) {
    if (paging_vis_window) {
        destroy_window(paging_vis_window);
        paging_vis_window = NULL;
        paging_vis_canvas = NULL;
    }
}

static void draw_process_blocks(GraphicsContext *ctx,
                                int px, int py,
                                int w, int h,
                                uint32_t color,
                                const char *label) {
    int block_h = h / 2;
    for (int i = 0; i < 2; i++) {
        int by = py + i * block_h;
        ctx_draw_rect(ctx, px, by, w, block_h - 1, color);
        // Outline
        ctx_draw_line(ctx, px, by, px+w, by, BLACK_16);
        ctx_draw_line(ctx, px, by+block_h-1, px+w, by+block_h-1, BLACK_16);
        ctx_draw_line(ctx, px, by, px, by+block_h-1, BLACK_16);
        ctx_draw_line(ctx, px+w, by, px+w, by+block_h-1, BLACK_16);
    }
    int label_w = (int)(8 * strlen(label));
    int lx = px + (w - label_w) / 2;
    int ly = py + (block_h / 2) - 4;
    ctx_draw_string(ctx, lx, ly, label, WHITE_16);
}

static int get_frame_center_y(int i, int mem_y, int frame_h, int frame_spacing) {
    return mem_y + i * (frame_h + frame_spacing) + frame_h / 2;
}

static int get_process_block_center_y(int py, int h, int block_index) {
    int block_h = h / 2;
    return py + block_index * block_h + (block_h / 2);
}

static void draw_paging_visualiser(void) {
    GraphicsContext ctx = create_context(paging_vis_canvas->buffer,
                                         paging_vis_canvas->width,
                                         paging_vis_canvas->height,
                                         g_BYTES_PER_PIXEL);

    // 1) Clear the canvas to LIGHT_GRAY_16.
    for (int i = 0; i < ctx.width * ctx.height; i++) {
        ((uint8_t *)ctx.buffer)[i] = LIGHT_GRAY_16;
    }

    // 2) Draw title at the top.
    ctx_draw_string(&ctx, (ctx.width / 2) - 72, 5, "Physical memory", BLACK_16);

    // 3) Draw a vertical column of frames.
    int frame_count = 8;
    int frame_w = 30;
    int frame_h = 20;
    int frame_spacing = 4;
    int mem_x = (ctx.width / 2) - (frame_w / 2);
    int mem_y = 30;

    // Define an example array of frames.
    struct FrameInfo frames[] = {
        {"A", BLUE_16}, {"B", BLUE_16}, {"C", BLUE_16},
        {"B", BLUE_16}, {"A", BLUE_16}, {"C", BLUE_16},
        {"C", BLUE_16}, {"C", BLUE_16}
    };

    for (int i = 0; i < frame_count; i++) {
        int fy = mem_y + i * (frame_h + frame_spacing);
        ctx_draw_rect(&ctx, mem_x, fy, frame_w, frame_h, frames[i].color);
        // Outline
        ctx_draw_line(&ctx, mem_x, fy, mem_x+frame_w, fy, BLACK_16);
        ctx_draw_line(&ctx, mem_x, fy+frame_h, mem_x+frame_w, fy+frame_h, BLACK_16);
        ctx_draw_line(&ctx, mem_x, fy, mem_x, fy+frame_h, BLACK_16);
        ctx_draw_line(&ctx, mem_x+frame_w, fy, mem_x+frame_w, fy+frame_h, BLACK_16);
        // Label
        int label_w = (int)(8 * strlen(frames[i].label));
        int lx = mem_x + (frame_w - label_w) / 2;
        int ly = fy + (frame_h / 2) - 4;
        ctx_draw_string(&ctx, lx, ly, frames[i].label, WHITE_16);
    }

    // 4) Draw three processes: A, B, C.
    int procA_x = mem_x + 60;
    int procA_y = 30;
    int procA_w = 50;
    int procA_h = 40;

    int procB_x = mem_x - 60 - 50;
    int procB_y = 60;
    int procB_w = 50;
    int procB_h = 40;

    int procC_x = mem_x + 60;
    int procC_y = 80;
    int procC_w = 50;
    int procC_h = 40;

    // Draw the processes.
    draw_process_blocks(&ctx, procA_x, procA_y, procA_w, procA_h, GREEN_16, "A");
    draw_process_blocks(&ctx, procB_x, procB_y, procB_w, procB_h, LIGHT_MAGENTA_16, "B");
    draw_process_blocks(&ctx, procC_x, procC_y, procC_w, procC_h, LIGHT_CYAN_16, "C");

    // Draw arrows
    {
        int ay_top = get_process_block_center_y(procA_y, procA_h, 0);
        int ax = procA_x;
        int fx = mem_x + frame_w;
        int fy = get_frame_center_y(0, mem_y, frame_h, frame_spacing);
        ctx_draw_arrow(&ctx, ax, ay_top, fx, fy, GREEN_16);
    }
    {
        int ay_bot = get_process_block_center_y(procA_y, procA_h, 1);
        int ax = procA_x;
        int fx = mem_x + frame_w;
        int fy = get_frame_center_y(4, mem_y, frame_h, frame_spacing);
        ctx_draw_arrow(&ctx, ax, ay_bot, fx, fy, GREEN_16);
    }

    // Process B references frames 1 and 3.
    {
        int by_top = get_process_block_center_y(procB_y, procB_h, 0);
        int bx = procB_x + procB_w;
        int fx = mem_x;
        int fy = get_frame_center_y(1, mem_y, frame_h, frame_spacing);
        ctx_draw_arrow(&ctx, bx, by_top, fx, fy, LIGHT_MAGENTA_16);
    }
    {
        int by_bot = get_process_block_center_y(procB_y, procB_h, 1);
        int bx = procB_x + procB_w;
        int fx = mem_x;
        int fy = get_frame_center_y(3, mem_y, frame_h, frame_spacing);
        ctx_draw_arrow(&ctx, bx, by_bot, fx, fy, LIGHT_MAGENTA_16);
    }

    // Process C references frames 2 and 5.
    {
        int cy_top = get_process_block_center_y(procC_y, procC_h, 0);
        int cx = procC_x;
        int fx = mem_x + frame_w;
        int fy = get_frame_center_y(2, mem_y, frame_h, frame_spacing);
        ctx_draw_arrow(&ctx, cx, cy_top, fx, fy, LIGHT_CYAN_16);
    }
    {
        int cy_bot = get_process_block_center_y(procC_y, procC_h, 1);
        int cx = procC_x;
        int fx = mem_x + frame_w;
        int fy = get_frame_center_y(5, mem_y, frame_h, frame_spacing);
        ctx_draw_arrow(&ctx, cx, cy_bot, fx, fy, LIGHT_CYAN_16);
    }
}
