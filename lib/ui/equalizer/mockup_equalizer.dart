import 'package:flutter/material.dart';

import '../../domain/equalizer_settings.dart';
import '../../theme/mockup_tokens.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_screen.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../session/session_messages.dart';
import 'eq_curve_painter.dart';

/// Mockup-faithful equalizer body (825×306).
///
/// Absolute layout matches `player-mockup-2.html` EQ section. Interactions emit
/// session commands ([EqEnabledCommand], [EqAutoCommand], [ApplyPresetCommand],
/// [EqPreampCommand], [EqGainCommand]) for the host [EqualizerController].
class MockupEqualizer extends StatelessWidget {
  const MockupEqualizer({
    super.key,
    required this.settings,
    this.onSessionCommand,
    this.presetNames = const [],
  });

  static const bodySize = Size(825, 306);
  static const windowSize = TrampMetrics.equalizer;

  static const bandLabels = [
    '60',
    '170',
    '310',
    '600',
    '1k',
    '3k',
    '6k',
    '12k',
    '14k',
    '16k',
  ];

  final EqualizerSettings settings;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final List<String> presetNames;

  void _emit(SessionCommand command) => onSessionCommand?.call(command);

  String get _curveCaption {
    final name = settings.presetName?.trim();
    if (name == null || name.isEmpty) return 'Curve · Custom';
    return 'Curve · $name';
  }

  @override
  Widget build(BuildContext context) {
    final names = presetNames.isEmpty
        ? EqualizerPresets.builtIn.keys.toList()
        : presetNames;

    return SizedBox(
      width: bodySize.width,
      height: bodySize.height,
      child: Stack(
        children: [
          const Positioned(left: 9, bottom: 8, child: MockupRivet()),
          const Positioned(right: 9, bottom: 8, child: MockupRivet()),
          Positioned(
            left: 22,
            top: 16,
            child: _EqHead(
              enabled: settings.enabled,
              auto: settings.auto,
              curveCaption: _curveCaption,
              presetNames: names,
              onToggleEnabled: () =>
                  _emit(EqEnabledCommand(!settings.enabled)),
              onToggleAuto: () => _emit(EqAutoCommand(!settings.auto)),
              onApplyPreset: (name) => _emit(ApplyPresetCommand(name)),
            ),
          ),
          Positioned(
            right: 22,
            top: 16,
            width: 372,
            height: 62,
            child: MockupScreen(
              child: CustomPaint(
                painter: EqCurvePainter(
                  preamp: settings.preamp,
                  gains: settings.gains,
                ),
                size: const Size(372, 62),
              ),
            ),
          ),
          Positioned(
            left: 22,
            right: 22,
            top: 92,
            height: 196,
            child: _EqBands(
              settings: settings,
              onPreamp: (gain) => _emit(EqPreampCommand(gain)),
              onBand: (band, gain) =>
                  _emit(EqGainCommand(band: band, gain: gain)),
            ),
          ),
        ],
      ),
    );
  }
}

class _EqHead extends StatelessWidget {
  const _EqHead({
    required this.enabled,
    required this.auto,
    required this.curveCaption,
    required this.presetNames,
    required this.onToggleEnabled,
    required this.onToggleAuto,
    required this.onApplyPreset,
  });

  final bool enabled;
  final bool auto;
  final String curveCaption;
  final List<String> presetNames;
  final VoidCallback onToggleEnabled;
  final VoidCallback onToggleAuto;
  final ValueChanged<String> onApplyPreset;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        MockupButton(
          key: const Key('eq-on'),
          label: 'On',
          on: enabled,
          height: 38,
          padding: const EdgeInsets.symmetric(horizontal: 16),
          onPressed: onToggleEnabled,
          semanticLabel: 'Equalizer on',
        ),
        const SizedBox(width: 8),
        MockupButton(
          key: const Key('eq-auto'),
          label: 'Auto',
          on: auto,
          height: 38,
          padding: const EdgeInsets.symmetric(horizontal: 16),
          onPressed: onToggleAuto,
          semanticLabel: 'Auto',
        ),
        const SizedBox(width: 8),
        _PresetsButton(
          presetNames: presetNames,
          onApplyPreset: onApplyPreset,
        ),
        const SizedBox(width: 14),
        Text(
          curveCaption.toUpperCase(),
          style: const TextStyle(
            fontFamily: 'TrampCondensed',
            fontWeight: FontWeight.w700,
            fontSize: 11,
            height: 1,
            letterSpacing: 11 * 0.2,
            color: MockupTokens.inkFaint,
            shadows: [
              Shadow(offset: Offset(0, 1), color: Color(0xB3000000)),
            ],
          ),
        ),
      ],
    );
  }
}

