
# Graphite chrome redesign — design

Redesign Tramp's UI to match a supplied mockup: dark graphite panels with an acid
chartreuse phosphor, a main player over a switchable equalizer/playlist region,
and discrete zoom levels for high-DPI displays.

Supersedes the visual direction in
[`2026-08-01-classic-main-player-design.md`](2026-08-01-classic-main-player-design.md).
Product requirements: [`docs/tramp-v1-spec.md`](../../tramp-v1-spec.md).
Domain vocabulary: [`CONTEXT.md`](../../../CONTEXT.md).

## Reference mockup

[`docs/mockups/graphite-chrome.png`](../../mockups/graphite-chrome.png)
(1663×946). The mockup is AI-generated, so its geometry is approximate and it is
branded "WINAMP". Two consequences are binding:

- **Branding is `TRAMP`**, all caps, everywhere the mockup says Winamp. The
  equalizer title reads `TRAMP EQUALIZER`.
- **The mockup is a style reference, not a pixel target.** Reproduce its palette,
  materials, proportions, and control vocabulary exactly. Do not chase
  pixel-identity with a lossy AI render. Measured values below are the
  authority; where the mockup contradicts them it is noise.
- **Nothing of Winamp's identity transfers.** Proportions, bevels, groove
  sliders, the dense control-forward layout and the band frequencies are generic
  90s-player vocabulary and are fair to reuse. The lightning-bolt mark, the
  wordmark, and the bundled demo track are Winamp's brand. No bolt appears
  anywhere in Tramp.

## Brand assets

Tramp uses two marks, because one asset cannot do both jobs.

| Asset | Where |
|---|---|
| `TrampLogo` — `lib/ui/chrome/logo.dart`, rendered from `logo.svg`. The full mark: a colour illustration of a pin-up in headphones inside a ring badge | App icon, splash, About, README — anywhere it has room to be itself |
| `TrampMark` — `lib/ui/chrome/tramp_mark.dart`, a `CustomPainter`. The compact mark: the ring and the headphones, single colour, tinted by the caller | Chrome at control size, including the title bar's leading slot |

The full logo was tried in the title bar first and does not work there. At the 19
logical pixels the slot allows, the figure collapses into a smudge; and its skin
tones read as a photograph pasted onto a metal panel even at 300% zoom, where it
is otherwise legible. The compact mark keeps the two structural cues that survive
reduction — the ring and the headphones — and drops the figure. Recognition
between the two assets therefore rests on the ring silhouette.

One known artifact: the main panel's top-left corner shows a visible gradient
stop. That is a rendering defect, not a design feature. Panel faces use a single
smooth top-to-bottom gradient, as the equalizer panel correctly shows.

## Palette

Replaces `TrampColors` wholesale. The current light-metal / pure-green tokens
(`metalFace #B8B8B8`, `lcdPhosphor #33FF33`) share nothing with this design and
are removed rather than extended.

| Token | Value | Role |
|---|---|---|
| `frame` | `#000000` | Outer border, gutter between panels |
| `panelTop` | `#2C3039` | Panel gradient start |
| `panelBottom` | `#1D2128` | Panel gradient end |
| `bevelHi` | `#555B65` | Top highlight on raised surfaces |
| `bevelLo` | `#0B0E12` | Bottom shadow on raised surfaces |
| `buttonTop` | `#363B45` | Raised button gradient start |
| `buttonBottom` | `#22262E` | Raised button gradient end |
| `wellDeep` | `#010306` | Inset groove and slider track interior |
| `lcdGlass` | `#03060A` | Display background |
| `phosphor` | `#CFEA45` | LCD text, spectrum bars, slider fills, lit toggles |
| `phosphorDim` | `#5C7022` | Unlit toggles, spectrum floor |
| `railAccent` | `#FEE670` | Title-bar rails |
| `label` | `#C9CED3` | Chrome button text, wordmark |
| `labelDim` | `#979DA6` | Total-time text, disabled labels |
| `thumbHi` | `#BFC8D1` | Slider thumb highlight |

