import 'package:flutter/painting.dart';

abstract final class GraphiteSkin {
  static const mainFace = 'assets/skin/graphite/main_face.png';
  static const equalizerFace = 'assets/skin/graphite/equalizer_face.png';

  /// Title-bar strip shown when the equalizer is collapsed to a windowshade.
  static const equalizerShadeFace =
      'assets/skin/graphite/equalizer_shade_face.png';

  /// Logical rect cleared in the PNG for spectrum + LCD overlays.
  ///
  /// Matches Task 1 alpha punch in `.scratch/graphite-skin/slice_mockup.py`.
  static const mainDisplayWell = Rect.fromLTRB(35.5, 37.0, 559.0, 167.0);

  // ---------------------------------------------------------------------------
  // Control art, cropped from the mockup by
  // `.scratch/graphite-skin/crop_controls.py`. Authored at 2x like the faces
  // (logical = png / 2). Missing states are tonal transforms of real pixels:
  // transport `pressed` is the idle crop recessed; toggle `active`/`idle` are
  // the glyph recoloured to phosphor / label grey. See the script header.
  // ---------------------------------------------------------------------------

  static const _controls = 'assets/skin/graphite/controls';

  /// Transport idle sprites, keyed by control name (`prev`..`next`). 138x80.
  static const transportIdle = <String, String>{
    'prev': '$_controls/transport_prev_idle.png',
    'play': '$_controls/transport_play_idle.png',
    'pause': '$_controls/transport_pause_idle.png',
    'stop': '$_controls/transport_stop_idle.png',
    'next': '$_controls/transport_next_idle.png',
  };

  /// Transport pressed sprites, keyed the same way. 138x80.
  static const transportPressed = <String, String>{
    'prev': '$_controls/transport_prev_pressed.png',
    'play': '$_controls/transport_play_pressed.png',
    'pause': '$_controls/transport_pause_pressed.png',
    'stop': '$_controls/transport_stop_pressed.png',
    'next': '$_controls/transport_next_pressed.png',
  };

  // Kept for source compatibility with Task 3 call sites/tests.
  static const transportPlayIdle = '$_controls/transport_play_idle.png';
  static const transportPlayPressed = '$_controls/transport_play_pressed.png';

  /// Lit (green) play sprite, shown while playing. The idle/pressed sprites are
  /// neutral grey (the mockup's baked green triangle is this active state).
  static const transportPlayActive = '$_controls/transport_play_active.png';

  // Title-bar window buttons, cropped from the pristine mockup so the metal
  // bezel is real PNG art (slice_mockup.py blanks these on the face). 90x50.
  // minimize / close keep their mockup glyphs; the two zoom buttons use the
  // blank bezel (minimize with its bar cloned out) and the widget stamps a
  // code +/- on top, since the mockup has no zoom art.
  static const winMinimizeIdle = '$_controls/win_minimize_idle.png';
  static const winMinimizePressed = '$_controls/win_minimize_pressed.png';
  static const winCloseIdle = '$_controls/win_close_idle.png';
  static const winClosePressed = '$_controls/win_close_pressed.png';
  static const winBlankIdle = '$_controls/win_blank_idle.png';
  static const winBlankPressed = '$_controls/win_blank_pressed.png';

  /// Mute sits in the OPEN-adjacent bezel. Placeholder sprites from
  /// `build_polish_chrome.py` — replace per `docs/design/ASSETS_NEEDED.md`.
  /// `active` is the muted (slashed) state.
  static const muteIdle = '$_controls/mute_idle.png';
  static const muteMuted = '$_controls/mute_muted.png';
  static const mutePressed = '$_controls/mute_pressed.png';

  /// Playlist toolbar label buttons (LOAD / SAVE / ADD). Placeholders —
  /// see `docs/design/ASSETS_NEEDED.md`.
  static const plLoadIdle = '$_controls/pl_load_idle.png';
  static const plLoadPressed = '$_controls/pl_load_pressed.png';
  static const plSaveIdle = '$_controls/pl_save_idle.png';
  static const plSavePressed = '$_controls/pl_save_pressed.png';
  static const plAddIdle = '$_controls/pl_add_idle.png';
  static const plAddPressed = '$_controls/pl_add_pressed.png';

  static const shuffleIdle = '$_controls/shuffle_idle.png';
  static const shuffleActive = '$_controls/shuffle_active.png';
  static const repeatIdle = '$_controls/repeat_idle.png';
  static const repeatActive = '$_controls/repeat_active.png';
  static const eqIdle = '$_controls/eq_idle.png';
  static const eqActive = '$_controls/eq_active.png';
  static const plIdle = '$_controls/pl_idle.png';
  static const plActive = '$_controls/pl_active.png';

  /// Metal fader grip shared by the sliders (equalizer bands, volume).
  static const sliderThumb = '$_controls/slider_thumb.png';

  /// Horizontal grips authored for the transport sliders (fidelity pass): the
  /// volume grip rides the L/R meter gap; the seek grip rides the seek bar.
  /// Both are fresh brushed-metal crops, not the squashed vertical EQ grip.
  static const volumeThumb = '$_controls/volume_thumb.png';
  static const seekThumb = '$_controls/seek_thumb.png';

  // ---------------------------------------------------------------------------
  // Equalizer controls (Task 7), cropped by `crop_controls.py`. ON/AUTO are
  // toggles (idle grey / active phosphor, like the main-player toggles);
  // PRESETS, collapse and close carry a `pressed` recess. `eqThumb` is the full
  // vertical fader grip (the shared `sliderThumb` is only a partial crop).
  // ---------------------------------------------------------------------------
  static const eqOnIdle = '$_controls/eq_on_idle.png';
  static const eqOnActive = '$_controls/eq_on_active.png';
  static const eqAutoIdle = '$_controls/eq_auto_idle.png';
  static const eqAutoActive = '$_controls/eq_auto_active.png';
  static const eqPresetsIdle = '$_controls/eq_presets_idle.png';
  static const eqPresetsPressed = '$_controls/eq_presets_pressed.png';
  static const eqCollapseIdle = '$_controls/eq_collapse_idle.png';
  static const eqCollapsePressed = '$_controls/eq_collapse_pressed.png';
  static const eqCloseIdle = '$_controls/eq_close_idle.png';
  static const eqClosePressed = '$_controls/eq_close_pressed.png';

  /// Full vertical fader grip for the equalizer bands + preamp. 68x46.
  static const eqThumb = '$_controls/eq_thumb.png';
}
