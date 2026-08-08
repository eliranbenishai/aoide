# Mockup Multi-Window Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship Tramp as three Winamp-style dockable windows whose chrome is a 100% code match to `player-mockup-2.html`, backed by full libmpv (audible EQ, real spectrum, force-mono).

**Architecture:** One process, three OS windows via `desktop_multi_window` + `window_manager`. The **main** Flutter engine owns `PlaybackController`, playlist, EQ, zoom, settings, docking, and libmpv. EQ and playlist engines are UI surfaces that sync over a typed `SessionBus` (method channel). Chrome is token-driven Flutter painting from the mockup CSS. Compressed `media_kit_libs_*` binaries are replaced with full libmpv builds we bundle and force-load.

**Tech Stack:** Flutter desktop (Windows/Linux/macOS), `window_manager`, `desktop_multi_window`, `media_kit` + **full** libmpv (custom bundle), bundled Tramp Condensed / Tramp Mono from the mockup, `flutter_svg` for the logo mark.

**Design spec:** [`docs/superpowers/specs/2026-08-08-mockup-multiwindow-redesign-design.md`](../specs/2026-08-08-mockup-multiwindow-redesign-design.md)  
**Mockup authority:** [`player-mockup-2.html`](../../../player-mockup-2.html)

## Global Constraints

- Dart SDK `>=3.5.0 <4.0.0`; do not raise the floor.
- Platforms: Windows, Linux, macOS. **Three** frameless windows; Winamp-style docking.
- Visual authority is `player-mockup-2.html` at the **825×** classic×3 grid. Fidelity bar: side-by-side indistinguishable at 100% zoom (approved delta: clutterbar **O/A/I** only — no ghost D/V).
- Palette exact: shell `#323744/#262b38/#1a1d26/#12141a/#0a0b0e`, phosphor `#3de7ff` / hot `#b8f6ff`, accent `#ff3d9a`, ink `#e8eaf0/#8b919e/#5b6270`, well `#050608`.
- Logical sizes at 100%: main `825×348`, EQ `825×348`, playlist default `825×696`. Title bar `42`. Zoom steps `[100,125,150,200,250,300]`; main ± changes **all** windows.
- Main/EQ: no edge resize. Playlist: free resize. Main close quits; EQ/PL close hides. Windowshade on EQ and PL.
- Full libmpv only — no media_kit compressed audio builds. EQ ships only after **measurement** proves bands. Spectrum must be `synthetic: false` in normal play.
- Branding: `TRAMP` / `TRAMP` + `1.0` on main. No Material icon font / tofu glyphs on chrome.
- Every task: `flutter analyze` clean and relevant `flutter test` green before commit.
- Docs/ADRs that conflict are rewritten in Task 1 / Task 12 — do not leave obsolete locks active.

### Pinned implementation choices (from design §15)

| Topic | Pin |
|-------|-----|
| Snap threshold | `12.0` logical px (pre-zoom) edge proximity |
| Undock | Drag break when separation > `48.0` logical px **or** hold `Shift` while dragging |
| Group minimize / always-on-top | Apply to all **visible** members of the docked component containing main (and any visible detached windows independently for AOT only when A is on — AOT flag is global: all visible tramp windows) |
| Always-on-top | Global boolean; all visible windows follow |
| Mono | `player.platform.setProperty('audio-channels', on ? 'mono' : 'auto')` via media_kit |
| EQ `af` | lavfi chain of `equalizer` filters; exact string builder in Task 10; never trust return code alone |
| Spectrum | Isolate STFT over PCM from a second mpv/`ao=pcm` decode of the current path (or shared ring if spike finds a cheaper tap); 20 bands; `synthetic: false` |
| Multi-window | `desktop_multi_window` + `window_manager`; main engine is session host |
| Product doc | Keep filename `docs/tramp-v1-spec.md` but rewrite contents for this direction |
| Repeat | LED lit for `all` and `one`; label stays `Repeat`; cycle off→all→one on click |

---

## File Structure

**Created:**

