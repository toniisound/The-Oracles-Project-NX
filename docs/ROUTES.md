# Routes, the harness and the suite

A route is the list of a session's inputs, anchored to the launcher's frame counter, which starts at zero at launch, boot ROM included. The core being deterministic (a fixed initial state, the same ROM and the same save), replaying a route reproduces the session frame for frame. Routes are how the port proves a change: the differential harness replays them without a window, and the suite holds their figures.

## The format

Text, one item per line, ASCII. Empty lines and lines starting with `#` are ignored.

```
oracles-route 2
game ages
rom_sha1 880374fb978b18af4aa529e2e32f7ffb4d7dd2f4
sram_sha1 none
core joypad-bouncing-off
inputs
0 keys 00
480 keys 80
484 keys 00
```

The header, before the `inputs` line, is a list of `name value` pairs:

| Name | Value |
| --- | --- |
| `oracles-route` | the format number: `8` is written when a cheat changed during the session (a `cheat` line), `7` when the route's options name a cheat, `6` when they name the transitions' diagonal, `5` when they name the swim's hold, `4` when they name a language, `3` when it names mods, `2` otherwise; `1` (the `keys` verb alone) is read too |
| `game` | `ages` or `seasons` |
| `rom_sha1` | the SHA-1 of the ROM image played, in lower-case hexadecimal; a fan game's is its patched image's |
| `sram_sha1` | the SHA-1 of the starting SRAM, or `none`; that SRAM is the file `ROUTE.sram` beside the route |
| `options` | optional: the session's gameplay options, comma-separated; today `continuous-transitions`, `continuous-swim`, which always follows it (the continuous transitions through a swim too), `continuous-diagonal`, which follows it too (a diagonal held carrying Link across the transitions' axis), `language-<code>` (`language-fr`), the language the session played in, whose texts' lengths pace the dialogues, `swim-hold`, the swim's strokes a held button started ([`PLAYING.md`](PLAYING.md)), which no key of the route shows, and `cheat-<name>` for each cheat on from the start (`cheat-no-damage`, `cheat-ammo`, `cheat-rupees`), whose answers no key shows either, a `cheat` line changing one later. The launcher and the harness replay the route with them, else it would diverge; a route in a language replays only in it (with its European ROM), one without in any, as the hand-written routes of the languages do; the launcher plays the transitions' diagonal, the swim's hold and the cheats as the route says, whatever its settings, and keeps the transitions and their swim when the route does not name them, saying that the replay diverges; the harness, a test tool, keeps every option it is given besides the route's, so that a row may replay a route with an option it was not recorded with (`ages/mermaid-hold.route` with `--continuous-swim --continuous-diagonal`), the game its own from the first case the option changes; absent, none |
| `core` | optional: how the core ran, comma-separated: `joypad-bouncing-off`, which the launcher writes in every route it records, and `mgba` for a route recorded on mGBA (`core joypad-bouncing-off,mgba`), which the launcher and the harness replay on mGBA unless the replay names a core (`--core`), a route without it on SameBoy; any other value stops the reader, like an unknown verb. Absent, the route was recorded with SameBoy's emulation of joypad bouncing, which the launcher and the harness turn back on to replay it; that emulation makes the state depend on the audio sample rate, so such a route's replay may differ from its session. The repository's routes recorded so were rewritten without it by `tools/debounce_route.py`: in the frames the game read the keys (the guest counts its input poll's writes, `--keys-read`), their keys are those it read on SameBoy, bouncing included, and in the others those recorded; each replays the same game without the bouncing, its live-state fingerprints compared frame for frame for every set of options its rows run with and with the Enhanced view, and keeps its recording as `#recorded` comments, which readers skip. A route whose rows ran two games (with and without `--continuous-transitions`) became two routes, the second named `-continuous` |
| `mods` | optional: the session's mods, `NAME@SHA1,NAME@SHA1…` in the order of their names; the route replays only with those mods (`--mods`, in any order). Their starting `mod.storage` is the file `ROUTE.store` beside the route; absent, empty. Absent, no mod; a route recorded without mods does not replay with one |
| `store_sha1` | optional, with `mods`: the SHA-1 of `ROUTE.store`, or `none` without a file; the launcher and the harness refuse the replay when the file does not match |

An unknown header name is ignored when reading, so that later formats can add names without breaking readers; a format number changes when events gain verbs.

