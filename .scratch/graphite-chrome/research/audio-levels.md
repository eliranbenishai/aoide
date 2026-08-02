# Real-time audio levels / spectrum for Tramp (media_kit + libmpv)

**Date:** 2026-08-02
**Scope:** Can the 20-bar spectrum analyzer in `lib/ui/chrome/spectrum_visualizer.dart` be driven by real audio instead of the current synthetic sine pulse?
**Verdict up front:** **No — not with the dependencies currently pinned.** The libmpv builds shipped by `media_kit_libs_*_audio` are compiled with `--disable-filters` and only two libavfilter filters enabled (`overlay`, `equalizer`). `astats` is **not present**, so the `af-metadata` route cannot work on Windows or macOS as-is. And even with `astats`, a true per-band spectrum is **not obtainable** from mpv's audio filter chain. See §7 for options.

---

## 0. Local project facts (verified)

| Fact | Value | Source |
|---|---|---|
| Declared media_kit | `^1.1.11` | `pubspec.yaml:13` |
| Resolved media_kit | **1.2.6** | `pubspec.lock:307-314` |
| Resolved `media_kit_libs_audio` | 1.0.7 | `pubspec.lock:323-330` |
| Resolved `media_kit_libs_windows_audio` | 1.0.9 | `pubspec.lock:355-362` |
| Resolved `media_kit_libs_macos_audio` | 1.1.4 | `pubspec.lock:347-354` |
| Resolved `media_kit_libs_linux` | 1.2.1 | `pubspec.lock:339-346` |
| Engine seam | `PlayerEngine` (abstract) | `lib/playback/player_engine.dart` |
| Existing `NativePlayer` usage | `platform is! NativePlayer` + `getProperty('metadata/title')` | `lib/playback/media_kit_player_engine.dart:64-69` |
| Current visualizer | 20 bars, `sin(phase + i*0.55)`, scaled by *volume setting* (not audio) | `lib/ui/chrome/spectrum_visualizer.dart:24,101-107` |

---

## 1. mpv `af-metadata` + FFmpeg `astats` — mechanism confirmed, but the filter is missing from our build

### 1a. The mechanism is real and documented

`af-metadata/<filter-label>` is a documented mpv property:

