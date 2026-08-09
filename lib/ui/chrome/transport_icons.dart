import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';

/// Code-drawn overlay glyphs (e.g. mute speaker, playlist drag handle).
///
/// Painted rather than drawn from a font so they stay crisp at every zoom
/// step and need no icon-font asset. Product chrome otherwise uses mockup
/// widgets under `lib/ui/chrome/mockup/`.
abstract final class TransportIcons {
  static const defaultGlyphColour = TrampColors.label;

  static Widget speaker({
    Color colour = defaultGlyphColour,
    bool muted = false,
    Size size = const Size(12, 12),
  }) =>
      _paint(SpeakerPainter(colour: colour, muted: muted), size);

  static Widget dragHandle({Color colour = defaultGlyphColour}) =>
      _paint(DragHandlePainter(colour: colour), const Size(12, 10));

  /// Circular reload / repeat arrows for LCD status chips.
  static Widget reload({
    Color colour = defaultGlyphColour,
    Size size = const Size(11, 11),
  }) =>
      _paint(ReloadPainter(colour: colour), size);

  static Widget _paint(CustomPainter painter, Size size) => SizedBox(
        width: size.width,
        height: size.height,
        child: CustomPaint(painter: painter),
      );
}

Paint _fill(Color colour) => Paint()..color = colour;

Paint _stroke(Color colour, double width) => Paint()
  ..color = colour
  ..style = PaintingStyle.stroke
  ..strokeWidth = width;

/// Speaker cone; when [muted], a slash crosses the glyph.
class SpeakerPainter extends CustomPainter {
  const SpeakerPainter({required this.colour, required this.muted});

  final Color colour;
  final bool muted;

  @override
  void paint(Canvas canvas, Size size) {
    final body = Path()
      ..moveTo(size.width * 0.08, size.height * 0.35)
      ..lineTo(size.width * 0.32, size.height * 0.35)
      ..lineTo(size.width * 0.58, size.height * 0.12)
      ..lineTo(size.width * 0.58, size.height * 0.88)
      ..lineTo(size.width * 0.32, size.height * 0.65)
      ..lineTo(size.width * 0.08, size.height * 0.65)
      ..close();
    canvas.drawPath(body, _fill(colour));

    if (!muted) {
      final wave = _stroke(colour, 1.4)..strokeCap = StrokeCap.round;
      canvas.drawArc(
        Rect.fromCenter(
          center: Offset(size.width * 0.55, size.height / 2),
          width: size.width * 0.55,
          height: size.height * 0.55,
        ),
        -math.pi / 3,
        2 * math.pi / 3,
        false,
        wave,
      );
    } else {
      final slash = _stroke(colour, 1.5)..strokeCap = StrokeCap.square;
      canvas.drawLine(
        Offset(size.width * 0.12, size.height * 0.88),
        Offset(size.width * 0.88, size.height * 0.12),
        slash,
      );
    }
  }

  @override
  bool shouldRepaint(SpeakerPainter old) =>
      old.colour != colour || old.muted != muted;
}

/// Three short horizontal bars for the playlist reorder affordance.
class DragHandlePainter extends CustomPainter {
  const DragHandlePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final barHeight = math.max(1.5, size.height * 0.15);
    final insetX = size.width * 0.08;
    final span = size.height - barHeight;
    for (var i = 0; i < 3; i++) {
      final y = span * (i / 2);
      canvas.drawRect(
        Rect.fromLTWH(insetX, y, size.width - insetX * 2, barHeight),
        _fill(colour),
      );
    }
  }

  @override
  bool shouldRepaint(DragHandlePainter old) => old.colour != colour;
}

/// Two opposing circular arrows (reload / repeat).
class ReloadPainter extends CustomPainter {
  const ReloadPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final width = math.max(1.2, size.shortestSide * 0.14);
    final stroke = Paint()
      ..color = colour
      ..style = PaintingStyle.stroke
      ..strokeWidth = width
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round;
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.shortestSide * 0.34;

    void arrow(double start, double sweep, double tipAngle) {
      canvas.drawArc(
        Rect.fromCircle(center: center, radius: radius),
        start,
        sweep,
        false,
        stroke,
      );
      final tip = Offset(
        center.dx + radius * math.cos(tipAngle),
        center.dy + radius * math.sin(tipAngle),
      );
      final tangent = tipAngle + math.pi / 2;
      final head = size.shortestSide * 0.22;
      final wing = size.shortestSide * 0.16;
      final path = Path()
        ..moveTo(
          tip.dx + head * math.cos(tangent),
          tip.dy + head * math.sin(tangent),
        )
        ..lineTo(tip.dx, tip.dy)
        ..lineTo(
          tip.dx - wing * math.cos(tipAngle),
          tip.dy - wing * math.sin(tipAngle),
        );
      canvas.drawPath(path, stroke);
    }

    arrow(-math.pi * 0.85, math.pi * 0.95, math.pi * 0.12);
    arrow(math.pi * 0.15, math.pi * 0.95, math.pi * 1.12);
  }

  @override
  bool shouldRepaint(ReloadPainter old) => old.colour != colour;
}