The events, after `inputs`: `<frame> keys <mask>`. The mask, two hexadecimal digits, is the state of the eight keys from that frame on, in the Game Boy joypad's bits: `01` right, `02` left, `04` up, `08` down, `10` A, `20` B, `40` Select, `80` Start. An event holds until the next; the route ends at its last event, after which the player takes over.

Format 2 adds the verbs that change the game's state, for the item hotkeys:

- `<frame> equip <slot>=<item> <slot>=<item> [<variant>=<value>]` notes an exchange of two inventory slots as the write point applied it at that frame: the slots are `b`, `a` and `s0` to `s15`, each with the item it held before the exchange, in hexadecimal; the variant written, if any, is `satchel`, `shooter` or `harp`. `1840 equip b=05 s3=0a`; `1900 equip harp=02` for a variant alone. On replay, the launcher and the harness check that both slots hold the items noted, then apply the exchange through the same transaction of the bus; a difference is a divergence of the route.
- `<frame> use <b|a> <item>` notes the start of a press the `use` mode simulated; it is not replayed, the press being in the `keys` mask already, and tells the harness which presses are simulated.

Frames never decrease from one line to the next and strictly increase for a given verb; at the same frame, lines apply in the file's order. An unknown verb stops the reading with a message: besides `keys`, a verb changes the game's state and cannot be ignored. Format 3 changes only the header: it is the format of a route with a `mods` line, which a format 2 reader would ignore and replay without the mods. Format 4 too: it is the format of a route whose options name a language (`language-fr`), which a reader before it would replay in English; a route of an earlier format that names one is refused. And format 5: that of a route whose options name `swim-hold`, which a reader before it would replay without the strokes the hold started; a route of an earlier format that names it is refused. Format 6 is that of a route whose options name `continuous-diagonal`, which a reader before it would replay with Link held across the axis of each diagonal crossing; a route of an earlier format that names it is refused. Format 7 is that of a route whose options name a cheat, which a reader before it would replay without; a route of an earlier format that names one is refused, and a cheat the launcher or the harness does not know stops the replay, as an unknown verb does.

Format 8 adds a verb, for the cheats the pause menu changes during a session:

- `<frame> cheat <name> on|off` turns a cheat on or off from that frame on, before it runs (`6120 cheat rupees on`), the name as the options write it after `cheat-`; those the session started with stay in the options. A cheat's lines strictly increase in frame, and two cheats may change at the same frame. The launcher writes a session's route in the format of its header, and raises it to 8 in place at its first `cheat` line. A cheat the launcher or the harness does not know refuses the replay, as in the options.

## Recording and replaying

`the-oracles-project --record ROUTE` writes the session's route: a copy of the starting SRAM in `ROUTE.sram` (absent when the session starts without a save), with mods their storage's in `ROUTE.store`, the header, then an event at each change of the mask, and `ROUTE.session.tsv`, the session's own fingerprints. The session saves normally; the copy does not move, and it is what the header names. `the-oracles-project --play ROUTE` loads `ROUTE.sram` as the starting SRAM, replays the route in the player's place until its last event, then gives the controls back, and never writes the session's save file: a replay can be repeated at will.

Both options together extend a route without playing its beginning again: `--play INPUT --record OUTPUT` replays the prefix, records the effective inputs from frame 0, then goes on with the player's. `OUTPUT.sram` takes `INPUT`'s starting snapshot, and the normal save stays untouched. A route that carries inventory exchanges cannot be extended (they would be lost), and the two paths must differ. A route whose ROM or starting SRAM does not match is refused. Loading a savestate (F7) stops a replay or a recording. An extension carries the replayed route's `cheat` lines. A session the home screen started has the pause menu, whose Cheats changes a cheat from the frame the game resumes on: recorded (`--record` without `--rom`), the route gets its `cheat` line there; a replay has no pause menu, `--play` being `--rom`'s. Without `--rom`, `--record ROUTE` records the first session the home screen starts.

A fan game's route names its patched image; it records and replays with `--rom BASE --patch PATCH.bps` as it is played, and the harness takes the same `--patch`.

## The harness

`oracles-harness` replays a route without a window and writes a fingerprint per frame (framebuffer, live WRAM without the dead stack bytes, HRAM, OAM, VRAM), then compares two runs:

```bash
build/oracles-harness --rom ROM --route R.route --hooks off --out off.tsv
build/oracles-harness --rom ROM --route R.route --hooks on  --out on.tsv
build/oracles-harness --compare off.tsv on.tsv
build/oracles-harness --compare-session R.route.session.tsv on.tsv
build/oracles-harness --rom ROM --route R.route --corrupt-thread1-at 5000 --out bad.tsv
```

