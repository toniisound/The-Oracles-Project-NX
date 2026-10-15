# The Oracles Project NX

A Nintendo Switch homebrew port (`.nro`) of [The Oracles Project](https://github.com/thibautbus/top) by
**thibautbus**: a faithful, moddable port of **Oracle of Ages** and **Oracle of Seasons**. Your own ROM runs
unmodified in an emulator core (mGBA's, or SameBoy), and a native host around it presents the game:

- **Faithful** (160×144) or **Enhanced**: a widescreen view with the neighbouring rooms, a smooth camera and the
  status bar in a band of its own
- **Languages**: Spanish, French, German, Italian and European English, taken from your European ROM
- **Gameplay options**: continuous transitions, item hotkeys, hold-to-swim and a drawn-back view
- **Lua mods** and **fan games** (Gifts of Kinomi, Moonrise Regalia, Temple of Seasons) from their BPS patches
- Save states, remappable controls, full screen in handheld (720p) and docked (1080p)

> **No game files are included.** You need your own Oracle of Ages / Oracle of Seasons (US) ROMs. This project is not
> affiliated with or endorsed by Nintendo.

## Credits

- **[thibautbus](https://github.com/thibautbus)** and contributors: [The Oracles Project](https://github.com/thibautbus/top), the port itself, its launcher, the Enhanced view, languages, mods and fan-game support. All the real work is theirs.
- **[Stewmath](https://github.com/Stewmath)** and contributors: [oracles-disasm](https://github.com/Stewmath/oracles-disasm), the disassembly the port's tables come from.
- **[mGBA](https://mgba.io)** (Vicki Pfau and contributors) and **[SameBoy](https://sameboy.github.io)** (Lior Halphon): the emulator cores.
- **[devkitPro](https://devkitpro.org)**: devkitA64, libnx and the Switch branch of **[SDL](https://libsdl.org)** 3.
- **[Lua](https://www.lua.org)**: the mods' runtime.
- The fan games' authors: ZerotoKoops, Stewmath, Gamma and Ralfaro (Gifts of Kinomi), PontiusStone (Moonrise Regalia), Jay and Ralfaro (Temple of Seasons).
- **Toniisound**: Nintendo Switch port.

## License

MIT, the same as The Oracles Project. See [LICENSE](LICENSE). The bundled libraries keep their own licences, which
ship with the release. The upstream README is kept as [README-original.md](README-original.md).

## Technical overview

The Switch build is the upstream launcher with a libnx target. SDL 3 is built from source (devkitPro's
`switch-sdl-3.4` branch) and draws through EGL / OpenGL ES 2 (switch-mesa). The launcher runs on a thread with an
8 MiB stack, the neighbours' workers are pinned to cores 1 and 2 while the game runs on core 0, and the CPU is raised
to 1785 MHz while the app is open (restored on exit). Instead of a file dialog, the app finds your ROMs and patches
on the SD card by their SHA-1.

## Building (WSL or Linux with devkitPro)

```
sudo dkp-pacman -S --needed switch-dev switch-cmake switch-pkg-config switch-mesa switch-libdrm_nouveau
sudo apt install ninja-build git zip
git clone https://github.com/toniisound/the-oracles-project-nx.git
cd the-oracles-project-nx
./build_switch.sh
```

The result is `build-switch/the-oracles-project-nx.nro`, and `build-switch/the-oracles-project-nx.zip`, ready for a
release: the SD card's `switch/the-oracles-project-nx/` folder with the `.nro`, the empty `roms/`, `patches/` and
`mods/` folders and the licences. The first build downloads and compiles SDL 3.

| Option | Effect |
| --- | --- |
| `-DORACLES_SWITCH_OVERCLOCK=OFF` | Keep the CPU at its normal clock (for emulators without the clock service) |
| `-DORACLES_DEFAULT_CORE=sameboy` | SameBoy, the accurate core, by default instead of mGBA |

Options go after the script, e.g. `./build_switch.sh -DORACLES_SWITCH_OVERCLOCK=OFF`.

## Installing

Unzip `the-oracles-project-nx.zip` at the root of the SD card, then copy your ROMs into `roms/`:

```
sdmc:/switch/the-oracles-project-nx/
├── the-oracles-project-nx.nro
├── roms/
│   ├── Oracle of Ages (USA).gbc         required to play Ages
│   ├── Oracle of Seasons (USA).gbc      required to play Seasons
│   ├── Oracle of Ages (Europe).gbc      optional, for the languages
│   └── Oracle of Seasons (Europe).gbc   optional, for the languages
├── patches/                             optional, the fan games' .bps patches
└── mods/                                optional, Lua mods, one folder each
```

### ROMs

- Any file name works: files are recognised by their SHA-1. Plain `.gbc` only, not `.zip`.
- The game always runs from the **US** ROM; a European ROM only provides the texts of the other languages.

| File | SHA-1 |
| --- | --- |
| Oracle of Ages (US) | `880374fb978b18af4aa529e2e32f7ffb4d7dd2f4` |
| Oracle of Seasons (US) | `ba1268290fb2b1b70505d2d7b5825fc8a4816a4b` |
| Oracle of Ages (EU) | `9f2dff3c46406839f839a8d535353dfc1e3e8670` |
| Oracle of Seasons (EU) | `360b9408e569ba826212eac1ebea1b09573cb48c` |
| Gifts of Kinomi 1.1.2 patch | `bb0837fd2c62a8b3ea05da22ed6049ab57cfa1e2` |
| Moonrise Regalia 1.0.6 patch | `fcef50de31ce3e57d3e23cd846c0749e42098e08` |
| Temple of Seasons 1.073 patch | `0646e0a8fd6830198e453fdfeeea4a0a7c01cdf4` |

At start the app assigns every file it recognises; **Choose ROM** on a game's page looks again. Saves and save states
go beside the ROM, in `roms/`. `settings.txt`, the language packs and the logs (`the-oracles-project-nx.log`,
`steps.log`) are in `sdmc:/switch/the-oracles-project-nx/`.

### Languages

With both ROMs of a game in `roms/`, open its page, then **Cartridge → Language**, and choose a language.

## Default controls

| Button | Action |
| --- | --- |
| D-pad / left stick | Move |
| A / B | A / B (in menus: select / back) |
| + / − | Start / Select |
| Left stick click | Pause menu |
| HOME | System menu |

Everything can be remapped in Controls. See [docs/PLAYING.md](docs/PLAYING.md) for the options and the item hotkeys.

## Performance

- **Display → Quality** sets the view, the core and the neighbour workers together. Medium is the default on the
  Switch; try **Low** if it runs slowly, or **High** docked.
- **View**: Near (256×144) is the lightest, Far (480×270) the heaviest.
- **Core**: Fast (mGBA) is much lighter than Accurate (SameBoy).
- **Profile → Faithful** shows the original 160×144 picture and needs no neighbour workers at all.

## Changes from The Oracles Project

- `CMakeLists.txt`, `third_party/CMakeLists.txt`: the Switch build: SDL 3 from devkitPro's branch, mGBA by default, mGBA without `mmap`, no harness or tests.
- `launcher/CMakeLists.txt`, `launcher/switch/icon.jpg`: the `.nro` with its icon and `.nacp`.
- `launcher/main.c`: the launcher on an 8 MiB thread, the SD card's folders, the logs.
- `launcher/switch_clock.c`: the CPU at 1785 MHz while running.
- `launcher/home_games.c`, `launcher/file_dialog.c`: ROMs, European ROMs and patches found on the SD card by SHA-1.
- `launcher/sdl_backend.c`: full-screen OpenGL window, the Switch's buttons by their letters, settings on the SD card.
- `launcher/session.c`, `launcher/home_choices.c`: two neighbour workers by default.
- `engine/ghost/ghost.c`: the workers on cores 1 and 2, with a 1 MiB stack.
- `build_switch.sh`, `docs/SWITCH.md`: build script and notes.

---

This emulator is completely free and open-source. Optional donations to support the development are always welcome:

[![Ko-fi](https://img.shields.io/badge/Ko--fi-F16061?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/toniisound)
[![PayPal](https://img.shields.io/badge/PayPal-00457C?style=for-the-badge&logo=paypal&logoColor=white)](https://paypal.me/toniisound)
