import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';

/// Vector glyphs for every icon in the chrome.
///
/// Painted rather than drawn from a font so they stay crisp at every zoom step
/// and need no icon-font asset.
abstract final class TransportIcons {
  static const defaultGlyphColour = TrampColors.label;

  static Widget prev({Color colour = defaultGlyphColour}) =>
      _paint(SkipPainter(colour: colour, forward: false), const Size(16, 12));

  static Widget next({Color colour = defaultGlyphColour}) =>
      _paint(SkipPainter(colour: colour, forward: true), const Size(16, 12));

  static Widget play({Color colour = TrampColors.phosphor}) =>
      _paint(PlayPainter(colour: colour), const Size(14, 14));

  static Widget pause({Color colour = defaultGlyphColour}) =>
      _paint(PausePainter(colour: colour), const Size(12, 14));

  static Widget stop({Color colour = defaultGlyphColour}) =>
      _paint(StopPainter(colour: colour), const Size(12, 12));

  static Widget shuffle({Color colour = defaultGlyphColour}) =>
      _paint(ShufflePainter(colour: colour), const Size(16, 12));

  static Widget repeat({Color colour = defaultGlyphColour, bool one = false}) =>
      _paint(RepeatPainter(colour: colour, one: one), const Size(16, 13));

  static Widget eject({Color colour = defaultGlyphColour}) =>
      _paint(EjectPainter(colour: colour), const Size(13, 12));

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
  ..strokeWidth = width
  ..strokeJoin = StrokeJoin.round;

class PlayPainter extends CustomPainter {
  const PlayPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, 0)
      ..lineTo(size.width, size.height / 2)
      ..lineTo(0, size.height)
      ..close();
    canvas.drawPath(path, _fill(colour));
  }

  @override
  bool shouldRepaint(PlayPainter old) => old.colour != colour;
}

class PausePainter extends CustomPainter {
  const PausePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final barWidth = size.width * 0.36;
    canvas.drawRect(
      Rect.fromLTWH(0, 0, barWidth, size.height),
      _fill(colour),
    );
    canvas.drawRect(
      Rect.fromLTWH(size.width - barWidth, 0, barWidth, size.height),
      _fill(colour),
    );
  }

  @override
  bool shouldRepaint(PausePainter old) => old.colour != colour;
}

class StopPainter extends CustomPainter {
  const StopPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    canvas.drawRect(Offset.zero & size, _fill(colour));
  }

  @override
  bool shouldRepaint(StopPainter old) => old.colour != colour;
}

class SkipPainter extends CustomPainter {
  const SkipPainter({required this.colour, required this.forward});

  final Color colour;
  final bool forward;

  @override
  void paint(Canvas canvas, Size size) {
    final triangle = size.width * 0.42;
    final barWidth = size.width * 0.12;

    void wedge(double left, bool pointsRight) {
      final path = Path();
      if (pointsRight) {
        path
          ..moveTo(left, 0)
          ..lineTo(left + triangle, size.height / 2)
          ..lineTo(left, size.height);
      } else {
        path
          ..moveTo(left + triangle, 0)
          ..lineTo(left, size.height / 2)
          ..lineTo(left + triangle, size.height);
      }
      canvas.drawPath(path..close(), _fill(colour));
    }

    if (forward) {
      wedge(0, true);
      wedge(triangle * 0.9, true);
      canvas.drawRect(
        Rect.fromLTWH(size.width - barWidth, 0, barWidth, size.height),
        _fill(colour),
      );
    } else {
      canvas.drawRect(Rect.fromLTWH(0, 0, barWidth, size.height), _fill(colour));
      wedge(barWidth + triangle * 0.1, false);
      wedge(barWidth + triangle, false);
    }
  }

  @override
  bool shouldRepaint(SkipPainter old) =>
      old.colour != colour || old.forward != forward;
}

class ShufflePainter extends CustomPainter {
  const ShufflePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final p = _stroke(colour, math.max(1.0, size.height * 0.13));
    // Two crossing paths with arrowheads: the classic shuffle mark.
    canvas.drawLine(Offset(0, size.height * 0.2),
        Offset(size.width * 0.78, size.height * 0.8), p);
    canvas.drawLine(Offset(0, size.height * 0.8),
        Offset(size.width * 0.78, size.height * 0.2), p);

    for (final y in [size.height * 0.2, size.height * 0.8]) {
      final path = Path()
        ..moveTo(size.width * 0.72, y - size.height * 0.18)
        ..lineTo(size.width, y)
        ..lineTo(size.width * 0.72, y + size.height * 0.18)
        ..close();
      canvas.drawPath(path, _fill(colour));
    }
  }

  @override
  bool shouldRepaint(ShufflePainter old) => old.colour != colour;
}

/// Loop glyph for repeat / repeat-one.
///
/// When [one] is true, paints a single vertical stroke inside the loop — the
/// numeral one reduced to what reads at 16 logical pixels. A literal `1` would
/// be roughly four pixels tall and unreadable at the glyph's real size.
class RepeatPainter extends CustomPainter {
  const RepeatPainter({required this.colour, required this.one});

  final Color colour;

  /// When true, draws a single vertical stroke inside the loop — the numeral
  /// one reduced to what reads at 16 logical pixels.
  final bool one;

  @override
  void paint(Canvas canvas, Size size) {
    final stroke = math.max(1.0, size.height * 0.13);
    final p = _stroke(colour, stroke);
    final rect = Rect.fromLTWH(
      stroke,
      stroke,
      size.width - stroke * 2,
      size.height - stroke * 2,
    );
    canvas.drawRRect(
      RRect.fromRectAndRadius(rect, Radius.circular(size.height * 0.35)),
      p,
    );

    final head = Path()
      ..moveTo(rect.right - size.width * 0.22, rect.top - size.height * 0.06)
      ..lineTo(rect.right, rect.top + size.height * 0.14)
      ..lineTo(rect.right - size.width * 0.22, rect.top + size.height * 0.34)
      ..close();
    canvas.drawPath(head, _fill(colour));

    if (one) {
      final bar = Rect.fromLTWH(
        size.width / 2 - stroke / 2,
        size.height * 0.32,
        stroke,
        size.height * 0.36,
      );
      canvas.drawRect(bar, _fill(colour));
    }
  }

  @override
  bool shouldRepaint(RepeatPainter old) =>
      old.colour != colour || old.one != one;
}

class EjectPainter extends CustomPainter {
  const EjectPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final triangle = Path()
      ..moveTo(0, size.height * 0.62)
      ..lineTo(size.width / 2, 0)
      ..lineTo(size.width, size.height * 0.62)
      ..close();
    canvas.drawPath(triangle, _fill(colour));
    canvas.drawRect(
      Rect.fromLTWH(0, size.height * 0.78, size.width, size.height * 0.22),
      _fill(colour),
    );
  }

  @override
  bool shouldRepaint(EjectPainter old) => old.colour != colour;
}

/// Small downward chevron for dropdown buttons.
class ChevronPainter extends CustomPainter {
  const ChevronPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, 0)
      ..lineTo(size.width, 0)
      ..lineTo(size.width / 2, size.height)
      ..close();
    canvas.drawPath(path, _fill(colour));
  }

  @override
  bool shouldRepaint(ChevronPainter old) => old.colour != colour;
}
