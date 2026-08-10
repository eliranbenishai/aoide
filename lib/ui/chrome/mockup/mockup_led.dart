import 'package:flutter/widgets.dart';

import '../../../look/look_palette.dart';
import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';

/// Phosphor/magenta status LED matching mockup `.led` / `.led--lit`.
class MockupLed extends StatelessWidget {
  const MockupLed({
    super.key,
    this.lit = false,
    this.size = 8,
  });

  final bool lit;
  final double size;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size.square(size),
      painter: _LedPainter(
        lit: lit,
        palette: LookScope.of(context).palette,
      ),
    );
  }
}

class _LedPainter extends CustomPainter {
  const _LedPainter({
    required this.lit,
    required this.palette,
  });

  final bool lit;
  final LookPalette palette;

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.shortestSide / 2;
    final glow = Rect.fromCircle(center: center, radius: radius);

    if (lit) {
      final bloom = LookPaint.accentBloom(palette);
      final hot = LookPaint.accentHot(palette);
      canvas.drawCircle(
        center,
        radius + 5,
        Paint()
          ..color = bloom
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 6),
      );
      canvas.drawCircle(
        center,
        radius + 1.5,
        Paint()
          ..color = palette.accentDefault.withValues(alpha: 0xB3 / 255)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3),
      );
      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..shader = RadialGradient(
            center: const Alignment(-0.2, -0.3),
            colors: [
              hot,
              palette.accentDefault,
              palette.accentDim,
            ],
            stops: const [0, 0.45, 1],
          ).createShader(glow),
      );
      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1
          ..color = LookPaint.litLedRim(palette).withValues(alpha: 0x99 / 255),
      );
    } else {
      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..shader = RadialGradient(
            center: const Alignment(-0.2, -0.3),
            colors: [
              LookPaint.idleLedHi(palette),
              LookPaint.idleLedLo(palette),
            ],
          ).createShader(glow),
      );
      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1
          ..color = const Color(0xCC000000),
      );
      canvas.drawCircle(
        Offset(center.dx, center.dy + radius * 0.55),
        radius * 0.85,
        Paint()
          ..color = LookPaint.hoverLiftTarget(palette).withValues(alpha: 0x1A / 255)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 0.5),
      );
    }
  }

  @override
  bool shouldRepaint(covariant _LedPainter oldDelegate) =>
      lit != oldDelegate.lit || palette != oldDelegate.palette;
}