`--hooks off` runs without the guest bus, so that a run with hooks can be compared with one without. `--compare-session` compares a recorded session with the replay of its route: the same game (live WRAM, HRAM, OAM, VRAM) over the frames the route covers, and the screen, which differs where the session had colour correction on (replay with `--colour-correction on` to compare it). `--sample-rate HZ` runs the core's audio at that rate (0, none, by default): a route recorded without joypad bouncing gives the same state whatever the rate. `--corrupt-thread1-at FRAME` flips a bit of the game thread's saved resume address at that frame: the live state must diverge from that frame and the framebuffer two frames later, which is the harness's sensitivity test. `--patch PATCH.bps` applies a fan game's patch to `--rom`.

The checks, each writing its figures:

| Option | What it checks |
| --- | --- |
| `--render-check DIR` | the native renderer against the core, frame by frame, by class of frame; samples of each class in `DIR`; exit 1 on a mismatch in a compared class, an expected class left empty (`--render-expect`), or a ceiling passed (commits during the scan, frames not compared, lines with a late scroll write) |
| `--ghost-check DIR` | the ghost instance against every scrolling transition: replayed from a snapshot taken `--ghost-lead` frames before (30 by default), its layout compared tile for tile with the real entry's; `--ghost-trace` records the addresses the substitutions read and counts those outside the cache key |
| `--enhanced-check DIR` | the Enhanced surface of every frame, composed and hashed: camera jumps, Link's sprite as the frame draws it going back on the screen against the step before (`enhanced.link_backsteps`, `_y`), the window's place against the scroll registers, seams, the terrain shown against the layout the game commits at each entry (black or equal, never different), the neighbours' objects and sprites, the world frames the drawn-back view showed only the normal band, centred in its own (`enhanced.normal_band_frames`, an interior); a room's load drawn under the HUD band as in play (its frames so drawn, and those left framed whole for showing one colour, `enhanced.load_band_frames`, `enhanced.load_blank_frames`; the steps from a world frame to such a load or back, those where the band's sixteen lines changed and those where the play area moved, `enhanced.load_band_steps`, `enhanced.load_band_changes`, `enhanced.load_area_moves`); `--enhanced-threaded` runs the ghost in its thread as in play (`--enhanced-ghosts 2`: two ghosts, each with its run, as the launcher's `ghosts=2`; the synchronous ghost stays one), `--zoom-out` the drawn-back view, `--view near\|medium\|far` and `--aspect 16:9\|4:3` the view's level and the screen's shape (`docs/PLAYING.md`; far is `--zoom-out`), `--screen WxH` a screen of that size (`--screen 1280x960`): its shape, unless `--aspect` names one, and the far view fitted to it by the launcher's rule (`far=screen`; the harness has no screen of its own, the size is always named), which the harness's log says (`harness: the far view fitted to the 1280x960 screen: 426x320 at 3x, ...`), `--enhanced-camera 1\|2` the camera's profile, `--hud original\|full` the status bar as the game draws it (the default, which every row ran with before it) or drawn full width ([`PLAYING.md`](PLAYING.md), `hud=`), with the world frames whose bands took the bar's colour and the gutters' lines left dark in every world frame with the bar (`enhanced.hud_full_frames`, `enhanced.hud_lines_kept`), `--enhanced-reload-at F` saves and reloads a state, `--surface-at FRAME FILE.ppm` writes a composed frame |
| `--neighbour-objects-check DIR` | the objects of every neighbour, ghost against the real entry |
| `--hotkeys-check DIR` | the item hotkeys: the inventory frame by frame; a route's exchanges are applied either way |
| `--hotkeys-live use\|equip` | the live policy with scripted keys (`--hotkey-slot`, `--hotkey-press`) over the route's inputs, and `--hotkeys-record` the format 2 route of that session |
| `--mods DIR`, `--mod-trace FILE` | the route's mods, and their state after every frame, to `--compare` with the session's `ROUTE.mod.tsv` |
| `--continuous-diagonal` | the transitions' diagonal ([`PLAYING.md`](PLAYING.md)), on by itself for a route that names it; the frames it carried Link across the axis, and those a collision held him, in the summary (`transitions.carried_frames`, `transitions.carry_withheld_frames`), and with the transitions the transitions Link swam into and came out of on land, at the first frame the game runs after them, where the straight path off which the diagonal carried him stays in water (`transitions.swim_dropped`); the Enhanced check counts the scrolls entered walking diagonally and their frames with Link standing across the axis two frames running (`enhanced.scrolls_diagonal`, `enhanced.scroll_across_stalled`) |
| `--swim-hold` | the swim's hold ([`PLAYING.md`](PLAYING.md)), on by itself for a route that names it; the strokes it started in the summary (`swim.mermaid_strokes`, `swim.flippers_strokes`) |
| `--cheats LIST` | the cheats named ([`PLAYING.md`](PLAYING.md)), with those a route names, which are on by themselves, a route's `cheat` lines changing them at their frames (the hooks on); refused for an image whose profile does not have them; in the summary, with the option or `cheat` lines, the hits whose damage was cancelled and that damage in quarters of a heart (`cheats.damage_cancelled`, `cheats.health_kept`), the seeds, bombs and Bombchus kept (`cheats.ammo_kept`), the payments that cost nothing and their rupees (`cheats.payments_cancelled`, `cheats.rupees_kept`), and the payments of a mod left to the game (`cheats.mod_payments`) |
| `--language CODE`, `--eu-rom FILE\|DIR`, `--language-pack FILE` | the route replayed in a language of the European ROM ([`PLAYING.md`](PLAYING.md)), the texts the game looked up in the summary (`language.texts_served`, `language.texts_english`, `language.texts_missing`); the hooks on |
| `--dump-at FRAME FILE` | the live state at the end of that frame, dead memory zeroed, to name the bytes two replays differ by |
| `--summary FILE` | every check's figures as `key=value` lines (`render.<class>.mismatches`, `ghost.different`, `enhanced.camera_jumps`, `enhanced.run_hash`...) |
| `--sram-out FILE` | the cartridge RAM at the end of the replay, raw, as a `.sav`: a save the game made on one core, read on the other |
| `--positions FILE` | the group, the room and Link's position after every frame, to compare two replays' rooms with `tools/compare_positions.py` (two cores, or two versions) |
| `--keys-read FILE` | the keys the game read in every frame, in a route's order, and whether it read them in that frame (`tools/debounce_route.py`) |
| `--frame-times FILE` | each frame's phases, timed on the monotonic clock in microseconds: the core with its hooks, the frame source, the presentation, and the frame's end, where the Enhanced check composes; to find where a frame's time goes, on a desktop or under an emulator |

