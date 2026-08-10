import 'package:flutter/widgets.dart';

import '../../../look/look_palette.dart';
import '../../../theme/look_paint.dart';
import '../../../theme/look_scope.dart';
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
    final palette = LookScope.of(context).palette;
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
                    palette: palette,
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
    required this.palette,
    required this.value,
    required this.trackHeight,
    required this.thumbSize,
    required this.seekStyle,
    required this.hover,
  });

  final LookPalette palette;
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
          ..color = LookPaint.phosphorBloom(palette, 0x66)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
      );
      canvas.drawRRect(
        fill,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              LookPaint.sliderFillHi(palette),
              palette.phosphorDefault,
              LookPaint.sliderFillLo(palette),
            ],
            stops: const [0, 0.4, 1],
          ).createShader(fillRect),
      );
      canvas.drawRRect(
        fill,
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1
          ..color = LookPaint.buttonOnLip(palette).withValues(alpha: 0x99 / 255),
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

    final lift = LookPaint.hoverLiftTarget(palette);
    if (hover > 0.001) {
      canvas.drawRRect(
        thumb.inflate(2),
        Paint()
          ..color = palette.phosphorDefault.withValues(alpha: 0.22 * hover)
          ..maskFilter = MaskFilter.blur(BlurStyle.normal, 5 + 3 * hover),
      );
    }

    canvas.drawRRect(
      thumb.shift(const Offset(0, 1)),
      Paint()
        ..color = const Color(0xA6000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 2),
    );
    final thumbHi = Color.lerp(palette.shellHighlight, palette.inkDim, 0.55)!;
    final thumbMid = Color.lerp(palette.shellBase, palette.shellHighlight, 0.35)!;
    final thumbLo = Color.lerp(palette.shellMid, palette.shellBase, 0.4)!;
    canvas.drawRRect(
      thumb,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            mockupHoverLift(thumbHi, hover, MockupHoverTokens.faceLift, lift),
            mockupHoverLift(thumbMid, hover, MockupHoverTokens.faceLift, lift),
            mockupHoverLift(thumbLo, hover, MockupHoverTokens.faceLift, lift),
          ],
          stops: const [0, 0.55, 1],
        ).createShader(thumbRect),
    );
    canvas.drawRRect(
      thumb,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..shader = LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            mockupHoverLift(
              Color.lerp(palette.inkDim, lift, 0.45)!,
              hover,
              0.2,
              lift,
            ),
            mockupHoverLift(
              Color.lerp(palette.shellBase, palette.shellMid, 0.3)!,
              hover,
              0.1,
              lift,
            ),
          ],
        ).createShader(thumbRect),
    );

    // Grip ridges (::after inset 8px 5px) — brighten toward phosphor on hover.
    final grip = Rect.fromLTRB(
      thumbRect.left + 5,
      thumbRect.top + 8,
      thumbRect.right - 5,
      thumbRect.bottom - 8,
    );
    final ridgeColor = Color.lerp(
      lift.withValues(alpha: 0x38 / 255),
      palette.phosphorDefault.withValues(alpha: 0.55),
      hover,
    )!;
    for (var y = grip.top; y < grip.bottom; y += 2.2) {
      canvas.drawLine(
        Offset(grip.left, y),
        Offset(grip.right, y),
        Paint()
          ..color = ridgeColor
          ..strokeWidth = 1,
      );
    }
  }

  @override
  bool shouldRepaint(covariant _SliderPainter oldDelegate) =>
      value != oldDelegate.value ||
      trackHeight != oldDelegate.trackHeight ||
      thumbSize != oldDelegate.thumbSize ||
      seekStyle != oldDelegate.seekStyle ||
      hover != oldDelegate.hover ||
      palette != oldDelegate.palette;
}
