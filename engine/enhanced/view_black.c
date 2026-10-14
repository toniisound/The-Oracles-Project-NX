/* The black the band shows: what the harness holds under a
 * ceiling (enhanced.black_pixels_mean) and what the launcher says at the end
 * of a session, counted once, here, so that a played session
 * measures what the player sees on the same terms as the replays. */
#include "view_internal.h"

/* A run of black at a room's place ends at `frame`: kept when past the
 * threshold, among the longest, longest first. */
static void keep_run(OraclesEnhancedBlack *b, uint8_t group, uint8_t room, uint32_t since, uint32_t frame)
{
    const uint32_t frames = frame - since;
    if (frames <= ORACLES_ENHANCED_BLACK_ROOM_FRAMES) return;
    const unsigned kept = b->rooms_over < ORACLES_ENHANCED_BLACK_ROOMS_KEPT ? b->rooms_over : ORACLES_ENHANCED_BLACK_ROOMS_KEPT;
    b->rooms_over++;
    unsigned at = kept;
    while (at > 0 && b->longest[at - 1u].frames < frames) at--;
    if (at >= ORACLES_ENHANCED_BLACK_ROOMS_KEPT) return;
    for (unsigned i = kept < ORACLES_ENHANCED_BLACK_ROOMS_KEPT ? kept : ORACLES_ENHANCED_BLACK_ROOMS_KEPT - 1u; i > at; i--) b->longest[i] = b->longest[i - 1u];
    b->longest[at] = (OraclesEnhancedBlackRoom){ group, room, since, frames };
}

static void close_run(OraclesEnhancedView *v, unsigned i, uint32_t frame)
{
    keep_run(&v->black, v->black_runs[i].group, v->black_runs[i].room, v->black_runs[i].since, frame);
    v->black_runs[i] = v->black_runs[--v->black_run_count];
}

/* The grid the band shows: the overworld's (1), not where the band shows a
 * large room, a room alone or the zone the game routes itself, or Ages' open
 * sea under its surface (2), which the drawn-back band shows as a map; 0 none. */
enum { GRID_NONE, GRID_OVERWORLD, GRID_SEA };
static int grid_shown(const OraclesEnhancedView *v)
{
    if (v->observation.large_grid || ev_isolated_room(v)) return GRID_NONE;
    if (ev_on_overworld(v)) return ev_in_routed_zone(v) ? GRID_NONE : GRID_OVERWORLD;
    return ev_on_map(v) && ev_open_water(v, v->observer.ref_group, v->observer.ref_room) ? GRID_SEA : GRID_NONE;
}

/* Whether a place of that grid is a room drawn there: on the map, or of the sea. */
static int grid_room(const OraclesEnhancedView *v, int grid, int32_t col, int32_t row)
{
    const uint8_t group = v->observer.ref_group;
    const unsigned width = grid == GRID_SEA ? 16u : ev_map_width(v, group), height = grid == GRID_SEA ? 16u : ev_map_height(v, group);
    if (row < 0 || col < 0 || (unsigned)col >= width || (unsigned)row >= height) return 0;
    const uint8_t room = (uint8_t)(row * 16 + col);
    return grid == GRID_SEA ? ev_open_water(v, group, room) : !ev_off_map(v, group, room);
}

/* The part of the band shown, in the world. */
static void shown_rect(const OraclesEnhancedView *v, int32_t camera_x, int32_t camera_y, int32_t *left, int32_t *top)
{
    *left = camera_x + (int32_t)(v->size.width - v->shown_width) / 2;
    *top = camera_y + (int32_t)(v->band_height - v->shown_height) / 2;
}

/* The rooms of the overworld's grid (`sea`: or of the open sea's) the band
 * shows, the room in play aside, where the composition drew no terrain: a
 * whole room black, but for a room whose part shown is under the game's
 * window (a scroll's). */
static unsigned black_places(OraclesEnhancedView *v, int32_t camera_x, int32_t camera_y, uint8_t *rooms, unsigned capacity, int sea)
{
    const int grid = grid_shown(v);
    if (grid == GRID_NONE || (grid == GRID_SEA && !sea)) return 0;
    const int32_t h = (int32_t)ORACLES_GHOST_AREA_HEIGHT;
    const int32_t col0 = camera_x / SMALL_ROOM_W - 1, row0 = camera_y / h - 1;
    int32_t shown_left, shown_top;
    shown_rect(v, camera_x, camera_y, &shown_left, &shown_top);
    const int32_t shown_right = shown_left + (int32_t)v->shown_width, shown_bottom = shown_top + (int32_t)v->shown_height;
    const int32_t window_left = v->observation.window_left, window_top = v->observation.window_top;
    unsigned count = 0;
    for (int32_t row = row0; row <= row0 + 4; row++)
        for (int32_t col = col0; col <= col0 + 5; col++) {
            if (!grid_room(v, grid, col, row)) continue;   /* never drawn there */
            const uint8_t room = (uint8_t)(row * 16 + col);
            if (room == v->observer.ref_room || !ev_shown_meets(v, camera_x, camera_y, col * SMALL_ROOM_W, row * h, SMALL_ROOM_W, h, 0)) continue;
            if (v->drawn_places[room >> 5u] & (1u << (room & 31u))) continue;   /* what the composition drew there, whichever entry */
            const int32_t l = col * SMALL_ROOM_W > shown_left ? col * SMALL_ROOM_W : shown_left, t = row * h > shown_top ? row * h : shown_top;
            const int32_t r = (col + 1) * SMALL_ROOM_W < shown_right ? (col + 1) * SMALL_ROOM_W : shown_right;
            const int32_t b = (row + 1) * h < shown_bottom ? (row + 1) * h : shown_bottom;
            if (l >= window_left && t >= window_top && r <= window_left + SMALL_ROOM_W && b <= window_top + h) continue;
            if (count < capacity) rooms[count++] = room;
        }
    return count;
}