| Path | Responsibility |
|------|----------------|
| `lib/theme/mockup_tokens.dart` | Colors, gradients, radii, shadows from mockup `:root` / shared rules |
| `lib/theme/tramp_metrics.dart` | Replaced sizes: 825 canvases, titleBar 42, clutterbar/display rects, band layout |
| `assets/fonts/TrampCondensed-Bold.ttf` | Extracted from mockup |
| `assets/fonts/TrampMono-Medium.ttf` | Extracted from mockup |
| `lib/ui/chrome/mockup/` | Shell, title bar, screen well, button, slider, LED, rivet, plate, rail painters/widgets |
| `lib/ui/session/session_bus.dart` | Typed IPC envelopes between engines |
| `lib/ui/session/session_host.dart` | Main-engine ownership of controllers + bus server |
| `lib/ui/session/session_client.dart` | Secondary-engine bus client + local view models |
| `lib/ui/docking/docking_coordinator.dart` | Snap, groups, persist layout (pure Dart + tests) |
| `lib/ui/docking/dock_layout.dart` | Serializable positions/shade/visibility |
| `lib/ui/windows/main_player_window.dart` | Main window root |
| `lib/ui/windows/equalizer_window.dart` | EQ window root |
| `lib/ui/windows/playlist_window.dart` | Playlist window root |
| `lib/ui/main_player/mockup_main_player.dart` | 825×348 main body |
| `lib/ui/equalizer/mockup_equalizer.dart` | 825×348 EQ body |
| `lib/ui/playlist/mockup_playlist.dart` | Playlist body + footer |
| `lib/platform/libmpv_bundle.dart` | Resolve/verify full libmpv load path |
| `third_party/libmpv/` | Full builds per OS (or download script + CMake hooks) |
| `tool/extract_mockup_fonts.dart` | One-shot extract TTFs from `player-mockup-2.html` |
| `tool/eq_measure.dart` | Measurement harness for EQ gate |
| `lib/eq/mpv_equalizer_sink.dart` | Real EQ sink |
| `lib/analysis/spectrum_analyzer.dart` | PCM → STFT → 20 bands |
| `lib/playback/mono_controller.dart` | Force-mono flag on engine |
| `docs/adr/0005-full-libmpv.md` | New ADR |
| `docs/adr/0006-multi-window-docking.md` | New ADR |
| `docs/adr/0007-code-constructed-mockup-chrome.md` | New ADR |

**Modified:**

| Path | Change |
|------|--------|
| `pubspec.yaml` | Add `desktop_multi_window`; fonts; remove graphite asset dirs when deleted; keep `media_kit` |
| `lib/main.dart` | Branch main vs secondary window entry |
| `lib/app.dart` | Session host wiring; drop single-shell layout |
| `lib/domain/tramp_settings.dart` | Replace `LowerRegion` with window visibility, shade, dock layout, alwaysOnTop, mono |
| `lib/platform/settings_store.dart` | Persist new settings shape |
| `lib/platform/tramp_window.dart` | Per-window size/resize/AOT/minimize group helpers |
| `lib/ui/zoom/zoom_controller.dart` | Broadcast zoom to all windows via bus |
| `lib/playback/player_engine.dart` | `setMono`, real levels contract |
| `lib/playback/media_kit_player_engine.dart` | Full libmpv props; stop synthetic-as-normal |
| `lib/eq/equalizer_controller.dart` | Drop “noop forever” comments; wire real sink |
| `docs/tramp-v1-spec.md`, `docs/architecture.md`, `CONTEXT.md` | Rewrite |
| `docs/adr/0002-*.md`, `0003-*.md`, `0004-*.md` | Revise / supersede |

**Deleted (after cutover):** `lib/ui/skin/**`, `assets/skin/graphite/**`, `lib/ui/tramp_shell.dart` (replaced by window roots), obsolete lower-region layout helpers that assume one window.

---

### Task 1: Decision hygiene — rewrite docs and supersede ADRs

**Files:**
- Modify: `docs/tramp-v1-spec.md`
- Modify: `docs/architecture.md`
- Modify: `CONTEXT.md`
- Modify: `docs/adr/0002-fixed-canvas-zoom.md`
- Modify: `docs/adr/0003-zoom-only-window-size.md` (Status: Superseded by 0006)
- Modify: `docs/adr/0004-png-graphite-skin.md` (Status: Superseded by 0007)
- Create: `docs/adr/0005-full-libmpv.md`
- Create: `docs/adr/0006-multi-window-docking.md`
- Create: `docs/adr/0007-code-constructed-mockup-chrome.md`

