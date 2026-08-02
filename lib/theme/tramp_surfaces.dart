import 'package:flutter/rendering.dart';

import 'tramp_colors.dart';

/// One chrome material: a fill, plus the bevel to paint on top of it.
///
/// Fill and bevel are separate because Flutter throws when a [BoxDecoration]
/// carries both a `borderRadius` and a non-uniform `Border`, and this design
/// needs rounded corners together with a light top edge against a dark bottom
/// edge. [BevelPainter] draws the edges, clipped to the same rounded rect.
class SurfaceSpec {
  const SurfaceSpec({
    required this.decoration,
    required this.highlight,
    required this.shadow,
    required this.radius,
    required this.bevel,
  });

  /// Fill only — gradient or colour and the corner radius. Never a border.
  final BoxDecoration decoration;

  /// Colour of the top and left edges.
  final Color highlight;

  /// Colour of the bottom and right edges.
  final Color shadow;

  final double radius;
  final double bevel;

  @override
  bool operator ==(Object other) =>
      other is SurfaceSpec &&
      other.decoration == decoration &&
      other.highlight == highlight &&
      other.shadow == shadow &&
      other.radius == radius &&
      other.bevel == bevel;

  @override
  int get hashCode =>
      Object.hash(decoration, highlight, shadow, radius, bevel);
}

/// The complete set of chrome materials.
///
/// Every panel, button, groove and display in the app draws from here. Widgets
/// must not compose their own gradients or bevels — drift between the
/// equalizer, the transport buttons and the playlist is exactly what this
/// single definition prevents.
abstract final class TrampSurfaces {
  static const double panelRadius = 3;
  static const double buttonRadius = 2;

  static SurfaceSpec raisedPanel({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(
          borderRadius: BorderRadius.all(Radius.circular(panelRadius)),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [TrampColors.panelTop, TrampColors.panelBottom],
          ),
        ),
        highlight: TrampColors.bevelHi,
        shadow: TrampColors.bevelLo,
        radius: panelRadius,
        bevel: bevel,
      );

  static SurfaceSpec raisedButton({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(
          borderRadius: BorderRadius.all(Radius.circular(buttonRadius)),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [TrampColors.buttonTop, TrampColors.buttonBottom],
          ),
        ),
        highlight: TrampColors.bevelHi,
        shadow: TrampColors.bevelLo,
        radius: buttonRadius,
        bevel: bevel,
      );

  static SurfaceSpec pressedButton({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(
          borderRadius: BorderRadius.all(Radius.circular(buttonRadius)),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [TrampColors.buttonBottom, TrampColors.buttonTop],
          ),
        ),
        highlight: TrampColors.bevelLo,
        shadow: TrampColors.bevelHi,
        radius: buttonRadius,
        bevel: bevel,
      );

  static SurfaceSpec insetWell({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(color: TrampColors.wellDeep),
        highlight: TrampColors.bevelLo,
        shadow: TrampColors.bevelHi,
        radius: 0,
        bevel: bevel,
      );

  static SurfaceSpec lcdGlass({double bevel = 1}) => SurfaceSpec(
        decoration: const BoxDecoration(color: TrampColors.lcdGlass),
        highlight: TrampColors.bevelLo,
        shadow: TrampColors.bevelHi,
        radius: 0,
        bevel: bevel,
      );
}

/// Paints a surface's two-tone bevel over its fill.
///
/// Use as a `foregroundPainter` so the edges sit above the fill.
class BevelPainter extends CustomPainter {
  const BevelPainter({required this.spec});

  final SurfaceSpec spec;

  @override
  void paint(Canvas canvas, Size size) {
    final inset = spec.bevel / 2;
    final rect = Rect.fromLTWH(
      inset,
      inset,
      (size.width - spec.bevel).clamp(0.0, double.infinity),
      (size.height - spec.bevel).clamp(0.0, double.infinity),
    );
    if (rect.isEmpty) return;

    final rrect = RRect.fromRectAndRadius(
      rect,
      Radius.circular(spec.radius),
    );

    final stroke = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = spec.bevel
      ..strokeCap = StrokeCap.square;

    // Top-left arc carries the highlight, bottom-right the shadow, so the
    // surface reads as lit from above.
    canvas.save();
    canvas.clipRRect(rrect);
    final path = Path()..addRRect(rrect);
    canvas.drawPath(path, stroke..color = spec.shadow);
    canvas.restore();

    // Redraw only the lit edges on top.
    final lit = Path()
      ..moveTo(rect.left, rect.bottom - spec.radius)
      ..lineTo(rect.left, rect.top + spec.radius)
      ..arcToPoint(
        Offset(rect.left + spec.radius, rect.top),
        radius: Radius.circular(spec.radius),
      )
      ..lineTo(rect.right - spec.radius, rect.top);

    canvas.save();
    canvas.clipRRect(rrect);
    canvas.drawPath(lit, stroke..color = spec.highlight);
    canvas.restore();
  }

  @override
  bool shouldRepaint(BevelPainter oldDelegate) => oldDelegate.spec != spec;
}
