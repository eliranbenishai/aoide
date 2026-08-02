import 'dart:math' as math;

import 'package:flutter/widgets.dart';

/// Carries the active zoom factor down the tree.
///
/// Widgets read this only to snap hairlines. Nothing scales itself — the factor
/// is applied once, by a single transform at the root of the panel stack.
class ZoomScope extends InheritedWidget {
  const ZoomScope({
    super.key,
    required this.factor,
    required this.devicePixelRatio,
    required super.child,
  });

  /// Nominal width of a chrome bevel, before snapping.
  static const double hairline = 1.0;

  final double factor;
  final double devicePixelRatio;

  static ZoomScope of(BuildContext context) {
    final scope = maybeOf(context);
    assert(scope != null, 'No ZoomScope found in context');
    return scope!;
  }

  static ZoomScope? maybeOf(BuildContext context) =>
      context.dependOnInheritedWidgetOfExactType<ZoomScope>();

  /// Snapped bevel width for [context].
  ///
  /// Falls back to an unsnapped hairline when there is no scope, so a single
  /// chrome widget can be pumped in a test or golden without ceremony.
  static double hairlineFor(BuildContext context) =>
      maybeOf(context)?.snap(hairline) ?? hairline;

  /// Rounds [logicalWidth] so it lands on whole device pixels once scaled.
  ///
  /// Without this a 1px bevel becomes 1.5 device pixels at 150% and renders as
  /// a soft grey smear instead of a crisp edge.
  double snap(double logicalWidth) {
    final scale = factor * devicePixelRatio;
    if (scale <= 0) return logicalWidth;
    final device = math.max(1.0, (logicalWidth * scale).roundToDouble());
    return device / scale;
  }

  @override
  bool updateShouldNotify(ZoomScope oldWidget) =>
      oldWidget.factor != factor ||
      oldWidget.devicePixelRatio != devicePixelRatio;
}
