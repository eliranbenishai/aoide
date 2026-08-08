import 'package:flutter/widgets.dart';

import '../../../theme/mockup_tokens.dart';

/// Window chassis matching mockup `.win`, plus `.rivet` / `.plate` / `.rail`.
class MockupShell extends StatelessWidget {
  const MockupShell({
    super.key,
    required this.child,
    this.width = 825,
    this.borderRadius = 6,
  });

  final Widget child;
  final double width;
  final double borderRadius;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: width,
      child: DecoratedBox(
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(borderRadius),
          boxShadow: const [
            BoxShadow(
              color: Color(0x80000000),
              offset: Offset(0, 2),
              blurRadius: 3,
            ),
            BoxShadow(
              color: Color(0xCC000000),
              offset: Offset(0, 18),
              blurRadius: 34,
              spreadRadius: -14,
            ),
          ],
        ),
        child: ClipRRect(
          borderRadius: BorderRadius.circular(borderRadius),
          child: Stack(
            children: [
              const Positioned.fill(child: CustomPaint(painter: _ShellPainter())),
              child,
            ],
          ),
        ),
      ),
    );
  }
}

/// Corner rivet matching mockup `.rivet` (7×7).
class MockupRivet extends StatelessWidget {
  const MockupRivet({super.key, this.size = 7});

  final double size;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size.square(size),
      painter: const _RivetPainter(),
    );
  }
}

/// Brushed metal plate matching mockup `.plate`.
class MockupPlate extends StatelessWidget {
  const MockupPlate({
    super.key,
    this.child,
    this.borderRadius = 4,
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
          const Positioned.fill(
            child: ColoredBox(color: Color(0xFF1E222C)),
          ),
          const Positioned.fill(
            child: CustomPaint(painter: _BrushPainter(opacity: 1)),
          ),
          const Positioned.fill(
            child: DecoratedBox(
              decoration: BoxDecoration(
                border: Border(
                  top: BorderSide(color: Color(0x1AE2ECFF)),
                  bottom: BorderSide(color: Color(0xB3000000)),
                ),
              ),
            ),
          ),
          if (child != null) child!,
        ],
      ),
    );
  }
}

/// Slack filler matching mockup `.rail`.
class MockupRail extends StatelessWidget {
  const MockupRail({
    super.key,
    this.height = 22,
    this.minWidth = 24,
  });

  final double height;
  final double minWidth;

  @override
  Widget build(BuildContext context) {
    return ConstrainedBox(
      constraints: BoxConstraints(minWidth: minWidth, minHeight: height),
      child: SizedBox(
        height: height,
        child: Opacity(
          opacity: 0.9,
          child: ClipRRect(
            borderRadius: BorderRadius.circular(3),
            child: const Stack(
              fit: StackFit.expand,
              children: [
                ColoredBox(color: Color(0xFF1E222C)),
                CustomPaint(painter: _BrushPainter(opacity: 1)),
                DecoratedBox(
                  decoration: BoxDecoration(
                    border: Border(
                      top: BorderSide(color: Color(0x14E2ECFF)),
                      bottom: BorderSide(color: Color(0x99000000)),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _ShellPainter extends CustomPainter {
  const _ShellPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    final rrect = RRect.fromRectAndRadius(rect, const Radius.circular(6));
    canvas.drawRRect(
      rrect,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            MockupTokens.shellHi,
            MockupTokens.shell,
            MockupTokens.shellMid,
            MockupTokens.shellLo,
            MockupTokens.shellDeep,
          ],
          stops: [0, 0.03, 0.46, 0.92, 1],
        ).createShader(rect),
    );

    // Inset bevels from mockup box-shadow stack.
    canvas.drawRRect(
      rrect,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = const Color(0x26E2ECFF),
    );
    canvas.drawLine(
      const Offset(1, 1),
      Offset(1, size.height - 1),
      Paint()..color = const Color(0x0FE2ECFF),
    );
    canvas.drawLine(
      Offset(size.width - 1, 1),
      Offset(size.width - 1, size.height - 1),
      Paint()..color = const Color(0x8C000000),
    );
    canvas.drawLine(
      Offset(1, size.height - 1),
      Offset(size.width - 1, size.height - 1),
      Paint()..color = const Color(0xE6000000),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

class _RivetPainter extends CustomPainter {
  const _RivetPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.shortestSide / 2;
    canvas.drawCircle(
      Offset(center.dx, center.dy + 0.5),
      radius,
      Paint()..color = const Color(0x1FE2ECFF),
    );
    canvas.drawCircle(
      center,
      radius,
      Paint()
        ..shader = RadialGradient(
          center: const Alignment(-0.3, -0.4),
          colors: const [
            Color(0xFF5C6373),
            Color(0xFF262B33),
            Color(0xFF101218),
          ],
          stops: const [0, 0.6, 1],
        ).createShader(Rect.fromCircle(center: center, radius: radius)),
    );
    canvas.drawCircle(
      center,
      radius,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 0.8
        ..color = const Color(0xCC000000),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

/// Mockup `--brush` repeating horizontal grain.
class _BrushPainter extends CustomPainter {
  const _BrushPainter({required this.opacity});

  final double opacity;

  @override
  void paint(Canvas canvas, Size size) {
    final a = Paint()..color = Color.fromRGBO(226, 236, 255, 0.045 * opacity);
    final b = Paint()..color = Color.fromRGBO(0, 0, 0, 0.10 * opacity);
    final c = Paint()..color = Color.fromRGBO(226, 236, 255, 0.015 * opacity);
    for (var y = 0.0; y < size.height; y += 3) {
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, 1), a);
      canvas.drawRect(Rect.fromLTWH(0, y + 1, size.width, 1), b);
      canvas.drawRect(Rect.fromLTWH(0, y + 2, size.width, 1), c);
    }
  }

  @override
  bool shouldRepaint(covariant _BrushPainter oldDelegate) =>
      opacity != oldDelegate.opacity;
}
