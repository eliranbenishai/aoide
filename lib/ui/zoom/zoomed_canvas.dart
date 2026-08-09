import 'package:flutter/widgets.dart';

import 'zoom_scope.dart';

/// Applies ADR 0002 zoom: one root scale over a logical canvas.
///
/// The OS window is sized to `logical × factor`. This widget fills that window
/// by scaling [child] via a uniform [Transform.scale] (never anisotropic
/// stretch), and exposes the factor through [ZoomScope] for hairline snapping.
///
/// Free-resize surfaces (playlist) pass a child sized to
/// `constraints / factor` so only spacing grows — proportions stay zoom-owned.
class ZoomedCanvas extends StatelessWidget {
  const ZoomedCanvas({
    super.key,
    required this.factor,
    required this.child,
  });

  final double factor;
  final Widget child;

  @override
  Widget build(BuildContext context) {
    final dpr = MediaQuery.devicePixelRatioOf(context);
    return ZoomScope(
      factor: factor,
      devicePixelRatio: dpr,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final maxW = constraints.maxWidth;
          final maxH = constraints.maxHeight;
          if (!maxW.isFinite || !maxH.isFinite || factor <= 0) {
            return child;
          }
          final logicalW = maxW / factor;
          final logicalH = maxH / factor;
          return SizedBox(
            width: maxW,
            height: maxH,
            child: ClipRect(
              child: Transform.scale(
                scale: factor,
                alignment: Alignment.topLeft,
                child: OverflowBox(
                  alignment: Alignment.topLeft,
                  minWidth: logicalW,
                  maxWidth: logicalW,
                  minHeight: logicalH,
                  maxHeight: logicalH,
                  child: SizedBox(
                    width: logicalW,
                    height: logicalH,
                    child: child,
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}
