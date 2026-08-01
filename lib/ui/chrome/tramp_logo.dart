import 'dart:ui' as ui;

import 'package:flutter/material.dart';

/// Soft illustrative pin-up mark: closed eyes, over-ear headphones, warm skin.
/// Vector gradients only — no raster assets.
class TrampLogo extends StatelessWidget {
  const TrampLogo({super.key, this.size = 28});

  final double size;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: size,
      height: size,
      child: CustomPaint(
        painter: TrampLogoPainter(),
      ),
    );
  }
}

/// Paints the Tramp headphones pin-up logo in unit space [0,1]².
class TrampLogoPainter extends CustomPainter {
  static const _skinHi = Color(0xFFFFE0C8);
  static const _skinMid = Color(0xFFF0B090);
  static const _skinDeep = Color(0xFFD08868);
  static const _hairHi = Color(0xFF4A3028);
  static const _hairMid = Color(0xFF2A1814);
  static const _hairDeep = Color(0xFF140C0A);
  static const _lip = Color(0xFFC86858);
  static const _lash = Color(0xFF1A1010);
  static const _metalHi = Color(0xFFE8E8E8);
  static const _metalMid = Color(0xFFB0B0B0);
  static const _metalDeep = Color(0xFF5A5A5A);

  @override
  void paint(Canvas canvas, Size size) {
    final s = size.shortestSide;
    canvas.save();
    canvas.scale(s, s);

    _paintHairBack(canvas);
    _paintNeck(canvas);
    _paintFace(canvas);
    _paintHairFront(canvas);
    _paintFeatures(canvas);
    _paintHeadphones(canvas);

    canvas.restore();
  }

  void _paintHairBack(Canvas canvas) {
    final path = Path()
      ..moveTo(0.18, 0.42)
      ..cubicTo(0.08, 0.55, 0.05, 0.78, 0.14, 0.98)
      ..cubicTo(0.28, 1.02, 0.42, 0.95, 0.50, 0.88)
      ..cubicTo(0.58, 0.95, 0.72, 1.02, 0.86, 0.98)
      ..cubicTo(0.95, 0.78, 0.92, 0.55, 0.82, 0.42)
      ..cubicTo(0.88, 0.28, 0.82, 0.12, 0.62, 0.06)
      ..cubicTo(0.50, 0.02, 0.38, 0.02, 0.28, 0.08)
      ..cubicTo(0.14, 0.16, 0.12, 0.30, 0.18, 0.42)
      ..close();

    final paint = Paint()
      ..shader = ui.Gradient.radial(
        const Offset(0.42, 0.28),
        0.72,
        const [_hairHi, _hairMid, _hairDeep],
        const [0.0, 0.45, 1.0],
      );
    canvas.drawPath(path, paint);
  }

  void _paintNeck(Canvas canvas) {
    final path = Path()
      ..moveTo(0.38, 0.72)
      ..cubicTo(0.40, 0.88, 0.42, 0.96, 0.50, 0.98)
      ..cubicTo(0.58, 0.96, 0.60, 0.88, 0.62, 0.72)
      ..close();

    final paint = Paint()
      ..shader = ui.Gradient.linear(
        const Offset(0.50, 0.70),
        const Offset(0.50, 1.0),
        const [_skinMid, _skinDeep],
      );
    canvas.drawPath(path, paint);
  }

