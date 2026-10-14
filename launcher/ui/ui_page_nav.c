#include "ui_page_nav.h"

#include "compositor.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* The pages' texts: the profiles, the ROM's states, the renderer's values. */
const char *const oracles_profile_names[ORACLES_PROFILES] = { "Faithful", "Enhanced" };
void oracles_profile_size(const OraclesHomeNav *nav, int profile, char *out, size_t capacity)
{
    if (profile == ORACLES_PROFILE_ENHANCED) oracles_display_view_size(nav, nav->display.view, out, capacity);
    else snprintf(out, capacity, "160\xc3\x97" "144");
}
static const char *const profile_notes[ORACLES_PROFILES] = {
    "The core alone, as an unrecognized ROM sees it.", "The wide world, drawn back, with continuous transitions and the smooth camera."
};

static int page_game(const OraclesHomeNav *nav)
{
    const int game = oracles_home_game(nav);
    return game >= 0 ? game : ORACLES_HOME_GAME_AGES;
}

OraclesHomeGame *oracles_page_game(OraclesHomeNav *nav) { return &nav->games[page_game(nav)]; }
const OraclesHomeGame *oracles_page_game_const(const OraclesHomeNav *nav) { return &nav->games[page_game(nav)]; }

int oracles_page_profile_allowed(const OraclesHomeGame *game, OraclesProfile profile)
{
    /* A fan game's image decides, as a ROM does: the one its patch is made to give plays in every profile. */
    const OraclesRomState state = game->fan ? game->image : game->rom;
    if (state == ORACLES_ROM_ORIGINAL) return 1;
    return state == ORACLES_ROM_UNRECOGNISED && profile == ORACLES_PROFILE_FAITHFUL;
}

int oracles_page_profile(const OraclesHomeGame *game, OraclesProfile chosen)
{
    if (oracles_page_profile_allowed(game, chosen)) return (int)chosen;
    for (int p = 0; p < ORACLES_PROFILES; p++) if (oracles_page_profile_allowed(game, (OraclesProfile)p)) return p;
    return -1;
}

const char *oracles_page_rom_file(const OraclesHomeGame *game) { return game->rom_file[0] ? game->rom_file : "None"; }

void oracles_page_rom_status(const OraclesHomeGame *game, char *out, size_t capacity, OraclesTone *tone)
{
    switch (game->rom) {
        case ORACLES_ROM_ORIGINAL:
            if (game->fan) snprintf(out, capacity, "Original ROM, %s", game->fan->base_name);
            else snprintf(out, capacity, "Original ROM, all profiles");
            *tone = ORACLES_TONE_OK;
            break;
        case ORACLES_ROM_UNRECOGNISED:
            /* A base that is not the original: the patch, made for it, is refused on it. */
            if (game->fan) snprintf(out, capacity, "Unrecognized ROM, not %s", game->fan->base_name);
            else snprintf(out, capacity, "Unrecognized ROM, Faithful only");
            *tone = ORACLES_TONE_WARN;
            break;
        case ORACLES_ROM_REFUSED: snprintf(out, capacity, "Refused: %s.", game->rom_reason); *tone = ORACLES_TONE_ERROR; break;
        default: snprintf(out, capacity, game->fan ? "No base ROM chosen" : "No ROM chosen"); *tone = ORACLES_TONE_NONE; break;
    }
}

const char *oracles_page_patch_file(const OraclesHomeGame *game) { return game->patch_file[0] ? game->patch_file : "None"; }

void oracles_page_patch_status(const OraclesHomeGame *game, char *out, size_t capacity, OraclesTone *tone)
{
    switch (game->patch) {
        case ORACLES_ROM_NONE:   /* the player sees which patch the game expects before choosing one */
            snprintf(out, capacity, "No patch: the BPS patch of %s is expected", game->fan ? game->fan->image_name : "the game");
            *tone = ORACLES_TONE_NONE;
            break;
        case ORACLES_ROM_REFUSED: snprintf(out, capacity, "Refused: %s.", game->patch_reason); *tone = ORACLES_TONE_ERROR; break;
        default: snprintf(out, capacity, "Patch file read"); *tone = ORACLES_TONE_OK; break;
    }
}

void oracles_page_image_status(const OraclesHomeGame *game, char *out, size_t capacity, OraclesTone *tone)
{
    switch (game->image) {
        case ORACLES_ROM_ORIGINAL: snprintf(out, capacity, "Patched image recognized: %s", game->fan ? game->fan->image_name : "?"); *tone = ORACLES_TONE_OK; break;
        case ORACLES_ROM_UNRECOGNISED: snprintf(out, capacity, "Patched image not recognized, Faithful only"); *tone = ORACLES_TONE_WARN; break;
        case ORACLES_ROM_REFUSED: snprintf(out, capacity, "Refused: %s.", game->image_reason); *tone = ORACLES_TONE_ERROR; break;
        default: snprintf(out, capacity, "The game needs both files."); *tone = ORACLES_TONE_NONE; break;
    }
}

const char *oracles_page_rom_note(const OraclesHomeNav *nav)
{
    const OraclesHomeGame *game = oracles_page_game_const(nav);
    /* A fan game's image line says Faithful only already. */
    if (!game->usable || game->fan || nav->display.profile != ORACLES_PROFILE_ENHANCED || oracles_page_profile_allowed(game, ORACLES_PROFILE_ENHANCED)) return "";
    return "Enhanced needs an original ROM: this one plays in Faithful";
}

