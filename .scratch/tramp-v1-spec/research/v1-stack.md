# Tramp v1 implementation stack

**Question:** Which stack should the Tramp v1 product spec assume for a UX-first, multi-platform desktop player (Windows, Linux, macOS)?

**Candidates:** Flutter (primary) vs Tauri 2 + Rust (runner-up). Electron is out of scope for this cut.

**Date:** 2026-08-01

---

## Recommendation

**Assume Flutter for Tramp v1**, with desktop packaging via `flutter build` for Windows / macOS / Linux, custom app chrome via the `window_manager` plugin, and local playback via **media_kit** (libmpv) so all v1 track kinds are covered on every desktop OS.

Prefer **Tauri 2 + Rust** only when the team’s strength is web UI + Rust systems work *and* they accept a WebView-rendered shell (with careful list virtualization) plus a multi-crate audio path (Symphonia/rodio for most formats, a separate Opus decoder for Opus). That is a viable engineering choice; it is a weaker fit for a UX-first, dense, high-frequency player chrome.

---

## Decision criteria (from ticket)

1. UX flexibility — custom app chrome, edge resize, drag-to-move, dense playlist UI, high-frequency UI updates  
2. Performance — must clear an “extremely performant” bar for a music player shell; hybrid web/native only if evidence supports it  
3. Local audio — MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus on Win/Linux/macOS  
4. Packaging — realistic desktop distribution from one codebase on all three OSes  

---

## 1. UX flexibility

### Flutter