**Interfaces:**
- Consumes: design spec sections 1–15
- Produces: docs that match the redesign; agents must not follow PNG/single-window locks

- [ ] **Step 1: Rewrite `docs/tramp-v1-spec.md`**

Replace window/chrome/EQ/spectrum/non-goals to match the design spec. Keep Flutter locked. State full libmpv, three dockable windows, code chrome, audible EQ, real spectrum, Mono, clutterbar O/A/I. Point UI authority to `player-mockup-2.html` and the 2026-08-08 design doc.

- [ ] **Step 2: Rewrite `docs/architecture.md`**

Update status, mermaid (three windows + session bus + full libmpv), modules table, known gaps (remove chrome-only EQ / synthetic-as-end-state). Link ADRs 0005–0007.

- [ ] **Step 3: Update `CONTEXT.md`**

Phosphor = cyan for current chrome. Add docking / session-host terms. Retire PNG graphite as *current* look. Synthetic levels = failure mode, not product end-state.

- [ ] **Step 4: Write ADRs 0005–0007; mark 0003/0004 superseded; revise 0002**

0002: global zoom across three logical canvases; playlist free resize.  
0005: full libmpv bundled; features first.  
0006: multi-window + Winamp docking.  
0007: code-constructed mockup chrome; PNG skin retired.

- [ ] **Step 5: Commit**

```bash
git add docs/tramp-v1-spec.md docs/architecture.md CONTEXT.md docs/adr/
git commit -m "docs: align spec, architecture, and ADRs with mockup multi-window redesign"
```

---

### Task 2: Extract mockup fonts + token/metrics foundation

**Files:**
- Create: `tool/extract_mockup_fonts.dart`
- Create: `assets/fonts/TrampCondensed-Bold.ttf`
- Create: `assets/fonts/TrampMono-Medium.ttf`
- Create: `lib/theme/mockup_tokens.dart`
- Modify: `lib/theme/tramp_metrics.dart`
- Modify: `lib/theme/tramp_colors.dart` (re-export or replace with mockup tokens)
- Modify: `lib/theme/tramp_text.dart`
- Modify: `pubspec.yaml` (font families)
- Modify: `test/support/test_fonts.dart`
- Test: `test/theme/mockup_tokens_test.dart`
- Test: `test/theme/tramp_metrics_test.dart`

**Interfaces:**
- Consumes: `player-mockup-2.html` `@font-face` base64
- Produces:
  - `MockupTokens.shellHi` … (Color constants)
  - `TrampMetrics.mainPlayer == Size(825, 348)`
  - `TrampMetrics.equalizer == Size(825, 348)`
  - `TrampMetrics.playlistDefault == Size(825, 696)`
  - `TrampMetrics.titleBar == 42.0`
  - Font families `"TrampCondensed"`, `"TrampMono"`

- [ ] **Step 1: Write failing metrics test**

```dart
import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_metrics.dart';

void main() {
  test('mockup canvases are classic×3', () {
    expect(TrampMetrics.mainPlayer, const Size(825, 348));
    expect(TrampMetrics.equalizer, const Size(825, 348));
    expect(TrampMetrics.playlistDefault, const Size(825, 696));
    expect(TrampMetrics.titleBar, 42.0);
  });
}
```

- [ ] **Step 2: Run test — expect FAIL**

Run: `flutter test test/theme/tramp_metrics_test.dart -v`  
Expected: FAIL (old 812×242 values)

- [ ] **Step 3: Implement metrics + tokens**

Set metrics to mockup sizes. Add `mockup_tokens.dart` with exact hex values from the design palette table. Point `TrampColors` phosphor/accent at cyan/magenta tokens (or make `TrampColors` a typedef facade over `MockupTokens` to avoid dual sources).

- [ ] **Step 4: Extract fonts**

`tool/extract_mockup_fonts.dart` parses `player-mockup-2.html` for the two `data:font/ttf;base64,` blobs, writes `assets/fonts/TrampCondensed-Bold.ttf` and `assets/fonts/TrampMono-Medium.ttf`. Register in `pubspec.yaml`. Update `loadTrampFonts()` to load these families (keep old faces only if still referenced — prefer remove Barlow/IBM from chrome path).