  void _paintFace(Canvas canvas) {
    final face = Path()
      ..moveTo(0.50, 0.16)
      ..cubicTo(0.68, 0.16, 0.80, 0.30, 0.80, 0.48)
      ..cubicTo(0.80, 0.66, 0.68, 0.80, 0.50, 0.82)
      ..cubicTo(0.32, 0.80, 0.20, 0.66, 0.20, 0.48)
      ..cubicTo(0.20, 0.30, 0.32, 0.16, 0.50, 0.16)
      ..close();

    final paint = Paint()
      ..shader = ui.Gradient.radial(
        const Offset(0.42, 0.38),
        0.48,
        const [_skinHi, _skinMid, _skinDeep],
        const [0.0, 0.55, 1.0],
      );
    canvas.drawPath(face, paint);

    // Soft cheek blush.
    canvas.drawOval(
      Rect.fromCenter(center: const Offset(0.30, 0.55), width: 0.14, height: 0.08),
      Paint()
        ..color = const Color(0x55E88878)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 0.03),
    );
    canvas.drawOval(
      Rect.fromCenter(center: const Offset(0.70, 0.55), width: 0.14, height: 0.08),
      Paint()
        ..color = const Color(0x55E88878)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 0.03),
    );
  }

  void _paintHairFront(Canvas canvas) {
    final leftWave = Path()
      ..moveTo(0.22, 0.22)
      ..cubicTo(0.18, 0.38, 0.16, 0.55, 0.22, 0.72)
      ..cubicTo(0.28, 0.58, 0.32, 0.42, 0.34, 0.28)
      ..cubicTo(0.30, 0.22, 0.26, 0.18, 0.22, 0.22)
      ..close();

    final rightWave = Path()
      ..moveTo(0.78, 0.22)
      ..cubicTo(0.82, 0.38, 0.84, 0.55, 0.78, 0.72)
      ..cubicTo(0.72, 0.58, 0.68, 0.42, 0.66, 0.28)
      ..cubicTo(0.70, 0.22, 0.74, 0.18, 0.78, 0.22)
      ..close();

    final bangs = Path()
      ..moveTo(0.28, 0.18)
      ..cubicTo(0.36, 0.28, 0.44, 0.26, 0.50, 0.22)
      ..cubicTo(0.56, 0.26, 0.64, 0.28, 0.72, 0.18)
      ..cubicTo(0.62, 0.10, 0.38, 0.10, 0.28, 0.18)
      ..close();

    final paint = Paint()
      ..shader = ui.Gradient.linear(
        const Offset(0.50, 0.08),
        const Offset(0.50, 0.70),
        const [_hairHi, _hairMid, _hairDeep],
        const [0.0, 0.4, 1.0],
      );
    canvas.drawPath(leftWave, paint);
    canvas.drawPath(rightWave, paint);
    canvas.drawPath(bangs, paint);

    // Highlight streaks.
    final hiPaint = Paint()
      ..color = const Color(0x334A3028)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.02
      ..strokeCap = StrokeCap.round;
    canvas.drawPath(
      Path()
        ..moveTo(0.34, 0.20)
        ..cubicTo(0.30, 0.36, 0.28, 0.50, 0.30, 0.62),
      hiPaint,
    );
    canvas.drawPath(
      Path()
        ..moveTo(0.66, 0.20)
        ..cubicTo(0.70, 0.36, 0.72, 0.50, 0.70, 0.62),
      hiPaint,
    );
  }

  void _paintFeatures(Canvas canvas) {
    final lash = Paint()
      ..color = _lash
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.025
      ..strokeCap = StrokeCap.round;

    // Closed eyes — soft arcs with a hint of lid.
    canvas.drawPath(
      Path()
        ..moveTo(0.30, 0.46)
        ..quadraticBezierTo(0.36, 0.50, 0.42, 0.46),
      lash,
    );
    canvas.drawPath(
      Path()
        ..moveTo(0.58, 0.46)
        ..quadraticBezierTo(0.64, 0.50, 0.70, 0.46),
      lash,
    );

    // Subtle brows.
    final brow = Paint()
      ..color = const Color(0x882A1814)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.014
      ..strokeCap = StrokeCap.round;
    canvas.drawPath(
      Path()
        ..moveTo(0.28, 0.40)
        ..quadraticBezierTo(0.36, 0.37, 0.43, 0.39),
      brow,
    );
    canvas.drawPath(
      Path()
        ..moveTo(0.57, 0.39)
        ..quadraticBezierTo(0.64, 0.37, 0.72, 0.40),
      brow,
    );

    // Nose hint.
    canvas.drawPath(
      Path()
        ..moveTo(0.50, 0.48)
        ..quadraticBezierTo(0.53, 0.56, 0.50, 0.58),
      Paint()
        ..color = const Color(0x55A06850)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 0.016
        ..strokeCap = StrokeCap.round,
    );

    // Soft lips.
    final lips = Path()
      ..moveTo(0.40, 0.66)
      ..quadraticBezierTo(0.50, 0.70, 0.60, 0.66)
      ..quadraticBezierTo(0.50, 0.72, 0.40, 0.66)
      ..close();
    canvas.drawPath(
      lips,
      Paint()
        ..shader = ui.Gradient.linear(
          const Offset(0.50, 0.64),
          const Offset(0.50, 0.72),
          const [Color(0xFFE88878), _lip],
        ),
    );
  }

  void _paintHeadphones(Canvas canvas) {
    // Headband arc over the hair.
    final bandPath = Path()
      ..moveTo(0.18, 0.38)
      ..cubicTo(0.22, 0.10, 0.78, 0.10, 0.82, 0.38);
    canvas.drawPath(
      bandPath,
      Paint()
        ..shader = ui.Gradient.linear(
          const Offset(0.50, 0.08),
          const Offset(0.50, 0.30),
          const [_metalHi, _metalMid, _metalDeep],
          const [0.0, 0.5, 1.0],
        )
        ..style = PaintingStyle.stroke
        ..strokeWidth = 0.07
        ..strokeCap = StrokeCap.round,
    );
    // Inner band highlight.
    canvas.drawPath(
      bandPath,
      Paint()
        ..color = const Color(0x66FFFFFF)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 0.02
        ..strokeCap = StrokeCap.round,
    );

    _paintCup(canvas, center: const Offset(0.16, 0.48));
    _paintCup(canvas, center: const Offset(0.84, 0.48));
  }

  void _paintCup(Canvas canvas, {required Offset center}) {
    final outer = Rect.fromCircle(center: center, radius: 0.13);
    canvas.drawOval(
      outer,
      Paint()
        ..shader = ui.Gradient.radial(
          center.translate(-0.03, -0.03),
          0.16,
          const [_metalHi, _metalMid, _metalDeep],
          const [0.0, 0.45, 1.0],
        ),
    );
    canvas.drawOval(
      Rect.fromCircle(center: center, radius: 0.08),
      Paint()
        ..shader = ui.Gradient.radial(
          center.translate(-0.02, -0.02),
          0.10,
          const [Color(0xFF3A3A3A), Color(0xFF1A1A1A)],
        ),
    );
    // Specular glint.
    canvas.drawOval(
      Rect.fromCenter(
        center: center.translate(-0.04, -0.04),
        width: 0.05,
        height: 0.03,
      ),
      Paint()..color = const Color(0x88FFFFFF),
    );
  }

  @override
  bool shouldRepaint(covariant TrampLogoPainter oldDelegate) => false;
}
