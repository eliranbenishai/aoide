import 'package:flutter/widgets.dart';

import '../../theme/tramp_surfaces.dart';
import '../zoom/zoom_scope.dart';

/// Which material a panel wears.
enum TrampSurface { raisedPanel, raisedButton, pressedButton, insetWell, lcdGlass }

/// Applies one of the shared surface recipes.
///
/// This widget deliberately holds no visual decisions of its own — it exists so
/// callers name a material instead of hand-rolling one. It composes the fill and
/// the bevel, which are separate because Flutter will not paint a rounded
/// decoration that also carries a two-tone border.
class MetalPanel extends StatelessWidget {
  const MetalPanel({
    super.key,
    required this.surface,
    required this.child,
    this.padding,
  });

  final TrampSurface surface;
  final Widget child;
  final EdgeInsets? padding;

  @override
  Widget build(BuildContext context) {
    final bevel = ZoomScope.hairlineFor(context);
    final spec = switch (surface) {
      TrampSurface.raisedPanel => TrampSurfaces.raisedPanel(bevel: bevel),
      TrampSurface.raisedButton => TrampSurfaces.raisedButton(bevel: bevel),
      TrampSurface.pressedButton => TrampSurfaces.pressedButton(bevel: bevel),
      TrampSurface.insetWell => TrampSurfaces.insetWell(bevel: bevel),
      TrampSurface.lcdGlass => TrampSurfaces.lcdGlass(bevel: bevel),
    };

    final inner =
        padding == null ? child : Padding(padding: padding!, child: child);

    return CustomPaint(
      foregroundPainter: BevelPainter(spec: spec),
      child: DecoratedBox(decoration: spec.decoration, child: inner),
    );
  }
}