A run with `--out` and one without `--enhanced-check` give the same fingerprints: the presentation writes nothing into the live instance.

## The diagnostics panel

`the-oracles-project --diagnostics` arms the hooks of [`GAME_HOOKS.md`](GAME_HOOKS.md), section 6, on the running game and prints the events as they happen: room entries and initialisations, objects created, transitions, warps, texts and choices, flag writes, treasures, menus, saves, key changes. At exit the launcher reports the work per frame (core, hooks, audio, presentation) and whether no frame's work, presentation excluded, went past a frame's period. `the-oracles-project --native-renderer` shows the native renderer's image and reports its comparison at exit.

## The suite

`tools/check_routes.py` replays the repository's routes through the harness and holds their figures, on SameBoy unless `--core mgba` is given; `ctest` runs it on each core, as `oracles-routes` and `oracles-routes-mgba`:

```bash
ORACLES_ROM_DIR=/path/to/roms ctest --test-dir build -R '^oracles-routes$' --output-on-failure      # SameBoy; oracles-routes-mgba for mGBA
ORACLES_ROM_DIR=/path/to/roms python3 tools/check_routes.py --harness build/oracles-harness
python3 tools/check_routes.py --rom-dir /path/to/roms --only seasons/lost-woods     # the rows of one route
python3 tools/check_routes.py --rom-dir /path/to/roms --update                      # after a change of behaviour that is meant
python3 tools/check_routes.py --rom-dir /path/to/roms --core mgba                   # the rows of mGBA and of both cores
```

Every `checks.tsv` under `routes/` holds the rows of the routes of its own directory, one directory per game (`ages/`, `seasons/`, and one per fan game). A row is `route mode options expect run_hash core`, tab-separated:

