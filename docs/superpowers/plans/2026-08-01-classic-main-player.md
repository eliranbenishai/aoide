# Classic Main Player Chrome Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace paper/ink player chrome with a scalable, vector-only classic Winamp–inspired main player (and matching playlist chrome) matching the locked mockup.

**Architecture:** Keep `PlaybackController` / `PlaylistController` unchanged. Introduce chrome tokens + vector painters, a fixed-aspect `ClassicMainPlayer` scaled uniformly in `TrampShell`, then restyle `PlaylistPanel` with the same tokens. No bitmap skin assets.

**Tech Stack:** Flutter desktop, existing `window_manager`, `CustomPainter` / decorations, IBM Plex Mono for LCD text, media_kit playback (spectrum pulse first; real levels only if cheap).

## Global Constraints

- Visual source of truth: `docs/superpowers/specs/assets/tramp-main-player-idealized-mockup.png`
- Design: `docs/superpowers/specs/2026-08-01-classic-main-player-design.md`
- No EQ, no balance slider, no eject on main player
- No raster images for chrome/logo/icons
- Player scales as one unit (uniform scale); playlist fills remaining window
- Mute via volume speaker affordance (no extra mute button)
- Update `docs/tramp-v1-spec.md` + `docs/architecture.md` in the docs task
- Platforms: Windows / Linux / macOS Flutter desktop

## File map

| File | Role |
|------|------|
| `lib/theme/tramp_colors.dart` | Replace paper/ink with metal/LCD tokens |
| `lib/theme/tramp_theme.dart` | Dark metal scaffold theme |
| `lib/ui/chrome/metal_panel.dart` | Beveled metal / LCD inset decorations |
| `lib/ui/chrome/chrome_button.dart` | Raised metal button + pressed bevel |
| `lib/ui/chrome/chrome_slider.dart` | Groove + metal thumb + green fill |
| `lib/ui/chrome/transport_icons.dart` | Vector prev/play/pause/stop/next painters |
| `lib/ui/chrome/tramp_logo.dart` | Soft pin-up logo `CustomPainter` |
| `lib/ui/chrome/spectrum_visualizer.dart` | Bar painter + pulse (optional levels later) |
| `lib/ui/classic_main_player.dart` | Full main-player layout + wiring |
| `lib/ui/tramp_shell.dart` | TitleBar+transport → scaled ClassicMainPlayer; metal shell |
| `lib/app.dart` | Pass `ClassicMainPlayer` instead of `TransportPanel` |
| `lib/ui/playlist_panel.dart` | Metal/LCD chrome; keep list behavior |
| `lib/ui/title_bar.dart` / `transport_panel.dart` | Delete or leave unused then delete |
| `lib/ui/widgets/ink_slider.dart` / `tramp_button.dart` | Retire from player/playlist; delete if unused |
| Tests under `test/theme/`, `test/ui/` | Token + chrome wiring tests |
| Docs | Spec / architecture / README UI direction |

---

### Task 1: Chrome color tokens

**Files:**
- Modify: `lib/theme/tramp_colors.dart`
- Modify: `lib/theme/tramp_theme.dart`
- Modify: `test/theme/tramp_colors_test.dart`

**Interfaces:**
- Consumes: nothing
- Produces: `TrampColors` metal/LCD constants used by all later UI

- [ ] **Step 1: Write the failing token test**

Replace `test/theme/tramp_colors_test.dart` with:

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/theme/tramp_colors.dart';

