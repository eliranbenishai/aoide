import 'dart:math' as math;

import 'package:flutter/widgets.dart';

import 'skin_image.dart';

/// A slider whose thumb (and optional groove) are skin art.
///
/// The track is a fixed [trackSize] box: an optional [grooveAsset] fills it,
/// and the [thumbAsset] is positioned along the travel axis from [value]
/// (`0..1`). For [Axis.vertical], 1 is the top of the travel — matching the
/// equalizer bands and the chrome slider it mirrors.
class SkinSlider extends StatefulWidget {
  const SkinSlider({
    super.key,
    required this.axis,
    required this.value,
    this.onChanged,
    this.onChangeEnd,
    required this.trackSize,
    this.grooveAsset,
    required this.thumbAsset,
    this.thumbSize = const Size(17, 10),
    this.semanticLabel,
  });

  final Axis axis;

  /// Position in `0..1`. For [Axis.vertical], 1 is the top of the travel.
  final double value;
  final ValueChanged<double>? onChanged;
  final ValueChanged<double>? onChangeEnd;

  final Size trackSize;
  final String? grooveAsset;
  final String thumbAsset;
  final Size thumbSize;
  final String? semanticLabel;

  bool get isInteractive => onChanged != null || onChangeEnd != null;

  @override
  State<SkinSlider> createState() => _SkinSliderState();
}

class _SkinSliderState extends State<SkinSlider> {
  double? _preview;

  /// Drag-end callbacks carry no local position, so the last update's position
  /// is remembered to resolve the final value.
  Offset? _lastLocal;

  double get _shown => (_preview ?? widget.value).clamp(0.0, 1.0);

  bool get _horizontal => widget.axis == Axis.horizontal;

  /// The thumb's length along the travel axis (used as the dead-zone at each
  /// end so the thumb centre can reach 0 and 1 without overhanging).
  double get _thumbExtent =>
      _horizontal ? widget.thumbSize.width : widget.thumbSize.height;

  double _fractionFor(Offset local) {
    if (_horizontal) {
      final usable = math.max(1.0, widget.trackSize.width - _thumbExtent);
      return ((local.dx - _thumbExtent / 2) / usable).clamp(0.0, 1.0);
    }
    final usable = math.max(1.0, widget.trackSize.height - _thumbExtent);
    // Screen y grows downward; a slider's value grows upward.
    return (1 - (local.dy - _thumbExtent / 2) / usable).clamp(0.0, 1.0);
  }

  void _update(Offset local, {required bool end}) {
    _lastLocal = local;
    final fraction = _fractionFor(local);
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
  void _endDrag() {
    final local = _lastLocal;
    if (local == null) {
      setState(() => _preview = null);
      return;
    }
    _update(local, end: true);
  }

  double get _thumbLeft {
    if (!_horizontal) return (widget.trackSize.width - widget.thumbSize.width) / 2;
    final usable = math.max(0.0, widget.trackSize.width - widget.thumbSize.width);
    return usable * _shown;
  }

  double get _thumbTop {
    if (_horizontal) {
      return (widget.trackSize.height - widget.thumbSize.height) / 2;
    }
    final usable =
        math.max(0.0, widget.trackSize.height - widget.thumbSize.height);
    // value 1 sits at the top (offset 0).
    return usable * (1 - _shown);
  }

  @override
  Widget build(BuildContext context) {
    Widget stack = SizedBox.fromSize(
      size: widget.trackSize,
      child: Stack(
        children: [
          if (widget.grooveAsset != null)
            SkinImage(asset: widget.grooveAsset!, logicalSize: widget.trackSize),
          Positioned(
            left: _thumbLeft,
            top: _thumbTop,
            child: SkinImage(
              asset: widget.thumbAsset,
              logicalSize: widget.thumbSize,
            ),
          ),
        ],
      ),
    );

    if (widget.isInteractive) {
      stack = GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTapDown: (d) => _update(d.localPosition, end: true),
        onHorizontalDragUpdate: _horizontal
            ? (d) => _update(d.localPosition, end: false)
            : null,
        onHorizontalDragEnd: _horizontal ? (_) => _endDrag() : null,
        onHorizontalDragCancel:
            _horizontal ? () => setState(() => _preview = null) : null,
        onVerticalDragUpdate: _horizontal
            ? null
            : (d) => _update(d.localPosition, end: false),
        onVerticalDragEnd: _horizontal ? null : (_) => _endDrag(),
        onVerticalDragCancel:
            _horizontal ? null : () => setState(() => _preview = null),
        child: stack,
      );
    }

    if (widget.semanticLabel != null) {
      stack = Semantics(
        slider: true,
        label: widget.semanticLabel,
        value: '${(_shown * 100).round()}%',
        child: stack,
      );
    }

    return stack;
  }
}