- [ ] **Step 5: Run tests — expect PASS; analyze**

Run: `flutter test test/theme/tramp_metrics_test.dart test/theme/mockup_tokens_test.dart`  
Run: `flutter analyze`

- [ ] **Step 6: Commit**

```bash
git add tool/extract_mockup_fonts.dart assets/fonts/Tramp*.ttf lib/theme/ pubspec.yaml test/theme/ test/support/test_fonts.dart
git commit -m "feat(theme): mockup tokens, 825 canvases, and Tramp Condensed/Mono fonts"
```

---

### Task 3: Settings model for multi-window layout

**Files:**
- Modify: `lib/domain/tramp_settings.dart`
- Modify: `lib/platform/settings_store.dart`
- Test: `test/domain/tramp_settings_test.dart` (create or extend)

**Interfaces:**
- Consumes: Task 2 metrics defaults
- Produces:

```dart
class WindowFrameState {
  const WindowFrameState({
    required this.visible,
    required this.shaded,
    required this.left,
    required this.top,
    this.width,
    this.height,
  });
  final bool visible;
  final bool shaded;
  final double left;
  final double top;
  final double? width;  // playlist only; null → default
  final double? height;
}

class DockEdge { /* a-id, b-id, side enum */ }

class TrampSettings {
  final int zoomPercent;
  final bool alwaysOnTop;
  final bool forceMono;
  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final List<DockEdge> dockEdges;
  final EqualizerSettings equalizerCurve; // rename field away from collision if needed
}
```

Remove `LowerRegion`. Migrate `fromJson`: if old `lowerRegion` present, map `equalizer`→EQ visible / PL hidden and vice versa; ignore unknown keys safely.

- [ ] **Step 1: Write failing round-trip tests** for visibility, shade, dock edges, mono, AOT, playlist size

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement model + store migration**

- [ ] **Step 4: Run — expect PASS; fix any broken tests that referenced `LowerRegion`**

Run: `flutter test test/domain/tramp_settings_test.dart`  
Also run full suite and update call sites that no longer compile (temporary stubs OK only inside this commit if Task 4 follows immediately — prefer fix compile breaks here with minimal `visible` mapping in old shell).

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(settings): persist multi-window layout, mono, and always-on-top"
```

---

### Task 4: DockingCoordinator (pure Dart)

**Files:**
- Create: `lib/ui/docking/dock_layout.dart`
- Create: `lib/ui/docking/docking_coordinator.dart`
- Test: `test/ui/docking/docking_coordinator_test.dart`

**Interfaces:**
- Consumes: `TrampMetrics`, snap `12.0`, break `48.0`
- Produces:

```dart
enum WindowId { main, equalizer, playlist }