const char *oracles_page_rom_hotkeys_note(const OraclesHomeNav *nav)
{
    const OraclesHomeGame *game = oracles_page_game_const(nav);
    if (!game->usable || !game->hotkeys || !game->hotkeys_failed) return "";
    return "Item hotkeys need an original ROM: this one plays without them";
}

/* What follows "<id>/files" in a path of the application's storage, `id` its first character; NULL when it is not one. */
static const char *after_files(const char *id)
{
    const char *slash = strchr(id, '/');
    if (!slash || slash == id || strncmp(slash + 1, "files", 5) || (slash[6] && slash[6] != '/')) return NULL;
    return slash + 6;
}

void oracles_page_folder(int android, const char *folder, char *out, size_t capacity)
{
    const char *rest = NULL;
    const char *shared = android ? strstr(folder, "/Android/data/") : NULL;
    if (shared) { snprintf(out, capacity, "%s", shared + 1); return; }
    if (android && !strncmp(folder, "/data/data/", 11)) {
        rest = after_files(folder + 11);
    } else if (android && !strncmp(folder, "/data/user/", 11) && strchr(folder + 11, '/')) {
        rest = after_files(strchr(folder + 11, '/') + 1);
    }
    if (!rest) { snprintf(out, capacity, "%s", folder); return; }
    size_t length = (size_t)snprintf(out, capacity, "%s", "App storage");
    while (*rest == '/' && length < capacity) {
        const size_t part = strcspn(rest + 1, "/");
        if (part) length += (size_t)snprintf(out + length, capacity - length, " \xe2\x80\xba %.*s", (int)part, rest + 1);
        rest += part + 1;
    }
}

const char *oracles_page_save_file(const OraclesHomeGame *game) { return game->save_file[0] ? game->save_file : "No save yet"; }

void oracles_page_save_line(const OraclesHomeGame *game, char *out, size_t capacity)
{
    if (game->save_file[0]) snprintf(out, capacity, "Last written %s \xc2\xb7 savestates (.state) sit next to it", game->save_written);
    else if (game->usable) snprintf(out, capacity, "Created next to the %s on first save \xc2\xb7 savestates (.state) too", game->fan ? "patch" : "ROM");
    else snprintf(out, capacity, "%s", game->fan ? "Choose both files first" : "Choose a ROM first");
}

const char *oracles_page_play_note(const OraclesHomeNav *nav)
{
    const OraclesHomeGame *game = oracles_page_game_const(nav);
    if (nav->row != ORACLES_ROW_PLAY || game->usable) return "";
    return oracles_home_fan_game(page_game(nav)) ? "Choose both files first" : "Choose a ROM first";
}

int oracles_page_eu_shown(const OraclesHomeGame *game) { return !game->fan && game->rom == ORACLES_ROM_ORIGINAL; }

int oracles_page_language_shown(const OraclesHomeNav *nav, const OraclesHomeGame *game)
{
    return game->fan ? nav->fan_languages : oracles_page_eu_shown(game);
}

const char *const oracles_page_language_names[ORACLES_PAGE_LANGUAGES] = {
    "English (US)", "English (EU)", "Fran\xc3\xa7" "ais", "Deutsch", "Italiano", "Espa\xc3\xb1" "ol"
};

int oracles_page_language_available(const OraclesHomeGame *game, int language)
{
    const int european = game->eu == ORACLES_ROM_ORIGINAL && (!game->fan || game->rom == ORACLES_ROM_ORIGINAL);
    return language == 0 || (language > 0 && language < ORACLES_PAGE_LANGUAGES && european);
}

/* The language chosen, when the game has it; its row shown or not. */
static int chosen_language(const OraclesHomeGame *game) { return oracles_page_language_available(game, game->language) ? game->language : 0; }

int oracles_page_language(const OraclesHomeNav *nav, const OraclesHomeGame *game)
{
    return oracles_page_language_shown(nav, game) ? chosen_language(game) : 0;
}

const char *oracles_page_eu_file(const OraclesHomeGame *game) { return game->eu != ORACLES_ROM_NONE && game->eu_file[0] ? game->eu_file : "None"; }

void oracles_page_eu_status(const OraclesHomeNav *nav, char *out, size_t capacity, OraclesTone *tone)
{
    const OraclesHomeGame *game = oracles_page_game_const(nav);
    switch (game->eu) {
        case ORACLES_ROM_ORIGINAL: {
            size_t used = (size_t)snprintf(out, capacity, "Recognized: ");
            for (int l = 1; l < ORACLES_PAGE_LANGUAGES && used < capacity; l++)
                used += (size_t)snprintf(out + used, capacity - used, "%s%s", l > 1 ? ", " : "", oracles_page_language_names[l]);
            *tone = ORACLES_TONE_OK;
            break;
        }
        case ORACLES_ROM_UNRECOGNISED:
            snprintf(out, capacity, "Not the European ROM of Oracle of %s", page_game(nav) == ORACLES_HOME_GAME_SEASONS ? "Seasons" : "Ages");
            *tone = ORACLES_TONE_ERROR;
            break;
        case ORACLES_ROM_REFUSED: snprintf(out, capacity, "Refused: %s.", game->eu_reason); *tone = ORACLES_TONE_ERROR; break;
        default: {
            size_t used = (size_t)snprintf(out, capacity, "Optional: ");
            for (int l = 1; l < ORACLES_PAGE_LANGUAGES && used < capacity; l++)
                used += (size_t)snprintf(out + used, capacity - used, "%s%s", l > 1 ? ", " : "", oracles_page_language_names[l]);
            *tone = ORACLES_TONE_NONE;
            break;
        }
    }
}

