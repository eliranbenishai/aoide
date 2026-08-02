# Spike: FFmpeg audio filters and PCM decode via the shipped libmpv

Runtime experiments against the actual `libmpv-2.dll` that
`media_kit_libs_windows_audio` ships, driven over `dart:ffi` with no Flutter
involved. Harness: `.scratch/spike/eq_spike.dart`, tone generator
`.scratch/spike/make_tone.py`, analyser `.scratch/spike/compare.py`.

Binary under test:
`.worktrees/tramp-v1/build/windows/x64/libmpv/libmpv-2.dll` (14.8 MB).

Method: synthesise a stereo 44.1 kHz WAV containing 60 Hz, 1 kHz and 8 kHz tones
at equal amplitude, run it through mpv with `ao=pcm` writing a WAV, then measure
per-tone amplitude with a Goertzel DFT and compare against the input.

## Result 1 — FFmpeg audio filters are unusable. Verified.

**The equalizer cannot be made audible with the shipped binary.**

`--enable-filter=equalizer` really is in the build, and mpv accepts the filter
chain without complaint: `mpv_set_option_string` returns success, and reading the
`af` property back returns the full ten-filter string verbatim. The audio is
nonetheless untouched — measured delta at every tone was `0.00 dB`.

Verbose logs (`msg-level=all=v`) give the reason:

```
[af] User filter list:
[af]   equalizer (equalizer.00)
[af]   equalizer (equalizer.01)
[af]   equalizer (equalizer.02)
[ffmpeg] 'aresample' filter not present, cannot convert formats.
[lavfi] failed to configure the filter graph
Disabling filter equalizer.00 because it has failed.
```

mpv's libavfilter bridge needs libavfilter's `aresample` filter to negotiate
sample formats into the graph. `--disable-filters` removed it. A byte scan finds
`aresample` in the binary only as the literal inside mpv's own error message;
`swresample` (17 hits) is the resampling *library*, which mpv uses directly for
its own conversion path — that is why playback and `--audio-format` work while
the filter graph cannot configure.

This is not format-specific. Tested `audio-format` values `s16`, `s16p`,
`float`, `floatp`, `double`, `doublep`, `s32`, `s32p` — all eight fail
identically. There is no combination of formats that avoids needing `aresample`.

**Consequence beyond the equalizer:** this also independently kills the
`astats`-metadata approach to audio levels. Even a custom build that added
`--enable-filter=astats` would still need `--enable-filter=aresample` to make the
graph configurable at all.

**The dangerous part:** mpv reports success at every API surface a caller would
check. `setProperty` succeeds, `getProperty('af')` echoes the chain, no error
event is emitted. Only the verbose log reveals that the filters were silently
disabled. An implementation that trusted the return codes would ship an
equalizer that appears wired and does nothing.

## Result 2 — `ao=pcm` works, and it is fast. Verified.

**The precomputed-spectrogram route is viable.**

`ao=pcm` with `ao-pcm-file` and `ao-pcm-waveheader=yes` produces a WAV, and the
decode is faithful: the output PCM payload is **byte-identical** to the input
payload on a passthrough run.

Throughput, decoding a 240-second track with `untimed=yes`:

| Measure | Value |
|---|---|
| Wall clock, including Dart VM startup | 1.13 s |
| Effective rate | ~212× realtime |

That figure is for WAV, which is trivial to decode; MP3 and FLAC will be slower,
but the margin is large enough that a full-track analysis pass is comfortably
sub-second to a few seconds for typical material.

Options that worked, exactly as set:

```
config=no  terminal=yes  msg-level=all=v  vid=no
ao=pcm  ao-pcm-file=<path>  ao-pcm-waveheader=yes
untimed=yes  audio-samplerate=44100  audio-format=s16
audio-channels=stereo  keep-open=no
```

### Implementation gotcha

mpv's `ao_pcm` writes **`WAVE_FORMAT_EXTENSIBLE`** (format tag `0xFFFE`), not
plain PCM (`0x0001`). Python's stdlib `wave` module rejects it outright
(`wave.Error: unknown format: 65534`), and a naive Dart reader will too. The real
format lives in the `SubFormat` GUID at byte offset 24 of the `fmt ` chunk. Any
WAV reader on this path must walk RIFF chunks and handle the extensible header.

## Not verified

- macOS and Linux were not tested — no machines available. Linux links the system
  libmpv, so filters are expected to be present there; the macOS media_kit build
  recipe is reported to be equally minimal.
- Whether mpv's downmix and resample to mono / lower sample rate engages
  correctly on the `ao=pcm` path was not confirmed. The test source already
  matched the requested format, so mpv's `convert` node stayed disabled. This
  needs a check with a mismatched source before relying on `audio-channels=mono`
  and a reduced `audio-samplerate` to shrink the analysis buffer.
- Concurrent operation of a second mpv instance alongside the playback instance
  was not tested.

## Bottom line

| Question | Answer |
|---|---|
| Audible 10-band EQ with shipped libmpv | **No.** Needs a custom build adding `aresample` |
| `astats`-based real level metering | **No.** Needs `aresample` *and* `astats` |
| True per-band spectrum via mpv filters | **No**, at any build — architectural, not a build flag |
| PCM decode for offline analysis | **Yes**, verified bit-exact at ~212× realtime |
| Precomputed spectrogram for a real spectrum | **Yes** — the only verified path |
