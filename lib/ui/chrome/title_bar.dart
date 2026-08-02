import 'package:flutter/widgets.dart';
import 'package:window_manager/window_manager.dart';

import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../zoom/zoom_scope.dart';

/// The title bar shared by the main player and the equalizer.
///
/// `draggable` exists so widget tests and goldens can render the bar without a
/// live `window_manager` binding.
class TrampTitleBar extends StatelessWidget {
  const TrampTitleBar({
    super.key,
    required this.title,
    this.leading,
    this.trailing = const [],
    this.draggable = true,
  });

  final String title;
  final Widget? leading;
  final List<Widget> trailing;
  final bool draggable;

  @override
  Widget build(BuildContext context) {
    final thickness = ZoomScope.hairlineFor(context) * 2;

    Widget centre = Row(
      children: [
        Expanded(
          child: CustomPaint(
            painter: RailPainter(
              colour: TrampColors.railAccent,
              thickness: thickness,
            ),
            child: const SizedBox(height: TrampMetrics.titleBar),
          ),
        ),
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12),
          child: Text(title, style: TrampText.wordmark),
        ),
        Expanded(
          child: CustomPaint(
            painter: RailPainter(
              colour: TrampColors.railAccent,
              thickness: thickness,
            ),
            child: const SizedBox(height: TrampMetrics.titleBar),
          ),
        ),
      ],
    );

    if (draggable) {
      centre = DragToMoveArea(child: centre);
    }

    Widget slot(Widget child) => SizedBox(
          height: TrampMetrics.titleBar,
          child: Center(child: child),
        );

    return SizedBox(
      height: TrampMetrics.titleBar,
      child: ClipRect(
        child: Row(
          children: [
            if (leading != null) ...[
              const SizedBox(width: 4),
              slot(leading!),
              const SizedBox(width: 6),
            ],
            Expanded(child: centre),
            for (final widget in trailing) ...[
              const SizedBox(width: 5),
              slot(widget),
            ],
            const SizedBox(width: 5),
          ],
        ),
      ),
    );
  }
}

/// Two horizontal accent lines, the classic title-bar grip motif.
class RailPainter extends CustomPainter {
  const RailPainter({
    required this.colour,
    required this.thickness,
    this.firstY = 17,
    this.secondY = 22,
  });

  final Color colour;
  final double thickness;

  /// Distance from the bar's top to each rail, in logical pixels.
  ///
  /// Absolute rather than fractional: these come from measuring the reference
  /// mockup, where the rails sit at logical y 17 and y 22 of the 35-tall bar.
  /// Expressing them as fractions of the height invites drift.
  final double firstY;
  final double secondY;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = colour;
    for (final y in [firstY, secondY]) {
      if (y + thickness > size.height) continue;
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, thickness), paint);
    }
  }

  @override
  bool shouldRepaint(RailPainter old) =>
      old.colour != colour ||
      old.thickness != thickness ||
      old.firstY != firstY ||
      old.secondY != secondY;
}