unsigned oracles_page_eu_buttons(const OraclesHomeGame *game, const char *labels[2])
{
    if (game->eu == ORACLES_ROM_NONE) {
        labels[0] = "Choose\xe2\x80\xa6";
        labels[1] = NULL;
        return 1;
    }
    labels[0] = "Change\xe2\x80\xa6";
    labels[1] = "Remove";
    return 2;
}

const char *oracles_page_language_explanation(const OraclesHomeGame *game)
{
    if (game->fan) {
        const int ages = game->fan->base == ORACLES_HOME_GAME_AGES;
        if (!oracles_page_language_available(game, 1))
            return ages ? "Choose the European ROM of Oracle of Ages in its Cartridge to play it in another language."
                        : "Choose the European ROM of Oracle of Seasons in its Cartridge to play it in another language.";
        if (!chosen_language(game)) return "The original text of your fan game.";
        return ages ? "Texts the fan game shares with Oracle of Ages, from its European edition."
                    : "Texts the fan game shares with Oracle of Seasons, from its European edition.";
    }
    return chosen_language(game) ? "The European edition's text, from your European ROM." : "The original text of your US ROM.";
}

void oracles_page_language_note(const OraclesHomeGame *game, char *out, size_t capacity)
{
    if (!chosen_language(game)) snprintf(out, capacity, "%s", "");
    else if (!game->fan) snprintf(out, capacity, "Where the official translation was wrong, the port corrects it.");
    else
        snprintf(out, capacity, "Only the texts it shares with Oracle of %s are translated; its own dialogues stay in English.",
                 game->fan->base == ORACLES_HOME_GAME_AGES ? "Ages" : "Seasons");
}

/* The page has the row: the Patch row is a fan game's only, European ROM an Oracle's with its original ROM, Language
 * that Oracle's and a fan game's. */
static int has_row(const OraclesHomeNav *nav, unsigned row)
{
    const OraclesHomeGame *game = oracles_page_game_const(nav);
    if (row == ORACLES_ROW_EU_ROM) return oracles_page_eu_shown(game);
    if (row == ORACLES_ROW_LANGUAGE) return oracles_page_language_shown(nav, game);
    return row < ORACLES_GAME_ROWS && (row != ORACLES_ROW_PATCH || game->fan);
}

/* The European ROM gone: no file, English (US), for the Oracle and the fan games made on it; the host forgets them
 * (ORACLES_HOME_STORE). */
static OraclesHomeCommand remove_eu(OraclesHomeNav *nav)
{
    OraclesHomeGame *game = oracles_page_game(nav);
    game->eu = ORACLES_ROM_NONE;
    game->eu_file[0] = game->eu_folder[0] = game->eu_reason[0] = 0;
    game->language = 0;
    for (unsigned f = 0; f < ORACLES_HOME_FAN_GAMES; f++)
        if (&nav->games[oracles_home_fan_games[f].base] == game) nav->games[oracles_home_fan_games[f].game].eu = ORACLES_ROM_NONE;
    nav->eu_action = 0;
    return ORACLES_HOME_STORE;
}

/* The language `language`, when the game has it. */
static OraclesHomeCommand language_set(OraclesHomeNav *nav, int language)
{
    OraclesHomeGame *game = oracles_page_game(nav);
    if (!oracles_page_language_available(game, language) || language == game->language) return ORACLES_HOME_STAY;
    game->language = language;
    return ORACLES_HOME_STORE;
}

/* The next language (`by` 1) or the one before (-1), wrapping; English alone changes nothing. */
static OraclesHomeCommand language_step(OraclesHomeNav *nav, int by)
{
    const OraclesHomeGame *game = oracles_page_game_const(nav);
    if (!oracles_page_language_available(game, 1)) return ORACLES_HOME_STAY;
    return language_set(nav, ((game->language + by) % ORACLES_PAGE_LANGUAGES + ORACLES_PAGE_LANGUAGES) % ORACLES_PAGE_LANGUAGES);
}

static OraclesHomeCommand activate(OraclesHomeNav *nav, unsigned row)
{
    const OraclesHomeGame *game = oracles_page_game(nav);
    switch (row) {
        case ORACLES_ROW_ROM: return ORACLES_HOME_CHOOSE_ROM;
        case ORACLES_ROW_PATCH: return ORACLES_HOME_CHOOSE_PATCH;
        case ORACLES_ROW_SAVE: return game->usable || game->save_file[0] ? ORACLES_HOME_OPEN_FOLDER : ORACLES_HOME_STAY;
        case ORACLES_ROW_EU_ROM: return game->eu != ORACLES_ROM_NONE && nav->eu_action == 1 ? remove_eu(nav) : ORACLES_HOME_CHOOSE_EU_ROM;
        case ORACLES_ROW_LANGUAGE: return language_step(nav, 1);
        case ORACLES_ROW_PLAY: return game->usable ? oracles_home_start_command(page_game(nav)) : ORACLES_HOME_STAY;
        default: return ORACLES_HOME_STAY;
    }
}

/* The next row of the page from `row`, one step up or down, wrapping. */
static unsigned step(const OraclesHomeNav *nav, unsigned row, unsigned by)
{
    do row = (row + by) % ORACLES_GAME_ROWS; while (!has_row(nav, row));
    return row;
}

