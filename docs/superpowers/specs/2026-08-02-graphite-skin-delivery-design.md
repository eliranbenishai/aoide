# Graphite skin delivery — design

Deliver the locked graphite look as a **PNG-first built-in skin**, with zoom-only
sizing for the main player and equalizer, and a freely resizable playlist region
for large lists.

Supersedes the *construction* model in
[`2026-08-02-graphite-chrome-redesign-design.md`](2026-08-02-graphite-chrome-redesign-design.md)
(painted `MetalPanel` / vector chrome as the look source). Palette, branding
rules (TRAMP, no bolt), fixed logical canvases, and control vocabulary from that
doc remain unless this document overrides them.

Product: [`docs/tramp-v1-spec.md`](../../tramp-v1-spec.md).  
Vocabulary: [`CONTEXT.md`](../../../CONTEXT.md).  
ADRs: [0002](../../adr/0002-fixed-canvas-zoom.md), [0003](../../adr/0003-zoom-only-window-size.md), [0004](../../adr/0004-png-graphite-skin.md).

## Goals

- Side-by-side visual match to the reference mockup: grain, brushed metal, bevels,
  emboss — not clean gradients or flat hairline borders.
- Agent-authored skin assets derived from the mockup (user has no art pipeline).
- Code draws only live readouts: spectrum, LCD/playlist text, hit targets, asset
  state, slider thumb position.
- Main and equalizer canvases **never** freely resize or stretch (permanent).
- Playlist panel **does** freely resize (width and height) so thousands of rows
  are usable.

## Non-goals

- Classic Winamp WSZ loading
- Audible EQ / real spectrum (existing gaps unchanged)
- Per-zoom-step asset exports
- Detachable multi-window
- Freely resizing or stretching main/EQ canvases — **permanently out of product
  scope**, not a later feature

## Reference art

| Source | Role |
|---|---|
| [`docs/mockups/graphite-chrome.png`](../../mockups/graphite-chrome.png) | Style and material authority |

Fidelity bar: **side-by-side match** — materials, proportions, lighting.
Branding in shipped art is `TRAMP` / `TRAMP EQUALIZER`. Accidental AI glitches
may be cleaned; grain and emboss must not be simplified away.

## Approach (hybrid C)

1. **Panel faces** cropped from the mockup at **2×** logical size, light
   cleanup/rebrand — preserve photographic grain and bevels.
2. **Control sheets** cropped from the same source; missing pressed/active
   states derived by lighting/bevel edits of idle crops — not flat redraws.
3. **Playlist face** invented in-family from main/EQ materials (no playlist
   mockup), using 9-slice / region slices so the list well can grow.
4. Runtime: `GraphiteSkin` pack under the existing root zoom transform.

## Runtime architecture

### Skin pack

Assets live under `assets/skin/graphite/` (name exact in implementation). A
manifest (Dart constants or JSON) maps logical rects → asset + states.

| Class | Contents |
|---|---|
| Panel faces | `main`, `equalizer`, `equalizer_shade`, `playlist` (9-slice regions) |
| Transport | prev / play / pause / stop / next × idle + pressed (+ play active) |
| Toggles | shuffle, repeat, EQ, PL, ON, AUTO as needed |
| Title bar | mark slot, minimize, zoom−, zoom+, close; EQ collapse |
| Sliders | grooves (if not in face), shared/metal thumb |
| Misc | OPEN, PRESETS (+ chevron) |

Master density: **2×** logical canvas. One set scales with the zoom step.

### Layer stack (per panel, bottom → top)

1. Panel face PNG (playlist: 9-slice chrome + stretchable well)
2. Control face PNGs for current states
3. Live overlays: spectrum, LCD/playlist text, slider thumbs (and fill if not a
   clipped groove asset)
4. Invisible hit targets aligned to manifest rects

### Shell and zoom

- Keep single root `Transform.scale` ([ADR 0002](../../adr/0002-fixed-canvas-zoom.md)).
- Logical canvases unchanged: main **812×242**, equalizer **812×206**.
- Title bar controls: minimize, **zoom−**, **zoom+**, close. No maximize-as-size.
- Drop the redundant **ZOOM** dropdown once title-bar ± ship; keep Ctrl±/0 and
  menu zoom. Keep **OPEN** as a skinned control.
- `MetalPanel` / painted bevels retire as the look source. Keep `LcdText`,
  `SpectrumVisualizer`, `TrampMark` / `TrampLogo`, zoom plumbing.

### Window sizing modes

| Lower region | Window behavior |
|---|---|
| Equalizer (or windowshade) | **Fixed** size = stack × zoom step. No edge resize. |
| Playlist | **Freely resizable** (width and height). Main stays fixed logical size × zoom, top-aligned; extra space is frame gutter + growing playlist well. |

- Switching PL → EQ snaps to the fixed EQ stack size.
- Switching EQ → PL restores the last playlist window size (or a default tall
  size). Persist playlist window size separately from zoom step.
- Main and EQ artwork **never** scale with free resize — only with zoom step.
  Permanent rule.

### Playlist skin

- 9-slice (or equivalent region slices): corners, edges, stretchable/tileable
  well — not one fixed-height full-panel PNG.
- Stretchable edges must tile or slice without collapsing to a flat fill.
- List rows remain code (virtualized for large playlists).
- Invented in-family from main/EQ grain; title bar matches EQ chrome language.

## Art pipeline

Performed in-repo by agents (Claude/GPT vision authorized for hard slices and
QA):

1. Measure and crop panel bounds from the mockup; export 2× faces.
2. Rebrand / cleanup (WINAMP → TRAMP, remove bolt); keep materials.
3. Punch transparent wells for spectrum, LCD text, playlist rows, moving thumbs.
4. Build control sheets; synthesize missing states from idle crops.
5. Build playlist 9-slice from sampled materials.
6. QA: side-by-side at 100% and 200% vs mockup — **fail** if faces read as
   smooth gradients or straight clean borders where the mockup has grain/emboss.

## Testing

- Unit: manifest rects; EQ-mode window size ↔ zoom; playlist size persistence;
  lower-region switch size snap.
- Widget: correct asset states; overlays sit in punched wells.
- Manual / vision: material fidelity vs mockup at 100% and 200%.
- Stress: thousands of playlist rows + free resize without distorting main/EQ.

## Doc impact

- Amend [ADR 0003](../../adr/0003-zoom-only-window-size.md) for playlist-mode
  free resize; main/EQ remain zoom-only forever.
- Update `tramp-v1-spec.md`, `architecture.md`, `CONTEXT.md` to match.
- Treat painted-chrome construction in the earlier graphite redesign design as
  superseded by this document.

## Success criteria

- At 100% and 200%, main + EQ side-by-side with the mockup read as the same
  materials and proportions (TRAMP branding).
- EQ mode: window not edge-resizable; size follows zoom only.
- Playlist mode: window freely resizable; main/EQ canvases unstretched; large
  lists usable.
- No painted `MetalPanel` faces required for the shipped look.
