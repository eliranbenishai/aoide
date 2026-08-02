import 'package:flutter/material.dart';

import '../../domain/equalizer_settings.dart';
import '../../eq/equalizer_controller.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/chrome_button.dart';
import '../chrome/chrome_slider.dart';
import '../chrome/metal_panel.dart';
import '../chrome/title_bar.dart';

/// The ten-band equalizer panel.
///
/// Authored against the fixed 812x206 canvas with absolute positions taken from
/// the design spec's geometry table, so the layout is the measured mockup rather
/// than an approximation of it.
class EqualizerPanel extends StatelessWidget {
  const EqualizerPanel({
    super.key,
    required this.controller,
    required this.onCollapse,
    required this.onClose,
    this.collapsed = false,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.equalizer;

  /// Logical x of each band slider's centre.
  static const List<double> bandCentres = [
    196, 245, 294, 343, 392, 441, 490, 539, 588, 636,
  ];

  static const List<String> bandLabels = [
    '60', '170', '310', '600', '1K', '3K', '6K', '12K', '14K', '16K',
  ];

  static const double _preampCentre = 73;
  static const double _sliderTop = 71;
  static const double _sliderBottom = 166;
  static const double _sliderWidth = 34;

  final EqualizerController controller;
  final VoidCallback onCollapse;
  final VoidCallback onClose;
  final bool collapsed;
  final bool draggableTitle;

  /// dB gain maps to a 0..1 slider position, centre being 0 dB.
  static double _toFraction(double gain) =>
      ((gain + EqualizerSettings.gainLimit) /
              (EqualizerSettings.gainLimit * 2))
          .clamp(0.0, 1.0);

  static double _toGain(double fraction) =>
      fraction * EqualizerSettings.gainLimit * 2 -
      EqualizerSettings.gainLimit;

  static String _format(double gain) =>
      '${gain >= 0 ? '+' : '-'}${gain.abs().toStringAsFixed(1)}';

  @override
  Widget build(BuildContext context) {
    final titleBar = TrampTitleBar(
      title: 'TRAMP EQUALIZER',
      draggable: draggableTitle,
      leading: ChromeButton.icon(
        key: const Key('eq-collapse'),
        icon: SizedBox(
          width: 9,
          height: 6,
          child: CustomPaint(
            painter: _CollapsePainter(colour: TrampColors.label),
          ),
        ),
        onPressed: onCollapse,
        semanticLabel: 'Collapse equalizer',
        size: const Size(27, 27),
      ),
      trailing: [
        ChromeButton.label(
          key: const Key('eq-close'),
          text: 'X',
          onPressed: onClose,
          semanticLabel: 'Close equalizer',
          size: const Size(27, 27),
        ),
      ],
    );

    if (collapsed) {
      return SizedBox(
        width: logicalSize.width,
        height: TrampMetrics.titleBar,
        child: MetalPanel(
          surface: TrampSurface.raisedPanel,
          child: titleBar,
        ),
      );
    }

    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: MetalPanel(
        surface: TrampSurface.raisedPanel,
        child: ListenableBuilder(
          listenable: controller,
          builder: (context, _) {
            final settings = controller.settings;

            return Stack(
              children: [
                Positioned(left: 0, right: 0, top: 0, child: titleBar),

                Positioned(
                  left: 36,
                  top: 42,
                  child: ChromeButton.label(
                    key: const Key('eq-on'),
                    text: 'ON',
                    active: settings.enabled,
                    onPressed: () => controller.setEnabled(!settings.enabled),
                    size: const Size(33, 20),
                  ),
                ),
                Positioned(
                  left: 93,
                  top: 42,
                  child: ChromeButton.label(
                    key: const Key('eq-auto'),
                    text: 'AUTO',
                    active: settings.auto,
                    onPressed: () => controller.setAuto(!settings.auto),
                    size: const Size(37, 20),
                  ),
                ),

                Positioned(
                  left: 677,
                  top: 44,
                  child: _PresetsButton(controller: controller),
                ),

                // Preamp, with its printed dB scale.
                const Positioned(
                  left: 40,
                  top: 56,
                  child: Text('PREAMP', style: TrampText.eqScale),
                ),
                _slider(
                  key: const Key('eq-preamp'),
                  centreX: _preampCentre,
                  value: _toFraction(settings.preamp),
                  onChanged: (f) => controller.setPreamp(_toGain(f)),
                  semanticLabel: 'Preamp',
                ),
                _valueLabel(_preampCentre, _format(settings.preamp)),
                const Positioned(
                  left: 100,
                  top: _sliderTop - 4,
                  child: Text('+12 dB', style: TrampText.eqScale),
                ),
                const Positioned(
                  left: 100,
                  top: (_sliderTop + _sliderBottom) / 2 - 5,
                  child: Text('0 dB', style: TrampText.eqScale),
                ),
                const Positioned(
                  left: 100,
                  top: _sliderBottom - 8,
                  child: Text('-12 dB', style: TrampText.eqScale),
                ),

                for (var i = 0; i < bandCentres.length; i++) ...[
                  Positioned(
                    left: bandCentres[i] - _sliderWidth / 2,
                    top: 53,
                    width: _sliderWidth,
                    child: Text(
                      bandLabels[i],
                      textAlign: TextAlign.center,
                      style: TrampText.eqScale,
                    ),
                  ),
                  _slider(
                    key: Key('eq-band-$i'),
                    centreX: bandCentres[i],
                    value: _toFraction(settings.gains[i]),
                    onChanged: (f) => controller.setGain(i, _toGain(f)),
                    semanticLabel: '${bandLabels[i]} hertz',
                  ),
                  _valueLabel(bandCentres[i], _format(settings.gains[i])),
                ],
              ],
            );
          },
        ),
      ),
    );
  }

  Widget _slider({
    required Key key,
    required double centreX,
    required double value,
    required ValueChanged<double> onChanged,
    required String semanticLabel,
  }) {
    return Positioned(
      left: centreX - _sliderWidth / 2,
      top: _sliderTop,
      width: _sliderWidth,
      height: _sliderBottom - _sliderTop,
      child: ChromeSlider(
        key: key,
        value: value,
        axis: Axis.vertical,
        semanticLabel: semanticLabel,
        onChanged: onChanged,
        onChangeEnd: onChanged,
      ),
    );
  }

  Widget _valueLabel(double centreX, String text) {
    return Positioned(
      left: centreX - _sliderWidth,
      top: 174,
      width: _sliderWidth * 2,
      child: Text(
        text,
        textAlign: TextAlign.center,
        style: TrampText.eqScale.copyWith(color: TrampColors.phosphor),
      ),
    );
  }
}

class _PresetsButton extends StatelessWidget {
  const _PresetsButton({required this.controller});

  final EqualizerController controller;

  @override
  Widget build(BuildContext context) {
    return ChromeButton.dropdown(
      key: const Key('eq-presets'),
      text: 'PRESETS',
      size: const Size(99, 22),
      onPressed: () async {
        final box = context.findRenderObject()! as RenderBox;
        final origin = box.localToGlobal(Offset.zero);
        final chosen = await showMenu<String>(
          context: context,
          position: RelativeRect.fromLTRB(
            origin.dx,
            origin.dy + box.size.height,
            origin.dx,
            origin.dy,
          ),
          items: [
            for (final name in controller.presetNames)
              PopupMenuItem<String>(value: name, child: Text(name)),
          ],
        );
        if (chosen != null) controller.applyPreset(chosen);
      },
    );
  }
}

/// The upward triangle on the collapse button.
class _CollapsePainter extends CustomPainter {
  const _CollapsePainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final path = Path()
      ..moveTo(0, size.height)
      ..lineTo(size.width / 2, 0)
      ..lineTo(size.width, size.height)
      ..close();
    canvas.drawPath(path, Paint()..color = colour);
  }

  @override
  bool shouldRepaint(_CollapsePainter old) => old.colour != colour;
}