OraclesHomeCommand oracles_page_act(OraclesHomeNav *nav, OraclesHomeAction action)
{
    const unsigned row = has_row(nav, nav->row) ? nav->row : 0;
    switch (action) {
        case ORACLES_HOME_UP: nav->row = step(nav, row, ORACLES_GAME_ROWS - 1u); return ORACLES_HOME_STAY;
        case ORACLES_HOME_DOWN: nav->row = step(nav, row, 1u); return ORACLES_HOME_STAY;
        case ORACLES_HOME_LEFT:
        case ORACLES_HOME_RIGHT:
            /* European ROM: between Change and Remove, once there is a file; Language: the language before or after. */
            if (row == ORACLES_ROW_EU_ROM && oracles_page_game_const(nav)->eu != ORACLES_ROM_NONE) nav->eu_action = action == ORACLES_HOME_RIGHT;
            if (row == ORACLES_ROW_LANGUAGE) return language_step(nav, action == ORACLES_HOME_RIGHT ? 1 : -1);
            return ORACLES_HOME_STAY;
        case ORACLES_HOME_OK: return activate(nav, row);
        case ORACLES_HOME_BACK:
            /* Back to the home screen, on the item that opened the page. */
            nav->screen = ORACLES_SCREEN_HOME;
            nav->focus = 1;
            return ORACLES_HOME_STAY;
    }
    return ORACLES_HOME_STAY;
}

void oracles_page_keep_row(OraclesHomeNav *nav)
{
    if (nav->screen != ORACLES_SCREEN_GAME || has_row(nav, nav->row)) return;
    for (unsigned d = 1; d < ORACLES_GAME_ROWS; d++) {
        if (nav->row >= d && has_row(nav, nav->row - d)) { nav->row -= d; return; }
        if (nav->row + d < ORACLES_GAME_ROWS && has_row(nav, nav->row + d)) { nav->row += d; return; }
    }
    nav->row = ORACLES_ROW_ROM;
}

void oracles_page_hover(OraclesHomeNav *nav, unsigned row)
{
    if (has_row(nav, row)) nav->row = row;
}

OraclesHomeCommand oracles_page_click(OraclesHomeNav *nav, unsigned row, int option)
{
    if (!has_row(nav, row)) return ORACLES_HOME_STAY;
    nav->row = row;
    /* A language clicked is chosen; the row itself only takes the highlight. */
    if (row == ORACLES_ROW_LANGUAGE) return option >= 0 ? language_set(nav, option) : ORACLES_HOME_STAY;
    /* European ROM: Remove clicked removes the file; elsewhere on the row, the dialog opens. */
    if (row == ORACLES_ROW_EU_ROM) nav->eu_action = option == 1 && oracles_page_game_const(nav)->eu != ORACLES_ROM_NONE;
    return activate(nav, row);
}

/* ---- Display --------------------------------------------------------------------- */

const char *const oracles_display_labels[ORACLES_DISPLAY_ROWS] = { "Profile", "Quality", "Window", "View", "HUD", "Color correction", "Advanced", "Core",
                                                                   "Continuous transitions", "Vsync", "Neighbor workers", "Scaling", "Screen margin" };
const char *const oracles_display_view_names[3] = { "Near", "Medium", "Far" };
const char *const oracles_display_colour_choices[2] = { "Off", "On" };
const char *const oracles_display_hud_choices[2] = { "Original", "Full width" };
const char *const oracles_display_margin_choices[ORACLES_DISPLAY_MARGINS] = { "Off", "Auto", "Small", "Large" };
const char *const oracles_display_vsync_choices[3] = { "Auto", "On", "Off" };
const char *const oracles_display_core_choices[2] = { "Accurate (SameBoy)", "Fast (mGBA)" };
const char *const oracles_display_core_names[2] = { "Accurate", "Fast" };
const char *const oracles_display_scaling_choices[2] = { "Sharp", "Fill" };
const char *const oracles_display_quality_names[ORACLES_DISPLAY_QUALITIES + 1] = { "Low", "Medium", "High", "Max", "Custom" };
int oracles_display_transitions_apply(const OraclesHomeNav *nav) { return nav->display.profile == ORACLES_PROFILE_ENHANCED; }
int oracles_display_hud_applies(const OraclesHomeNav *nav)
{
    return (nav->in_game ? nav->playing : nav->display.profile) == ORACLES_PROFILE_ENHANCED;
}

int oracles_display_row_shown(const OraclesHomeNav *nav, unsigned row)
{
    return nav->advanced ? row >= ORACLES_DISPLAY_CORE && row < ORACLES_DISPLAY_ROWS : row <= ORACLES_DISPLAY_ADVANCED;
}

int oracles_display_auto_workers(const OraclesHomeNav *nav) { return nav->display.cores >= 4 ? 2 : 1; }

void oracles_display_workers_choice(const OraclesHomeNav *nav, int choice, char *out, size_t capacity)
{
    if (choice == 0) snprintf(out, capacity, "Auto \xc2\xb7 %d", oracles_display_auto_workers(nav));
    else snprintf(out, capacity, "%d", choice);
}

const char *oracles_display_section(const OraclesHomeNav *nav) { return nav->advanced ? "Display \xe2\x80\xba Advanced" : "Display"; }

int oracles_display_row_fixed(const OraclesHomeNav *nav, unsigned row)
{
    return (nav->in_game && (row == ORACLES_DISPLAY_CORE || row == ORACLES_DISPLAY_WORKERS || row == ORACLES_DISPLAY_QUALITY))
           || (row == ORACLES_DISPLAY_MARGIN && nav->display.margin_fixed);
}

