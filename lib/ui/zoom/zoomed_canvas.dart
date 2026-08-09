import 'package:flutter/widgets.dart';

import 'zoom_scope.dart';

/// Applies ADR 0002 zoom: one root scale over a logical canvas.
///
/// The OS window is sized to `logical × factor`. This widget fills that window
/// by scaling [child] via a uniform [Transform.scale] (never anisotropic
/// stretch), and exposes the factor through [ZoomScope] for hairline snapping.
///
/// - **Fixed panels** (main / EQ): pass [logicalSize] so the child is always
///   the authored canvas. Sub-pixel OS window rounding is clipped — it must
///   not shrink the logical layout (that causes RenderFlex overflow).
/// - **Free-resize** (playlist): omit [logicalSize]; the logical canvas is
///   `constraints / factor` so only spacing grows with the window.
///
/// Hit-testing: [OverflowBox] must wrap [Transform.scale], not the reverse.
/// With `factor < 1`, inverse-scaled pointer coords exceed the window-sized
/// box; if that box is the hit-test boundary, bottom/right controls go dead.
class ZoomedCanvas extends StatelessWidget {
  const ZoomedCanvas({
    super.key,
    required this.factor,
    required this.child,
    this.logicalSize,
  });

  final double factor;
  final Widget child;

  /// Authored canvas size for zoom-only windows. Null = free-resize mode.
  final Size? logicalSize;

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
          final logical = logicalSize ?? Size(maxW / factor, maxH / factor);
          return SizedBox(
            width: maxW,
            height: maxH,
            child: ClipRect(
              child: OverflowBox(
                alignment: Alignment.topLeft,
                minWidth: logical.width,
                maxWidth: logical.width,
                minHeight: logical.height,
                maxHeight: logical.height,
                child: Transform.scale(
                  scale: factor,
                  alignment: Alignment.topLeft,
                  child: SizedBox(
                    width: logical.width,
                    height: logical.height,
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
