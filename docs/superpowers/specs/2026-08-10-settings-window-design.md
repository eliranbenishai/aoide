# Settings window design

Agreed 2026-08-10. Implemented in the same change set.

## Summary

Tramp gains a fourth frameless session window — **Settings** — opened from the main player options cog. It is movable and shadeable like EQ/PL, persists position/visibility/shade, is **not** snappable, and stays raised above other Tramp windows (not desktop-wide always-on-top unless that global preference is on).

## UI

- Side tabs: **General** | **Skins**
- Instant apply for all controls (no OK/Apply)
- Single footer action: **Reset Settings** (confirmation dialog; restores all of `settings.json` defaults including layout and active skin)

### General

- Resume last session (playlist + playback index/position/was-playing via `session_resume.json`)
- Confirm before quit
- Scroll title (display-well marquee)
- Minimize hides secondaries
- Dock snap strength: Off / Normal / Strong (0 / 20 / 40 px)

Does **not** duplicate main-player controls (AOT, zoom, mono).

### Skins

Former look-pack manager, product name **Skins**. Catalog dir `skins/`, manifest `skin.json` (legacy `look.json` / look keys still load). Cog entry is **Settings…** (look-packs dialog path removed from the menu).

## Window model

`WindowRole.settings` / `WindowId.settings` secondary engine; skip taskbar; `SessionBus.order_top_window` for z-order above tramp peers.
