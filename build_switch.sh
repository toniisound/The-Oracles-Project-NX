#!/usr/bin/env bash
# Builds the-oracles-project-nx.nro for the Nintendo Switch (libnx homebrew) with devkitPro.
# One-time setup (WSL/Linux with devkitPro pacman):
#   sudo dkp-pacman -S --needed switch-dev switch-cmake switch-pkg-config switch-mesa switch-libdrm_nouveau
#   sudo apt install ninja-build git zip
set -euo pipefail
: "${DEVKITPRO:=/opt/devkitpro}"
export DEVKITPRO
cd "$(dirname "$0")"
cmake -S . -B build-switch -G Ninja -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake" "$@"
# The Homebrew Menu's title, author and version are in the .nacp, which nothing would make again after they change.
rm -f build-switch/launcher/the-oracles-project-nx.nacp
cmake --build build-switch --target oracles_nro
# The release: the SD card's own folders, switch/the-oracles-project-nx/ with the .nro, the empty roms/, patches/ and
# mods/, and the licences that go with it (the port's, and those of what it embeds), to unzip at the card's root.
APP=switch/the-oracles-project-nx
( cd build-switch && rm -rf release the-oracles-project-nx.zip &&
  mkdir -p "release/$APP/roms" "release/$APP/patches" "release/$APP/mods" "release/$APP/licences" &&
  cp the-oracles-project-nx.nro "release/$APP/" &&
  cp ../LICENSE "release/$APP/licences/LICENSE.txt" &&
  cp ./*-LICENSE.txt ./*-NOTICE.txt ./*-OFL.txt mGBA-boot-rom.patch "release/$APP/licences/" &&
  cd release && zip -qr ../the-oracles-project-nx.zip switch ) ||
  echo "(no zip: install zip to make the release archive)"
echo
echo "Done: build-switch/the-oracles-project-nx.nro and build-switch/the-oracles-project-nx.zip"
echo "Unzip the zip at the SD card's root, then put your ROMs in sdmc:/switch/the-oracles-project-nx/roms/"
