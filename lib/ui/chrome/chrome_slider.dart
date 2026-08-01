import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

class ChromeSlider extends StatefulWidget {
  const ChromeSlider({
    super.key,
    required this.value,
    this.onChanged,
    this.onChangeEnd,
  });

  /// Normalized position in `0..1`.
  final double value;
  final ValueChanged<double>? onChanged;
  final ValueChanged<double>? onChangeEnd;

  @override
  State<ChromeSlider> createState() => _ChromeSliderState();
}

class _ChromeSliderState extends State<ChromeSlider> {
  static const double _trackHeight = 8;
  static const double _thumbWidth = 12;
  static const double _thumbHeight = 18;

  double? _dragValue;

  double get _displayValue =>
      (_dragValue ?? widget.value).clamp(0.0, 1.0);

  double _valueFromLocalDx(double dx, double width) {
    final usable = (width - _thumbWidth).clamp(1.0, double.infinity);
    return ((dx - _thumbWidth / 2) / usable).clamp(0.0, 1.0);
  }

  void _setDragValue(double value) {
    setState(() => _dragValue = value);
    widget.onChanged?.call(value);
  }

  void _endDrag() {
    final value = _displayValue;
    widget.onChangeEnd?.call(value);
    setState(() => _dragValue = null);
  }

  void _cancelDrag() {
    setState(() => _dragValue = null);
  }

  @override
  Widget build(BuildContext context) {
    final enabled = widget.onChanged != null || widget.onChangeEnd != null;

    return LayoutBuilder(
      builder: (context, constraints) {
        final width = constraints.maxWidth;
        final value = _displayValue;
        final thumbLeft =
            value * (width - _thumbWidth).clamp(0.0, double.infinity);
        final fillWidth = thumbLeft + _thumbWidth / 2;

        return GestureDetector(
          behavior: HitTestBehavior.opaque,
          onHorizontalDragStart: enabled
              ? (details) =>
                  _setDragValue(_valueFromLocalDx(details.localPosition.dx, width))
              : null,
          onHorizontalDragUpdate: enabled
              ? (details) =>
                  _setDragValue(_valueFromLocalDx(details.localPosition.dx, width))
              : null,
          onHorizontalDragEnd: enabled ? (_) => _endDrag() : null,
          onHorizontalDragCancel: enabled ? _cancelDrag : null,
          onTapDown: enabled
              ? (details) =>
                  _setDragValue(_valueFromLocalDx(details.localPosition.dx, width))
              : null,
          onTapUp: enabled ? (_) => _endDrag() : null,
          onTapCancel: enabled ? _cancelDrag : null,
          child: SizedBox(
            width: width,
            height: constraints.maxHeight.isFinite
                ? constraints.maxHeight
                : _thumbHeight,
            child: Center(
              child: Stack(
                clipBehavior: Clip.none,
                alignment: Alignment.centerLeft,
                children: [
                  // Groove track
                  Container(
                    height: _trackHeight,
                    decoration: BoxDecoration(
                      color: TrampColors.groove,
                      border: Border(
                        top: BorderSide(
                          color: TrampColors.metalDeep,
                          width: TrampColors.borderWidth,
                        ),
                        left: BorderSide(
                          color: TrampColors.metalDeep,
                          width: TrampColors.borderWidth,
                        ),
                        right: BorderSide(
                          color: TrampColors.metalShadow,
                          width: TrampColors.borderWidth,
                        ),
                        bottom: BorderSide(
                          color: TrampColors.metalShadow,
                          width: TrampColors.borderWidth,
                        ),
                      ),
                    ),
                  ),
                  // Green fill to value
                  Align(
                    alignment: Alignment.centerLeft,
                    child: Container(
                      width: fillWidth.clamp(0.0, width),
                      height: _trackHeight - 2,
                      color: TrampColors.fillAccent,
                    ),
                  ),
                  // Metal thumb
                  Positioned(
                    left: thumbLeft,
                    child: Container(
                      width: _thumbWidth,
                      height: _thumbHeight,
                      decoration: const BoxDecoration(
                        gradient: LinearGradient(
                          begin: Alignment.topCenter,
                          end: Alignment.bottomCenter,
                          colors: [
                            TrampColors.metalHi,
                            TrampColors.metalFace,
                            TrampColors.metalShadow,
                          ],
                        ),
                        border: Border(
                          top: BorderSide(
                            color: TrampColors.metalHi,
                            width: TrampColors.borderWidth,
                          ),
                          left: BorderSide(
                            color: TrampColors.metalHi,
                            width: TrampColors.borderWidth,
                          ),
                          right: BorderSide(
                            color: TrampColors.metalDeep,
                            width: TrampColors.borderWidth,
                          ),
                          bottom: BorderSide(
                            color: TrampColors.metalDeep,
                            width: TrampColors.borderWidth,
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ),
        );
      },
    );
  }
}
