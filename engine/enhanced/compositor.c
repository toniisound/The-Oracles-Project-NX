#include "compositor.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ppu.h"

#define HUD ORACLES_ENHANCED_HUD_HEIGHT
#define CORE_W ORACLES_ENHANCED_CORE_WIDTH
#define AREA_H ORACLES_ENHANCED_AREA_HEIGHT

/* The game's palette fade applied to an RGB555 colour of a source that does
 * not carry it, as the palette thread does it (updateFadingPalettes): the
 * offset added to each five-bit channel, clamped; then the pixel of that
 * colour in the live pipeline, the one the core's window went through. */
static uint32_t pixel_of(const uint32_t *colours, unsigned colour)
{
    if (colours) return colours[colour & (ORACLES_PPU_COLOURS - 1u)];
    uint32_t out = 0xff000000u;
    for (unsigned shift = 0; shift < 15u; shift += 5u) out |= (uint32_t)(((colour >> shift) & 31u) * 255u / 31u) << (shift / 5u * 8u);
    return out;
}

uint32_t oracles_enhanced_faded_pixel(const uint32_t *colours, uint32_t rgb555, int fade)
{
    unsigned colour = 0;
    for (unsigned shift = 0; shift < 15u; shift += 5u) {
        const int v = (int)((rgb555 >> shift) & 31u) + fade;
        colour |= (unsigned)(v < 0 ? 0 : v > 31 ? 31 : v) << shift;
    }
    return pixel_of(colours, colour);
}

static uint32_t faded(const OraclesEnhancedCompose *in, uint32_t rgb555, int fade) { return oracles_enhanced_faded_pixel(in->colours, rgb555, fade); }

uint16_t oracles_enhanced_bar_pixel(const OraclesEnhancedBar *bar, unsigned x, unsigned y, int *hides_sprite)
{
    const unsigned cell = (y / 8u) * 32u + x / 8u;
    const uint8_t attr = bar->attributes[cell];
    const unsigned tile = (bar->lcdc & 0x10u) ? bar->tiles[cell] : 256u + (uint8_t)(bar->tiles[cell] + 128u) - 128u;
    const unsigned line = (attr & 0x40u) ? 7u - y % 8u : y % 8u, bit = (attr & 0x20u) ? x % 8u : 7u - x % 8u;
    const uint8_t *data = ((attr & 0x08u) ? bar->vram1 : bar->vram0) + tile * 16u + line * 2u;
    const unsigned index = ((data[0] >> bit) & 1u) | (((data[1] >> bit) & 1u) << 1);
    const uint8_t *colour = bar->palettes + ((attr & 7u) * 4u + index) * 2u;
    if (hides_sprite) *hides_sprite = (bar->lcdc & 0x01u) && (attr & 0x80u) && index;
    return (uint16_t)((colour[0] | (colour[1] << 8)) & 0x7fffu);
}

unsigned oracles_enhanced_hud_fill(const OraclesEnhancedBar *bar, uint32_t (*pixel)(void *opaque, uint16_t rgb555), void *opaque,
                                   const uint32_t *core, uint32_t fill[2u * HUD])
{
    unsigned filled = 0;
    for (unsigned y = 0; y < HUD; y++) {
        const uint32_t *line = core + y * CORE_W;
        unsigned x = 1;
        while (x < CORE_W && line[x] == line[0]) x++;
        const int blank = x == CORE_W;
        for (unsigned side = 0; side < 2u; side++) {
            const unsigned edge = side ? CORE_W - 1u : 0u;
            const uint16_t rgb555 = oracles_enhanced_bar_pixel(bar, edge, y, NULL);
            const uint32_t colour = pixel ? pixel(opaque, rgb555) : pixel_of(NULL, rgb555);
            fill[side * HUD + y] = line[edge] == colour || blank ? line[edge] : 0u;
            filled += fill[side * HUD + y] != 0u;
        }
    }
    return filled;
}

/* The gutters and what no source covers: black, faded with the game (the
 * border colour when nothing fades). */
static uint32_t gutter_pixel(const OraclesEnhancedCompose *in, uint32_t border)
{
    return in->fade ? faded(in, 0, in->fade) : border;
}