class DockingCoordinator extends ChangeNotifier {
  DockingCoordinator(DockLayout initial);
  DockLayout get layout;
  void move(WindowId id, Offset topLeft, {required bool shiftUndock});
  void resizePlaylist(Size logical);
  void setShaded(WindowId id, bool shaded);
  void setVisible(WindowId id, bool visible);
  Set<WindowId> groupOf(WindowId id);
  /// Pixel frames at current zoom for window_manager.
  Rect frameFor(WindowId id, double zoom);
}
```

Rules: when `move` ends (caller invokes `move` on drag-update/end), if an edge of `id` is within 12px of another visible window edge, record a `DockEdge` and snap position. Moving a window moves all in `groupOf`. If `shiftUndock` or separation > 48 after drag, remove edges involving `id`.

- [ ] **Step 1: Write failing tests** — snap below, group drag, shift undock, shade height collapses to `titleBar`, playlist resize keeps top-left

```dart
test('snaps playlist below main when within 12px', () {
  final c = DockingCoordinator(DockLayout.defaults);
  c.move(WindowId.playlist, Offset(0, 348 - 10), shiftUndock: false);
  expect(c.layout.playlist.top, 348);
  expect(c.groupOf(WindowId.playlist), contains(WindowId.main));
});
```

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Implement coordinator**

- [ ] **Step 4: Run — expect PASS**

- [ ] **Step 5: Commit**

```bash
git commit -m "feat(docking): Winamp-style snap, group drag, and shade layout"
```

---

### Task 5: Session bus + multi-window entrypoints

**Files:**
- Create: `lib/ui/session/session_messages.dart`
- Create: `lib/ui/session/session_bus.dart`
- Create: `lib/ui/session/session_host.dart`
- Create: `lib/ui/session/session_client.dart`
- Modify: `lib/main.dart`
- Modify: `pubspec.yaml` (add `desktop_multi_window`)
- Modify: `windows/`, `macos/`, `linux/` as required by the plugin README (register plugins per window)
- Test: `test/ui/session/session_messages_test.dart`

**Interfaces:**
- Consumes: controllers on host
- Produces: JSON-serializable messages, e.g.

```dart
sealed class SessionEvent { /* ZoomChanged, PlaybackSnapshot, EqSnapshot, PlaylistSnapshot, DockSnapshot, LevelsFrame */ }
sealed class SessionCommand { /* Transport, Seek, Volume, Mono, ToggleWindow, EqGain, PlaylistOp, ZoomStep, AlwaysOnTop */ }
```

Host registers `WindowMethodChannel('tramp/session')`. Secondary windows start via `DesktopMultiWindow.createWindow(jsonEncode({'role':'equalizer'}))` etc. `main.dart`:

```dart
Future<void> main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();
  final role = parseMultiWindowRole(args); // plugin-specific
  if (role == WindowRole.main) {
    runApp(SessionHostApp());
  } else {
    runApp(SessionClientApp(role: role));
  }
}
```

Secondary apps render placeholder `ColoredBox` + title text until Task 6–8 (but must show/hide/move via host docking frames).

- [ ] **Step 1: Add dependency; message codec unit tests**

- [ ] **Step 2: Implement bus + host/client shells; create EQ/PL windows from host on startup per settings visibility**

- [ ] **Step 3: Manual smoke on Windows** — three windows appear; closing EQ hides; closing main exits  

Document smoke in commit message; automate what can be unit-tested (codec, role parse).

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(session): multi-window host/client bus with desktop_multi_window"
```

---

### Task 6: Mockup chrome primitives

**Files:**
- Create: `lib/ui/chrome/mockup/mockup_shell.dart`
- Create: `lib/ui/chrome/mockup/mockup_title_bar.dart`
- Create: `lib/ui/chrome/mockup/mockup_screen.dart`
- Create: `lib/ui/chrome/mockup/mockup_button.dart`
- Create: `lib/ui/chrome/mockup/mockup_slider.dart`
- Create: `lib/ui/chrome/mockup/mockup_led.dart`
- Create: `lib/ui/chrome/mockup/mockup_icons.dart` (SVG paths from mockup)
- Test: `test/ui/chrome/mockup/mockup_chrome_golden_test.dart`

**Interfaces:**
- Consumes: `MockupTokens`, fonts
- Produces: reusable widgets matching `.win`, `.tbar`, `.screen`, `.btn`, `.btn--on`, `.track`, `.thumb`, `.led`, `.plate`, `.rail`, `.rivet`

- [ ] **Step 1: Write golden tests** for title bar strip, on/off button, slider at 66%, LED lit/unlit — compare to cropped mockup regions OR tight pixel tolerances against first committed goldens generated from the widgets (then visually diff against HTML in review)

- [ ] **Step 2: Implement painters/widgets by porting CSS (gradients, radii, shadows, scanlines)**

- [ ] **Step 3: `loadTrampFonts()` in golden setUpAll; update goldens on Windows

