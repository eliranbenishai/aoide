import 'package:flutter/painting.dart';

abstract final class GraphiteSkin {
  static const mainFace = 'assets/skin/graphite/main_face.png';
  static const equalizerFace = 'assets/skin/graphite/equalizer_face.png';

  /// Logical rect cleared in the PNG for spectrum + LCD overlays.
  ///
  /// Matches Task 1 alpha punch in `.scratch/graphite-skin/slice_mockup.py`.
  static const mainDisplayWell = Rect.fromLTRB(35.5, 37.0, 559.0, 167.0);

  // Control art cropped from the mockup by
  // `.scratch/graphite-skin/crop_controls.py`. Authored at 2x like the faces:
  // the transport buttons are logical 69x40 (PNG 138x80). Task 3 ships the play
  // sprites and one slider thumb; the rest of the transport set lands in
  // Task 6-7.
  static const transportPlayIdle =
      'assets/skin/graphite/controls/transport_play_idle.png';
  static const transportPlayPressed =
      'assets/skin/graphite/controls/transport_play_pressed.png';

  /// Metal fader grip shared by the sliders (equalizer bands, volume).
  static const sliderThumb = 'assets/skin/graphite/controls/slider_thumb.png';
}
