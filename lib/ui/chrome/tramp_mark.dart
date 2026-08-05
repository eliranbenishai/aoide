import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';

/// The compact Tramp mark, for chrome at control size.
///
/// The full logo — [TrampLogo] in `logo.dart`, a colour illustration of a pin-up
/// in headphones inside a ring badge — is the brand's primary asset and belongs
/// anywhere it has room: app icon, splash, and the About dialog opened from this
/// menu. It does not work here. At the 19 logical pixels a title-bar button
/// allows it collapses into a smudge, and its skin tones read as a photograph
/// pasted onto a metal panel.
///
/// So this is a reduction of the same idea to what survives that size: the ring
/// badge and the headphones, one colour, tinted by the caller like every other
/// glyph in the chrome.
class TrampMark extends StatelessWidget {
  const TrampMark({
    super.key,
    this.size = 19,
    this.colour = TrampColors.label,
  });

  final double size;
  final Color colour;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      label: 'Tramp',
      child: SizedBox(
        width: size,
        height: size,
        child: CustomPaint(painter: TrampMarkPainter(colour: colour)),
      ),
    );
  }
}

/// Paints the compact mark: a ring enclosing a headphone band and two earcups.
class TrampMarkPainter extends CustomPainter {
  const TrampMarkPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final unit = size.shortestSide;
    if (unit <= 0) return;

    final centre = Offset(size.width / 2, size.height / 2);
    final fill = Paint()..color = colour;

    // Ring. Kept thin so the interior stays readable at small sizes.
    final ringStroke = math.max(1.0, unit * 0.08);
    canvas.drawCircle(
      centre,
      unit * 0.46 - ringStroke / 2,
      Paint()
        ..color = colour
        ..style = PaintingStyle.stroke
        ..strokeWidth = ringStroke,
    );

    // Headband: an arc over the top, thick enough to hold at 19px.
    final bandStroke = math.max(1.0, unit * 0.10);
    final bandRadius = unit * 0.24;
    canvas.drawArc(
      Rect.fromCircle(center: centre.translate(0, unit * 0.02), radius: bandRadius),
      math.pi * 1.08,
      math.pi * 0.84,
      false,
      Paint()
        ..color = colour
        ..style = PaintingStyle.stroke
        ..strokeWidth = bandStroke
        ..strokeCap = StrokeCap.round,
    );

    // Earcups, sitting at the ends of the band.
    final cupWidth = unit * 0.13;
    final cupHeight = unit * 0.20;
    final cupY = centre.dy + unit * 0.06;
    for (final dx in [-bandRadius, bandRadius]) {
      canvas.drawRRect(
        RRect.fromRectAndRadius(
          Rect.fromCenter(
            center: Offset(centre.dx + dx, cupY),
            width: cupWidth,
            height: cupHeight,
          ),
          Radius.circular(cupWidth * 0.45),
        ),
        fill,
      );
    }
  }

  @override
  bool shouldRepaint(TrampMarkPainter oldDelegate) =>
      oldDelegate.colour != colour;
}