`railAccent` is deliberately warmer than `phosphor`; the mockup uses yellow for
chrome accents and yellow-green for anything reading as lit phosphor. Keeping
them distinct is what makes the display read as a screen rather than as paint.

## Surfaces

`lib/theme/tramp_surfaces.dart` defines every material once. No widget composes
its own gradient or bevel; drift between the equalizer, the transport buttons,
and the playlist is the failure mode this file exists to prevent.

| Surface | Construction |
|---|---|
| `raisedPanel` | `panelTop`→`panelBottom` vertical gradient, 1px `bevelHi` top edge, 1px `bevelLo` bottom edge, 1px `frame` outer border, 3px corner radius |
| `raisedButton` | `buttonTop`→`buttonBottom` vertical gradient, `bevelHi` top/left, `bevelLo` bottom/right, 2px radius |
| `pressedButton` | `raisedButton` with the gradient inverted and bevels swapped |
| `insetWell` | `wellDeep` fill, `bevelLo` top/left, `bevelHi` bottom/right |
| `lcdGlass` | `lcdGlass` fill, `insetWell` bevels, no gloss overlay |

Bevels are hairlines: 1 logical pixel, snapped to whole device pixels via
`ZoomScope.snap` so they stay crisp at fractional zoom.

## Typography

Both faces are bundled as assets. Runtime `google_fonts` fetching is removed: it
breaks offline rendering and makes golden tests non-deterministic.

| Family | Use |
|---|---|
| Barlow Semi Condensed (600, 700) | Chrome labels, wordmark, EQ frequency and dB labels |
| IBM Plex Mono (500, 600) | All LCD text — title, times, bitrate, indicators, EQ gain values |

## Geometry

Panels are authored against fixed logical canvases derived by halving the
mockup's measured panel bounds (main 1625×484, equalizer 1625×411).

| Panel | Logical canvas | Aspect |
|---|---|---|
| Main player | 812 × 242 | 3.355 |
| Equalizer | 812 × 206 | 3.942 |

Panels are separated by a 6-logical-pixel `frame` gutter.

### Main player

| Element | Logical bounds |
|---|---|
| Title bar | full width × 35 tall |
| Logo / menu button | 27 × 27 at x 30 |
| Title rails | two 2px lines with their tops at y 17 and y 22; left run x 71–361, right run x 472–676 |
| `TRAMP` wordmark | centred in the rail gap, x 361–472 |
| Window buttons | 27 wide, at x 691 / 733 / 775 (minimize, maximize, close) |
| Display well | x 41–568, y 41–178 |
| Spectrum | inside the well, x 47–274 |
| Seek bar | 2 tall at y 158, track x 47–274 |
| `EQ` / `PL` indicators | x 293–324, y 146–161 |
| Shuffle / repeat buttons | 52 × 26 at x 665 and x 727, y 52 |
| Volume slider | x 607–782, y 108–120 |
| Mute button | 26 × 26 right of the volume groove |
| `EQ` / `PL` region buttons | 52 × 26 at x 610 and x 663, y 149 |
| Transport buttons | 69 × 40 at x 43 / 116 / 190 / 264 / 338, y 182 |
| `ZOOM` dropdown | 108 × 26 at x 609, y 194 |
| `OPEN` button | 54 × 26 at x 726, y 194 |

The transport buttons keep the mockup's order — previous, play, pause, stop,
next — with only the play glyph in `phosphor`; the rest are `label`.

### Equalizer

| Element | Logical bounds |
|---|---|
| Title bar | full width × 35 tall |
| Collapse (`▲`) button | 27 × 27 at x 27 |
| Title rails | left run x 59–315, right run x 495–752 |
| Close button | 27 × 27 at x 775 |
| `ON` button | 33 × 20 at x 36, y 42 |
| `AUTO` button | 37 × 20 at x 93, y 42 |
| `PRESETS` button | 76 × 22 at x 677, y 44, with a 23-wide chevron at x 765 |
| Preamp slider | thumb centred x 73, travel y 71–166 |
| Band sliders | thumbs centred x 196, 245, 294, 343, 392, 441, 490, 539, 588, 636 (48.9 spacing) |
| Thumb size | 17 × 10 |
| Frequency labels | y 53–63, above each band |
| Gain value labels | y 174–182, below each band |

