# Nintendo Switch

A homebrew `.nro` of the launcher, built with devkitPro (libnx). SDL 3 comes from devkitPro's Switch branch of SDL
and draws through EGL / OpenGL ES 2 (switch-mesa). The default core is mGBA's, the lighter one, as on Android.

## Building (WSL or Linux)

```bash
sudo dkp-pacman -S --needed switch-dev switch-cmake switch-pkg-config switch-mesa switch-libdrm_nouveau
sudo apt install ninja-build git zip
./build_switch.sh
```

The result is `build-switch/the-oracles-project-nx.nro`, and `build-switch/the-oracles-project-nx.zip`: the SD card's
`switch/the-oracles-project-nx/` folder, to unzip at the card's root. The first build downloads and compiles SDL 3.
The Homebrew Menu's title, author and version are set in `launcher/CMakeLists.txt`; the script makes the `.nacp`
again at each build.

## Installing

```
sdmc:/switch/the-oracles-project-nx/
├── the-oracles-project-nx.nro
├── roms/       your ROMs: Ages and/or Seasons (US), and their European ROMs for the languages
├── patches/    the fan games' .bps patches
└── mods/       Lua mods, one folder each
```

There is no file dialog on the Switch. At start the launcher takes, for each game, the first file of `roms/` and
`patches/` that it recognises by its SHA-1 (the US ROM, the European ROM, the fan game's patch) when settings.txt
names none yet; a page's Choose looks in the same folders again. File names do not matter.

`settings.txt`, the language packs and the log (`the-oracles-project-nx.log`) are in `sdmc:/switch/the-oracles-project-nx/`;
each game's save and savestates go beside its ROM, in `roms/`.

## Controls

The Switch's buttons are read by their letters: A is the game's A and the launcher's OK, B its B and Back. The
left stick's click opens the pause menu (Controls, Menu). Everything else is as in [`PLAYING.md`](PLAYING.md).