- **route**, a file of the manifest's directory; the identity of a route is its path relative to `routes/` (`ages/intro.route`), from which its labels (`ages-intro-faithful`) and working directories derive;
- **mode**: `faithful` (the native renderer against the core with colour correction off, and the ghost against every transition with its reads traced against the cache key), `faithful-cc` (the renderer under colour correction), `enhanced` (the Enhanced surface of every frame, camera profile 2, the ghost synchronous: reproducible and hashed), `enhanced-threaded` (the same with the ghost in its thread, as played: the figures that do not depend on timing), `enhanced-zoom` and `enhanced-zoom-threaded` (the same two in the drawn-back view);
- **options**: extra harness flags, `-` for none; a `--mods` directory is resolved against the repository; a row with `--language` and no `--eu-rom` gets the ROM directory as its `--eu-rom`, where the harness finds the European ROM by its SHA-1 (such rows are private: the CI has no European ROM); a row of the view's other sizes carries `--view` and `--aspect`, or `--view far` and `--screen` for the far view fitted to a screen, and its live state is compared with the faithful row of its route, as is a row with `--hud full`, presentation alone;
- **expect**: `;`-separated `key=value` or `key<=value` over the harness's summary, on top of the rules every row of its mode must hold (no renderer mismatch, no wrong terrain, no wrong room, no ghost read outside the key and its exemptions, no horizontal camera jump, no misplaced window...), and for a row at another of the view's sizes (`--view`) the synchronous ghost in the right room and the band filled at least once; a rule prefixed with a core (`mgba:enhanced.black_pixels_mean<=149`) holds on that core only, so that a row of both cores holds each core's measure;
- **run_hash**: the Enhanced run hash the row last produced, `-` when the mode has none; each core has its own, so that a row of both cores in a hashed mode holds `sameboy:HASH,mgba:HASH`, and `--update` rewrites the hash of the core replayed;
- **core**: the core the row replays on, `sameboy` or `mgba`, `-` for both.

The routes are recorded on SameBoy. Replayed on mGBA, a few part from the game they recorded: where the game's logic of a frame ends on one side of a vblank on one core and on the other side on the other, a key is read, or a game frame runs, a frame apart, and the game goes elsewhere. Such a route keeps its rows for SameBoy and gets its own for mGBA, with floors from mGBA's replay of it; a comment above them names where it parts and why. On mGBA every `faithful` row is also replayed on SameBoy and the rooms both replays go through are compared (`tools/compare_positions.py`): a row of both cores must go through the same rooms to the end, and a row of mGBA's holds a floor on the rooms in common before it parts (`cores.rooms_same`), so that a parting that comes sooner fails. A line whose SCX or SCY changes while it is drawn is compared from its second tile on: mGBA starts drawing a line sooner than SameBoy, and which of the first pixels take the write is each core's fetch timing (`render.first_tile_lines`).

For each route the live-state fingerprints of its `faithful` row and of each Enhanced row are compared: the presentation writes nothing into the live instance, on every route and in the mode that is played. A route recorded since joypad bouncing was cut has its faithful row replayed again at 48 kHz, and the state must be the same.

The ROMs are found by the SHA-1 of each route's header in `ORACLES_ROM_DIR`; without that variable the test is skipped (the CI has no ROM). A fan game's route replays from the game's BPS patch placed beside the base ROM it applies to in that directory, the suite applying it as the launcher does, or from the patched image placed there. `--jobs N` runs N rows in parallel (eight by default); `--only TEXT` runs the rows whose route, label or mode contains the text, or, for a text that starts with `--`, whose options contain it (`--only="--view far --aspect 4:3"`, the `=` keeping it a value); `--threaded-args "--enhanced-ghosts 2"` adds harness flags to the rows whose ghost runs in its thread.

**When a run hash may change.** The replay is deterministic: a changed run hash is a change of what the Enhanced view shows on that route, and it fails until `--update` rewrites it in its manifest. Run `--update` only when the change is the one you meant, and say in the commit which rows moved and why. A ceiling on a row's expectation (the black a synchronous row shows) is the value last measured: it may be lowered by the change that lowers it, never raised without saying why.

## Accompanying a change of the Enhanced view with a route

A change of what the Enhanced view shows is proven on a route that exercises it:

1. Play the case with the launcher and record it: `build/the-oracles-project --rom ROM --enhanced [options] --record routes/<game>/<what-it-covers>.route` (a new game, or from a save that reaches the case quickly; the save becomes `ROUTE.sram`).
2. Add its rows to the directory's `checks.tsv`: at least `faithful` and `enhanced` (and `enhanced-zoom` if the drawn-back view is concerned), with the expectations that name the case (`enhanced.transitions_black=0`...), and run `tools/check_routes.py --only <game>/<route> --update` once to record its run hashes.
3. Replay the whole suite: every other row must keep its figures and its run hash, or the commit says which moved and why.