/* One row of the world band, the sources painted as rectangles, span by
 * span: the neighbours from the last to the first, then the core's window, so
 * that the window wins, then the first neighbour whose area covers a pixel (a
 * neighbour of another row does not hide the one behind it); what no source
 * painted is black, counted as uncovered inside the counted rectangle.  The
 * line's own scroll (a ripple under water) moves the world under the row:
 * every source is read at the same shifted place, so the whole band ripples
 * with the room, and the window, drawn with that scroll, stays in place.
 * `row` holds the band's columns x0 to x1. */
static unsigned compose_row(const OraclesEnhancedCompose *in, const uint32_t *core, uint32_t *row, unsigned x0, unsigned x1,
                            int32_t world_y, int32_t shift_x, int32_t shift_y, uint32_t black)
{
    uint8_t painted[ORACLES_ENHANCED_MAX_WIDTH];
    memset(painted + x0, 0, x1 - x0);
    /* The band column x shows world column left + x (the shift applied). */
    const int32_t left = in->world_left + shift_x, wy = world_y + shift_y;
    for (unsigned n = in->neighbour_count; n-- > 0;) {
        const OraclesEnhancedNeighbour *nb = &in->neighbours[n];
        const unsigned width = nb->width ? nb->width : CORE_W, height = nb->height ? nb->height : AREA_H;
        const int32_t ny = wy - nb->world_top;
        if (!nb->game_area || ny < 0 || ny >= (int32_t)height) continue;
        int32_t from = nb->world_left - left, to = from + (int32_t)width;
        if (from < (int32_t)x0) from = (int32_t)x0;
        if (to > (int32_t)x1) to = (int32_t)x1;
        const uint32_t *src = nb->game_area + (size_t)ny * width;
        const int32_t offset = left - nb->world_left;   /* the source column of band column x is x + offset, in the span */
        if (from >= to) continue;
        if (nb->faded) memcpy(row + from, src + from + offset, (size_t)(to - from) * sizeof *row);
        else for (int32_t x = from; x < to; x++) row[x] = faded(in, src[x + offset], in->fade);
        memset(painted + from, 1, (size_t)(to - from));
    }
    /* The window: the game area starts after the status bar; the scroll moves it with the world. */
    const int32_t line = world_y - in->window_world_top;
    if (line >= 0 && line < (int32_t)AREA_H) {
        int32_t from = in->window_world_left - in->world_left, to = from + (int32_t)CORE_W;
        const uint32_t *src = core + (HUD + (unsigned)line) * CORE_W;
        const int32_t offset = in->world_left - in->window_world_left;
        if (from < (int32_t)x0) from = (int32_t)x0;
        if (to > (int32_t)x1) to = (int32_t)x1;
        if (from < to) {
            memcpy(row + from, src + from + offset, (size_t)(to - from) * sizeof *row);
            memset(painted + from, 1, (size_t)(to - from));
        }
    }
    unsigned uncovered = 0;
    const int counted_row = !in->counted_width || (wy >= in->counted_top && wy < in->counted_top + (int32_t)in->counted_height);
    for (unsigned x = x0; x < x1; x++) {
        if (painted[x]) continue;
        row[x] = black;
        const int32_t wx = left + (int32_t)x;
        if (counted_row && (!in->counted_width || (wx >= in->counted_left && wx < in->counted_left + (int32_t)in->counted_width))) uncovered++;
    }
    return uncovered;
}

/* The framed core: its 160x144 image centred in the surface (vertically too
 * in the drawn-back view's taller one), the rest the border. */
static void compose_framed(const uint32_t *core, const OraclesEnhancedCompose *in, OraclesEnhancedSize size, uint32_t *out)
{
    const uint32_t gutter = gutter_pixel(in, in->border);   /* a flash covers the frame's gutters as well */
    const unsigned W = size.width, H = size.height, HUD_X = oracles_enhanced_hud_x(size), top = (H - ORACLES_PPU_HEIGHT) / 2u;
    for (unsigned y = 0; y < H; y++)
        for (unsigned x = 0; x < W; x++)
            out[y * W + x] = (x >= HUD_X && x < HUD_X + CORE_W && y >= top && y < top + ORACLES_PPU_HEIGHT) ? core[(y - top) * CORE_W + (x - HUD_X)] : gutter;
}

