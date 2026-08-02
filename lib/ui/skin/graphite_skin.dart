import 'package:flutter/painting.dart';

abstract final class GraphiteSkin {
  static const mainFace = 'assets/skin/graphite/main_face.png';
  static const equalizerFace = 'assets/skin/graphite/equalizer_face.png';

  /// Logical rect cleared in the PNG for spectrum + LCD overlays.
  ///
  /// Matches Task 1 alpha punch in `.scratch/graphite-skin/slice_mockup.py`.
  static const mainDisplayWell = Rect.fromLTRB(35.5, 37.0, 559.0, 167.0);
}