Run: `flutter test test/ui/chrome/mockup/mockup_chrome_golden_test.dart --update-goldens`

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(chrome): code-constructed mockup shell primitives and goldens"
```

---

### Task 7: Main player window (full mockup body)

**Files:**
- Create: `lib/ui/main_player/mockup_main_player.dart`
- Create: `lib/ui/windows/main_player_window.dart`
- Modify: `lib/ui/session/session_host.dart` (mount main player)
- Test: `test/ui/main_player/mockup_main_player_golden_test.dart`
- Test: `test/ui/main_player/mockup_main_player_test.dart` (callbacks)

**Interfaces:**
- Consumes: chrome primitives, `PlaybackController` snapshots, levels stream, zoom commands
- Produces: full **825×306** body + title bar behaviors

Layout absolute rects from mockup (clutterbar 22,18,26×129; display 96,14,705×132; vol row top 156; seek 206; transport 246). Clutterbar **O/A/I** only. Wire: Mono command, EQ/PL visibility, shuffle/repeat, separate play/pause, open files, zoom ±, minimize group, close quit.

- [ ] **Step 1: Golden at 100% vs mockup capture** (export PNG from opening `player-mockup-2.html` main section or full-page crop). Fail until match within tolerance `0` for structure; allow tiny AA delta if needed but treat material mismatch as fail.

- [ ] **Step 2: Implement layout + wiring**

- [ ] **Step 3: Interaction tests** with fake controller (EQ toggle sends `ToggleWindow`, Mono sends `SetMono`)

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(ui): mockup-faithful main player window"
```

---

### Task 8: Equalizer window

**Files:**
- Create: `lib/ui/equalizer/mockup_equalizer.dart`
- Create: `lib/ui/windows/equalizer_window.dart`
- Create: `lib/ui/equalizer/eq_curve_painter.dart`
- Test: goldens + controller callback tests

**Interfaces:**
- Consumes: `EqualizerSettings`, session commands `SetEqGain`, `SetPreamp`, `SetEqEnabled`, `SetEqAuto`, `ApplyPreset`
- Produces: 825×348 EQ matching mockup; shade via title collapse; live curve from gains

Band frequencies remain `[60,170,310,600,1000,3000,6000,12000,14000,16000]`; ±12 dB.

- [ ] **Step 1: Golden + failing callback tests**

- [ ] **Step 2: Implement**

- [ ] **Step 3: Pass tests; commit**

```bash
git commit -m "feat(ui): mockup-faithful equalizer window with live curve"
```

---

### Task 9: Playlist window

**Files:**
- Create: `lib/ui/playlist/mockup_playlist.dart`
- Create: `lib/ui/windows/playlist_window.dart`
- Modify: docking resize hooks in host
- Test: goldens + list/selection tests

**Interfaces:**
- Consumes: `PlaylistController`, DnD via `desktop_drop` on this window
- Produces: resizable playlist matching mockup footer (add/remove/sort/options/mini transport/TOTAL/status)

Sort menu: title, artist, duration, path, reverse. Options: load, save, clear, select all, invert selection.

- [ ] **Step 1: Tests/goldens**

- [ ] **Step 2: Implement; enable edge resize only on playlist window**

- [ ] **Step 3: Commit**

```bash
git commit -m "feat(ui): mockup-faithful resizable playlist window"
```

---

### Task 10: Full libmpv bundle + load verification

**Files:**
- Create: `third_party/libmpv/README.md` (build/download instructions)
- Create: `tool/fetch_full_libmpv.ps1` / `.sh` as needed
- Create: `lib/platform/libmpv_bundle.dart`
- Modify: Windows/macOS/Linux CMake/Podfile/`CMakeLists` to prefer `third_party/libmpv/...` over media_kit slim libs
- Modify: `pubspec.yaml` if needed to stop packing slim libs (document override)
- Test: `test/platform/libmpv_bundle_test.dart` (path resolution; feature string contains filters when running integration)

**Interfaces:**
- Produces: `Future<LibmpvInfo> LibmpvBundle.verify()` → `{path, hasFilters: true}`  
  Fail app startup in debug/profile if slim binary detected (search config string for `--disable-filters` without full equalizer/aresample support).

- [ ] **Step 1: Spike on Windows** — place full `libmpv-2.dll`, confirm media_kit loads it; `getProperty`/`setProperty` for `af` works

- [ ] **Step 2: Automate fetch/build docs + CMake hook**

- [ ] **Step 3: `LibmpvBundle.verify()` + unit test for detection heuristics on fixture strings

- [ ] **Step 4: Commit binaries via Git LFS **or** scripted download in CI/README — do not commit slim libs. Prefer script + verified URL pins in README.

```bash
git commit -m "feat(mpv): bundle and force-load full libmpv instead of compressed media_kit libs"
```

---

### Task 11: Audible EQ sink (measurement-gated)

