import 'package:flutter/widgets.dart';

/// CRT-style well matching mockup `.screen` (radial wash + scanlines).
class MockupScreen extends StatelessWidget {
  const MockupScreen({
    super.key,
    this.child,
    this.borderRadius = 3,
  });

  final Widget? child;
  final double borderRadius;

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: BorderRadius.circular(borderRadius),
      child: Stack(
        fit: StackFit.passthrough,
        children: [
          const Positioned.fill(child: CustomPaint(painter: _ScreenWellPainter())),
          if (child != null) child!,
          const Positioned.fill(
            child: IgnorePointer(
              child: CustomPaint(painter: _ScanlinePainter()),
            ),
          ),
        ],
      ),
    );
  }
}

class _ScreenWellPainter extends CustomPainter {
  const _ScreenWellPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    canvas.drawRect(
      rect,
      Paint()
        ..shader = const RadialGradient(
          center: Alignment(-0.64, -1.4),
          radius: 1.35,
          colors: [
            Color(0xFF0F1C2A),
            Color(0xFF071018),
            Color(0xFF04070C),
          ],
          stops: [0, 0.48, 1],
        ).createShader(rect),
    );

    // Outer top highlight (0 1px 0 rgba(226,236,255,0.12)).
    canvas.drawLine(
      const Offset(0, 0.5),
      Offset(size.width, 0.5),
      Paint()..color = const Color(0x1FE2ECFF),
    );

    // Inset cyan rim + deep well.
    final rrect = RRect.fromRectAndRadius(
      rect.deflate(0.5),
      const Radius.circular(2.5),
    );
    canvas.drawRRect(
      rrect,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = const Color(0x1A3DE7FF),
    );
    canvas.drawRRect(
      rrect,
      Paint()
        ..color = const Color(0x0D3DE7FF)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 12),
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(rect.deflate(1), const Radius.circular(2)),
      Paint()
        ..color = const Color(0xE6000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.inner, 3),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

class _ScanlinePainter extends CustomPainter {
  const _ScanlinePainter();

  @override
  void paint(Canvas canvas, Size size) {
    final line = Paint()..color = const Color(0x52000000);
    for (var y = 0.0; y < size.height; y += 3) {
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, 1), line);
    }
    canvas.drawRect(
      Offset.zero & size,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            const Color(0x0DFFFFFF),
            const Color(0x00FFFFFF),
          ],
          stops: const [0, 0.38],
        ).createShader(Offset.zero & size),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