/* Each quality's view, core (0 Accurate, 1 Fast) and workers (0 Auto): settings.c's, which tests/test_launcher_settings.c
 * checks these against. */
static const struct { int view, core, workers; } qualities[ORACLES_DISPLAY_QUALITIES] = {
    { 0, 1, 0 },   /* Low: near */
    { 1, 1, 0 },   /* Medium */
    { 2, 1, 0 },   /* High: far */
    { 2, 0, 2 },   /* Max */
};

int oracles_display_quality(const OraclesHomeNav *nav)
{
    for (int q = 0; q < ORACLES_DISPLAY_QUALITIES; q++)
        if (nav->display.view == qualities[q].view && nav->display.core == qualities[q].core && nav->display.workers == qualities[q].workers) return q;
    return ORACLES_DISPLAY_QUALITY_CUSTOM;
}

int oracles_display_quality_applies(const OraclesHomeNav *nav) { return nav->display.profile == ORACLES_PROFILE_ENHANCED && !nav->in_game; }

void oracles_display_quality_sets(int quality, char *out, size_t capacity)
{
    if (quality < 0 || quality >= ORACLES_DISPLAY_QUALITIES) snprintf(out, capacity, "By hand");
    else snprintf(out, capacity, "%s \xc2\xb7 %s", oracles_display_view_names[qualities[quality].view], oracles_display_core_names[qualities[quality].core]);
}

void oracles_display_quality_text(const OraclesHomeNav *nav, int short_form, char *out, size_t capacity)
{
    const int view = nav->display.view >= 0 && nav->display.view < 3 ? nav->display.view : 2, core = nav->display.core ? 1 : 0;
    const int workers = nav->display.workers;
    char on[16];
    if (workers) snprintf(on, sizeof on, "at %d", workers);
    else snprintf(on, sizeof on, "on auto");
    const char *const view_name = oracles_display_view_names[view], *const core_name = oracles_display_core_names[core];
    if (oracles_display_quality(nav) != ORACLES_DISPLAY_QUALITY_CUSTOM)
        snprintf(out, capacity, short_form ? "%s view, %s core, workers %s." : "%s view, %s core, neighbor workers %s.", view_name, core_name, on);
    else if (short_form) {
        char count[8];
        if (workers) snprintf(count, sizeof count, "%d", workers);
        else snprintf(count, sizeof count, "auto");
        snprintf(out, capacity, "Set by hand: %s, %s, workers %s.", view_name, core_name, count);
    } else snprintf(out, capacity, "Set by hand: %s view, %s core, neighbor workers %s.", view_name, core_name, on);
}

const char *oracles_display_quality_note(const OraclesHomeNav *nav, int short_form)
{
    if (oracles_display_quality(nav) == ORACLES_DISPLAY_QUALITY_CUSTOM) return "Pick a quality to set all three again.";
    return short_form ? "Changing one by hand makes it Custom." : "Changing one of them by hand makes it Custom.";
}

/* The quality `quality` applied: the view, the core and the workers it sets. */
static OraclesHomeCommand quality_set(OraclesHomeNav *nav, int quality)
{
    if (!oracles_display_quality_applies(nav) || quality < 0 || quality >= ORACLES_DISPLAY_QUALITIES || quality == oracles_display_quality(nav))
        return ORACLES_HOME_STAY;
    nav->display.view = qualities[quality].view;
    nav->display.core = qualities[quality].core;
    nav->display.workers = qualities[quality].workers;
    return ORACLES_HOME_STORE;
}

/* One quality lighter (`by` -1) or heavier (1), stopping at Low and at Max; from Custom, heavier is Low, lighter Max. */
static OraclesHomeCommand quality_step(OraclesHomeNav *nav, int by)
{
    const int quality = oracles_display_quality(nav);
    if (quality == ORACLES_DISPLAY_QUALITY_CUSTOM) return quality_set(nav, by > 0 ? 0 : ORACLES_DISPLAY_QUALITIES - 1);
    const int next = quality + by;
    return quality_set(nav, next < 0 ? 0 : next >= ORACLES_DISPLAY_QUALITIES ? ORACLES_DISPLAY_QUALITIES - 1 : next);
}

/* The shape the settings' aspect= names (an OraclesAspect: 1 16:9, 2 4:3), ORACLES_ENHANCED_ASPECTS for auto. */
static OraclesEnhancedAspect named_aspect(const OraclesHomeNav *nav)
{
    return nav->display.aspect == 2 ? ORACLES_ENHANCED_4_3 : nav->display.aspect == 1 ? ORACLES_ENHANCED_16_9 : ORACLES_ENHANCED_ASPECTS;
}

/* The display's size, 0x0 when unknown. */
static void screen_size(const OraclesHomeNav *nav, unsigned *w, unsigned *h)
{
    const int known = nav->display.screen_w > 0 && nav->display.screen_h > 0;
    *w = known ? (unsigned)nav->display.screen_w : 0u;
    *h = known ? (unsigned)nav->display.screen_h : 0u;
}

int oracles_display_screen_4_3(const OraclesHomeNav *nav)
{
    /* As the session decides it (oracles_session_view_4_3): the settings' shape, else the screen's. */
    const OraclesEnhancedAspect named = named_aspect(nav);
    if (named != ORACLES_ENHANCED_ASPECTS) return named == ORACLES_ENHANCED_4_3;
    unsigned w, h;
    screen_size(nav, &w, &h);
    return oracles_enhanced_screen_aspect(w, h) == ORACLES_ENHANCED_4_3;
}

