# Changelog

## 0.1.4 — 2026-11-03

### Changed
- A fresh install starts with **no shader** (the plain picture), the lightest
  option. A full-screen CRT shader made the game drop frames on weaker laptops.
  Pick CRT Lottes or another preset under Settings → Shader; your choice is
  still saved and kept. An install that already has a shader keeps it.
- On laptops with two graphics cards (NVIDIA Optimus, AMD switchable
  graphics), the game now asks for the **dedicated graphics card** instead of
  the integrated one.

## 0.1.3 — 2026-11-03

### Fixed
- A stall at a scene change, where the game's wait for the next frame inside
  its video interrupt was skipped. In Donkey Kong Country (same engine) the
  same case left the screen black after saving.

## 0.1.2 — 2026-11-02

### New
- **Settings during play:** Ctrl+L (or Select+L3 on a controller) reopens the
  launcher. Shader, window scale, volume and controls apply when you press
  Resume, without restarting the game.
- **Audio interpolation:** press **I** to cycle Gaussian (the console's own,
  default), Cubic (crisper highs) and Sinc. It changes only what you hear;
  the game, its echo and save states are unaffected. The choice is kept.
- New upscaler shaders: **ScaleFX**, **NNEDI3 (neural)** and **NNEDI3
  (neural, high)**.
- A fresh install starts with the **CRT Lottes** shader. An install that
  already has a shader chosen keeps it.
- **Updates never cost you a save.** From the next update on, the updater
  copies your save states to `save-backups\<date>_before_<version>\` before
  installing, leaves save files out of the copy and puts the backup back if
  they changed anyway. Save states from earlier versions keep working, and
  every release is now checked for that before it is published.

### Fixed
- HDMA raster effects (per-line color, layer and scroll changes) started one
  line late; they now start on the line the game asks for.
- Choosing **None** as the shader during play no longer leaves a black
  screen.
- Multi-pass shaders that read the original game frame from a later pass
  (ScaleFX) no longer draw black.

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