Bands are 60, 170, 310, 600, 1K, 3K, 6K, 12K, 14K, 16K — Winamp 2.x's set, which
the mockup reproduces. Gains span ±12 dB, matching the printed `+12 dB` / `0 dB` /
`-12 dB` scale beside the preamp.

## Zoom

`ZoomController` (a `ChangeNotifier`) owns the current step and persists it.
`ZoomScope`, an `InheritedWidget`, exposes the factor and `snap(double)` for
hairline rounding. The root of the panel stack applies a single
`Transform.scale`; no widget scales itself.

Steps are 100, 125, 150, 200, 250 and 300 percent. Because the design is entirely
vector — gradients, `CustomPainter` glyphs, an SVG logo — and Flutter rasterizes
text at final device resolution, every step is crisp, and there is exactly one
source of geometry so no step can drift from the mockup.

Changing a step resizes the window and recomputes its minimum size. A step whose
minimum width exceeds the current display's work area is disabled in the `ZOOM`
dropdown rather than allowed to clip the chrome — at 300% the player alone needs
2436 logical pixels of width. On first run the initial step is the largest one
that fits comfortably in the work area, capped at 150%.

Zoom is also reachable by `Ctrl+=` / `Ctrl+-` / `Ctrl+0` and from the logo menu.

At 100% the default window is 824 × 500: an 812 panel plus 6px of frame either
side; and vertically 6 frame + 242 player + 6 gutter + 240 lower region + 6 frame.

## Layout

```
┌─ MainPlayerPanel (812 × 242) ──────────────────────────┐
│ logo │ ══════ TRAMP ══════ │ min  max  close           │
│ ┌ display well ─────────────────┐   shuffle   repeat   │
│ │ spectrum  │ ▶ 1. Artist - Ti  │   ◄── volume ──► ⏻   │
│ │ + seek    │ 0:05  0:22        │                      │
│ │           │ 128kbps 44kHz st  │   [ EQ ]  [ PL ]     │
│ └───────────────────────────────┘                      │
│ ⏮  ▶  ⏸  ⏹  ⏭            [ZOOM 150% ▾]  [OPEN]        │
└────────────────────────────────────────────────────────┘
                    ── 6px frame gutter ──
┌─ lower region ─────────────────────────────────────────┐
│  EqualizerPanel (fixed 812 × 206, collapsible)         │
│    ── or ──                                            │
│  PlaylistPanel (fills remaining height)                │
└────────────────────────────────────────────────────────┘
```

The lower region shows the equalizer or the playlist, never both, switched by the
`EQ` and `PL` buttons. `TrampShell` owns a `LowerRegion` enum and persists the
choice. The small `EQ` / `PL` text inside the display well are indicators of that
state, lit for the visible region.

The equalizer holds its fixed 206-tall canvas and centres in the region; its `▲`
button collapses it to just its 35-tall title bar. The playlist keeps filling
whatever height remains, as it does today.

## Control mapping

Every control the mockup draws is assigned real behaviour. Nothing is decorative.

