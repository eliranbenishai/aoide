import 'package:flutter/widgets.dart';

import '../../../theme/mockup_tokens.dart';

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
      painter: _LedPainter(lit: lit),
    );
  }
}

class _LedPainter extends CustomPainter {
  const _LedPainter({required this.lit});

  final bool lit;

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.shortestSide / 2;
    final glow = Rect.fromCircle(center: center, radius: radius);

    if (lit) {
      canvas.drawCircle(
        center,
        radius + 6,
        Paint()
          ..color = const Color(0x66FF3D9A)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 8),
      );
      canvas.drawCircle(
        center,
        radius + 2,
        Paint()
          ..color = const Color(0xD9FF3D9A)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
      );
      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..shader = RadialGradient(
            center: const Alignment(-0.2, -0.3),
            colors: const [
              Color(0xFFFFD6EA),
              MockupTokens.accent,
              Color(0xFF8A2258),
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
          ..color = const Color(0x995A0F32),
      );
    } else {
      canvas.drawCircle(
        center,
        radius,
        Paint()
          ..shader = const RadialGradient(
            center: Alignment(-0.2, -0.3),
            colors: [
              Color(0xFF3D4350),
              Color(0xFF22262F),
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
          ..color = const Color(0x1AE2ECFF)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 0.5),
      );
    }
  }

  @override
  bool shouldRepaint(covariant _LedPainter oldDelegate) =>
      lit != oldDelegate.lit;
}
