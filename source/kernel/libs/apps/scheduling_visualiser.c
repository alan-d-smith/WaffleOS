// scheduling_visualiser.c
#include "scheduling_visualiser.h"
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

// Set a smaller window: 240x160
#define SCHED_VIS_WIDTH   240
#define SCHED_VIS_HEIGHT  160

static Window *sched_vis_window = NULL;
static Canvas *sched_vis_canvas = NULL;

// A small struct to hold color/label for demonstration blocks.
struct BlockInfo {
    uint32_t color;
    const char* label;
};

// Forward declaration of our main drawing function.
static void draw_scheduling(void);

// --------------------------------------------------------------------------
// Helper function: Draw blocks in a single column.
static void draw_blocks_in_column(GraphicsContext *ctx,
                                  int colIndex,
                                  const struct BlockInfo *blocks,
                                  int blockCount,
                                  const int *col_left,
                                  int col_top,
                                  int col_width,
                                  int col_height)
{
    int x = col_left[colIndex];
    int y = col_top;
    int block_h = 20;  // block height remains 20px
    int spacing = 5;   // spacing remains 5px
    int current_y = y + col_height - block_h;

    for (int i = 0; i < blockCount; i++) {
        // Draw the block rectangle.
        ctx_draw_rect(ctx, x+1, current_y, col_width-2, block_h - spacing, blocks[i].color);
        // Center the label using unstyled string function.
        int label_w = (int)(8 * strlen(blocks[i].label));
        int label_x = x + (col_width - label_w) / 2;
        int label_y = current_y + (block_h/2) - 4;
        ctx_draw_string_styled(ctx, label_x, label_y, blocks[i].label, WHITE_16, FontStyle_8x8_Basic);
        current_y -= (block_h + spacing);
        if (current_y < y) break; // no more space
    }
}

