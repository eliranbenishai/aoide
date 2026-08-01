import 'package:flutter/material.dart';

import '../../theme/tramp_colors.dart';

class InkSlider extends StatelessWidget {
  const InkSlider({
    super.key,
    required this.value,
    required this.onChanged,
    this.onChangeEnd,
  });

  final double value;
  final ValueChanged<double> onChanged;
  final ValueChanged<double>? onChangeEnd;

  @override
  Widget build(BuildContext context) {
    return Material(
      type: MaterialType.transparency,
      child: SliderTheme(
        data: SliderThemeData(
          trackHeight: 4,
          trackShape: const RoundedRectSliderTrackShape(),
          thumbShape: const RoundSliderThumbShape(enabledThumbRadius: 6),
          overlayShape: SliderComponentShape.noOverlay,
          activeTrackColor: TrampColors.ink.withValues(alpha: 0.25),
          inactiveTrackColor: TrampColors.ink.withValues(alpha: 0.15),
          thumbColor: TrampColors.accent,
        ),
        child: Slider(
          value: value.clamp(0.0, 1.0),
          onChanged: onChanged,
          onChangeEnd: onChangeEnd,
        ),
      ),
    );
  }
}
