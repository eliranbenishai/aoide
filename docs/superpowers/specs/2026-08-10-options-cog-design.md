# Options cog (replace clutterbar)

Approved 2026-08-10. Replaces main-player clutterbar **O / A / I** with a single options cog; fixes options menu / dialogs crashing without `MaterialLocalizations`.

## Goal

- Remove the tall clutter rail from the main player body.
- Place a small cog at the same top-left origin (`left: 22`, `top: 18`) with no rail behind it.
- Cog opens a popup menu with: Always on top (✓), Look packs…, Track info, About Tramp, Quit.
- About opens a freestanding secondary window (like Settings); Look packs moved into Settings.
- Menu / dialogs must use a context under `MaterialApp` (fix host-State context bug).

## Interaction

| Control | Behavior |
|---------|----------|
| Cog tap | `showMenu` anchored to cog `RenderBox` (playlist menu pattern) |
| Always on top | Toggle via `AlwaysOnTopCommand` |
| Look packs… | `showLookPackDialog` |
| Track info | Existing track-info dialog |
| About Tramp | Show `WindowId.about` (`AboutWindow`) |
| Quit | Existing quit path |

## Wiring

- Player owns the menu (child of `MaterialApp`) so `showMenu` has `MaterialLocalizations`.
- Host handles look packs / track info / about / quit using the **callback’s** `BuildContext` (not `SessionHostApp`’s State context).
- Always on top stays a `SessionCommand` from the player (same as former clutter **A**).
- Callback shape: `void Function(BuildContext context, String action)? onOptionsAction` with values `looks` / `info` / `about` / `quit`. Remove `onOpenOptions` / `onShowTrackInfo`.

## Visual

- Glyph: existing `MockupIcons.options` (cog), phosphor-tint chrome, ~26×26 hit target.
- Menu chrome: `shellMid` + playlist-style labels.

## Mockup delta

Intentional divergence from `player-mockup-2.html` clutter **O / A / I**. Product chrome uses the cog; HTML mockup may lag.

## Tests

- Cog present; clutter keys gone.
- Tap cog → menu items visible.
- Always on top menu item → `AlwaysOnTopCommand`.
- Non-AOT actions invoke `onOptionsAction` with the correct string + a valid Material context.

## Out of scope

- Logo title-bar menu / Skins submenu
- About redesign / Proxima Magnifica credit
- Look packs dialog behavior changes
- Updating `player-mockup-2.html` clutter markup