| Control | Behaviour |
|---|---|
| Tramp mark (title bar) | `TrampMark`, the compact mark — not the full colour logo, which is illegible at this size. Opens the main menu: open files, open folder, open playlist, save playlist, zoom, about, exit |
| Title bar centre | `TRAMP` wordmark; drag region for moving the window |
| Minimize / maximize / close | Real window operations. Maximize is not required by the v1 spec, but the button is in the mockup and wiring it is trivial, so it works rather than being drawn |
| Display well, left | Spectrum analyser over a 2px seek bar; the bar is draggable to seek |
| Display well, centre | Play-state glyph, `N. Artist - Title`, large elapsed time, small total time, and real bitrate / sample rate / channel count from the decoded stream |
| `EQ` / `PL` indicators | Which lower region is showing |
| Shuffle / repeat buttons | Toggles, lit in `phosphor` when active. Repeat cycles off → all → one. Repeat-one adds a single vertical stroke inside the loop — the numeral one reduced to what reads at 16 logical pixels, where a real glyph would be about four pixels tall and illegible |
| Volume slider | Volume, in the equalizer's slider language, horizontal. Replaces the mockup's L/R meters |
| Mute button | Toggles mute; the slider fill dims to `phosphorDim` while muted |
| `EQ` / `PL` buttons | Switch the lower region |
| Transport buttons | Previous, play, pause, stop, next |
| `ZOOM` dropdown | Shows the current step; opens the step menu, with steps too large for the display disabled |
| `OPEN` button | Opens files — the eject control's position in Winamp 2.x |
| EQ `ON` | Enables the equalizer |
| EQ `AUTO` | When lit, re-applies the last-used preset whenever a track loads, so manual per-track tweaks do not persist across tracks. It reads no per-track state |
| EQ `PRESETS` | Preset menu: built-in curves, save current, delete |
| EQ `▲` | Collapses the panel to its title bar |
| EQ close | Switches the lower region to the playlist |

## Modules

New and rewritten files, each with one responsibility.

| Path | Responsibility |
|---|---|
| `lib/theme/tramp_colors.dart` | Palette tokens (rewritten) |
| `lib/theme/tramp_surfaces.dart` | The five surface recipes (new) |
| `lib/theme/tramp_text.dart` | Bundled text styles for chrome and LCD (new) |
| `lib/ui/zoom/zoom_controller.dart` | Steps, current step, persistence, display-fit filtering (new) |
| `lib/ui/zoom/zoom_scope.dart` | `InheritedWidget` exposing the factor and `snap` (new) |
| `lib/ui/chrome/title_bar.dart` | Rails, wordmark, drag region, leading and trailing button slots (new; shared by both panels) |
| `lib/ui/chrome/chrome_button.dart` | Raised button: icon, label, toggle, and dropdown variants (rewritten) |
| `lib/ui/chrome/chrome_slider.dart` | Groove + fill + thumb, horizontal and vertical (rewritten) |
| `lib/ui/chrome/metal_panel.dart` | Thin wrapper applying a named surface (rewritten) |
| `lib/ui/chrome/lcd_text.dart` | Phosphor text with the correct dim/lit treatment (new) |
| `lib/ui/chrome/spectrum_visualizer.dart` | Bars from real levels, with smoothing and peak caps (rewritten) |
| `lib/ui/chrome/transport_icons.dart` | Adds shuffle, repeat, repeat-one, eject, chevron (extended) |
| `lib/ui/main_player/main_player_panel.dart` | The 812×242 canvas and its three rows (new; replaces `classic_main_player.dart`) |
| `lib/ui/equalizer/equalizer_panel.dart` | The 812×206 canvas, preamp and bands (new) |
| `lib/ui/playlist_panel.dart` | Unchanged list behaviour, new skin |
| `lib/ui/tramp_shell.dart` | Zoom host, `LowerRegion` switching, drag/resize edges (rewritten) |
| `lib/domain/equalizer_settings.dart` | Immutable preamp + 10 gains + enabled + preset name (new) |
| `lib/eq/equalizer_controller.dart` | Mutation, persistence, presets, `EqualizerSink` seam (new) |
| `lib/platform/settings_store.dart` | Persists zoom step, lower region, EQ settings (new) |
| `lib/analysis/spectrogram_analyzer.dart` | Isolate-hosted offline analysis: mpv PCM decode, STFT, band folding (new) |
| `lib/analysis/wav_reader.dart` | RIFF walker handling `WAVE_FORMAT_EXTENSIBLE` (new) |
| `lib/analysis/spectrogram_cache.dart` | Keyed by path, size and mtime (new) |