static unsigned compose_world(const uint32_t *core, const OraclesEnhancedCompose *in, OraclesEnhancedSize size, uint32_t *out)
{
    unsigned uncovered = 0;
    const unsigned W = size.width, HUD_X = oracles_enhanced_hud_x(size), band_h = oracles_enhanced_band_height(size);
    /* HUD band: the status bar, centred, gutters in the border colour (faded with the game), or the bar's own
     * background on the lines it is given. */
    const uint32_t gutter = gutter_pixel(in, in->border);
    for (unsigned y = 0; y < HUD; y++) {
        const uint32_t left = in->hud_fill && in->hud_fill[y] ? in->hud_fill[y] : gutter;
        const uint32_t right = in->hud_fill && in->hud_fill[HUD + y] ? in->hud_fill[HUD + y] : gutter;
        for (unsigned x = 0; x < W; x++)
            out[y * W + x] = x < HUD_X ? left : x < HUD_X + CORE_W ? core[y * CORE_W + (x - HUD_X)] : right;
    }
    /* World band: the game area placed by the camera; uncovered pixels black.
     * A band row shows the LCD line of the window that stands there, and that
     * line's own scroll: the shifts are read by line, not by band row, or the
     * neighbours would ripple out of step with the window beside them (the
     * band row and the line differ by the vertical camera).  The rows above and
     * below the window take the line a taller screen would have there, modulo
     * the area's 128: the game's waves repeat every 128 lines (docs/GAME_HOOKS.md,
     * section 1), so the area's lines are whole periods and the
     * phase goes on past them.  Outside the part of the band shown, black,
     * not counted. */
    const unsigned shown_w = in->shown_width ? in->shown_width : W, shown_h = in->shown_height ? in->shown_height : band_h;
    const unsigned x0 = (W - shown_w) / 2u, y0 = (band_h - shown_h) / 2u;
    const uint32_t black = gutter_pixel(in, 0xff000000u);
    for (unsigned y = 0; y < band_h; y++) {
        uint32_t *row = out + (HUD + y) * W;
        if (y < y0 || y >= y0 + shown_h) { for (unsigned x = 0; x < W; x++) row[x] = black; continue; }
        const int32_t world_y = in->world_top + (int32_t)y;
        const int32_t line = (int32_t)((uint32_t)(world_y - in->window_world_top) % AREA_H);   /* AREA_H a power of two: a floor modulo */
        const int32_t shift_x = in->line_shift ? in->line_shift[line] : 0, shift_y = in->line_shift_y ? in->line_shift_y[line] : 0;
        for (unsigned x = 0; x < x0; x++) row[x] = black;
        for (unsigned x = x0 + shown_w; x < W; x++) row[x] = black;
        uncovered += compose_row(in, core, row, x0, x0 + shown_w, world_y, shift_x, shift_y, black);
    }
    return uncovered;
}

unsigned oracles_enhanced_compose(const uint32_t *core, const OraclesEnhancedCompose *in, uint32_t *out)
{
    const OraclesEnhancedSize size = in->size.width ? in->size : oracles_enhanced_size(0);
    if (in->mode == ORACLES_ENHANCED_FRAMED) { compose_framed(core, in, size, out); return 0; }
    return compose_world(core, in, size, out);
}

void oracles_enhanced_far_fitting_words(unsigned screen_w, unsigned screen_h, char *out, size_t capacity)
{
    const OraclesEnhancedFitting f = oracles_enhanced_far_fitting(screen_w, screen_h);
    const OraclesEnhancedSize far = oracles_enhanced_view_size(ORACLES_ENHANCED_FAR, f.aspect);
    if (f.fit == ORACLES_ENHANCED_FIT_FITTED)
        snprintf(out, capacity, "the far view fitted to the %ux%u screen: %ux%u at %ux, where %ux%u covers %u%% of it at %ux",
                 screen_w, screen_h, f.size.width, f.size.height, f.scale, far.width, far.height, f.far_percent, f.far_scale);
    else if (f.fit == ORACLES_ENHANCED_FIT_FILLS)
        snprintf(out, capacity, "the far view at %ux%u: it covers %u%% of the %ux%u screen at %ux", f.size.width, f.size.height, f.far_percent, screen_w, screen_h, f.far_scale);
    else
        snprintf(out, capacity, "the far view at %ux%u: the %ux%u screen at %ux gives %ux%u, %s", f.size.width, f.size.height, screen_w, screen_h, f.scale,
                 f.computed.width, f.computed.height, f.fit == ORACLES_ENHANCED_FIT_SMALL ? "under 400x224 or taller than wide" : "not a size the view takes");
}
