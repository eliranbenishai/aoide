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

    return SizedBox(
      height: TrampMetrics.titleBar,
      child: Row(
        children: [
          if (leading != null) ...[
            const SizedBox(width: 4),
            leading!,
            const SizedBox(width: 6),
          ],
          Expanded(child: centre),
          for (final widget in trailing) ...[
            const SizedBox(width: 5),
            widget,
          ],
          const SizedBox(width: 5),
        ],
      ),
    );
  }
}

/// Two horizontal accent lines, the classic title-bar grip motif.
class RailPainter extends CustomPainter {
  const RailPainter({required this.colour, required this.thickness});

  final Color colour;
  final double thickness;

  @override
  void paint(Canvas canvas, Size size) {
    // Positions mirror the mockup: rails at y 26 and y 31 of a 35-tall bar,
    // measured from the panel top.
    final paint = Paint()..color = colour;
    for (final y in [size.height * 0.40, size.height * 0.55]) {
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, thickness), paint);
    }
  }

  @override
  bool shouldRepaint(RailPainter old) =>
      old.colour != colour || old.thickness != thickness;
}