/* The view's surface as a game started from here takes it (oracles_session_view_size): far fitted to the screen unless
 * far=fixed or aspect= names a shape. */
static OraclesEnhancedSize view_surface(const OraclesHomeNav *nav, int view)
{
    const int level = view >= 0 && view < ORACLES_ENHANCED_LEVELS ? view : ORACLES_ENHANCED_FAR;
    unsigned w, h;
    screen_size(nav, &w, &h);
    return oracles_enhanced_screen_view_size((OraclesEnhancedLevel)level, named_aspect(nav), nav->display.far_fixed, w, h);
}

void oracles_display_view_size(const OraclesHomeNav *nav, int view, char *out, size_t capacity)
{
    const OraclesEnhancedSize s = view_surface(nav, view);
    snprintf(out, capacity, "%u\xc3\x97%u", s.width, s.height);
}

void oracles_display_far_note(const OraclesHomeNav *nav, char *out, size_t capacity)
{
    unsigned w, h;
    screen_size(nav, &w, &h);
    const OraclesEnhancedFitting f = oracles_enhanced_far_fitting(w, h);
    if (named_aspect(nav) == ORACLES_ENHANCED_ASPECTS && !nav->display.far_fixed && f.fit == ORACLES_ENHANCED_FIT_FITTED)
        snprintf(out, capacity, "Far fits this screen: %u\xc3\x97%u at %u\xc3\x97.", f.size.width, f.size.height, f.scale);
    else if (capacity) out[0] = 0;
}

OraclesProfile oracles_display_played_profile(const OraclesHomeNav *nav)
{
    const int game = oracles_home_game(nav);
    if (game >= 0 && nav->games[game].usable) {
        const int played = oracles_page_profile(&nav->games[game], nav->display.profile);
        if (played >= 0) return (OraclesProfile)played;
    }
    return nav->display.profile;
}

void oracles_display_surface(const OraclesHomeNav *nav, int *width, int *height)
{
    if (oracles_display_played_profile(nav) == ORACLES_PROFILE_ENHANCED) {
        const OraclesEnhancedSize s = view_surface(nav, nav->display.view);
        *width = (int)s.width;
        *height = (int)s.height;
    } else {
        *width = 160;
        *height = 144;
    }
}

/* The largest whole scale of the surface w x h in room_w x room_h, at least 1. */
static int whole_scale(int room_w, int room_h, int w, int h)
{
    const int kx = room_w / w, ky = room_h / h;
    const int k = kx < ky ? kx : ky;
    return k > 1 ? k : 1;
}

int oracles_display_scale(const OraclesHomeNav *nav, int window)
{
    if (window < 3) return window + 2;
    int w, h;
    oracles_display_surface(nav, &w, &h);
    return whole_scale(nav->display.screen_w, nav->display.screen_h, w, h);
}

int oracles_display_fit(const OraclesHomeNav *nav)
{
    int w, h;
    oracles_display_surface(nav, &w, &h);
    return whole_scale(nav->display.room_w, nav->display.room_h, w, h);
}

int oracles_display_one_size_for(const OraclesHomeDisplay *display, int w, int h)
{
    const int fit = whole_scale(display->room_w, display->room_h, w, h);
    return display->fullscreen_only || (fit <= 2 && fit == whole_scale(display->screen_w, display->screen_h, w, h));
}

int oracles_display_one_size(const OraclesHomeNav *nav)
{
    int w, h;
    oracles_display_surface(nav, &w, &h);
    return oracles_display_one_size_for(&nav->display, w, h);
}

int oracles_display_fullscreen(const OraclesHomeNav *nav)
{
    return nav->display.window == 3 || nav->display.fullscreen_only || (nav->display.scaling && oracles_display_one_size(nav));
}

const char oracles_display_one_size_note[] = "One size only here; Scaling in Advanced can fill the screen.";

void oracles_display_reduced(const OraclesHomeNav *nav, char *out, size_t capacity)
{
    const int fit = oracles_display_fit(nav);
    if (oracles_display_one_size(nav)) snprintf(out, capacity, "%s", oracles_display_one_size_note);
    else if (nav->display.window < 3 && oracles_display_scale(nav, nav->display.window) > fit)
        snprintf(out, capacity, "Reduced to %d\xc3\x97 to fit this screen.", fit);
    else if (capacity) out[0] = 0;
}

void oracles_display_window_texts(const OraclesHomeNav *nav, int window, char *name, char *size, size_t capacity)
{
    int w, h;
    oracles_display_surface(nav, &w, &h);
    const int k = oracles_display_scale(nav, window);
    if (window < 3) {
        snprintf(name, capacity, "%d\xc3\x97", k);
        snprintf(size, capacity, "%d\xc3\x97%d", k * w, k * h);
    } else {
        snprintf(name, capacity, "Fullscreen");
        snprintf(size, capacity, "%d\xc3\x97 \xc2\xb7 %d\xc3\x97%d", k, k * w, k * h);
    }
}

const char oracles_display_window_note[] = "Each step is a whole multiple of the game's picture, so pixels stay sharp.";

/* The characters of a window's size on the surface w x h, as oracles_display_window_texts writes it. */
static int window_size_length(const OraclesHomeNav *nav, int window, int w, int h)
{
    char text[48];
    int k = window + 2;
    if (window >= 3) {
        const int kx = nav->display.screen_w / w, ky = nav->display.screen_h / h;
        k = kx < ky ? kx : ky;
        if (k < 1) k = 1;
        snprintf(text, sizeof text, "%d\xc3\x97 \xc2\xb7 %d\xc3\x97%d", k, k * w, k * h);
    } else snprintf(text, sizeof text, "%d\xc3\x97%d", k * w, k * h);
    int length = 0;
    for (const char *p = text; *p; p++) length += ((unsigned char)*p & 0xc0u) != 0x80u;
    return length;
}

