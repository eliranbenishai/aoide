import 'package:flutter/widgets.dart';

import 'zoom_scope.dart';

/// Applies ADR 0002 zoom: one root scale over a fixed logical canvas.
///
/// The OS window is sized to `logical × factor`. This widget fills that window
/// by scaling [child] (authored at logical size) via [FittedBox], and exposes
/// the factor through [ZoomScope] for hairline snapping.
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
      child: FittedBox(
        fit: BoxFit.fill,
        alignment: Alignment.topLeft,
        child: child,
      ),
    );
  }
}