/* Whether the band shows a place of no room of its grid: past the map's
 * edge, a place the map never draws, the sea's shore. */
static int past_edge(const OraclesEnhancedView *v, int32_t camera_x, int32_t camera_y)
{
    const int grid = grid_shown(v);
    if (grid == GRID_NONE) return 0;
    const int32_t h = (int32_t)ORACLES_GHOST_AREA_HEIGHT;
    int32_t left, top;
    shown_rect(v, camera_x, camera_y, &left, &top);
    const int32_t right = left + (int32_t)v->shown_width, bottom = top + (int32_t)v->shown_height;
    if (left < 0 || top < 0) return 1;
    for (int32_t row = top / h; row <= (bottom - 1) / h; row++)
        for (int32_t col = left / SMALL_ROOM_W; col <= (right - 1) / SMALL_ROOM_W; col++)
            if (!grid_room(v, grid, col, row)) return 1;
    return 0;
}

/* An episode of a kind ends at `frame`, its last frame the one before. */
static void end_episode(OraclesEnhancedBlack *b, unsigned kind, uint32_t since, uint32_t frame, int in_transition)
{
    OraclesEnhancedBlackKind *k = &b->kinds[kind];
    k->episodes++;
    if (frame - since > k->longest) k->longest = frame - since;
    if (in_transition) k->in_transition++;
}

static void end_room_episode(OraclesEnhancedView *v, unsigned i, uint32_t frame)
{
    const unsigned kind = v->black_episodes[i].kind;
    end_episode(&v->black, kind, v->black_episodes[i].since, frame, v->black_episodes[i].in_transition);
    const size_t used = strlen(v->black_log);
    if (kind != ORACLES_ENHANCED_BLACK_COLD && used + 24u < sizeof v->black_log)
        snprintf(v->black_log + used, sizeof v->black_log - used, "%s%c %u:%02x %u+%u%s", used ? ", " : "", kind == ORACLES_ENHANCED_BLACK_CORNER ? 'c' : 'n',
                 v->black_episodes[i].group, v->black_episodes[i].room, v->black_episodes[i].since, frame - v->black_episodes[i].since,
                 v->black_episodes[i].in_transition ? "t" : "");
    v->black_episodes[i] = v->black_episodes[--v->black_episode_count];
}

/* The episodes by kind, a scroll's frames included: the rooms black this
 * frame (none in a frame not of the world), the band past the map's edge,
 * and what is black besides, a ripple's fringe or else. */