**Files:**
- Create: `lib/eq/mpv_equalizer_sink.dart`
- Create: `tool/eq_measure.dart`
- Modify: `lib/eq/equalizer_controller.dart` (comments + default sink wiring)
- Modify: `lib/app.dart` / session host to use `MpvEqualizerSink` only when verify+measure pass; else keep noop and force EQ On UI to show disabled tooltip — **prefer:** run measure in `tool/` as release gate; production uses real sink once gate is green on the branch
- Test: `test/eq/mpv_equalizer_sink_test.dart` (string builder unit tests)
- Integration: `tool/eq_measure.dart` exit 0 on Windows with full libmpv

**Interfaces:**

```dart
class MpvEqualizerSink implements EqualizerSink {
  MpvEqualizerSink(Player player);
  Future<void> apply(EqualizerSettings settings);
}

String buildEqualizerAf(EqualizerSettings s); // pure, unit-tested
```

When `settings.enabled == false`, set `af` to empty/clear. When true, build lavfi equalizer chain for preamp + 10 bands.

Measurement tool: generate or use fixture tones → play through mpv with EQ → compare band energy to flat baseline; assert ≥6 dB delta at boosted band center.

- [ ] **Step 1: Unit-test `buildEqualizerAf`**

- [ ] **Step 2: Implement sink + wire**

- [ ] **Step 3: Run `dart run tool/eq_measure.dart` — must PASS before marking task done**

- [ ] **Step 4: Commit**

```bash
git commit -m "feat(eq): audible mpv equalizer sink verified by measurement"
```

---

### Task 12: Real spectrum analyser + Mono + cutover cleanup

**Files:**
- Create: `lib/analysis/spectrum_analyzer.dart`
- Modify: `lib/playback/media_kit_player_engine.dart` — publish analyser levels (`synthetic: false`); remove synthesised path from normal play
- Create: `lib/playback/mono_controller.dart` or methods on engine: `setForceMono(bool)`
- Modify: main player Mono button → settings + engine
- Modify: `lib/ui/chrome/spectrum_visualizer.dart` for cyan gradient mockup look
- Delete: `lib/ui/skin/**`, `assets/skin/graphite/**`, `lib/ui/tramp_shell.dart` if still present
- Modify: `pubspec.yaml` asset list
- Update goldens for main display with real-looking quiet/silent frames
- Test: analyser unit tests with known PCM impulse; mono property tests with fake platform

**Interfaces:**

```dart
class SpectrumAnalyzer {
  Stream<AudioLevels> attach({required String path, required Stream<bool> playing});
}
```

- [ ] **Step 1: Analyser tests with synthetic PCM fixture (not `AudioLevels.synthesised` product path)**

- [ ] **Step 2: Implement PCM→STFT→20 bands; wire engine**

- [ ] **Step 3: Mono property integration**

- [ ] **Step 4: Delete PNG skin; ensure app runs only mockup chrome**

- [ ] **Step 5: Full `flutter analyze` + `flutter test`; manual fidelity pass vs mockup**

- [ ] **Step 6: Final docs touch if anything drifted; commit**

```bash
git commit -m "feat(audio): real spectrum and force-mono; remove PNG graphite skin"
```

---

## Self-review (plan vs spec)

| Spec section | Task(s) |
|--------------|---------|
| Fidelity / tokens / fonts | 2, 6, 7–9 |
| Three windows + docking + zoom + shade + close | 3, 4, 5, 7–9 |
| Code chrome / no PNG | 6–9, 12 |
| Clutterbar O/A/I, Mono, separate play/pause | 7, 12 |
| EQ curve + audible EQ | 8, 11 |
| Playlist footer/menus/resize | 9 |
| Full libmpv | 10 |
| Real spectrum | 12 |
| Docs/ADR hygiene | 1, 12 |
| Measurement gate | 11 |

No TBD placeholders. Pins table locks docking/mpv/multi-window choices.

---

## Execution notes

- Prefer implementing on a feature branch / worktree.
- Task 10–11 may need network to fetch libmpv sources/binaries (`all` permissions).
- Goldens are Windows-authored unless CI gains per-OS sets (existing project convention).
- If `desktop_multi_window` + current `window_manager` conflict, use the plugin’s documented `window_manager` git fork pin — record the pin in `pubspec.yaml` and ADR 0006.
