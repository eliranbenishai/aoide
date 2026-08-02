import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import '../../theme/tramp_colors.dart';
import '../zoom/zoom_scope.dart';

/// Groove, phosphor fill and metal thumb — the slider language the equalizer
/// bands and the volume control share.
class ChromeSlider extends StatefulWidget {
  const ChromeSlider({
    super.key,
    required this.value,
    required this.axis,
    this.onChanged,
    this.onChangeEnd,
    this.dimmed = false,
    this.thumbExtent = 17,
    this.thumbThickness = 10,
    this.semanticLabel,
  });

  /// Position in `0..1`. For [Axis.vertical], 1 is the top of the travel.
  final double value;
  final Axis axis;
  final ValueChanged<double>? onChanged;
  final ValueChanged<double>? onChangeEnd;

  /// Renders the fill in [TrampColors.phosphorDim] — used while muted.
  final bool dimmed;

  /// Thumb size measured across and along the travel axis.
  final double thumbExtent;
  final double thumbThickness;

  final String? semanticLabel;

  bool get isInteractive => onChanged != null || onChangeEnd != null;

  @override
  State<ChromeSlider> createState() => _ChromeSliderState();
}

class _ChromeSliderState extends State<ChromeSlider> {
  double? _preview;

  /// Drag-end callbacks carry no local position, so the last update's position
  /// is remembered to resolve the final value.
  Offset? _lastLocal;

  double get _shown => (_preview ?? widget.value).clamp(0.0, 1.0);

  double _fractionFor(Offset local, Size size) {
    if (widget.axis == Axis.horizontal) {
      final usable = math.max(1.0, size.width - widget.thumbExtent);
      return ((local.dx - widget.thumbExtent / 2) / usable).clamp(0.0, 1.0);
    }
    final usable = math.max(1.0, size.height - widget.thumbExtent);
    // Screen y grows downward; a slider's value grows upward.
    return (1 - (local.dy - widget.thumbExtent / 2) / usable).clamp(0.0, 1.0);
  }

  void _update(Offset local, Size size, {required bool end}) {
    _lastLocal = local;
    final fraction = _fractionFor(local, size);
    setState(() => _preview = fraction);
    widget.onChanged?.call(fraction);
    if (end) {
      widget.onChangeEnd?.call(fraction);
      setState(() => _preview = null);
    }
  }

  /// Commits the last recorded drag position, or no-ops when none exists.
  ///
  /// Drag-end events carry no local position. Falling back to [Offset.zero]
  /// would jump a horizontal slider to 0 and a vertical one to 1.
  void _endDrag(Size size) {
    final local = _lastLocal;
    if (local == null) {
      setState(() => _preview = null);
      return;
    }
    _update(local, size, end: true);
  }

  @override
  Widget build(BuildContext context) {
    final bevel = ZoomScope.hairlineFor(context);

    Widget slider = LayoutBuilder(
      builder: (context, constraints) {
        final size = Size(constraints.maxWidth, constraints.maxHeight);

        final paint = CustomPaint(
          painter: SliderPainter(
            value: _shown,
            axis: widget.axis,
            fill: widget.dimmed
                ? TrampColors.phosphorDim
                : TrampColors.phosphor,
            bevel: bevel,
            thumbExtent: widget.thumbExtent,
            thumbThickness: widget.thumbThickness,
          ),
          size: size,
        );

        if (!widget.isInteractive) return paint;

        return GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTapDown: (d) => _update(d.localPosition, size, end: true),
          onVerticalDragUpdate: widget.axis == Axis.vertical
              ? (d) => _update(d.localPosition, size, end: false)
              : null,
          onVerticalDragEnd: widget.axis == Axis.vertical
              ? (_) => _endDrag(size)
              : null,
          onHorizontalDragUpdate: widget.axis == Axis.horizontal
              ? (d) => _update(d.localPosition, size, end: false)
              : null,
          onHorizontalDragEnd: widget.axis == Axis.horizontal
              ? (_) => _endDrag(size)
              : null,
          onHorizontalDragCancel: () => setState(() => _preview = null),
          onVerticalDragCancel: () => setState(() => _preview = null),
          child: paint,
        );
      },
    );

    if (widget.semanticLabel != null) {
      slider = Semantics(
        slider: true,
        label: widget.semanticLabel,
        value: '${(_shown * 100).round()}%',
        child: slider,
      );
    }

    return slider;
  }
}

/// Paints the groove, the phosphor fill and the thumb.
class SliderPainter extends CustomPainter {
  const SliderPainter({
    required this.value,
    required this.axis,
    required this.fill,
    required this.bevel,
    required this.thumbExtent,
    required this.thumbThickness,
  });

  final double value;
  final Axis axis;
  final Color fill;
  final double bevel;
  final double thumbExtent;
  final double thumbThickness;

  @override
  void paint(Canvas canvas, Size size) {
    final horizontal = axis == Axis.horizontal;
    final grooveThickness = math.max(3.0, bevel * 3);

    final groove = horizontal
        ? Rect.fromLTWH(
            thumbExtent / 2,
            (size.height - grooveThickness) / 2,
            math.max(0, size.width - thumbExtent),
            grooveThickness,
          )
        : Rect.fromLTWH(
            (size.width - grooveThickness) / 2,
            thumbExtent / 2,
            grooveThickness,
            math.max(0, size.height - thumbExtent),
          );

    canvas.drawRect(groove, Paint()..color = TrampColors.wellDeep);
    canvas.drawRect(
      groove,
      Paint()
        ..color = TrampColors.bevelLo
        ..style = PaintingStyle.stroke
        ..strokeWidth = bevel,
    );

    // Fill runs from the low end of the travel to the thumb.
    final filled = horizontal
        ? Rect.fromLTWH(groove.left, groove.top, groove.width * value, groove.height)
        : Rect.fromLTWH(
            groove.left,
            groove.bottom - groove.height * value,
            groove.width,
            groove.height * value,
          );
    canvas.drawRect(filled, Paint()..color = fill);

    final centre = horizontal
        ? Offset(groove.left + groove.width * value, size.height / 2)
        : Offset(size.width / 2, groove.bottom - groove.height * value);

    final thumb = Rect.fromCenter(
      center: centre,
      width: horizontal ? thumbThickness : thumbExtent,
      height: horizontal ? thumbExtent : thumbThickness,
    );

    canvas.drawRect(
      thumb,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [TrampColors.thumbHi, TrampColors.buttonBottom],
        ).createShader(thumb),
    );
    canvas.drawRect(
      thumb,
      Paint()
        ..color = TrampColors.bevelLo
        ..style = PaintingStyle.stroke
        ..strokeWidth = bevel,
    );
  }

  @override
  bool shouldRepaint(SliderPainter old) =>
      old.value != value ||
      old.axis != axis ||
      old.fill != fill ||
      old.bevel != bevel ||
      old.thumbExtent != thumbExtent ||
      old.thumbThickness != thumbThickness;
}
