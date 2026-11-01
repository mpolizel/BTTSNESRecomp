# Changelog

## 0.1.0 — 2026-11-01

First release: a setup pack that recompiles the game from your own ROM.

- Boots through the Tradewest and Rare logos, the title screen and the
  waving copyright flag into player select, the intro cutscene and the first
  level.
- NMI idle-loop fix (as in the Donkey Kong Country ports): without it the
  game froze on the player-select screen.
- Offset-per-tile fix: the copyright flag after the title screen waves as
  one picture instead of coming out cut into seams.
- Scanline, CRT and upscaler shader presets; a fresh install starts with
  **CRT Geom (curved)**, and whatever you pick in the launcher is kept.
- **Check for updates on start** (next to *Skip launcher on boot*, on by
  default): installs newer releases over the install folder, keeping save
  states, settings and your ROM selection.
- Renderer fixes shared with the Super Mario Kart and Donkey Kong Country
  ports.
- Console-free Windows build; diagnostics go to
  `BattletoadsInBattlemaniacsSNESRecomp.log`.
