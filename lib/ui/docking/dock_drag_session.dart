import 'package:flutter/painting.dart';

/// Converts a title-bar pointer drag into logical top-left for [DockingCoordinator.move].
///
/// Uses **global** pointer deltas so OS frame updates during the drag do not
/// corrupt the gesture (local deltas would fight the moving window).
class DockDragSession {
  DockDragSession({
    required this.originLogical,
    required this.originGlobal,
    required this.zoom,
  }) : assert(zoom > 0);

  /// Logical top-left of the dragged window at pan-start.
  final Offset originLogical;

  /// Global pointer position at pan-start.
  final Offset originGlobal;

  /// Current global zoom factor (e.g. `1.25` for 125%).
  final double zoom;

  /// Logical top-left for the current [globalPosition].
  Offset logicalTopLeftFor(Offset globalPosition) {
    final pixelDelta = globalPosition - originGlobal;
    return originLogical + pixelDelta / zoom;
  }
}
