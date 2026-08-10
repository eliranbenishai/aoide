import 'dart:ui' as ui;

import 'package:flutter/widgets.dart';

import '../../domain/equalizer_settings.dart';

/// Live EQ curve: zero line, cyan fill, `#8df2ff` stroke + glow.
///
/// Plots preamp (left) then the ten band gains across [size], mapping
/// ±[EqualizerSettings.gainLimit] dB to the vertical span (top = +12).
class EqCurvePainter extends CustomPainter {
  const EqCurvePainter({
    required this.preamp,
    required this.gains,
  });

  final double preamp;
  final List<double> gains;

  static const _stroke = Color(0xFF8DF2FF);
  static const _zero = Color(0x24E2ECFF);

  @override
  void paint(Canvas canvas, Size size) {
    if (size.width <= 0 || size.height <= 0) return;

    final midY = size.height / 2;
    canvas.drawLine(
      Offset(0, midY),
      Offset(size.width, midY),
      Paint()
        ..color = _zero
        ..strokeWidth = 1,
    );

    final points = _samplePoints(size);
    if (points.length < 2) return;

    final strokePath = _smoothPath(points);
    final fillPath = Path.from(strokePath)
      ..lineTo(size.width, size.height)
      ..lineTo(0, size.height)
      ..close();

    canvas.drawPath(
      fillPath,
      Paint()
        ..shader = ui.Gradient.linear(
          Offset.zero,
          Offset(0, size.height),
          const [
            Color(0x663DE7FF),
            Color(0x003DE7FF),
          ],
        ),
    );

    canvas.drawPath(
      strokePath,
      Paint()
        ..color = const Color(0x993DE7FF)
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.4
        ..strokeCap = StrokeCap.round
        ..strokeJoin = StrokeJoin.round
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2.5),
    );
    canvas.drawPath(
      strokePath,
      Paint()
        ..color = _stroke
        ..style = PaintingStyle.stroke
        ..strokeWidth = 2.4
        ..strokeCap = StrokeCap.round
        ..strokeJoin = StrokeJoin.round,
    );
  }

  List<Offset> _samplePoints(Size size) {
    final values = <double>[preamp, ...gains];
    if (values.isEmpty) return const [];
    final last = values.length - 1;
    return [
      for (var i = 0; i < values.length; i++)
        Offset(
          last == 0 ? 0 : size.width * (i / last),
          _yForGain(values[i], size.height),
        ),
    ];
  }

  double _yForGain(double gain, double height) {
    final clamped =
        gain.clamp(-EqualizerSettings.gainLimit, EqualizerSettings.gainLimit);
    // +12 at top (y=0), −12 at bottom (y=height).
    final t = (EqualizerSettings.gainLimit - clamped) /
        (EqualizerSettings.gainLimit * 2);
    return t * height;
  }

  /// Catmull-Rom → cubic Bézier through the band sample points.
  Path _smoothPath(List<Offset> points) {
    final path = Path()..moveTo(points.first.dx, points.first.dy);
    if (points.length == 2) {
      path.lineTo(points[1].dx, points[1].dy);
      return path;
    }
    for (var i = 0; i < points.length - 1; i++) {
      final p0 = i == 0 ? points[i] : points[i - 1];
      final p1 = points[i];
      final p2 = points[i + 1];
      final p3 = i + 2 < points.length ? points[i + 2] : p2;
      final c1 = Offset(
        p1.dx + (p2.dx - p0.dx) / 6,
        p1.dy + (p2.dy - p0.dy) / 6,
      );
      final c2 = Offset(
        p2.dx - (p3.dx - p1.dx) / 6,
        p2.dy - (p3.dy - p1.dy) / 6,
      );
      path.cubicTo(c1.dx, c1.dy, c2.dx, c2.dy, p2.dx, p2.dy);
    }
    return path;
  }

  @override
  bool shouldRepaint(covariant EqCurvePainter oldDelegate) {
    if (oldDelegate.preamp != preamp) return true;
    if (oldDelegate.gains.length != gains.length) return true;
    for (var i = 0; i < gains.length; i++) {
      if (oldDelegate.gains[i] != gains[i]) return true;
    }
    return false;
  }
}
