import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

/// Vector transport glyphs sized for [ChromeButton] children.
abstract final class TransportIcons {
  static Widget prev({
    double size = 14,
    Color color = TrampColors.metalDeep,
  }) {
    return _TransportIcon(
      size: size,
      painter: _PrevPainter(color),
    );
  }

  static Widget play({
    double size = 14,
    Color color = TrampColors.metalDeep,
  }) {
    return _TransportIcon(
      size: size,
      painter: _PlayPainter(color),
    );
  }

  static Widget pause({
    double size = 14,
    Color color = TrampColors.metalDeep,
  }) {
    return _TransportIcon(
      size: size,
      painter: _PausePainter(color),
    );
  }

  static Widget stop({
    double size = 14,
    Color color = TrampColors.metalDeep,
  }) {
    return _TransportIcon(
      size: size,
      painter: _StopPainter(color),
    );
  }

  static Widget next({
    double size = 14,
    Color color = TrampColors.metalDeep,
  }) {
    return _TransportIcon(
      size: size,
      painter: _NextPainter(color),
    );
  }
}

class _TransportIcon extends StatelessWidget {
  const _TransportIcon({
    required this.size,
    required this.painter,
  });

  final double size;
  final CustomPainter painter;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size.square(size),
      painter: painter,
    );
  }
}

class _PrevPainter extends CustomPainter {
  _PrevPainter(this.color);

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = color;
    final w = size.width;
    final h = size.height;
    // Vertical bar + left-pointing triangle.
    canvas.drawRect(
      Rect.fromLTWH(w * 0.12, h * 0.2, w * 0.16, h * 0.6),
      paint,
    );
    final path = Path()
      ..moveTo(w * 0.88, h * 0.18)
      ..lineTo(w * 0.88, h * 0.82)
      ..lineTo(w * 0.32, h * 0.5)
      ..close();
    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(covariant _PrevPainter oldDelegate) =>
      oldDelegate.color != color;
}

class _PlayPainter extends CustomPainter {
  _PlayPainter(this.color);

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = color;
    final w = size.width;
    final h = size.height;
    final path = Path()
      ..moveTo(w * 0.28, h * 0.15)
      ..lineTo(w * 0.28, h * 0.85)
      ..lineTo(w * 0.88, h * 0.5)
      ..close();
    canvas.drawPath(path, paint);
  }

  @override
  bool shouldRepaint(covariant _PlayPainter oldDelegate) =>
      oldDelegate.color != color;
}

class _PausePainter extends CustomPainter {
  _PausePainter(this.color);

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = color;
    final w = size.width;
    final h = size.height;
    canvas.drawRect(
      Rect.fromLTWH(w * 0.22, h * 0.18, w * 0.2, h * 0.64),
      paint,
    );
    canvas.drawRect(
      Rect.fromLTWH(w * 0.58, h * 0.18, w * 0.2, h * 0.64),
      paint,
    );
  }

  @override
  bool shouldRepaint(covariant _PausePainter oldDelegate) =>
      oldDelegate.color != color;
}

class _StopPainter extends CustomPainter {
  _StopPainter(this.color);

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = color;
    final inset = size.shortestSide * 0.22;
    canvas.drawRect(
      Rect.fromLTRB(inset, inset, size.width - inset, size.height - inset),
      paint,
    );
  }

  @override
  bool shouldRepaint(covariant _StopPainter oldDelegate) =>
      oldDelegate.color != color;
}

class _NextPainter extends CustomPainter {
  _NextPainter(this.color);

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = color;
    final w = size.width;
    final h = size.height;
    final path = Path()
      ..moveTo(w * 0.12, h * 0.18)
      ..lineTo(w * 0.12, h * 0.82)
      ..lineTo(w * 0.68, h * 0.5)
      ..close();
    canvas.drawPath(path, paint);
    canvas.drawRect(
      Rect.fromLTWH(w * 0.72, h * 0.2, w * 0.16, h * 0.6),
      paint,
    );
  }

  @override
  bool shouldRepaint(covariant _NextPainter oldDelegate) =>
      oldDelegate.color != color;
}
