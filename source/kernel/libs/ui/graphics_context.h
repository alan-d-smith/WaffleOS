#pragma once
#include <stdint.h>
#include "../../drivers/font.h"

typedef struct {
    volatile uint8_t *buffer;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
} GraphicsContext;

// TODO: Make this not assume 1BPP
static inline GraphicsContext create_context(uint8_t *buffer, int width, int height, int bytes_per_pixel) {
    GraphicsContext ctx;
    ctx.buffer = buffer;
    ctx.width = width;
    ctx.height = height;
    ctx.pitch = width;
    ctx.bytes_per_pixel = bytes_per_pixel;
    return ctx;
}

void ctx_put_pixel(GraphicsContext *ctx, int x, int y, uint32_t color);
void ctx_draw_line(GraphicsContext *ctx, int x1, int y1, int x2, int y2, uint32_t color);
void ctx_draw_rect(GraphicsContext *ctx, int x, int y, int width, int height, uint32_t color);
void ctx_draw_arrow(GraphicsContext *ctx, int x1, int y1, int x2, int y2, uint32_t color);
void ctx_draw_char(GraphicsContext *ctx, int x, int y, char ch, uint32_t color);
void ctx_draw_string(GraphicsContext *ctx, int x, int y, const char *str, uint32_t color);
void ctx_draw_char_styled(GraphicsContext *ctx, int x, int y, char ch, uint32_t color, FontStyle style);
void ctx_draw_string_styled(GraphicsContext *ctx, int x, int y, const char *str, uint32_t color, FontStyle style);