// --------------------------------------------------------------------------
static void draw_scheduling(void) {
    GraphicsContext ctx = create_context(sched_vis_canvas->buffer,
                                         sched_vis_canvas->width,
                                         sched_vis_canvas->height,
                                         g_BYTES_PER_PIXEL);

    for (int i = 0; i < ctx.width * ctx.height; i++) {
        ((uint8_t *)ctx.buffer)[i] = LIGHT_GRAY_16;
    }

    ctx_draw_string(&ctx, (ctx.width / 2) - 48, 5, "CPU Scheduling", BLACK_16);

    int col_width  = 50;
    int col_height = 60;   // reduced height for columns
    int col_top    = 30;   // start below the title
    int col_left[4] = { 10, 70, 130, 190 };  // these should fit in 240px width

    const char* col_labels[4] = { "Proc 1", "Proc 2", "Proc 3", "Proc 4" };

    // Draw each column's label and outline.
    for (int c = 0; c < 4; c++) {
        int x = col_left[c];
        int y = col_top;
        int label_w = (int)(8 * strlen(col_labels[c]));
        ctx_draw_string(&ctx, x + (col_width - label_w)/2, y - 10, col_labels[c], BLACK_16);
        ctx_draw_rect(&ctx, x, y, col_width, col_height, LIGHT_GRAY_16); // fill
        ctx_draw_line(&ctx, x, y, x, y+col_height, BLACK_16);
        ctx_draw_line(&ctx, x+col_width, y, x+col_width, y+col_height, BLACK_16);
        ctx_draw_line(&ctx, x, y, x+col_width, y, BLACK_16);
        ctx_draw_line(&ctx, x, y+col_height, x+col_width, y+col_height, BLACK_16);
    }

    // Example blocks for each column.
    struct BlockInfo col1_blocks[] = { { LIGHT_GREEN_16, "1" } };
    struct BlockInfo col2_blocks[] = { { BROWN_16, "1" }, { BLUE_16, "3" } };
    struct BlockInfo col3_blocks[] = { { LIGHT_CYAN_16, "1" }, { MAGENTA_16, "2" } };
    struct BlockInfo col4_blocks[] = { { LIGHT_RED_16, "1" }, { YELLOW_16, "2" } };

    draw_blocks_in_column(&ctx, 0, col1_blocks, 1, col_left, col_top, col_width, col_height);
    draw_blocks_in_column(&ctx, 1, col2_blocks, 2, col_left, col_top, col_width, col_height);
    draw_blocks_in_column(&ctx, 2, col3_blocks, 2, col_left, col_top, col_width, col_height);
    draw_blocks_in_column(&ctx, 3, col4_blocks, 2, col_left, col_top, col_width, col_height);

    // Draw "Round Robin Queue" dropdown.
    int dropdown_x = 10;
    int dropdown_y = 100;
    int dropdown_w = 110;
    int dropdown_h = 16;
    ctx_draw_rect(&ctx, dropdown_x, dropdown_y, dropdown_w, dropdown_h, LIGHT_GRAY_16);
    ctx_draw_line(&ctx, dropdown_x, dropdown_y, dropdown_x+dropdown_w, dropdown_y, BLACK_16);
    ctx_draw_line(&ctx, dropdown_x, dropdown_y+dropdown_h, dropdown_x+dropdown_w, dropdown_y+dropdown_h, BLACK_16);
    ctx_draw_line(&ctx, dropdown_x, dropdown_y, dropdown_x, dropdown_y+dropdown_h, BLACK_16);
    ctx_draw_line(&ctx, dropdown_x+dropdown_w, dropdown_y, dropdown_x+dropdown_w, dropdown_y+dropdown_h, BLACK_16);
    ctx_draw_string(&ctx, dropdown_x+3, dropdown_y+3, "Round Robin", BLACK_16);
    ctx_draw_line(&ctx, dropdown_x+dropdown_w-12, dropdown_y+6, dropdown_x+dropdown_w-8, dropdown_y+6, BLACK_16);
    ctx_draw_line(&ctx, dropdown_x+dropdown_w-11, dropdown_y+7, dropdown_x+dropdown_w-9, dropdown_y+7, BLACK_16);

    // Draw a row of colored squares at the bottom.
    uint32_t bottom_colors[] = { GREEN_16, BROWN_16, BLUE_16, MAGENTA_16, LIGHT_RED_16, YELLOW_16, CYAN_16 };
    int squares_count = sizeof(bottom_colors)/sizeof(bottom_colors[0]);
    int square_size = 16;
    int sq_spacing = 4;
    int start_x = 10;
    int row_y = 130;

    for (int i = 0; i < squares_count; i++) {
        int sx = start_x + i * (square_size + sq_spacing);
        ctx_draw_rect(&ctx, sx, row_y, square_size, square_size, bottom_colors[i]);
        ctx_draw_line(&ctx, sx,         row_y,          sx+square_size, row_y,          BLACK_16);
        ctx_draw_line(&ctx, sx,         row_y+square_size, sx+square_size, row_y+square_size, BLACK_16);
        ctx_draw_line(&ctx, sx,         row_y,          sx,             row_y+square_size, BLACK_16);
        ctx_draw_line(&ctx, sx+square_size, row_y,       sx+square_size, row_y+square_size, BLACK_16);
    }
}

void scheduling_visualiser_callback(UIWidget *widget, Window *parent) {
    if (sched_vis_window) {
        bring_to_front(sched_vis_window);
        return;
    }
    int win_x = (g_SCREEN_WIDTH - SCHED_VIS_WIDTH) / 2;
    int win_y = (g_SCREEN_HEIGHT - SCHED_VIS_HEIGHT) / 2;
    sched_vis_window = create_window(win_x, win_y, SCHED_VIS_WIDTH, SCHED_VIS_HEIGHT, LIGHT_GRAY_16, "Scheduling");
    if (!sched_vis_window) {
        printf("[SCHED VIS] Failed to create Scheduling Visualiser window\r\n");
        return;
    }
    int margin = 0;
    sched_vis_canvas = create_window_canvas(sched_vis_window, margin, margin,
                                            SCHED_VIS_WIDTH - 2 * margin,
                                            SCHED_VIS_HEIGHT - 2 * margin);
    if (!sched_vis_canvas) {
        printf("[SCHED VIS] Failed to create canvas for Scheduling\r\n");
        return;
    }
    printf("[SCHED VIS] Scheduling Visualiser started\r\n");
}

void update_scheduling_visualiser(void) {
    if (!sched_vis_window || !sched_vis_canvas) return;
    draw_scheduling();
}

void close_scheduling_visualiser(void) {
    if (sched_vis_window) {
        destroy_window(sched_vis_window);
        sched_vis_window = NULL;
        sched_vis_canvas = NULL;
    }
}