`ClassicMainPlayer` is deleted. Its transport wiring moves to
`MainPlayerPanel` unchanged in behaviour; only presentation differs.

## Equalizer scope

The equalizer is **chrome and state only** in this change. Sliders move, values
display, presets save and restore, and everything persists — but gains do not
reach the audio path. `docs/tramp-v1-spec.md` lists the equalizer as a v1
non-goal, and honouring that while still matching the mockup means building the
surface without the DSP.

`EqualizerController` writes through an `EqualizerSink` interface with a single
no-op implementation. Making the equalizer audible later means writing one
mpv-backed `EqualizerSink` and registering it — no UI or state changes.

## Audio levels

The spectrum analyser should be driven by real audio. The current engine cannot
supply it, and that constraint is verified rather than assumed.

### Verified constraint

The `libmpv-2.dll` that `media_kit_libs_windows_audio` ships is built with
`--disable-filters`, re-enabling only two: `--enable-filter=overlay` and
`--enable-filter=equalizer`. A byte scan of the shipped binary finds zero
occurrences of `astats`, `aspectralstats`, `ebur128`, `bandpass`, `amix` or
`amerge`. The `af-metadata` property exists, but nothing in the build can
populate it. The macOS build recipe is equally minimal; Linux links the system
libmpv and would work.

Two independent limits follow, both confirmed in local graphite-chrome audio-levels notes (historical; the lock is full libmpv):

1. **No levels at all** on Windows and macOS without replacing the libmpv binary.
2. **No true per-band spectrum, ever, from mpv's filter chain** — mpv reads
   metadata from a single output pad, `amix`/`amerge`/`join` do not propagate
   frame metadata, the `af` chain is serial so parallel measurement taps would
   corrupt the audible output, and no FFmpeg audio filter emits per-bin
   magnitudes. Even with a full libmpv, `astats` yields per-channel RMS and peak
   at roughly 20 Hz, running about 200 ms ahead of what is audible.

A third limit, found while spiking the equalizer: the `astats` route would need
`aresample` too, which is also absent. So even a build that added `astats` alone
would not work.

### The approach: a precomputed spectrogram

Rather than metering in real time, each track is analysed once and the result is
replayed against the playback clock. This is the **only verified route to a true
per-band spectrum**, and it sidesteps every limit above: no filter graph, no 20 Hz
cadence ceiling, and no 200 ms lead error, because frames are indexed by position
rather than arriving live.

Verified viable by experiment (historical PCM spike; the lock is full libmpv):
mpv's `ao=pcm` output is compiled in, produces a **byte-identical** PCM payload,
and decodes a 240-second track in 1.13 s wall clock — roughly 212× realtime.

`SpectrogramAnalyzer` runs in a background isolate and, per track:

1. Creates its own mpv instance over FFI, independent of the playback instance,
   configured `ao=pcm`, `untimed=yes`, `vid=no`, writing a temporary WAV.
2. Reads that WAV, walking RIFF chunks. mpv writes `WAVE_FORMAT_EXTENSIBLE`
   (tag `0xFFFE`) with the real format in the `SubFormat` GUID, so a plain
   `fmt.audioFormat == 1` reader will reject its own input.
3. Computes an STFT — 1024-sample Hann windows, hop sized to 30 frames per
   second — and folds bins into the 20 display bands on a log frequency scale.
4. Emits `Uint8List` frames of 20 bytes, about 600 bytes per second of audio, so
   a four-minute track costs roughly 144 KB.
5. Caches the result keyed by path, size and mtime.

`PlayerEngine` exposes the result through one seam, so the visualiser never owns
its own animation:

```dart
class AudioLevels {
  final List<double> bands; // normalised 0..1, low to high
  final double leftRms;     // normalised 0..1
  final double rightRms;
  final bool synthetic;     // true before analysis completes
}

Stream<AudioLevels> get levels;
```

