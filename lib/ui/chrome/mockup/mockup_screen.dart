import 'package:flutter/widgets.dart';

import '../../../look/look_palette.dart';
import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';

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
          Positioned.fill(
            child: CustomPaint(
              painter: _ScreenWellPainter(
                palette: LookScope.of(context).palette,
              ),
            ),
          ),
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
  const _ScreenWellPainter({required this.palette});

  final LookPalette palette;

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final wash = LookPaint.screenWash(palette);
    canvas.drawRect(
      rect,
      Paint()
        ..shader = RadialGradient(
          center: const Alignment(-0.64, -1.4),
          radius: 1.35,
          colors: wash,
          stops: const [0, 0.48, 1],
        ).createShader(rect),
    );

    // Outer top highlight (0 1px 0 rgba(226,236,255,0.12)).
    canvas.drawLine(
      const Offset(0, 0.5),
      Offset(size.width, 0.5),
      Paint()
        ..color = LookPaint.coolSheen(palette).withValues(alpha: 0x1F / 255),
    );

    // Inset phosphor rim + deep well.
    final rrect = RRect.fromRectAndRadius(
      rect.deflate(0.5),
      const Radius.circular(2.5),
    );
    canvas.drawRRect(
      rrect,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = palette.phosphorDefault.withValues(alpha: 0x1A / 255),
    );
    canvas.drawRRect(
      rrect,
      Paint()
        ..color = palette.phosphorDefault.withValues(alpha: 0x0D / 255)
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
  bool shouldRepaint(covariant _ScreenWellPainter oldDelegate) =>
      palette != oldDelegate.palette;
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
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color(0x0DFFFFFF),
            Color(0x00FFFFFF),
          ],
          stops: [0, 0.38],
        ).createShader(Offset.zero & size),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