Flutter owns pixel-level UI: the framework is a reactive widget toolkit that paints through its own engine, not OS controls or a browser DOM ([Flutter architectural overview](https://docs.flutter.dev/resources/architectural-overview)). That matches Tramp’s **app chrome** and **scalable UI** goals (full custom surface).

Desktop window chrome is handled by the widely used `window_manager` plugin (Linux / macOS / Windows):

- Hide the OS title bar with `TitleBarStyle.hidden` in `WindowOptions` ([window_manager docs](https://pub.dev/documentation/window_manager/latest/)).
- Drag-to-move via `DragToMoveArea` when the title bar is hidden ([DragToMoveArea API](https://pub.dev/documentation/window_manager/latest/window_manager/DragToMoveArea-class.html)).
- Edge resize via `DragToResizeArea` with configurable edge size ([DragToResizeArea API](https://pub.dev/documentation/window_manager/latest/window_manager/DragToResizeArea-class.html)).

Dense, playlist-centric lists are a first-class pattern: `ListView.builder` builds only visible children, which is the documented approach for large or infinite lists ([ListView.builder](https://api.flutter.dev/flutter/widgets/ListView/ListView.builder.html)). High-frequency UI (seek bar, now-playing row) fits Flutter’s model where `build()` is expected to be cheap and may run as often as once per frame ([architectural overview — reactive UIs](https://docs.flutter.dev/resources/architectural-overview)).

### Tauri 2 + Rust

Tauri renders HTML/CSS/JS in an OS WebView (WRY + platform webviews) with a Rust backend ([Tauri Architecture](https://v2.tauri.app/concept/architecture/)). Custom chrome is officially supported:

- `decorations: false` removes OS borders/title bar ([Window Customization](https://v2.tauri.app/learn/window-customization/)).
- Drag-to-move via `data-tauri-drag-region` or `getCurrentWindow().startDragging()` ([same page](https://v2.tauri.app/learn/window-customization/); [Window API](https://v2.tauri.app/reference/javascript/api/namespacewindow/)).
- Edge resize via `startResizeDragging(direction)` ([Window API — startResizeDragging](https://v2.tauri.app/reference/javascript/api/namespacewindow/#startresizing)).

**Caveat:** frameless resize has historically been fragile on Windows in Tauri 2 (community reports that native edge resize broke or was uneven with `decorations: false`; maintainers pointed at `startResizeDragging` / native resize fixes — e.g. [tauri#8519](https://github.com/tauri-apps/tauri/issues/8519), [tauri#9053](https://github.com/tauri-apps/tauri/issues/9053), fix discussion in [PR #9862](https://github.com/tauri-apps/tauri/pull/9862)). Achievable, but more chrome plumbing than Flutter’s dedicated resize widgets.

Dense playlists and high-frequency seek updates are doable in a WebView (virtualized lists, `requestAnimationFrame`), but the UI stack is still DOM/CSS compositing inside the system webview, not a game-style retained scene graph designed for fully custom chrome.

**Verdict (UX):** Flutter is the better default for Winamp-like custom chrome and dense, update-heavy UI. Tauri can match window behaviors with more manual work and WebView constraints.

---

## 2. Performance

### Flutter

- Release builds compile Dart to native machine code for desktop targets; the engine rasterizes composited scenes ([architectural overview](https://docs.flutter.dev/resources/architectural-overview)).
- Official desktop support for Windows, macOS, and Linux with `flutter run -d windows|macos|linux` and `flutter build windows|macos|linux` ([Desktop support](https://docs.flutter.dev/platform-integration/desktop)).
- Impeller targets predictable GPU performance (offline shader compilation, upfront PSOs) ([Impeller](https://docs.flutter.dev/perf/impeller)). On desktop, Impeller is opt-in/maturing for **macOS**; Impeller docs currently emphasize iOS/Android defaults and macOS flag — Windows/Linux desktop still rely on the existing Skia path unless/until Impeller ships there. Either way, the shell is **not** a browser document tree.
- Dense lists: only visible rows are built ([ListView.builder](https://api.flutter.dev/flutter/widgets/ListView/ListView.builder.html)).

This clears the map’s bar that a hybrid web/native shell must prove “extremely performant” for a dense player UI: Flutter is not that hybrid.

### Tauri 2 + Rust

- Architecture is explicitly WebView + Rust message passing ([Tauri Architecture](https://v2.tauri.app/concept/architecture/)). Binaries stay small because they use the OS webview and do not ship Chromium.
- That is excellent for app size and for compute-heavy work in Rust, but the **player shell** (playlist scrolling, custom chrome, frequent seek/progress paints) still runs in WebView2 / WKWebView / WebKitGTK. Meeting an “extremely performant dense UI” bar is possible with disciplined virtualization, but it is not the same class of guarantee as Flutter’s AOT + own compositor — and the map’s charting note already flags hybrid stacks as needing that bar cleared.

**Verdict (performance):** Flutter better matches a UX-first, dense shell. Tauri wins on binary size / “thin native host,” not on shell rendering model.

---

## 3. Local audio (v1 formats)

Required kinds (from `CONTEXT.md`): **MP3, AAC/M4A, FLAC, WAV, Ogg Vorbis, Opus** on Win/Linux/macOS.

### Flutter path

| Approach | Platforms | Format coverage vs v1 |
| --- | --- | --- |
| **media_kit** (+ `media_kit_libs_audio`) | macOS, Windows, GNU/Linux (and more) ([media-kit README](https://github.com/media-kit/media-kit)) | Uses **libmpv**; claims strong/wide codec support. Demuxer list includes `aac`, `flac`, `mp3`, `ogg`, and `mov,mp4,m4a,...` ([README format list](https://raw.githubusercontent.com/media-kit/media-kit/main/README.md)). Suitable as the **v1 playback engine**. |
| **flutter_soloud** (Flutter cookbook) | Linux, Windows, macOS, … ([pub.dev](https://pub.dev/packages/flutter_soloud); [cookbook](https://docs.flutter.dev/cookbook/audio/soloud)) | Documents **MP3, WAV, OGG, FLAC**; streaming docs add **Ogg Opus / Vorbis / FLAC**. **AAC / M4A are not supported** (confirmed in package docs and maintainer discussion, e.g. [issue #102](https://github.com/alnitak/flutter_soloud/issues/102)). Insufficient alone for Tramp v1. |

**Spec assumption for Flutter:** use **media_kit** (libmpv) for playback; treat flutter_soloud as optional only if format requirements shrink.

### Tauri / Rust path

| Component | Coverage |
| --- | --- |
| **Symphonia** | Documents AAC, FLAC, MP3, MP4/ISO, OGG, Vorbis, WAV (among others). **Opus decoder status is “-” (in work / not started)** ([Symphonia README](https://raw.githubusercontent.com/pdeljanov/Symphonia/master/README.md)). |
| **rodio** | Default decoder is Symphonia; default features enable flac, mp3, mp4, vorbis, wav ([rodio docs](https://docs.rs/rodio/latest/rodio/)). Inherits Symphonia’s Opus gap. |
| **Opus separately** | Needs another crate (e.g. libopus bindings or a pure-Rust Opus decoder) wired into the demux/playback pipeline — doable in Rust, but not a single turnkey “all v1 formats” crate today. |

Alternatively, a Tauri app could shell out to / embed **mpv/FFmpeg** similarly to media_kit — that equalizes formats but drops the “pure Rust audio stack” advantage and adds native binary weight.

**Verdict (audio):** Both stacks can ship v1 formats. Flutter has a packaged, documented desktop path (media_kit/libmpv) covering AAC/M4A and Opus together. Rust/Symphonia covers AAC well but **not Opus yet**, so Tauri needs an extra Opus strategy.

---

## 4. Packaging

### Flutter

One codebase; per-OS release builds:

```text
flutter build windows
flutter build macos
flutter build linux
```

([Desktop support — Build a release app](https://docs.flutter.dev/platform-integration/desktop))

Distribution docs:

- **Windows:** MSIX / Microsoft Store packaging ([Windows deployment](https://docs.flutter.dev/deployment/windows)).
- **macOS:** App Store / notarized distribution via Xcode archive after `flutter build macos` ([macOS deployment](https://docs.flutter.dev/deployment/macos)); open-source packaging guide linked from the same page for non–App Store.
- **Linux:** Snap Store guide ([Linux / Snap](https://docs.flutter.dev/deployment/linux)); official page also points at **fastforge** (AppImage, deb, pacman, rpm, …) and **flatpak-flutter**.

Realistic for “ship on three desktops from one repo,” with platform-specific signing/store steps as usual.

### Tauri 2

`tauri build` bundles for the host platform; bundle targets include `deb`, `rpm`, `appimage`, `nsis`, `msi`, `app`, `dmg` (or `"all"`) ([BundleConfig / BundleTarget](https://v2.tauri.app/reference/config/#bundleconfig); [Distribute](https://v2.tauri.app/distribute/)). First-party guides cover Linux (AppImage, deb, RPM, Snap, Flatpak, AUR), macOS (app bundle, DMG, App Store), and Windows (NSIS/MSI, Store).

Tauri’s bundler story is mature and often *lighter* for installers; Flutter’s is equally realistic for a product of this scope.

**Verdict (packaging):** Tie / both viable. Slight edge to Tauri for small installers and built-in multi-format bundling; Flutter is fully workable with official + ecosystem packaging tools.

---

## Comparison matrix

| Criterion | Flutter | Tauri 2 + Rust |
| --- | --- | --- |
| Custom app chrome | Strong (`window_manager` + full custom paint) | Strong APIs; more manual resize/drag regions; WebView chrome |
| Dense playlist + high-freq UI | Strong (own compositor + `ListView.builder`) | Viable with virtualization; WebView-bound |
| “Extremely performant” shell | Clears bar as native-rendered UI | Hybrid; must be proven / tuned |
| v1 audio formats | media_kit/libmpv covers all | Symphonia/rodio miss Opus; need add-on or mpv |
| Win/Linux/macOS packaging | Official build + deploy docs | First-party bundler + distribute docs |
| Team fit | Dart/Flutter | Web frontend + Rust |

---

## When the runner-up still wins

Choose **Tauri 2 + Rust** if most of these are true:

1. The team already ships complex web UIs and Rust backends, and does **not** want a Dart/Flutter skill investment for v1.  
2. Small download size / OS WebView reuse is a hard product constraint.  
3. Audio work is expected to live in Rust anyway (with an explicit Opus plan: dedicated decoder crate or embedded mpv/FFmpeg).  
4. The UI is willing to stay within Web performance practices (virtualized playlist, throttled seek paints) and accept frameless-window edge-case work on Windows.

Otherwise, for a **UX-first** spiritual successor with custom chrome and a dense playlist shell, **Flutter** is the stack the v1 product spec should assume.

---

## Spec assumptions to record

If the map locks Flutter:

- **UI:** Flutter desktop (Windows, macOS, Linux).  
- **App chrome:** frameless/custom via `window_manager` (`TitleBarStyle.hidden`, `DragToMoveArea`, `DragToResizeArea`).  
- **Playback:** media_kit + audio native libs (libmpv), not flutter_soloud alone.  
- **Packaging:** `flutter build` per OS; store/direct channels TBD in later tickets.

---

## Sources (primary)

- Flutter desktop: https://docs.flutter.dev/platform-integration/desktop  
- Flutter architecture: https://docs.flutter.dev/resources/architectural-overview  
- Flutter Impeller: https://docs.flutter.dev/perf/impeller  
- Flutter ListView.builder: https://api.flutter.dev/flutter/widgets/ListView/ListView.builder.html  
- Flutter audio cookbook (soloud): https://docs.flutter.dev/cookbook/audio/soloud  
- flutter_soloud: https://pub.dev/packages/flutter_soloud  
- window_manager: https://pub.dev/documentation/window_manager/latest/  
- Flutter deploy Windows / macOS / Linux: https://docs.flutter.dev/deployment/windows · https://docs.flutter.dev/deployment/macos · https://docs.flutter.dev/deployment/linux  
- media-kit: https://github.com/media-kit/media-kit  
- Tauri architecture: https://v2.tauri.app/concept/architecture/  
- Tauri window customization: https://v2.tauri.app/learn/window-customization/  
- Tauri Window API: https://v2.tauri.app/reference/javascript/api/namespacewindow/  
- Tauri distribute / bundle config: https://v2.tauri.app/distribute/ · https://v2.tauri.app/reference/config/#bundleconfig  
- Symphonia: https://github.com/pdeljanov/Symphonia (README)  
- rodio: https://docs.rs/rodio/latest/rodio/  