`SpectrumVisualizer` interpolates between frames and applies falling peak caps.
Until analysis finishes — a fraction of a second for typical material — the
engine emits `synthetic: true` frames derived from playback state, and the flag
keeps that honest in the data rather than hidden. `FakePlayerEngine` emits a
scripted sequence, keeping widget and golden tests deterministic.

One unverified assumption, to be checked before relying on it: whether mpv's
downmix and resample engage on the `ao=pcm` path, which is what would let the
analysis buffer shrink via `audio-channels=mono` and a reduced
`audio-samplerate`. The spike's source already matched the requested format, so
mpv's conversion node stayed disabled and the question went untested. If it does
not work, the analyser folds stereo to mono itself at full rate — more memory
during analysis, no change to the cached output.

## Equalizer audio path

Settled by experiment: **the equalizer cannot be made audible with the libmpv
media_kit ships.** Evidence was a historical PCM/EQ spike; the lock is full libmpv.

`--enable-filter=equalizer` is genuinely present, and mpv accepts a ten-filter
chain on the `af` property — but the graph never configures, because mpv's
libavfilter bridge needs libavfilter's `aresample` to negotiate sample formats
and `--disable-filters` removed it. Measured effect on a three-tone test signal
was `0.00 dB` at every band. All eight sample formats mpv accepts fail
identically, so there is no way around it short of a custom libmpv build.

The trap worth recording: mpv reports success at every surface a caller would
check. `setProperty` returns success, `getProperty('af')` echoes the chain back
verbatim, and no error event fires. Only the verbose log shows
`Disabling filter equalizer.00 because it has failed`. **Any future
`EqualizerSink` implementation must verify audibly or by measurement, never by
return code** — otherwise it ships an equalizer that looks wired and does
nothing.

This confirms the chrome-and-state scope below rather than changing it.

## Testing

| Layer | Coverage |
|---|---|
| Palette | `TrampColors` is dark graphite with a chartreuse phosphor — guards against regressing to light metal or pure green |
| Layout | Both panels hold their exact aspect, and the stack overflows at no zoom step |
| Zoom | Step cycling, clamping, persistence, and disabling steps that exceed the work area |
| Controls | Every control in the mapping table has a wiring test asserting its effect on a controller |
| Region | `EQ` / `PL` switch the lower region, indicators follow, collapse works |
| Equalizer | Gain and preamp edits, preset save/restore, persistence, and that the no-op sink is what is installed |
| Levels | `SpectrumVisualizer` renders scripted engine levels; interpolation and peak decay behave; synthetic frames are flagged as such |
| Analysis | `WavReader` parses `WAVE_FORMAT_EXTENSIBLE` as well as plain PCM; the STFT produces known band energies for a synthesised three-tone fixture; the cache hits on repeat and misses when mtime changes |
| Golden | Main player and equalizer at 100% and 200%, with bundled fonts |

Goldens are the only mechanism that actually catches visual drift, which is the
central risk in a redesign whose acceptance criterion is "looks the same". They
are viable here precisely because fonts are bundled and every surface is vector.

## Out of scope

- Audible equalization (the sink seam is built; the DSP is not)
- Real-time level metering — impossible with this libmpv; the spectrum comes from
  offline analysis indexed by playback position instead
- Classic Winamp WSZ / bitmap skins
- Detachable main / equalizer / playlist windows — one window, stacked panels
- Continuous free-scaling zoom; steps are discrete
- Per-track EQ presets — `AUTO` re-applies one global preset, it does not store
  a curve per track
- L/R VU meters; that chrome becomes the volume slider instead

## Documentation to update

- `docs/architecture.md` — new UI module map, zoom layer, EQ state, levels seam
- `docs/tramp-v1-spec.md` — EQ chrome, zoom, and maximize enter scope; VU meters leave it
- `CONTEXT.md` — vocabulary for zoom step, lower region, phosphor, rail, well
