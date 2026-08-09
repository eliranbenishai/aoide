import 'package:flutter/widgets.dart';

import '../../../theme/mockup_tokens.dart';
import 'mockup_hover.dart';

/// Horizontal slider matching mockup `.track` / `.fill` / `.thumb`.
class MockupSlider extends StatefulWidget {
  const MockupSlider({
    super.key,
    required this.value,
    this.onChanged,
    this.trackHeight = 14,
    this.thumbSize,
    this.seekStyle = false,
  });

  /// Normalized position in `0..1`.
  final double value;
  final ValueChanged<double>? onChanged;
  final double trackHeight;

  /// Defaults to `20×30` (`.thumb`) or `22×32` when [seekStyle] (`.seek-track`).
  final Size? thumbSize;

  /// Seek bar uses a slightly asymmetric fill radius (mockup `.seek-track`)
  /// and thumb `22×32` (default `.thumb` is `20×30`).
  final bool seekStyle;

  @override
  State<MockupSlider> createState() => _MockupSliderState();
}

class _MockupSliderState extends State<MockupSlider> {
  double get _clamped => widget.value.clamp(0.0, 1.0);

  Size get _thumbSize =>
      widget.thumbSize ??
      (widget.seekStyle ? const Size(22, 32) : const Size(20, 30));

  void _emit(double dx, double width) {
    if (widget.onChanged == null || width <= 0) return;
    widget.onChanged!((dx / width).clamp(0.0, 1.0));
  }

  @override
  Widget build(BuildContext context) {
    final effectiveThumb = _thumbSize;
    final enabled = widget.onChanged != null;
    return LayoutBuilder(
      builder: (context, constraints) {
        final width = constraints.maxWidth;
        final height = effectiveThumb.height > widget.trackHeight
            ? effectiveThumb.height
            : widget.trackHeight;
        return SizedBox(
          width: width,
          height: height,
          child: MockupHover(
            enabled: enabled,
            cursor: enabled
                ? SystemMouseCursors.click
                : SystemMouseCursors.basic,
            builder: (context, hover) {
              return GestureDetector(
                behavior: HitTestBehavior.opaque,
                onTapDown: enabled
                    ? (details) => _emit(details.localPosition.dx, width)
                    : null,
                onHorizontalDragUpdate: enabled
                    ? (details) => _emit(details.localPosition.dx, width)
                    : null,
                child: CustomPaint(
                  painter: _SliderPainter(
                    value: _clamped,
                    trackHeight: widget.trackHeight,
                    thumbSize: effectiveThumb,
                    seekStyle: widget.seekStyle,
                    hover: hover,
                  ),
                ),
              );
            },
          ),
        );
      },
    );
  }
}

class _SliderPainter extends CustomPainter {
  const _SliderPainter({
    required this.value,
    required this.trackHeight,
    required this.thumbSize,
    required this.seekStyle,
    required this.hover,
  });

  final double value;
  final double trackHeight;
  final Size thumbSize;
  final bool seekStyle;
  final double hover;

  @override
  void paint(Canvas canvas, Size size) {
    final trackTop = (size.height - trackHeight) / 2;
    final track = RRect.fromRectAndRadius(
      Rect.fromLTWH(0, trackTop, size.width, trackHeight),
      const Radius.circular(999),
    );

    canvas.drawRRect(
      track,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color(0xFF06070A),
            Color(0xFF141821),
            Color(0xFF1E222C),
          ],
          stops: [0, 0.6, 1],
        ).createShader(track.outerRect),
    );
    canvas.drawRRect(
      track,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = const Color(0x14E2ECFF),
    );
    canvas.drawRRect(
      track,
      Paint()
        ..color = const Color(0xF2000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.inner, 2),
    );

    final fillWidth =
        ((size.width - 4) * value).clamp(0.0, size.width - 4);
    if (fillWidth > 0) {
      final fillRect = Rect.fromLTWH(2, trackTop + 2, fillWidth, trackHeight - 4);
      final fill = seekStyle
          ? RRect.fromRectAndCorners(
              fillRect,
              topLeft: const Radius.circular(999),
              bottomLeft: const Radius.circular(999),
              topRight: const Radius.circular(3),
              bottomRight: const Radius.circular(3),
            )
          : RRect.fromRectAndRadius(fillRect, const Radius.circular(999));
      canvas.drawRRect(
        fill,
        Paint()
          ..color = const Color(0x663DE7FF)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
      );
      canvas.drawRRect(
        fill,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: const [
              Color(0xFFCBF9FF),
              MockupTokens.phos,
              Color(0xFF0F7F96),
            ],
            stops: const [0, 0.4, 1],
          ).createShader(fillRect),
      );
      canvas.drawRRect(
        fill,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1
          ..color = const Color(0x99F0FDFF),
      );
    }

    final thumbX = (size.width * value).clamp(
      thumbSize.width / 2,
      size.width - thumbSize.width / 2,
    );
    final thumbRect = Rect.fromCenter(
      center: Offset(thumbX, size.height / 2),
      width: thumbSize.width,
      height: thumbSize.height,
    );
    final thumb = RRect.fromRectAndRadius(thumbRect, const Radius.circular(4));
    canvas.drawRRect(
      thumb.shift(const Offset(0, 1.5)),
      Paint()
        ..color = const Color(0xB3000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2),
    );

    // Soft phosphor bloom on the thumb face (not the whole track).
    if (hover > 0.001) {
      canvas.drawRRect(
        thumb.inflate(1.5),
        Paint()
          ..color = Color.fromRGBO(61, 231, 255, 0.35 * hover)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3),
      );
    }

    final thumbColors = [
      mockupHoverLift(const Color(0xFF6F7688), hover),
      mockupHoverLift(const Color(0xFF3D4350), hover),
      mockupHoverLift(const Color(0xFF22262F), hover),
    ];
    canvas.drawRRect(
      thumb,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: thumbColors,
          stops: const [0, 0.4, 1],
        ).createShader(thumbRect),
    );
    canvas.drawRRect(
      thumb,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = Color.lerp(
          const Color(0x80ECF4FF),
          const Color(0xCC3DE7FF),
          hover,
        )!,
    );

    // Grip ridges (::after inset 8px 5px) — brighten toward phosphor on hover.
    final grip = Rect.fromLTRB(
      thumbRect.left + 5,
      thumbRect.top + 8,
      thumbRect.right - 5,
      thumbRect.bottom - 8,
    );
    var y = grip.top;
    final dark = Paint()..color = const Color(0x8C000000);
    final light = Paint()
      ..color = Color.lerp(
        const Color(0x38E2ECFF),
        const Color(0xA63DE7FF),
        hover,
      )!;
    while (y < grip.bottom - 1) {
      canvas.drawRect(Rect.fromLTWH(grip.left, y, grip.width, 1), dark);
      canvas.drawRect(Rect.fromLTWH(grip.left, y + 1, grip.width, 1), light);
      y += 2;
    }
  }

  @override
  bool shouldRepaint(covariant _SliderPainter oldDelegate) =>
      value != oldDelegate.value ||
      trackHeight != oldDelegate.trackHeight ||
      thumbSize != oldDelegate.thumbSize ||
      seekStyle != oldDelegate.seekStyle ||
      hover != oldDelegate.hover;
}