class _PresetsButton extends StatelessWidget {
  const _PresetsButton({
    required this.presetNames,
    required this.onApplyPreset,
  });

  final List<String> presetNames;
  final ValueChanged<String> onApplyPreset;

  Future<void> _open(BuildContext context) async {
    final box = context.findRenderObject() as RenderBox?;
    if (box == null) return;
    final origin = box.localToGlobal(Offset.zero);
    final size = box.size;
    final selected = await showMenu<String>(
      context: context,
      position: RelativeRect.fromLTRB(
        origin.dx,
        origin.dy + size.height + 4,
        origin.dx + size.width,
        origin.dy,
      ),
      color: MockupTokens.shellMid,
      items: [
        for (final name in presetNames)
          PopupMenuItem<String>(
            value: name,
            child: Text(
              name,
              style: const TextStyle(
                color: MockupTokens.ink,
                fontFamily: 'TrampCondensed',
                fontWeight: FontWeight.w700,
                fontSize: 13,
                letterSpacing: 13 * 0.12,
              ),
            ),
          ),
      ],
    );
    if (selected != null) onApplyPreset(selected);
  }

  @override
  Widget build(BuildContext context) {
    return MockupButton(
      key: const Key('eq-presets'),
      label: 'Presets',
      menu: true,
      height: 38,
      padding: const EdgeInsets.fromLTRB(16, 0, 22, 0),
      onPressed: () => _open(context),
      semanticLabel: 'Presets',
    );
  }
}

class _EqBands extends StatelessWidget {
  const _EqBands({
    required this.settings,
    required this.onPreamp,
    required this.onBand,
  });

  final EqualizerSettings settings;
  final ValueChanged<double> onPreamp;
  final void Function(int band, double gain) onBand;

  @override
  Widget build(BuildContext context) {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        const _EqScale(),
        _BandColumn(
          key: const Key('eq-preamp'),
          width: 62,
          marginRight: 16,
          label: 'PREAMP',
          preampStyle: true,
          gain: settings.preamp,
          onChanged: onPreamp,
          semanticLabel: 'Preamp',
        ),
        for (var i = 0; i < EqualizerSettings.bandFrequencies.length; i++)
          _BandColumn(
            key: Key('eq-band-$i'),
            width: 50,
            label: MockupEqualizer.bandLabels[i],
            gain: settings.gains[i],
            onChanged: (gain) => onBand(i, gain),
            semanticLabel: '${MockupEqualizer.bandLabels[i]} hertz',
          ),
      ],
    );
  }
}

class _EqScale extends StatelessWidget {
  const _EqScale();

  @override
  Widget build(BuildContext context) {
    return const SizedBox(
      width: 44,
      child: Padding(
        padding: EdgeInsets.fromLTRB(0, 4, 8, 26),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          crossAxisAlignment: CrossAxisAlignment.end,
          children: [
            _ScaleLabel('+12'),
            _ScaleLabel('0'),
            _ScaleLabel('−12'),
          ],
        ),
      ),
    );
  }
}

class _ScaleLabel extends StatelessWidget {
  const _ScaleLabel(this.text);

  final String text;

  @override
  Widget build(BuildContext context) {
    return Text(
      text,
      style: const TextStyle(
        fontFamily: 'TrampMono',
        fontWeight: FontWeight.w500,
        fontSize: 11,
        height: 1,
        color: MockupTokens.inkFaint,
      ),
    );
  }
}

class _BandColumn extends StatelessWidget {
  const _BandColumn({
    super.key,
    required this.width,
    required this.label,
    required this.gain,
    required this.onChanged,
    required this.semanticLabel,
    this.marginRight = 0,
    this.preampStyle = false,
  });

  final double width;
  final double marginRight;
  final String label;
  final double gain;
  final ValueChanged<double> onChanged;
  final String semanticLabel;
  final bool preampStyle;

