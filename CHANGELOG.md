# Changelog

## 0.1.1 — 2026-11-01

### Fixed
- **HUD pieces over the level:** in busy scenes a band of HUD tiles (score
  digits, player-two icons) was drawn across the middle of the screen. The
  game runs its logic inside the NMI with interrupts enabled; its line-33 IRQ,
  which switches from the HUD to the level, now interrupts that logic on time
  instead of waiting for it to finish (the split slipped down to line 95).

Updating from 0.1.0: accept the update offer when the game starts (or unpack
this zip over your install folder) and press **Generate & rebuild**.

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