int oracles_display_window_size_length(const OraclesHomeNav *nav, int window)
{
    int longest = window_size_length(nav, window, 160, 144);
    for (int v = 0; v < ORACLES_ENHANCED_LEVELS; v++) {
        const OraclesEnhancedSize s = view_surface(nav, v);
        const int length = window_size_length(nav, window, (int)s.width, (int)s.height);
        if (length > longest) longest = length;
    }
    return longest;
}

/* The scale Fill gives the surface w x h on the screen: the largest that keeps its proportions. */
static float fill_scale(const OraclesHomeNav *nav, int w, int h)
{
    const float kx = (float)nav->display.screen_w / (float)w, ky = (float)nav->display.screen_h / (float)h;
    return kx < ky ? kx : ky;
}

void oracles_display_scaling_size(const OraclesHomeNav *nav, int scaling, char *out, size_t capacity)
{
    int w, h;
    oracles_display_surface(nav, &w, &h);
    if (scaling) {
        const float k = fill_scale(nav, w, h);
        snprintf(out, capacity, "%d\xc3\x97%d", (int)floorf(k * (float)w + 0.5f), (int)floorf(k * (float)h + 0.5f));
    } else {
        const int k = oracles_display_scale(nav, 3);
        snprintf(out, capacity, "%d\xc3\x97%d", k * w, k * h);
    }
}

void oracles_display_diagram(const OraclesHomeNav *nav, float box_w, float box_h, float *w, float *h, char *label, size_t capacity)
{
    int sw, sh;
    oracles_display_surface(nav, &sw, &sh);
    int k = oracles_display_scale(nav, nav->display.window);
    if (nav->display.window < 3 && k > oracles_display_fit(nav)) k = oracles_display_fit(nav);   /* as Play reduces it */
    /* Fill takes the screen in fullscreen only. */
    const int fill = nav->display.scaling && oracles_display_fullscreen(nav);
    const float f = fill_scale(nav, sw, sh);
    const float image_w = fill ? f * (float)sw : (float)(k * sw), image_h = fill ? f * (float)sh : (float)(k * sh);
    /* The window on the screen drawn to the box's scale, rounded to the pixel, and kept inside. */
    *w = floorf(image_w / (float)nav->display.screen_w * box_w + 0.5f);
    *h = floorf(image_h / (float)nav->display.screen_h * box_h + 0.5f);
    if (*w > box_w) *w = box_w;
    if (*h > box_h) *h = box_h;
    snprintf(label, capacity, "%d\xc3\x97%d on a %d\xc3\x97%d screen", (int)floorf(image_w + 0.5f), (int)floorf(image_h + 0.5f),
             nav->display.screen_w, nav->display.screen_h);
}

const char *oracles_display_profile_note(const OraclesHomeNav *nav)
{
    if (oracles_display_played_profile(nav) != nav->display.profile) return "Enhanced needs an original ROM: this game plays in Faithful.";
    return profile_notes[nav->display.profile == ORACLES_PROFILE_ENHANCED];
}

const char *oracles_display_colour_explanation(const OraclesHomeNav *nav, int four_three)
{
    static const char *const texts[2][2] = {
        { "On: colors as the Game Boy Color screen showed them. Off: the raw palette.",
          "On: colors as the Game Boy Color screen showed them. Off: the raw palette. Also F2 in game." },
        { "On: as the Game Boy Color showed it.", "On: as the Game Boy Color showed it. Also F2." },
    };
    return texts[four_three != 0][oracles_home_keys_shown(nav)];
}

const char *oracles_display_explanation(unsigned row)
{
    switch (row) {
        case ORACLES_DISPLAY_VIEW: return "How much of the world the Enhanced view shows; farther asks more of the device.";
        case ORACLES_DISPLAY_HUD: return "Full width paints the status bar's color across the screen; Original keeps it as the game draws it.";
        case ORACLES_DISPLAY_TRANSITIONS: return "Rooms scroll into one another instead of stopping at each edge, Link swimming too.";
#ifdef __ANDROID__
        case ORACLES_DISPLAY_VSYNC: return "Auto: off on Android, which composes the frames itself.";
#else
        case ORACLES_DISPLAY_VSYNC: return "Auto: follows your display when it is close to 60 Hz.";
#endif
        case ORACLES_DISPLAY_CORE: return "Accurate: the reference. Fast: lighter, for small devices.";
        case ORACLES_DISPLAY_WORKERS:
            return "Rooms around you are prepared by background workers. Two fill the view faster after a warp or a load, using one more processor core.";
        case ORACLES_DISPLAY_SCALING: return "Sharp: whole steps, every pixel exact. Fill: the whole screen, pixels slightly softer.";
        case ORACLES_DISPLAY_MARGIN:
            return "Keeps the picture clear of screen edges a device hides. Auto follows what the system reports; Small and Large keep a fixed margin at "
                   "the top and bottom.";
        case ORACLES_DISPLAY_ADVANCED: return "Core, transitions, Vsync, workers, scaling, margin.";
        default: return "";   /* Quality's says what is in effect: oracles_display_quality_text */
    }
}