  @override
  Widget build(BuildContext context) {
    return Container(
      width: width,
      margin: EdgeInsets.only(right: marginRight),
      child: Column(
        children: [
          Expanded(
            child: _VerticalBandSlider(
              gain: gain,
              onChanged: onChanged,
              semanticLabel: semanticLabel,
            ),
          ),
          SizedBox(
            height: 26,
            child: Center(
              child: Text(
                label,
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontFamily: 'TrampCondensed',
                  fontWeight: FontWeight.w700,
                  fontSize: 11,
                  height: 1,
                  letterSpacing: 11 * (preampStyle ? 0.18 : 0.1),
                  color: preampStyle
                      ? const Color(0x8C3DE7FF)
                      : MockupTokens.inkFaint,
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

/// Vertical fader matching mockup `.vtrack` / `.vthumb` (±12 dB).
class _VerticalBandSlider extends StatelessWidget {
  const _VerticalBandSlider({
    required this.gain,
    required this.onChanged,
    required this.semanticLabel,
  });

  final double gain;
  final ValueChanged<double> onChanged;
  final String semanticLabel;

  static double _toFraction(double g) =>
      ((g + EqualizerSettings.gainLimit) / (EqualizerSettings.gainLimit * 2))
          .clamp(0.0, 1.0);

  static double _toGain(double fraction) =>
      fraction * EqualizerSettings.gainLimit * 2 - EqualizerSettings.gainLimit;

  void _emit(double localY, double height) {
    if (height <= 0) return;
    // Top of travel = +12 dB (fraction 1).
    final fraction = (1.0 - (localY / height)).clamp(0.0, 1.0);
    onChanged(_toGain(fraction));
  }

  @override
  Widget build(BuildContext context) {
    return Semantics(
      slider: true,
      label: semanticLabel,
      value: '${gain.toStringAsFixed(1)} dB',
      child: LayoutBuilder(
        builder: (context, constraints) {
          final height = constraints.maxHeight;
          return GestureDetector(
            behavior: HitTestBehavior.opaque,
            onTapDown: (d) => _emit(d.localPosition.dy, height),
            onVerticalDragUpdate: (d) => _emit(d.localPosition.dy, height),
            child: Center(
              child: SizedBox(
                width: 34,
                height: height,
                child: CustomPaint(
                  painter: _VTrackPainter(fraction: _toFraction(gain)),
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}

class _VTrackPainter extends CustomPainter {
  const _VTrackPainter({required this.fraction});

  final double fraction;

  @override
  void paint(Canvas canvas, Size size) {
    const trackW = 12.0;
    final trackLeft = (size.width - trackW) / 2;
    final track = RRect.fromRectAndRadius(
      Rect.fromLTWH(trackLeft, 0, trackW, size.height),
      const Radius.circular(999),
    );

    canvas.drawRRect(
      track,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.centerLeft,
          end: Alignment.centerRight,
          colors: [
            Color(0xFF06070A),
            Color(0xFF161A22),
            Color(0xFF1E222C),
          ],
          stops: [0, 0.55, 1],
        ).createShader(track.outerRect),
    );
    canvas.drawRRect(
      track,
      Paint()
        ..color = const Color(0xF2000000)
        ..maskFilter = const MaskFilter.blur(BlurStyle.inner, 2),
    );

    // Zero notch.
    final midY = size.height / 2;
    canvas.drawRect(
      Rect.fromLTWH(trackLeft - 13, midY - 0.5, trackW + 26, 1),
      Paint()..color = const Color(0x24E2ECFF),
    );

    // Thumb (top = +12 → fraction 1).
    final thumbY = (1.0 - fraction.clamp(0.0, 1.0)) * size.height;
    const thumbW = 34.0;
    const thumbH = 18.0;
    final thumb = RRect.fromRectAndRadius(
      Rect.fromCenter(
        center: Offset(size.width / 2, thumbY),
        width: thumbW,
        height: thumbH,
      ),
      const Radius.circular(3),
    );
    canvas.drawRRect(
      thumb,
      Paint()
        ..shader = const LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            Color(0xFF757C8F),
            Color(0xFF3D4350),
            Color(0xFF1E222C),
          ],
          stops: [0, 0.42, 1],
        ).createShader(thumb.outerRect),
    );
    canvas.drawRRect(
      thumb,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = const Color(0x59ECF4FF),
    );

    final line = RRect.fromRectAndRadius(
      Rect.fromCenter(
        center: Offset(size.width / 2, thumbY),
        width: thumbW - 12,
        height: 2,
      ),
      const Radius.circular(1),
    );
    canvas.drawRRect(
      line,
      Paint()
        ..shader = const LinearGradient(
          colors: [Color(0xFFCBF9FF), MockupTokens.phos],
        ).createShader(line.outerRect),
    );
    canvas.drawRRect(
      line,
      Paint()
        ..color = const Color(0xBF3DE7FF)
        ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 4),
    );
  }

  @override
  bool shouldRepaint(covariant _VTrackPainter oldDelegate) =>
      oldDelegate.fraction != fraction;
}
