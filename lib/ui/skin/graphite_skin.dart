import 'package:flutter/painting.dart';

abstract final class GraphiteSkin {
  static const mainFace = 'assets/skin/graphite/main_face.png';
  static const equalizerFace = 'assets/skin/graphite/equalizer_face.png';

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
}
