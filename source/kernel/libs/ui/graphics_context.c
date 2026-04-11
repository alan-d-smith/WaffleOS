#include "graphics_context.h"

void ctx_put_pixel(GraphicsContext *ctx, int x, int y, uint32_t color) {
    if (x < 0 || x >= ctx->width || y < 0 || y >= ctx->height)
        return;
    int offset = y * ctx->pitch + x * ctx->bytes_per_pixel;
    switch (ctx->bytes_per_pixel) {
        case 1:
            ((volatile uint8_t*)ctx->buffer)[offset] = (uint8_t)color;
            break;
        case 2:
            *(volatile uint16_t*)(ctx->buffer + offset) = (uint16_t)color;
            break;
        case 4:
            *(volatile uint32_t*)(ctx->buffer + offset) = color;
            break;
        default:
            break;
    }
}

void ctx_draw_line(GraphicsContext *ctx, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = (x2 > x1 ? x2 - x1 : x1 - x2);
    int dy = (y2 > y1 ? y2 - y1 : y1 - y2);
    int sx = (x1 < x2 ? 1 : -1);
    int sy = (y1 < y2 ? 1 : -1);
    int err = dx - dy;

    while (1) {
        ctx_put_pixel(ctx, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

//TODO: Make more efficient
void ctx_draw_rect(GraphicsContext *ctx, int x, int y, int width, int height, uint32_t color) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            ctx_put_pixel(ctx, x + i, y + j, color);
        }
    }
}

// Draw an arrow from (x1, y1) to (x2, y2) using lines.
void ctx_draw_arrow(GraphicsContext *ctx, int x1, int y1, int x2, int y2, uint32_t color) {
    // Draw main line.
    ctx_draw_line(ctx, x1, y1, x2, y2, color);

    // Use a fixed arrowhead size.
    int arrow_size = 3;

    // Compute differences.
    int dx = x2 - x1;
    int dy = y2 - y1;

    // Determine arrowhead offsets based on dx and dy signs.
    // This is a crude approximation.
    if (dx >= 0 && dy >= 0) {
        ctx_draw_line(ctx, x2, y2, x2 - arrow_size, y2, color);
        ctx_draw_line(ctx, x2, y2, x2, y2 - arrow_size, color);
    } else if (dx >= 0 && dy < 0) {
        ctx_draw_line(ctx, x2, y2, x2 - arrow_size, y2, color);
        ctx_draw_line(ctx, x2, y2, x2, y2 + arrow_size, color);
    } else if (dx < 0 && dy >= 0) {
        ctx_draw_line(ctx, x2, y2, x2 + arrow_size, y2, color);
        ctx_draw_line(ctx, x2, y2, x2, y2 - arrow_size, color);
    } else { // dx < 0 && dy < 0
        ctx_draw_line(ctx, x2, y2, x2 + arrow_size, y2, color);
        ctx_draw_line(ctx, x2, y2, x2, y2 + arrow_size, color);
    }
}

// Draw a single character at (x,y) using an 8x16 font.
// This function uses the font8x16 array defined in your display library.
void ctx_draw_char(GraphicsContext *ctx, int x, int y, char ch, uint32_t color) {
    uint8_t c = (uint8_t)ch;
    for (int row = 0; row < 16; row++) {
        uint8_t bits = font8x16[c][row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                ctx_put_pixel(ctx, x + col, y + row, color);
            }
        }
    }
}

// Draw a null-terminated string starting at (x,y).
void ctx_draw_string(GraphicsContext *ctx, int x, int y, const char *str, uint32_t color) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 16;   // Move down by 16 pixels (font height)
            x = orig_x;
        } else {
            ctx_draw_char(ctx, x, y, *str, color);
            x += 8;    // Advance by 8 pixels (font width)
        }
        str++;
    }
}

void ctx_draw_char_styled(GraphicsContext *ctx, int x, int y, char ch, uint32_t color, FontStyle style) {
    // Compute the scaled dimensions.
    int scaled_width = (int)ceil(style.CharacterWidth * style.Scale);
    int scaled_height = (int)ceil(style.CharacterHeight * style.Scale);

    for (int row = 0; row < scaled_height; row++) {
        // Map the output row to a source row
        float src_y_f = ((float)row + 0.5f) / style.Scale;
        int src_y = (int)src_y_f;
        if (src_y >= (int)style.CharacterHeight)
            src_y = style.CharacterHeight - 1;
        for (int col = 0; col < scaled_width; col++) {
            float src_x_f = ((float)col + 0.5f) / style.Scale;
            int src_x = (int)src_x_f;
            if (src_x >= (int)style.CharacterWidth)
                src_x = style.CharacterWidth - 1;

            // Only for the 8x8 basic font style, invert horizontally
            if (style.Reference == (const uint8_t*)font8x8_basic) {
                src_x = style.CharacterWidth - 1 - src_x;
            }

            // For ASCII character 'ch', the flattened font data is laid out sequentially
            int char_index = (int)ch;  // ASCII range 0-127
            int offset = char_index * style.CharacterHeight + src_y;
            uint8_t row_data = ((const uint8_t*)style.Reference)[offset];
            // Create a mask for the current column
            uint8_t mask = 1 << (style.CharacterWidth - 1 - src_x);
            if (row_data & mask) {
                ctx_put_pixel(ctx, x + col, y + row, color);
            }
        }
    }
}

// Draw a null-terminated string using the given style.
void ctx_draw_string_styled(GraphicsContext *ctx, int x, int y, const char *str, uint32_t color, FontStyle style) {
    int orig_x = x;
    while (*str) {
        if (*str == '\n') {
            y += (int)ceil(style.CharacterHeight * style.Scale);
            x = orig_x;
        } else {
            ctx_draw_char_styled(ctx, x, y, *str, color, style);
            x += (int)ceil(style.CharacterWidth * style.Scale);
        }
        str++;
    }
}