> ``vf-metadata/<filter-label>`` — Metadata added by video filters. Accessed by the filter label, which, if not explicitly specified using the ``@filter-label:`` syntax, will be ``<filter-name>.NN``. Works similar to ``metadata`` property. It allows the same access methods (using sub-properties).
> ``af-metadata/<filter-label>`` — Equivalent to ``vf-metadata/<filter-label>``, but for audio filters.
> — mpv `DOCS/man/input.rst` lines 2522-2534 ([property list](https://mpv.io/manual/master/#property-list))

Filter label syntax (documented under `--vf`, explicitly stated to apply to `--af` too):

> The general filter entry syntax is: ``["@"<label-name>":"] ["!"] <filter-name> [ "=" <filter-parameter-list> ]``
> ``param-value`` can further be quoted in ``[`` / ``]`` in case the value contains characters like ``,`` or ``=``. This is used in particular with the ``lavfi`` filter…
> — mpv `DOCS/man/vf.rst` lines 5-6, 30, 44-47

Source-level confirmation of the plumbing:

- `player/command.c:4652` registers `{"af-metadata", mp_property_filter_metadata, .priv = "af"}`.
- `mp_property_filter_metadata` (`command.c:1386-1432`) only implements `M_PROPERTY_KEY_ACTION`. It resolves `mpctx->ao_chain->filter`, issues `MP_FILTER_COMMAND_GET_META` targeted at the **label**, and hands the resulting `mp_tags` to `tag_property`. If there is no `ao_chain` it returns `M_PROPERTY_UNAVAILABLE` (i.e. the property errors when nothing is playing).
- `filters/f_lavfi.c:823-829` implements `GET_META` as `*ptags = mp_tags_dup(NULL, c->out_pads[0]->metadata)`, and `f_lavfi.c:718` fills that from `mp_tags_copy_from_av_dictionary(pad->metadata, c->tmp_frame->metadata)` — i.e. **the metadata of the most recently produced output AVFrame** of that lavfi graph. It is a single-slot snapshot, not a queue.

The exact property naming and a working real-world invocation come from the mpv issue where this feature was designed ([mpv#2645](https://github.com/mpv-player/mpv/issues/2645)); `richardpl` (the astats author) suggested `astats`, and a user later posted working code:

```lua
function send_state()
    local astats = mp.get_property_native("af-metadata/level")
    if astats ~= nil then
        print(astats["lavfi.astats.Overall.Max_level"])
    end
end
mp.add_periodic_timer(0.1, send_state)
```
> "Also, I had to add `--af=@level:lavfi="astats=metadata=1:reset=4"` to the command line options."
> — lukaszmatczak, [mpv#2645](https://github.com/mpv-player/mpv/issues/2645)

Note that in the same thread, **dweymouth (author of the Supersonic libmpv music player) asked for exactly this** — a peak meter — which is a good signal that this is the canonical approach for libmpv-based players.

`af-metadata` was added by [`4e0e24c3c2`](https://github.com/mpv-player/mpv/commit/4e0e24c3c2) (2015-09-11, "af_lavfi: implement af-metadata property", fixes [mpv#2311](https://github.com/mpv-player/mpv/issues/2311)) — so any modern libmpv has the property. That is not the problem.

### 1b. Exact key names (verified against FFmpeg source, docs are incomplete)

`libavfilter/af_astats.c:463-474`:

```c
static void set_meta(AVDictionary **metadata, int chan, const char *key, const char *fmt, double val)
{
    ...
    if (chan)
        snprintf(key2, sizeof(key2), "lavfi.astats.%d.%s", chan, key);
    else
        snprintf(key2, sizeof(key2), "lavfi.astats.%s", key);
```

So the keys are `lavfi.astats.1.RMS_level`, `lavfi.astats.2.Peak_level`, `lavfi.astats.Overall.RMS_level`, etc. (channels are 1-based; "Overall" keys are emitted with `chan=0` and the literal prefix `Overall.` baked into the key name — `af_astats.c:598-639`).

**Documentation discrepancy worth knowing:** `doc/filters.texi:3432-3456` lists the per-channel keys and **omits `RMS_level`**, but `af_astats.c:556-557` clearly emits `set_meta(metadata, c + 1, "RMS_level", ...)`. Per-channel `RMS_level` and `Peak_level` **do** exist. Trust the source, not the manual, here.

Values are dB (`LINEAR_TO_DB(x) = log10(x)*20`, `af_astats.c:477`), formatted `%f`, so a silent frame yields `-inf`. Your parser must handle `-inf`/`nan`.

`reset` semantics (`af_astats.c:732-738`): with `reset=N` the accumulator is cleared every `N` input frames. `reset=0` (default) means stats accumulate over the whole file — **useless for a live meter**. You want `reset=1` (per-frame) or a small N. `metadata=1` is required for any metadata to be emitted at all (`af_astats.c:745-746`).

CPU can be reduced a lot with `measure_perchannel` / `measure_overall` flags (`filters.texi:3491-3499`) — e.g. `measure_perchannel=Peak_level+RMS_level:measure_overall=none` skips entropy, bit depth, zero crossings, noise floor, etc.

### 1c. How the value comes back through `mpv_get_property_string`

media_kit's `getProperty` uses `mpv_get_property_string`. Traced:

- `M_PROPERTY_GET_STRING` → `m_property_do` (`options/m_property.c:157-164`) fetches `GET_TYPE` then `GET`, then `m_option_print`.
- For `af-metadata/<label>` with no sub-key, `tag_property` returns `CONF_TYPE_NODE` / `MPV_FORMAT_NODE_MAP` (`command.c:1283-1305`).
- `m_option_type_node.print = print_node` → `json_write` (`options/m_option.c:3931-3939, 4054-4058`).

**So `getProperty('af-metadata/lvl')` returns a JSON object string containing every astats key at once.** That is the efficient call: one FFI round-trip + one `jsonDecode` per sample, rather than N property reads. Sub-key access (`af-metadata/lvl/lavfi.astats.Overall.RMS_level`) also works and returns a bare string (`command.c:1330-1339`).

### 1d. **BLOCKER: `astats` is not compiled into the libmpv media_kit ships**

I read the FFmpeg configuration string embedded in the actual DLL in this repo (`D:\code\tramp\build\windows\x64\libmpv\libmpv-2.dll`, 15,525,902 bytes). It contains:

```
--disable-filters ... --enable-filter=overlay --enable-filter=equalizer
```

A byte scan of the same DLL for filter-name strings:

| string | occurrences |
|---|---|
| `astats` | **0** |
| `aspectralstats` | **0** |
| `ebur128`, `volumedetect` | 0 |
| `bandpass`, `firequalizer`, `superequalizer` | 0 |
| `asplit`, `amix`, `amerge`, `ametadata`, `atempo` | 0 |
| `showspectrum`, `showfreqs`, `showcqt`, `abitscope` | 0 |
| `avfilter`, `abuffersink`, `af-metadata` | present (37 / 3 / 2) |

libavfilter *is* linked and `af-metadata` *is* in the binary; the filters simply do not exist. `--af=lavfi=[astats=metadata=1:reset=1]` will fail at filter-graph creation.

This is by design, not an accident:

- Windows: `media_kit_libs_windows_audio-1.0.9/windows/CMakeLists.txt:63,69` pulls `https://github.com/media-kit/libmpv-win32-audio-build/releases/download/2023-09-24/...`, described in-file as "minimal libmpv & FFmpeg audio specific builds".
- macOS: `media_kit_libs_macos_audio-1.1.4/macos/Makefile:10` pulls `libmpv-darwin-build` **v0.6.0**, variant `macos-universal-audio-default`. That repo's `scripts/ffmpeg/meson.build` lines 166-169 (`audio_default_options`) enables exactly `--enable-filter=overlay` and `--enable-filter=equalizer`; the base `disable_all_options` disables the rest.
- **The video variant does not help either**: `media-kit/libmpv-win32-video-build/packages/ffmpeg.cmake` lines 55, 261-262 also have `--disable-filters` + only `overlay` and `equalizer`. Same for `libmpv-darwin-build`'s `audio_full_options` (lines 263-273) — even the "full" flavor keeps only those two filters. Only the `encodersgpl` flavor sets `--enable-filters` (line 289), and media_kit does not ship that for plain audio apps.
- **Linux is the exception:** `media_kit_libs_linux-1.2.1/linux/CMakeLists.txt:141-145` sets `media_kit_libs_linux_bundled_libraries` to `""` — it bundles no libmpv at all and links the **system** libmpv. Distro libmpv is normally built against full FFmpeg, so `astats` is very likely available there.

> **Unverified:** I could not execute anything on macOS or Linux from this machine. The macOS claim is from the build recipe, not from a shipped binary. The Linux claim ("system libmpv has astats") is inference from typical distro packaging — verify with `mpv --af=help | grep astats` on the target distro.

---

## 2. Per-band spectrum via lavfi — **not achievable**, independent of §1d

Even assuming a libmpv with all filters, ~20 real frequency bands cannot be read out of mpv's `af` chain.

**(a) No FFmpeg audio filter emits per-frequency-bin metadata.** The closest is `aspectralstats` (`filters.texi:3310-3376`), which does compute an FFT but only exports *aggregate descriptors*: `mean, variance, centroid, spread, skewness, kurtosis, entropy, flatness, crest, flux, slope, decrease, rolloff`. No bins.

**(b) `showspectrum` / `showfreqs` / `showcqt` / `abitscope` are audio→video filters.** In an mpv `af` chain the graph must terminate in an audio pad; mpv's docs also note `--af` "can only take a single track as input" and multi-input filters can't be used (`vf.rst:49-53`). These are unusable here.

**(c) The `asplit` → N×`bandpass` → N×`astats` → `amix` fan-out does not work.** Two independent reasons:

1. **mpv reads metadata from exactly one pad.** `f_lavfi.c:823-829` returns `out_pads[0]->metadata` — the metadata attached to the single output frame. Anything measured on a side branch has to ride back on that frame.
2. **The mixers drop metadata.** I grepped `libavfilter/af_amix.c`, `af_amerge.c` and `af_join.c` for `av_frame_copy_props` and `->metadata`: **zero matches in all three**. Frames produced by `amix`/`amerge`/`join` carry no input metadata at all. Even if they did, all branches would write colliding `lavfi.astats.*` keys, and FFmpeg has no filter that renames a metadata key to a different key with a dynamic value (`ametadata` only add/modify/delete/print/select with literal values).

**(d) A serial cascade of `@label:lavfi=[bandpass,astats]` entries would work metadata-wise** (each mpv `af` entry is a separate lavfi graph with its own label and its own `out_pads[0]->metadata`, so `af-metadata/b1`…`af-metadata/b20` are each individually readable) — **but mpv's `af` chain is serial and its output is what reaches the AO.** Cascading 20 bandpasses would destroy the audio you hear. There is no way to tap without mutating.

**(e) `--lavfi-complex` doesn't rescue it** either: labels must be `aidN`/`vidN`/`ao`/`vo`, "Multiple video or audio outputs are not possible" (`options.rst:8461-8473`), and metadata still only comes off the one output pad.

**CPU cost is therefore moot**, but for the record: 20× `bandpass` + 20× `astats` on 44.1 kHz stereo is roughly 40 biquad passes plus 20 full statistical passes per sample block. That is a meaningful single-core load (order of several % of a modern core) but not the reason to reject it — it simply cannot be read out.

**Conclusion for §2: mpv can give you level(s). It cannot give you a spectrum.**

---

## 3. media_kit reachability from Dart — fully confirmed (real signatures)

`Player.platform` is a plain public field, not a getter:

```dart
// media_kit-1.2.6/lib/src/player/player.dart:125
PlatformPlayer? platform;
```

`NativePlayer` (in `lib/src/player/native/player/real.dart`) exposes exactly these, verbatim:

```dart
// real.dart:1223-1227
Future<void> setProperty(
  String property,
  String value, {
  bool waitForInitialization = true,
}) async

// real.dart:1255-1258
Future<String> getProperty(
  String property, {
  bool waitForInitialization = true,
}) async

// real.dart:1288-1292
Future<void> observeProperty(
  String property,
  Future<void> Function(String) listener, {
  bool waitForInitialization = true,
}) async

// real.dart:1328-1331
Future<void> unobserveProperty(
  String property, {
  bool waitForInitialization = true,
}) async

// real.dart:1359-1362
Future<void> command(
  List<String> command, {
  bool waitForInitialization = true,
}) async

// real.dart:1211
Future<int> get handle async   // returns ctx.address (raw mpv_handle)
```

Implementation notes that matter:

- `setProperty` → `mpv_set_property_string`; `getProperty` → `mpv_get_property_string`, returning `""` on failure (`real.dart:1269-1278`) — **it does not throw on an unknown/unavailable property, it returns an empty string.** Good for graceful degradation.
- `observeProperty` registers with **`MPV_FORMAT_NONE`** (`real.dart:1312-1317`) and throws `ArgumentError` if the same property is observed twice (`real.dart:1302-1308`). When the change event arrives, media_kit does a *synchronous* `mpv_get_property_string` and passes the string to your listener (`real.dart:2051-2067`).
- **Threading:** on desktop, media_kit installs `mpv_set_wakeup_callback` with a `NativeCallable.listener` (`core/initializer_native_callable.dart:50-55`, selected by `core/initializer.dart:41-45` when not execmem-restricted). `NativeCallable.listener` delivers on the **owning Dart isolate — the Flutter UI isolate**. Every mpv event, and every property-change `mpv_get_property_string`, runs on the UI thread. This is fine at 20 Hz for one small JSON blob; it is *not* a place to shovel PCM.
- **Conflict risk on `af`:** media_kit itself writes the `af` property when `PlayerConfiguration.pitch == true` (`real.dart:829-830, 880-881`, `'af' → 'scaletempo:scale=…'`). Default is `pitch: false` (`platform_player.dart:521`), and Tramp constructs `Player()` with defaults (`lib/playback/media_kit_player_engine.dart:15`), so today there is no conflict — but if pitch is ever enabled, whoever writes `af` last wins and clobbers the other. Any level implementation must own the whole `af` string.
- Custom libmpv is supported: `MediaKit.ensureInitialized({String? libmpv})` (`lib/src/media_kit.dart:23`) → `NativeLibrary.ensureInitialized` which also honours the `LIBMPV_LIBRARY_PATH` environment variable (`core/native_library.dart:32-42`). This is the hook for §7 Option C.

---

## 4. Cadence — **~20 Hz ceiling for audio-only playback**, and it is a documented hack

This is worse than one might hope, and I have three independent confirmations.

**(a) The mpv commit that made this observable says so.** [`da612acacd`](https://github.com/mpv-player/mpv/commit/da612acacd) (2019-09-19), "command: make vf-metadata/af-metadata somewhat observable":

> Until now they weren't observable and never reported any updates. Apply a shitty hack to make them mostly-observable. It relies on the "idle" event, which is basically triggered on every frame displayed, or similar. This can lead to property change notifications not being sent quickly enough.
>
> The cleaner solution would be adding a notification mechanisms from filters, but I'm too lazy for that.

**(b) The wiring.** `af-metadata` is listed under `MPV_EVENT_TICK` in `mp_event_property_change[]` (`command.c:4852-4861`), and `match_property`/`prefix_len` (`command.c:4896-4915`) split on `/`, so observing `af-metadata/lvl` or `af-metadata/lvl/<key>` **does** match and does fire. Good.

**(c) The tick rate for audio-only is capped at 50 ms.** `player/playloop.c:1113-1125`:

```c
// Potentially needed by some Lua scripts, which assume TICK always comes.
static void handle_dummy_ticks(struct MPContext *mpctx)
{
    if ((mpctx->video_status != STATUS_PLAYING &&
         mpctx->video_status != STATUS_DRAINING) || mpctx->paused)
    {
        if (mp_time_sec() - mpctx->last_idle_tick > 0.050) {
            mpctx->last_idle_tick = mp_time_sec();
            mp_notify(mpctx, MPV_EVENT_TICK, NULL);
        }
    }
}
```

With no video track (our case — media_kit also sets `audio-display=no`, `real.dart:2393`), the TICK path is the dummy-tick path: **at most one every 50 ms → 20 Hz.** For video playback it would be per displayed frame; that does not apply to a music player.

**(d) Polling faster does not help.** You may call `getProperty` from Dart at 60 Hz, but the *underlying* value only changes when the lavfi graph emits a new output frame, and `f_lavfi.c` keeps only the **last** frame's metadata. mpv's playloop refills the AO in bursts, so a burst of decoded frames passes through astats and you observe only the final one. You therefore get an *instantaneous window* (one decoder frame: ~26 ms for MP3's 1152 samples, ~23 ms for AAC's 1024, ~93 ms for a 4096-sample FLAC frame) sampled every ~50 ms — **not** a contiguous envelope. Transients between samples are simply invisible.

**(e) There is a latency offset.** The af chain runs ahead of what you hear by the AO buffer: `--audio-buffer` default is **0.2 s** (`options.rst:2444-2457`), plus the device buffer. So astats readings **lead the audible audio by ≥200 ms**. For a bar meter this is perceptible if you are looking for beat-sync. You would need to delay the level stream by roughly the buffer depth to make it feel locked to the music, and mpv does not hand you an exact number for this (`audio-delay`/`avsync` do not describe it for audio-only).

**Bottom line on cadence: ~10-20 Hz of usable, jittery, ~200 ms-early, non-contiguous samples.** A 60 Hz analyzer would be roughly 3 real samples per 10 frames — mandatory interpolation and decay smoothing in the UI, and honestly closer to "VU meter with inertia" than "spectrum analyzer".

---

## 5. Alternatives

### (a) Parallel PCM tap via dart:ffi (miniaudio / SoLoud) purely for analysis
**Replaces media_kit? No.** You keep libmpv for playback and decode the same file a second time just to analyze it.
- Real, but it is a second audio pipeline: you must follow seeks, pause, track changes, and keep a decode cursor aligned to `Player.state.position` (which itself only updates at the same ~20 Hz tick).
- It analyzes the *file*, not the *output* — volume, EQ, and ReplayGain are not reflected. For a Winamp-style analyzer that is arguably fine.
- `flutter_soloud`'s [`readSamplesFromFile`](https://pub.dev/documentation/flutter_soloud/latest/flutter_soloud/SoLoud/readSamplesFromFile.html) is **not** suitable for this: it takes `startTime`/`endTime`, returns samples "equally spaced in time" across the range, and runs through `compute()` (a fresh isolate hop per call). It is built for static waveform overviews, not 60 Hz windows.
- Doing it properly means FFI bindings to miniaudio (or similar) for streaming decode + a Dart FFT (e.g. `fftea`). That is a genuine sub-project, and it silently loses formats libmpv supports but miniaudio/dr_libs do not (AAC/M4A, WMA, APE, WavPack, DSD, TAK…). Graceful degradation is at least possible: no analysis decoder → fall back to the current animation.

### (b) mpv `--ao=` capture options
**Not viable.** mpv's `pcm` AO writes to a file (`--ao-pcm-file`, `ao.rst:284-299`) and mpv uses **one** AO at a time — selecting it means no sound. There is no tee/monitor AO, and libmpv exposes no audio render callback (the video-side `mpv_render_context` has no audio counterpart). Confirmed by mpv maintainers in [mpv#9464](https://github.com/mpv-player/mpv/issues/9464) ("I think it would be not so trivial to support. Patches welcome…").
OS-level loopback is the theoretical fallback and it is genuinely hostile cross-platform: Windows has `IAudioMeterInformation::GetPeakValue()` for a per-session *level* (this is exactly what the [mpv#2645](https://github.com/mpv-player/mpv/issues/2645) reporter said external apps have to do) and process-loopback capture for full PCM on Win10 20H1+; macOS needs the CoreAudio process-tap API (macOS 14.2+) or a virtual device, both permission-gated; Linux needs a PipeWire/Pulse monitor source. Three separate native implementations for one decoration. Not recommended.

### (c) media_kit-adjacent package exposing waveform/FFT
**None exists.** [media-kit/media-kit#489 "[Enhancement] Expose audio-samples/PCM as event in `Player`"](https://github.com/media-kit/media-kit/issues/489) is open since 2023-09-24, and the maintainer's reply is unambiguous:
> "This isn't possible with current libmpv API, it'll require additional implementation. Interest for me is virtually zero as I'm occupied with other important issues." — alexmercerind
> "NOTE: Edited the title since offering 'audio visualization' is a bigger non-goal."

`audio_waveform_kit` is microphone-recording oriented; `audio_flux` is a renderer that requires `flutter_soloud`/`flutter_recorder` as its data source. Neither bridges media_kit.

### (d) Other Flutter desktop engines that DO expose PCM/FFT
**`flutter_soloud`** is the only credible one. **Replaces media_kit? Yes, entirely.**
- Platform support: Linux / Windows / macOS all "Any / Any / 10.15+" ([pub.dev](https://pub.dev/packages/flutter_soloud), [repo](https://github.com/alnitak/flutter_soloud)).
- API is exactly what a Winamp analyzer wants: `SoLoud.instance.setVisualizationEnabled(true)`, then `AudioData(GetSamplesKind.linear)` → `updateSamples()` → `getAudioData()` returning a `Float32List` of 512 (first 256 = FFT bins, last 256 = wave), polled from a `Ticker` at frame rate, with `SoLoud.instance.setFftSmoothing(0.7)` built in ([official docs](https://docs.page/alnitak/flutter_soloud_docs/visualization/audio_data), [AudioData API](https://pub.dev/documentation/flutter_soloud/latest/flutter_soloud/AudioData-class.html)). 256 bins → 20 log-spaced bars is trivial.
- **The cost is format support.** SoLoud decodes MP3, WAV, OGG (Vorbis/Opus) and FLAC. libmpv/FFmpeg in our current build decodes `aac*, ac3, alac, als, ape, atrac*, eac3, flac, gsm*, mp1/2/3*, mpc*, opus, ra*, ralf, shorten, tak, tta, vorbis, wavpack, wma*, pcm*, dsd*, dca` (from the DLL's embedded configure string). Dropping AAC/M4A/ALAC/WMA/APE/WavPack/DSD from a desktop music player is a serious regression, plus you lose libmpv's tag reading, ReplayGain, network protocols and battle-tested seeking.
- `minisound` (miniaudio) and `just_audio`/`audioplayers` do not expose FFT; not options.

---

## 6. What I could NOT verify

1. **No end-to-end runtime test.** I did not run Tramp or a Dart harness against libmpv to observe an actual `af-metadata` value. Given §1d (no `astats` in the shipped DLL) such a test would fail on this machine anyway, but the JSON-string shape of `getProperty('af-metadata/<label>')` is derived from source reading, not from an observed value.
2. **macOS binary not inspected** — the `--enable-filter=overlay/equalizer`-only claim there comes from `libmpv-darwin-build` v0.6.0's `meson.build`, not from the shipped `.xcframework`.
3. **Linux system libmpv** — not verified that a given distro's libmpv includes `astats`. Highly likely; check with `mpv --af=help`.
4. **Exact af-chain frame size / observed jitter** for our decoders. The ~23-93 ms figures are derived from codec frame sizes, not measured.
5. **The real audible latency offset.** `--audio-buffer=0.2` is the documented minimum; actual device buffers vary (WASAPI shared mode typically adds ~10-30 ms, but I did not measure).
6. **Whether observing `af-metadata/...` in media_kit is stable across track changes.** `mp_property_filter_metadata` returns `M_PROPERTY_UNAVAILABLE` with no `ao_chain`; media_kit's observer would then get an empty string. Not exercised.

---

## 7. Bottom line — recommendation

### The honest summary

- The `af-metadata` + `astats` technique is **real, documented, and used in production by other libmpv apps** — but it gives you **levels only** (overall + per-channel peak/RMS in dB), at **~20 Hz**, **~200 ms ahead** of what you hear, sampled non-contiguously.
- **A real 20-band spectrum is not obtainable from mpv at all** (§2), at any effort level, short of replacing the audio path.
- **And on Windows and macOS today, even the level route is dead**, because media_kit ships a libmpv whose FFmpeg has `--disable-filters` with only `overlay` and `equalizer` (§1d).

So there is no "just wire it up" answer. Pick a lane:

### Recommended: stage it

**Stage 1 (do now) — add the seam, keep it honest.** Extend the engine interface but do not pretend to have data you don't:

```dart
// lib/playback/player_engine.dart
class AudioLevels {
  const AudioLevels({
    required this.bands,      // normalized 0..1, length == UI bar count
    required this.rmsLeft,    // 0..1 linear, from dB
    required this.rmsRight,
    required this.peak,
    required this.isSynthetic, // true when no real audio data is available
  });
  final List<double> bands;
  final double rmsLeft, rmsRight, peak;
  final bool isSynthetic;
}

abstract class PlayerEngine {
  // ...existing members...
  Stream<AudioLevels> get levelsStream;
}
```

`MediaKitPlayerEngine` emits `isSynthetic: true` levels driven by the current animation, and `SpectrumVisualizer` becomes a dumb consumer of `AudioLevels` instead of owning an `AnimationController`. This is small, testable (`FakePlayerEngine` can emit scripted levels), and it means Stage 2/3 is a swap of one producer rather than a UI rewrite. Also update `docs/architecture.md` for the new stream on the playback boundary.

**Stage 2 (if you want real levels) — ship your own libmpv.** Fork `media-kit/libmpv-win32-audio-build` and `media-kit/libmpv-darwin-build`, add `--enable-filter=astats` (one line each; the darwin one is `audio_default_options` in `scripts/ffmpeg/meson.build`), host the artifacts, and point media_kit at them via `MediaKit.ensureInitialized(libmpv: path)` or `LIBMPV_LIBRARY_PATH`. Linux needs nothing. Then the implementation is genuinely small:

```dart
// once, after the player exists:
final mpv = _player.platform as NativePlayer;
await mpv.setProperty(
  'af',
  '@lvl:lavfi=[astats=metadata=1:reset=1:measure_perchannel=Peak_level+RMS_level:measure_overall=RMS_level+Peak_level]',
);
await mpv.observeProperty('af-metadata/lvl', (json) async {
  // json is the JSON object string produced by mpv's print_node
  final m = jsonDecode(json) as Map<String, dynamic>;
  final l = _db(m['lavfi.astats.1.RMS_level']);   // dB, may be "-inf"
  final r = _db(m['lavfi.astats.2.RMS_level']);
  // ...
});
```

with `_db` mapping `-inf`/`nan` → 0 and otherwise `pow(10, db/20)`. Bands would have to be **derived** from level (e.g. a fixed spectral shape modulated by amplitude with per-bar randomized inertia) — visually much better than today's fixed sine because it actually tracks the music's dynamics, but it is not a spectrum and should not be described as one.

UI smoothing is mandatory: attack/release envelope per bar (fast attack ~ 1 frame, release ~ 300 ms), linear interpolation between the ~20 Hz samples, plus the classic falling peak-hold caps. Optionally delay the stream ~200 ms to compensate the AO buffer.

**Stage 3 (only if a true spectrum is a must-have) — change the audio engine.** `flutter_soloud` is the only path to a genuine 256-bin FFT at frame rate on all three desktops. Because Stage 1 put everything behind `PlayerEngine`, this is "write `SoLoudPlayerEngine`" rather than a rewrite — but you must accept losing AAC/M4A/ALAC/WMA/APE/WavPack/DSD playback. **My recommendation is not to do this for a general-purpose music player**, unless you first confirm the target library is MP3/FLAC/OGG/WAV only.

### Confidence

| Claim | Confidence |
|---|---|
| `astats` absent from shipped Windows libmpv | **Very high** — read from the shipped binary's own configure string |
| `astats` absent from shipped macOS libmpv | High — build recipe, binary not inspected |
| Per-band spectrum impossible via mpv af chain | **High** — three independent source-level reasons |
| `af-metadata`/`astats` mechanism + key names correct | **High** — mpv + FFmpeg source, plus a working third-party example |
| ~20 Hz cadence ceiling for audio-only | **High** — `handle_dummy_ticks` 50 ms gate + maintainer's own commit message |
| ~200 ms lead over audible output | Medium-high — `--audio-buffer` default is documented; real device buffers unmeasured |
| media_kit API signatures | **Very high** — quoted from the resolved 1.2.6 source in the pub cache |
| flutter_soloud viability & limits | Medium-high — official docs; not built or run here |
| Linux system libmpv has `astats` | Medium — inference, unverified |

---

## Sources

**mpv (primary, `master` @ 2026-08-02):** [`DOCS/man/input.rst`](https://raw.githubusercontent.com/mpv-player/mpv/master/DOCS/man/input.rst) · [`DOCS/man/vf.rst`](https://raw.githubusercontent.com/mpv-player/mpv/master/DOCS/man/vf.rst) · [`DOCS/man/af.rst`](https://raw.githubusercontent.com/mpv-player/mpv/master/DOCS/man/af.rst) · [`DOCS/man/options.rst`](https://raw.githubusercontent.com/mpv-player/mpv/master/DOCS/man/options.rst) · [`DOCS/man/ao.rst`](https://raw.githubusercontent.com/mpv-player/mpv/master/DOCS/man/ao.rst) · [`player/command.c`](https://raw.githubusercontent.com/mpv-player/mpv/master/player/command.c) · [`player/playloop.c`](https://raw.githubusercontent.com/mpv-player/mpv/master/player/playloop.c) · [`filters/f_lavfi.c`](https://raw.githubusercontent.com/mpv-player/mpv/master/filters/f_lavfi.c) · [`options/m_property.c`](https://raw.githubusercontent.com/mpv-player/mpv/master/options/m_property.c) · [`options/m_option.c`](https://raw.githubusercontent.com/mpv-player/mpv/master/options/m_option.c) · commits [`4e0e24c3c2`](https://github.com/mpv-player/mpv/commit/4e0e24c3c2), [`da612acacd`](https://github.com/mpv-player/mpv/commit/da612acacd) · issues [#2311](https://github.com/mpv-player/mpv/issues/2311), [#2645](https://github.com/mpv-player/mpv/issues/2645), [#9464](https://github.com/mpv-player/mpv/issues/9464)

**FFmpeg (primary):** [`doc/filters.texi`](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/doc/filters.texi) (astats §3413-3501, aspectralstats §3310-3376) · [`libavfilter/af_astats.c`](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavfilter/af_astats.c) · [`af_amix.c`](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavfilter/af_amix.c) · [`af_amerge.c`](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavfilter/af_amerge.c) · [`af_join.c`](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavfilter/af_join.c)

**media_kit (primary, installed 1.2.6 in pub cache):** `lib/src/player/player.dart` · `lib/src/player/platform_player.dart` · `lib/src/player/native/player/real.dart` · `lib/src/player/native/core/initializer{,_native_callable}.dart` · `lib/src/player/native/core/native_library.dart` · `lib/src/media_kit.dart` · [issue #489](https://github.com/media-kit/media-kit/issues/489)

**Build recipes:** `media_kit_libs_windows_audio-1.0.9/windows/CMakeLists.txt` · `media_kit_libs_macos_audio-1.1.4/macos/Makefile` · `media_kit_libs_linux-1.2.1/linux/CMakeLists.txt` · [libmpv-darwin-build v0.6.0 `scripts/ffmpeg/meson.build`](https://raw.githubusercontent.com/media-kit/libmpv-darwin-build/v0.6.0/scripts/ffmpeg/meson.build) · [libmpv-win32-video-build `packages/ffmpeg.cmake`](https://raw.githubusercontent.com/media-kit/libmpv-win32-video-build/master/packages/ffmpeg.cmake) · embedded configure string of `D:\code\tramp\build\windows\x64\libmpv\libmpv-2.dll`

**Alternatives:** [flutter_soloud pub.dev](https://pub.dev/packages/flutter_soloud) · [visualization docs](https://docs.page/alnitak/flutter_soloud_docs/visualization/audio_data) · [`AudioData` API](https://pub.dev/documentation/flutter_soloud/latest/flutter_soloud/AudioData-class.html) · [`readSamplesFromFile` API](https://pub.dev/documentation/flutter_soloud/latest/flutter_soloud/SoLoud/readSamplesFromFile.html) · [audio_flux](https://pub.dev/packages/audio_flux)
