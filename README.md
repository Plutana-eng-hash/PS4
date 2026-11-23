# PS4 Overlay

A GoldHEN plugin that draws a black-and-white in-game overlay: FPS/frame-time HUD,
a configurable crosshair, and a tabbed menu with toggles, sliders and choices.
No game-specific or gameplay-advantage features.

## How it works
- Hooks `sceVideoOutRegisterBuffers` (to learn the frame buffers), `sceVideoOutSubmitFlip`
  (to draw on each frame before it is shown) and `scePadReadState` (menu input, and hiding
  input from the game while the menu is open).
- A small inline-hook engine (`platform.c`) and a software renderer with a built-in
  8x8 font (`draw.c`). The module does not use the C library at runtime.
- Writes `/data/GoldHEN/plugins/ps4_overlay_plugin/ovl.log` for diagnostics.

## Menu
HUD (FPS, frame time, resolution, uptime, corner, scale) / SIGHT (crosshair style, size,
thickness, gap, color, outline, dot) / MENU (theme, scale, position, open combo, input
blocking, hints) / INFO (live stats and hook status) / SYS (save, reload, reset, buffer layout).

## Minecraft test profile
Ships configured for Minecraft (survival): loads for any game (so any copy of Minecraft), shows a
`Starting. . .` PS notification on load and `Ready` once rendering works, an on-screen banner, and
opens the menu with Touchpad+Options (Circle/Triangle are drop/inventory in Minecraft). The open combo
is hidden from the game.

## Build
`make` and `make package` with `OO_PS4_TOOLCHAIN` set, or push to GitHub and let the
workflow build `dist/ps4_overlay_plugin.zip`.

## Status
Compiles; the renderer and menu logic are tested on a PC. Hooking, pad parsing and
scan-out buffer layout have NOT been verified on a console. If the menu is scrambled,
change `render.layout`; if nothing shows, read `ovl.log`.