/* The value of a row, set to `value` (wrapped to the row's choices); the view and the transitions do not change in
 * Faithful, nor the HUD in Faithful or, from the pause, in a game running in Faithful, nor the core and the workers in a game, nor the window at one size.  Advanced has no value, nor Quality, which the
 * view, the core and the workers make (quality_set). */
static OraclesHomeCommand display_set(OraclesHomeNav *nav, unsigned row, int value)
{
    static const int counts[ORACLES_DISPLAY_ROWS] = { ORACLES_PROFILES, 0, 4, 3, 2, 2, 0, 2, 2, 3, 3, 2, ORACLES_DISPLAY_MARGINS };
    int profile = (int)nav->display.profile;
    int *fields[ORACLES_DISPLAY_ROWS] = { &profile, NULL, &nav->display.window, &nav->display.view, &nav->display.hud, &nav->display.colour, NULL,
                                          &nav->display.core, &nav->display.transitions, &nav->display.vsync, &nav->display.workers, &nav->display.scaling,
                                          &nav->display.margin };
    const int enhanced_only = row == ORACLES_DISPLAY_TRANSITIONS || row == ORACLES_DISPLAY_VIEW;
    if (row >= ORACLES_DISPLAY_ROWS || !fields[row] || (enhanced_only && !oracles_display_transitions_apply(nav))
        || (row == ORACLES_DISPLAY_HUD && !oracles_display_hud_applies(nav))
        || oracles_display_row_fixed(nav, row) || (row == ORACLES_DISPLAY_WINDOW && oracles_display_one_size(nav)))
        return ORACLES_HOME_STAY;
    value = (value % counts[row] + counts[row]) % counts[row];
    if (*fields[row] == value) return ORACLES_HOME_STAY;
    *fields[row] = value;
    nav->display.profile = (OraclesProfile)profile;
    return ORACLES_HOME_STORE;
}

static int display_value(const OraclesHomeNav *nav, unsigned row)
{
    const int values[ORACLES_DISPLAY_ROWS] = { (int)nav->display.profile, 0, nav->display.window, nav->display.view, nav->display.hud, nav->display.colour,
                                               0, nav->display.core, nav->display.transitions, nav->display.vsync, nav->display.workers,
                                               nav->display.scaling, nav->display.margin };
    return row < ORACLES_DISPLAY_ROWS ? values[row] : 0;
}

/* Advanced opens on the first of its rows that changes: Core on the home screen; from a game, whose core is fixed,
 * Continuous transitions in Enhanced chosen, else Vsync.  Closed, Display shows Advanced highlighted. */
static void display_advanced(OraclesHomeNav *nav, int open)
{
    nav->advanced = open;
    nav->row = ORACLES_DISPLAY_ADVANCED;
    for (unsigned r = ORACLES_DISPLAY_CORE; open && r < ORACLES_DISPLAY_ROWS; r++) {
        if (oracles_display_row_fixed(nav, r) || (r == ORACLES_DISPLAY_TRANSITIONS && !oracles_display_transitions_apply(nav))) continue;
        nav->row = r;
        break;
    }
}

OraclesHomeCommand oracles_display_act(OraclesHomeNav *nav, OraclesHomeAction action)
{
    /* The rows shown, from `first`, `count` of them: up and down go round them. */
    const unsigned first = nav->advanced ? ORACLES_DISPLAY_CORE : ORACLES_DISPLAY_PROFILE;
    const unsigned count = nav->advanced ? ORACLES_DISPLAY_ROWS - ORACLES_DISPLAY_CORE : ORACLES_DISPLAY_ADVANCED + 1u;
    const unsigned row = oracles_display_row_shown(nav, nav->row) ? nav->row : first;
    switch (action) {
        case ORACLES_HOME_UP: nav->row = first + (row - first + count - 1u) % count; return ORACLES_HOME_STAY;
        case ORACLES_HOME_DOWN: nav->row = first + (row - first + 1u) % count; return ORACLES_HOME_STAY;
        case ORACLES_HOME_LEFT:
            if (row == ORACLES_DISPLAY_QUALITY) return quality_step(nav, -1);
            return row == ORACLES_DISPLAY_ADVANCED ? ORACLES_HOME_STAY : display_set(nav, row, display_value(nav, row) - 1);
        case ORACLES_HOME_RIGHT:
        case ORACLES_HOME_OK:
            if (row == ORACLES_DISPLAY_ADVANCED) { display_advanced(nav, 1); return ORACLES_HOME_STAY; }
            if (row == ORACLES_DISPLAY_QUALITY) return quality_step(nav, 1);
            return display_set(nav, row, display_value(nav, row) + 1);
        case ORACLES_HOME_BACK:
            if (nav->advanced) { display_advanced(nav, 0); return ORACLES_HOME_STAY; }
            /* Back to the menu Display was opened from, on Display. */
            nav->screen = nav->in_game ? ORACLES_SCREEN_PAUSE : ORACLES_SCREEN_HOME;
            nav->focus = nav->in_game ? 4 : 3;
            return ORACLES_HOME_STAY;
    }
    return ORACLES_HOME_STAY;
}

OraclesHomeCommand oracles_display_click(OraclesHomeNav *nav, unsigned row, int option)
{
    if (!oracles_display_row_shown(nav, row)) return ORACLES_HOME_STAY;
    if (row == ORACLES_DISPLAY_ADVANCED) { display_advanced(nav, 1); return ORACLES_HOME_STAY; }
    nav->row = row;
    if (option < 0) return ORACLES_HOME_STAY;
    return row == ORACLES_DISPLAY_QUALITY ? quality_set(nav, option) : display_set(nav, row, option);
}