void main() {
  test('classic chrome tokens are metal + LCD (not paper/ink)', () {
    expect(TrampColors.metalFace, isNot(const Color(0xFFF2EBE0)));
    expect(TrampColors.lcdBackground.value & 0x00FF00, greaterThan(0));
    expect(TrampColors.lcdPhosphor.green, greaterThan(TrampColors.lcdPhosphor.red));
    expect(TrampColors.metalMid, isNot(TrampColors.metalFace));
    expect(TrampColors.metalShadow, isNot(TrampColors.metalFace));
  });
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `flutter test test/theme/tramp_colors_test.dart`

Expected: FAIL (missing symbols / still paper tokens)

- [ ] **Step 3: Implement tokens + theme**

Rewrite `lib/theme/tramp_colors.dart` (sample mockup-derived values; tweak to match PNG while implementing):

```dart
import 'package:flutter/material.dart';

abstract final class TrampColors {
  static const metalHi = Color(0xFFE4E4E4);
  static const metalFace = Color(0xFFB8B8B8);
  static const metalMid = Color(0xFF9A9A9A);
  static const metalShadow = Color(0xFF6E6E6E);
  static const metalDeep = Color(0xFF4A4A4A);
  static const groove = Color(0xFF3A3A3A);

  static const lcdBackground = Color(0xFF0A1A0A);
  static const lcdPhosphor = Color(0xFF33FF33);
  static const lcdPhosphorDim = Color(0xFF1A8A1A);
  static const lcdPeak = Color(0xFFCCFF33);

  static const fillAccent = Color(0xFF2ECC40);
  static const windowClose = Color(0xFFC44C4C);

  static const skinBorder = Color(0xFF555555);
  static const borderWidth = 1.0;

  // Compatibility aliases while migrating call sites in later tasks:
  static const surface = metalFace;
  static const ink = metalDeep;
  static const accent = fillAccent;
  static const muted = metalShadow;
}
```

Update `buildTrampTheme()` to `Brightness.dark`-ish metal scaffold: `scaffoldBackgroundColor: TrampColors.metalMid`, `ColorScheme` on metal/LCD, keep `GoogleFonts.ibmPlexMonoTextTheme` for body; drop Syne as primary brand face (wordmark is chrome typography in the player).

- [ ] **Step 4: Run token test**

Run: `flutter test test/theme/tramp_colors_test.dart`

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add lib/theme/tramp_colors.dart lib/theme/tramp_theme.dart test/theme/tramp_colors_test.dart
git commit -m "feat: add classic metal/LCD chrome color tokens"
```

---

### Task 2: Metal panel, chrome button, chrome slider

**Files:**
- Create: `lib/ui/chrome/metal_panel.dart`
- Create: `lib/ui/chrome/chrome_button.dart`
- Create: `lib/ui/chrome/chrome_slider.dart`
- Create: `test/ui/chrome/chrome_slider_test.dart`

**Interfaces:**
- Consumes: `TrampColors`
- Produces:
  - `MetalPanel({required Widget child, MetalPanelStyle style})` with `MetalPanelStyle.raised | insetLcd`
  - `ChromeButton({required Widget child, VoidCallback? onPressed, bool primary})`
  - `ChromeSlider({required double value, ValueChanged<double>? onChanged, ValueChanged<double>? onChangeEnd})` value in `0..1`

- [ ] **Step 1: Write failing slider semantics test**

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/ui/chrome/chrome_slider.dart';

void main() {
  testWidgets('ChromeSlider reports drag end value', (tester) async {
    double? ended;
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: SizedBox(
            width: 200,
            height: 40,
            child: ChromeSlider(
              value: 0.2,
              onChanged: (_) {},
              onChangeEnd: (v) => ended = v,
            ),
          ),
        ),
      ),
    );
    await tester.drag(find.byType(ChromeSlider), const Offset(100, 0));
    await tester.pumpAndSettle();
    expect(ended, isNotNull);
    expect(ended!, greaterThan(0.2));
  });
}
```

- [ ] **Step 2: Run test — expect FAIL**

Run: `flutter test test/ui/chrome/chrome_slider_test.dart`

- [ ] **Step 3: Implement primitives**

`MetalPanel`: `DecoratedBox` with gradient (`metalHi`→`metalMid`→`metalShadow`) for raised; for `insetLcd` use `lcdBackground`, inner shadow via border (`skinBorder` outer, dark inset).

`ChromeButton`: `GestureDetector` + raised metal decoration; `onTap` → `onPressed`; disabled when `onPressed == null` (dim icons). Child centered. Min size ~ square for transport.

`ChromeSlider`: horizontal track groove (`groove`), green `fillAccent` fill to `value`, metal thumb; drag updates `onChanged` / `onChangeEnd` with clamped `0..1`.

Keep painters vector-only (gradients/borders), no images.

- [ ] **Step 4: Run test — expect PASS**

Run: `flutter test test/ui/chrome/chrome_slider_test.dart`

- [ ] **Step 5: Commit**

```bash
git add lib/ui/chrome/ test/ui/chrome/chrome_slider_test.dart
git commit -m "feat: add metal panel, chrome button, and chrome slider"
```

---

### Task 3: Transport icons + ClassicMainPlayer geometry host

**Files:**
- Create: `lib/ui/chrome/transport_icons.dart`
- Create: `lib/ui/classic_main_player.dart` (geometry shell first)
- Modify: `lib/ui/tramp_shell.dart`
- Modify: `lib/app.dart`
- Modify: `test/widget_test.dart`
- Create: `test/ui/classic_main_player_layout_test.dart`

**Interfaces:**
- Consumes: chrome widgets, `PlaybackController` (later wiring)
- Produces:
  - `ClassicMainPlayer.logicalSize` → `Size(550, 232)` (classic ~275∶116 ×2; adjust if measuring mockup chrome box)
  - `ClassicMainPlayer({required PlaybackController playback, required bool hasTracks, VoidCallback? onFocusPlaylist})`
  - `TransportIcons.prev/play/pause/stop/next` as `CustomPainter` or icon widgets sized to button

- [ ] **Step 1: Write failing layout test**

```dart
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/ui/classic_main_player.dart';

class _Mem implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;
  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  testWidgets('ClassicMainPlayer keeps logical aspect under wide parent', (tester) async {
    final playlist = PlaylistController(store: _Mem());
    final playback = PlaybackController(playlist: playlist, engine: FakePlayerEngine());
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SizedBox(
            width: 900,
            height: 400,
            child: FittedBox(
              fit: BoxFit.contain,
              child: ClassicMainPlayer(playback: playback, hasTracks: false),
            ),
          ),
        ),
      ),
    );
    await tester.pump();
    final size = tester.getSize(find.byType(ClassicMainPlayer));
    expect(size.width / size.height, closeTo(550 / 232, 0.05));
    await playback.dispose();
  });
}
```

- [ ] **Step 2: Run — expect FAIL**

Run: `flutter test test/ui/classic_main_player_layout_test.dart`

- [ ] **Step 3: Implement geometry shell + shell integration**

`ClassicMainPlayer.build`:

```dart
SizedBox(
  width: logicalSize.width,
  height: logicalSize.height,
  child: MetalPanel(
    style: MetalPanelStyle.raised,
    child: Column(
      children: [
        // title bar stub: TRAMP text + min/close
        // LCD stubs (empty MetalPanel insetLcd)
        // seek ChromeSlider stub
        // transport row stubs + volume ChromeSlider stub
      ],
    ),
  ),
);
```

Title bar: `DragToMoveArea` wrapping brand row; minimize/close via `window_manager` (same as old `TitleBar`).

`TrampShell`: remove separate `TitleBar`; column is `[Scaled player, Expanded(playlist)]` where scaled player is:

```dart
LayoutBuilder(
  builder: (context, constraints) {
    return Align(
      alignment: Alignment.topCenter,
      child: FittedBox(
        fit: BoxFit.contain,
        child: transport, // ClassicMainPlayer from app
      ),
    );
  },
);
```

Shell background/`border` → metal tokens (not paper/ink).

`app.dart`: `transport: ClassicMainPlayer(playback: _playback, hasTracks: ...)`.

Update `test/widget_test.dart` to still find `TRAMP`.

Delete usage of old `TitleBar` from shell (file can remain until Task 8 cleanup).

- [ ] **Step 4: Run layout + widget tests**

Run: `flutter test test/ui/classic_main_player_layout_test.dart test/widget_test.dart test/ui/tramp_shell_shortcuts_test.dart`

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add lib/ui/classic_main_player.dart lib/ui/chrome/transport_icons.dart lib/ui/tramp_shell.dart lib/app.dart test/
git commit -m "feat: add scaled ClassicMainPlayer shell geometry"
```

---

### Task 4: Wire LCD, seek, volume, transport, shuffle/repeat/PL

**Files:**
- Modify: `lib/ui/classic_main_player.dart`
- Create: `test/ui/classic_main_player_controls_test.dart`
- Modify: `lib/ui/tramp_shell.dart` (optional `playlistFocusNode`)

**Interfaces:**
- Consumes: `PlaybackController` APIs: `playPause`, `stop`, `previous`, `next`, `seek`, `setVolume`, `toggleMute`, `toggleShuffle`, `cycleRepeatMode`, `playing`, `position`, `duration`, `volume`, `muted`, `shuffle`, `repeatMode`, `currentTrack`
- Produces: working controls; LCD title line; time; SHUF/REP/PL; kbps/kHz show `—` when unknown (Track has no bitrate yet — do not expand domain in this task); STEREO label when a track is open

- [ ] **Step 1: Write failing controls test**

```dart
// pump ClassicMainPlayer with FakePlayerEngine + one track
// tap find.byTooltip('Play') or Key('transport-play') → expect playback.playing
// tap Stop → expect !playing
// verify find.textContaining('TRAMP')
```

Use `Key('transport-play')`, `Key('transport-stop')`, etc. on buttons for stable finds.

- [ ] **Step 2: Run — expect FAIL**

- [ ] **Step 3: Wire UI**

Layout matching mockup:

1. Title: logo placeholder `SizedBox` (real logo Task 6) | chrome **TRAMP** text | drag | min | close  
2. LCD row: left spectrum placeholder + `formatDuration(position)`; right scrolling title (`displayTitle` / artist), `— kbps`, `— kHz`, `STEREO`, SHUF/REP tappable, PL calls `onFocusPlaylist`  
3. Seek `ChromeSlider` bound to position/duration  
4. Five `ChromeButton`s with `TransportIcons` + volume `ChromeSlider`; speaker icon toggles mute  

Reuse duration formatting (move `formatDuration` from `transport_panel.dart` into `classic_main_player.dart` or `lib/ui/format.dart`).

Focus playlist: pass `FocusNode? playlistFocusNode` from shell into playlist `Focus` and into `ClassicMainPlayer.onFocusPlaylist: () => playlistFocusNode.requestFocus()`.

- [ ] **Step 4: Run controls + shortcut tests**

Run: `flutter test test/ui/classic_main_player_controls_test.dart test/ui/tramp_shell_shortcuts_test.dart`

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add lib/ui/classic_main_player.dart lib/ui/tramp_shell.dart test/ui/classic_main_player_controls_test.dart
git commit -m "feat: wire classic player transport and LCD controls"
```

---

### Task 5: Spectrum visualizer (pulse)

**Files:**
- Create: `lib/ui/chrome/spectrum_visualizer.dart`
- Modify: `lib/ui/classic_main_player.dart`
- Create: `test/ui/chrome/spectrum_visualizer_test.dart`

**Interfaces:**
- Consumes: `bool playing`, `double volume`
- Produces: `SpectrumVisualizer({required bool playing, double volume = 1})` — `CustomPainter` bars; pulse while playing; idle/low when not

- [ ] **Step 1: Write test that widget builds and uses CustomPaint**

```dart
testWidgets('SpectrumVisualizer paints while playing', (tester) async {
  await tester.pumpWidget(
    const MaterialApp(
      home: SpectrumVisualizer(playing: true, volume: 0.8),
    ),
  );
  expect(find.byType(CustomPaint), findsWidgets);
});
```

- [ ] **Step 2: Run — FAIL then implement pulse**

Use `AnimationController` repeating while `playing`; bar heights from `sin(time + i)` × volume. Colors `lcdPhosphor` / `lcdPeak`. No bitmaps. Skip real FFT unless media_kit exposes levels with <1h effort; if added, keep pulse fallback.

- [ ] **Step 3: Run test PASS + commit**

```bash
git add lib/ui/chrome/spectrum_visualizer.dart lib/ui/classic_main_player.dart test/ui/chrome/spectrum_visualizer_test.dart
git commit -m "feat: add vector spectrum visualizer with play pulse"
```

---

### Task 6: Soft pin-up logo painter

**Files:**
- Create: `lib/ui/chrome/tramp_logo.dart`
- Modify: `lib/ui/classic_main_player.dart`
- Create: `test/ui/chrome/tramp_logo_test.dart`

**Interfaces:**
- Produces: `TrampLogo({double size = 28})` — `CustomPaint` soft illustrative face (eyes closed, headphones), vector gradients only; compare visually to mockup

- [ ] **Step 1: Test finds CustomPaint / TrampLogo in player title bar**

- [ ] **Step 2: Implement painter** (paths for head silhouette, hair, closed eyes, headphones cups/band; soft radial gradients for skin). Iterate against mockup at title-bar size.

- [ ] **Step 3: Manual check on Windows (`flutter run -d windows`) at normal and large window widths — logo stays sharp.

- [ ] **Step 4: Commit**

```bash
git add lib/ui/chrome/tramp_logo.dart lib/ui/classic_main_player.dart test/ui/chrome/tramp_logo_test.dart
git commit -m "feat: add vector Tramp headphones pin-up logo"
```

---

### Task 7: Playlist panel chrome restyle

**Files:**
- Modify: `lib/ui/playlist_panel.dart`
- Modify: `test/playlist/` only if assertions depend on paper colors
- Keep list/reorder/open/save/add behavior identical

**Interfaces:**
- Consumes: `TrampColors`, `ChromeButton` (or metal text buttons) for Open/Save/Add
- Produces: metal background, LCD-ish selection highlight, same callbacks

- [ ] **Step 1: Identify paper/ink usages in `playlist_panel.dart` and replace with metal/LCD tokens; toolbar buttons → `ChromeButton` or compact metal text buttons.**

- [ ] **Step 2: Run** `flutter test test/playlist/ test/widget_test.dart`

Expected: PASS

- [ ] **Step 3: Commit**

```bash
git add lib/ui/playlist_panel.dart
git commit -m "feat: restyle playlist panel with classic chrome tokens"
```

---

### Task 8: Cleanup + docs alignment

**Files:**
- Delete unused: `lib/ui/transport_panel.dart`, `lib/ui/title_bar.dart`, and `ink_slider.dart` / `tramp_button.dart` if no remaining references
- Modify: `docs/tramp-v1-spec.md` (UI direction → classic chrome; remove paper/ink lock; note skins still non-goal as WSZ)
- Modify: `docs/architecture.md` (modules list ClassicMainPlayer / chrome)
- Modify: `README.md` if it describes paper/ink UI
- Modify: design status line in `docs/superpowers/specs/2026-08-01-classic-main-player-design.md` → Approved / implemented-in-progress as appropriate
- Run full `flutter test`

- [ ] **Step 1: `rg "TransportPanel|TitleBar|InkSlider|TrampButton|transportWash|Syne" lib test` — fix or delete leftovers**

- [ ] **Step 2: Update product + architecture docs to describe classic vector chrome, scaled main player, matching playlist chrome**

- [ ] **Step 3: Full test suite**

Run: `flutter test`

Expected: PASS

- [ ] **Step 4: Manual visual compare vs mockup on Windows at ~100% and stretched window**

- [ ] **Step 5: Commit**

```bash
git add -A lib docs README.md
git commit -m "docs: align v1 UI direction with classic chrome; remove paper/ink player"
```

---

## Spec coverage checklist

| Spec item | Task |
|-----------|------|
| Metal/LCD tokens, no bitmaps | 1–2, 6 |
| Fixed aspect + uniform scale | 3 |
| Layout: title/LCD/seek/transport/volume | 3–4 |
| No EQ/balance/eject | 4 (omit) |
| Shuffle/repeat/PL/mute affordance | 4 |
| Spectrum pulse (± real later) | 5 |
| Soft pin-up logo + TRAMP | 4 stub, 6 polish |
| Playlist matching chrome | 7 |
| Docs update | 8 |
| Controller APIs unchanged | all |

## Placeholder / consistency self-review

- No TBD steps; bitrate stays `—` without domain change.
- `logicalSize` `550×232` consistent across Task 3 tests and widget.
- `ChromeSlider` / `ChromeButton` names consistent in later tasks.
