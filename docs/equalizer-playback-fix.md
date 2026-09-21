# Equalizer playback failure

Enabling EQ always prepended `volume=<preamp>dB` to the lavfi graph. The bundled
FFmpeg has no `volume` filter. The ten `equalizer` stages were already present;
neither a missing multi-band filter nor an unsupported equalizer option caused
the original failure.

The original flat graph was:

```text
lavfi=[volume=0dB,equalizer=f=60:t=o:w=1:g=0,equalizer=f=170:t=o:w=1:g=0,equalizer=f=310:t=o:w=1:g=0,equalizer=f=600:t=o:w=1:g=0,equalizer=f=1000:t=o:w=1:g=0,equalizer=f=3000:t=o:w=1:g=0,equalizer=f=6000:t=o:w=1:g=0,equalizer=f=12000:t=o:w=1:g=0,equalizer=f=14000:t=o:w=1:g=0,equalizer=f=16000:t=o:w=1:g=0]
```

`build/mpv_engine_test flatEnabledProducesAudio` reproduced the failure against
the repository's macOS arm64 mpv 0.36.0 framework. `mpv_set_property_string(af)`
returned `0 (success)` while idle. At playback startup, FFmpeg logged
`No option name near '0dB'`; `MPV_EVENT_END_FILE` reported
`-16 (no audio or video data played)`. Previously, setter results were ignored
and mpv logs were never requested, with its terminal output disabled.

Removing only `volume` exposed a second failure in this checkout:
`'aresample' filter not present, cannot convert formats.` Runtime calls to
`avfilter_get_by_name` found `equalizer`, `abuffer` and `abuffersink`, but neither
`volume` nor `aresample`. This differs from the supplied inventory of the
installed app; binary string checks can match error text for absent filters.

## Fix

Use option (a): keep ten one-octave peaking filters at the existing frequencies,
with mpv's native `format=format=floatp` before `lavfi`. Native format conversion
uses libswresample without requiring FFmpeg's `aresample` filter. This needs no
new libraries or build-toolchain changes and works when `aresample` is present too.

Preamp is passed separately to the engine. mpv's software volume cubes its
percentage to obtain amplitude, so the engine uses
`100 * slider * 10^(preampDb / 60)` and permits a maximum volume of 160
(+12 dB at full slider needs about 158.49). This preserves the existing slider
response. Disabling or bypassing EQ restores the slider's ordinary volume.
The cubic mapping is in [mpv 0.36.0's audio_update_volume](https://github.com/mpv-player/mpv/blob/v0.36.0/player/audio.c#L156-L170)
and is verified by measuring captured PCM, including +12 dB and -12 dB.

All string option/property setters in `MpvEngine` now check errors and decode
them with `mpv_error_string`; each EQ application logs the exact graph, status
and decoded result. mpv warning/error events reach Qt's warning output, even
with `terminal=no`.

Recovery covers synchronous rejection, errors at track startup, and delayed
filter failures during playback. mpv 0.36 can report a failed live rebuild only
through its `cplayer` error log, and can disable a graph at format negotiation
through its `af` warning log without an end-file error. Both signals trigger
bypass. They are checked only while Aoide has an EQ graph installed. A failed
track load gets one retry with EQ and preamp cleared; a second failure reaches
the ordinary playback error handler. Recovery preserves a requested seek and
pause state. New tracks inherit runtime bypass until an explicit EQ edit.

Settings are never rewritten by recovery: the schema, enabled flag, gains,
preamp and preset remain intact.

## Verification

Build and run with the same linked libraries as the app:

```sh
cmake --build build --target mpv_engine_test domain_test
ctest --test-dir build -R '^(mpv_engine|domain)$' --output-on-failure
```

The builder tests cover disabled/enabled, all band frequencies, flat and mixed
curves, extreme/non-finite gains and preamp independence. Integration tests
capture real PCM for flat EQ and the supplied curve, measure preamp at full and
partial volume, reload saved settings into fresh engines, inject invalid mpv
and lavfi filters, exercise live toggles, verify paused resume after recovery,
and ensure unrelated missing-file errors remain bounded and visible.

The supplied curve produces approximately +12.83 dB at 60 Hz in the captured
output, proving that it is applied. These are automated audio-output and clock
checks, not a human listening test or a packaged-app GUI relaunch test.
