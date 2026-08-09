import 'dart:async';

import 'package:flutter/widgets.dart';
import 'package:window_manager/window_manager.dart';

import '../../theme/mockup_tokens.dart';

/// Bottom-right size grip for frameless windows (`windowManager.startResizing`).
///
/// Frameless HWNDs do not get OS edge hit-testing from `setResizable(true)`
/// alone — this paints a Winamp-style corner hook and starts a native resize.
class WindowResizeGrip extends StatelessWidget {
  const WindowResizeGrip({
    super.key,
    this.size = 16,
    this.enabled = true,
    this.startResizing,
  });

  final double size;
  final bool enabled;

  /// Override for tests; defaults to [windowManager.startResizing].
  final Future<void> Function(ResizeEdge edge)? startResizing;

  @override
  Widget build(BuildContext context) {
    if (!enabled) return const SizedBox.shrink();
    return Semantics(
      label: 'Resize window',
      button: true,
      child: MouseRegion(
        cursor: SystemMouseCursors.resizeUpLeftDownRight,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onPanStart: (_) {
            final start = startResizing ?? windowManager.startResizing;
            unawaited(start(ResizeEdge.bottomRight));
          },
          child: CustomPaint(
            size: Size.square(size),
            painter: const _ResizeGripPainter(),
          ),
        ),
      ),
    );
  }
}

class _ResizeGripPainter extends CustomPainter {
  const _ResizeGripPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = MockupTokens.inkDim.withValues(alpha: 0.85)
      ..strokeWidth = 1.2
      ..strokeCap = StrokeCap.round;
    final dim = size.shortestSide;
    // Three diagonal ticks, classic SE size-grip.
    for (var i = 0; i < 3; i++) {
      final t = (i + 1) / 4.0;
      final inset = dim * 0.12;
      final start = Offset(dim * t + inset * 0.2, dim - inset);
      final end = Offset(dim - inset, dim * t + inset * 0.2);
      canvas.drawLine(start, end, paint);
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