static void count_kinds(OraclesEnhancedView *v, int world, int32_t camera_x, int32_t camera_y, uint32_t frame)
{
    if (world) {
        /* The band fills from nothing at the first world frame, after a load
         * and after a warp: the room in play not beside the one before. */
        const uint8_t group = v->observer.ref_group, room = v->observer.ref_room;
        const int dc = (int)(room & 0x0fu) - (int)(v->black_ref_room & 0x0fu), dr = (int)(room >> 4u) - (int)(v->black_ref_room >> 4u);
        if (!v->black_have_ref || v->black_loaded || group != v->black_ref_group || dc < -1 || dc > 1 || dr < -1 || dr > 1) v->black_cold_from = frame;
        v->black_have_ref = 1; v->black_loaded = 0;
        v->black_ref_group = group; v->black_ref_room = room;
    }
    uint8_t rooms[32];
    const unsigned count = world && v->uncovered ? black_places(v, camera_x, camera_y, rooms, 32u, 1) : 0u;
    const int transition = v->observation.in_transition;
    for (unsigned i = 0; i < v->black_episode_count;) {
        int still = 0;
        for (unsigned k = 0; k < count; k++) still |= rooms[k] == v->black_episodes[i].room && v->black_episodes[i].group == v->observer.ref_group;
        if (still) i++; else end_room_episode(v, i, frame);
    }
    unsigned present = 0;
    for (unsigned k = 0; k < count; k++) {
        unsigned i = 0;
        while (i < v->black_episode_count && !(v->black_episodes[i].room == rooms[k] && v->black_episodes[i].group == v->observer.ref_group)) i++;
        if (i == sizeof v->black_episodes / sizeof v->black_episodes[0]) continue;
        if (i == v->black_episode_count) {
            const int dc = (int)(rooms[k] & 0x0fu) - (int)(v->observer.ref_room & 0x0fu), dr = (int)(rooms[k] >> 4u) - (int)(v->observer.ref_room >> 4u);
            v->black_episodes[i].group = v->observer.ref_group;
            v->black_episodes[i].room = rooms[k];
            v->black_episodes[i].kind = (uint8_t)(frame - v->black_cold_from < ORACLES_ENHANCED_BLACK_COLD_FRAMES ? ORACLES_ENHANCED_BLACK_COLD
                                                  : (dc == 2 || dc == -2) && (dr == 2 || dr == -2) ? ORACLES_ENHANCED_BLACK_CORNER : ORACLES_ENHANCED_BLACK_NEIGHBOUR);
            v->black_episodes[i].in_transition = (uint8_t)transition;
            v->black_episodes[i].since = frame;
            v->black_episode_count++;
        }
        present |= 1u << v->black_episodes[i].kind;
    }
    const int edge = world && v->uncovered && past_edge(v, camera_x, camera_y);
    const int ripple = world && v->uncovered && !count && !edge && v->have_wave;
    const int places[3] = { edge, ripple, world && v->uncovered && !count && !edge && !ripple };
    for (unsigned p = 0; p < 3u; p++) {
        const unsigned kind = ORACLES_ENHANCED_BLACK_EDGE + p;
        if (places[p]) {
            present |= 1u << kind;
            if (!v->black_place_since[p]) { v->black_place_since[p] = frame + 1u; v->black_place_transition[p] = (uint8_t)transition; }
        } else if (v->black_place_since[p]) {
            end_episode(&v->black, kind, v->black_place_since[p] - 1u, frame, v->black_place_transition[p]);
            v->black_place_since[p] = 0;
        }
    }
    for (unsigned kind = 0; kind < ORACLES_ENHANCED_BLACK_KINDS; kind++) if (present & (1u << kind)) v->black.kinds[kind].frames++;
}

void ev_measure_black(OraclesEnhancedView *v, OraclesEnhancedMode mode, int32_t camera_x, int32_t camera_y)
{
    OraclesEnhancedBlack *b = &v->black;
    const uint32_t frame = v->frame - 1u;   /* the frame composed, numbered as the route's */
    count_kinds(v, mode == ORACLES_ENHANCED_WORLD, camera_x, camera_y, frame);
    if (mode != ORACLES_ENHANCED_WORLD) {
        while (v->black_run_count) close_run(v, 0, frame);
        return;
    }
    if (!b->have_world) { b->have_world = 1; b->first_world = frame; }
    if (!b->have_full && v->uncovered == 0) { b->have_full = 1; b->first_full = frame; }
    if (b->have_full) {
        b->full_world_frames++;
        b->pixels += v->uncovered;
        if (v->uncovered > b->max) b->max = v->uncovered;
    }
    if (v->observation.in_transition) return;   /* the runs neither grow nor end across a transition */
    uint8_t rooms[32];
    const unsigned count = black_places(v, camera_x, camera_y, rooms, 32u, 0);
    for (unsigned i = 0; i < v->black_run_count;) {
        int still = 0;
        for (unsigned k = 0; k < count; k++) still |= rooms[k] == v->black_runs[i].room && v->black_runs[i].group == v->observer.ref_group;
        if (still) i++; else close_run(v, i, frame);
    }
    for (unsigned k = 0; k < count; k++) {
        int known = 0;
        for (unsigned i = 0; i < v->black_run_count; i++) known |= v->black_runs[i].room == rooms[k] && v->black_runs[i].group == v->observer.ref_group;
        if (known || v->black_run_count == sizeof v->black_runs / sizeof v->black_runs[0]) continue;
        v->black_runs[v->black_run_count].group = v->observer.ref_group;
        v->black_runs[v->black_run_count].room = rooms[k];
        v->black_runs[v->black_run_count++].since = frame;
    }
}

/* The figures so far, the runs and the episodes still going counted as they stand. */
void oracles_enhanced_view_black(const OraclesEnhancedView *v, OraclesEnhancedBlack *out)
{
    *out = v->black;
    for (unsigned i = 0; i < v->black_run_count; i++) keep_run(out, v->black_runs[i].group, v->black_runs[i].room, v->black_runs[i].since, v->frame);
    for (unsigned i = 0; i < v->black_episode_count; i++) end_episode(out, v->black_episodes[i].kind, v->black_episodes[i].since, v->frame, v->black_episodes[i].in_transition);
    for (unsigned p = 0; p < 3u; p++)
        if (v->black_place_since[p]) end_episode(out, ORACLES_ENHANCED_BLACK_EDGE + p, v->black_place_since[p] - 1u, v->frame, v->black_place_transition[p]);
}

const char *oracles_enhanced_black_kind_name(unsigned kind)
{
    static const char *const names[ORACLES_ENHANCED_BLACK_KINDS] = { "corner", "neighbour", "cold", "edge", "ripple", "other" };
    return kind < ORACLES_ENHANCED_BLACK_KINDS ? names[kind] : "?";
}

const char *oracles_enhanced_view_black_log(const OraclesEnhancedView *v) { return v->black_log; }
