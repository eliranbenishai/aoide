import 'package:flutter/material.dart';

import '../../domain/equalizer_settings.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/logo.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_hover.dart';
import '../chrome/mockup/mockup_popup_menu.dart';
import '../chrome/mockup/mockup_screen.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../session/session_messages.dart';
import 'eq_curve_painter.dart';
import '../../look/look_materials.dart';
import '../../theme/look_scope.dart';

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
        clipBehavior: Clip.none,
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
          // Empty band-row gap (~159px) right of the last slider.
          const Positioned(
            right: 36,
            top: 120,
            child: IgnorePointer(
              child: Opacity(
                opacity: 0.14,
                child: TrampLogo(size: 120),
              ),
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
          style: TextStyle(
            fontFamily: LookScope.of(context).chromeFamily,
            fontWeight: FontWeight.w700,
            fontSize: 11,
            height: 1,
            letterSpacing: 11 * 0.2,
            color: LookScope.of(context).palette.inkFaint,
            shadows: [
              Shadow(offset: Offset(0, 1), color: Color(0xB3000000)),
            ],
          ),
        ),
      ],
    );
  }
}

class _PresetsButton extends StatefulWidget {
  const _PresetsButton({
    required this.presetNames,
    required this.onApplyPreset,
  });

  final List<String> presetNames;
  final ValueChanged<String> onApplyPreset;

  @override
  State<_PresetsButton> createState() => _PresetsButtonState();
}

class _PresetsButtonState extends State<_PresetsButton> {
  bool _menuOpen = false;

  Future<void> _open(BuildContext context) async {
    final box = context.findRenderObject() as RenderBox?;
    if (box == null) return;
    final look = LookScope.of(context);
    final labelStyle = TextStyle(
      color: look.palette.inkDefault,
      fontFamily: look.chromeFamily,
      fontWeight: FontWeight.w700,
      fontSize: 13,
      letterSpacing: 13 * 0.12,
    );
    setState(() => _menuOpen = true);
    String? selected;
    try {
      selected = await showMockupMenu<String>(
        context: context,
        anchor: box,
        placement: MockupMenuPlacement.below,
        color: look.palette.shellMid,
        items: [
          for (final name in widget.presetNames)
            PopupMenuItem<String>(
              value: name,
              child: Text(name, style: labelStyle),
            ),
        ],
      );
    } finally {
      if (mounted) setState(() => _menuOpen = false);
    }
    if (selected != null) widget.onApplyPreset(selected);
  }

  @override
  Widget build(BuildContext context) {
    return Builder(
      builder: (buttonContext) => MockupButton(
        key: const Key('eq-presets'),
        label: 'Presets',
        menu: true,
        on: _menuOpen,
        height: 38,
        padding: const EdgeInsets.fromLTRB(16, 0, 22, 0),
        onPressed: () => _open(buttonContext),
        semanticLabel: 'Presets',
      ),
    );
  }
}

class _EqBands extends StatelessWidget {
  const _EqBands({
    required this.settings,
    required this.onPreamp,
    required this.onBand,
  });

  /// Slightly shorter than the old full-column Expanded track (~170).
  static const trackHeight = 148.0;
  static const valueHeight = 18.0;
  static const labelHeight = 26.0;

  final EqualizerSettings settings;
  final ValueChanged<double> onPreamp;
  final void Function(int band, double gain) onBand;

  @override
  Widget build(BuildContext context) {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        const _EqScale(
          valueHeight: valueHeight,
          trackHeight: trackHeight,
          labelHeight: labelHeight,
        ),
        _BandColumn(
          key: const Key('eq-preamp'),
          width: 62,
          marginRight: 16,
          label: 'PREAMP',
          preampStyle: true,
          gain: settings.preamp,
          onChanged: onPreamp,
          semanticLabel: 'Preamp',
          valueHeight: valueHeight,
          trackHeight: trackHeight,
          labelHeight: labelHeight,
        ),
        for (var i = 0; i < EqualizerSettings.bandFrequencies.length; i++)
          _BandColumn(
            key: Key('eq-band-$i'),
            width: 50,
            label: MockupEqualizer.bandLabels[i],
            gain: settings.gains[i],
            onChanged: (gain) => onBand(i, gain),
            semanticLabel: '${MockupEqualizer.bandLabels[i]} hertz',
            valueHeight: valueHeight,
            trackHeight: trackHeight,
            labelHeight: labelHeight,
          ),
      ],
    );
  }
}

class _EqScale extends StatelessWidget {
  const _EqScale({
    required this.valueHeight,
    required this.trackHeight,
    required this.labelHeight,
  });

  final double valueHeight;
  final double trackHeight;
  final double labelHeight;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 44,
      child: Padding(
        padding: EdgeInsets.fromLTRB(0, valueHeight, 8, labelHeight),
        child: SizedBox(
          height: trackHeight,
          child: const Column(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              _ScaleLabel('+12'),
              _ScaleLabel('0'),
              _ScaleLabel('−12'),
            ],
          ),
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
      style: TextStyle(
        fontFamily: LookScope.of(context).lcdFamily,
        fontWeight: FontWeight.w500,
        fontSize: 11,
        height: 1,
        color: LookScope.of(context).palette.inkFaint,
      ),
    );
  }
}

String _formatEqGain(double gain) {
  if (gain.abs() < 0.05) return '0.0';
  final sign = gain > 0 ? '+' : '−';
  return '$sign${gain.abs().toStringAsFixed(1)}';
}

class _BandColumn extends StatelessWidget {
  const _BandColumn({
    super.key,
    required this.width,
    required this.label,
    required this.gain,
    required this.onChanged,
    required this.semanticLabel,
    required this.valueHeight,
    required this.trackHeight,
    required this.labelHeight,
    this.marginRight = 0,
    this.preampStyle = false,
  });

  final double width;
  final double marginRight;
  final String label;
  final double gain;
  final ValueChanged<double> onChanged;
  final String semanticLabel;
  final double valueHeight;
  final double trackHeight;
  final double labelHeight;
  final bool preampStyle;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    return Container(
      width: width,
      margin: EdgeInsets.only(right: marginRight),
      child: Column(
        children: [
          SizedBox(
            height: valueHeight,
            child: Center(
              child: Text(
                _formatEqGain(gain),
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontFamily: look.lcdFamily,
                  fontWeight: FontWeight.w500,
                  fontSize: 11,
                  height: 1,
                  color: look.palette.inkDefault,
                ),
              ),
            ),
          ),
          SizedBox(
            height: trackHeight,
            child: _VerticalBandSlider(
              gain: gain,
              onChanged: onChanged,
              semanticLabel: semanticLabel,
            ),
          ),
          SizedBox(
            height: labelHeight,
            child: Center(
              child: Text(
                label,
                textAlign: TextAlign.center,
                style: TextStyle(
                  fontFamily: look.chromeFamily,
                  fontWeight: FontWeight.w700,
                  fontSize: 11,
                  height: 1,
                  letterSpacing: 11 * (preampStyle ? 0.18 : 0.1),
                  color: preampStyle
                      ? const Color(0x8C3DE7FF)
                      : look.palette.inkFaint,
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
          return MockupHover(
            builder: (context, hover) {
              return GestureDetector(
                behavior: HitTestBehavior.opaque,
                onTapDown: (d) => _emit(d.localPosition.dy, height),
                onVerticalDragUpdate: (d) => _emit(d.localPosition.dy, height),
                child: Center(
                  child: SizedBox(
                    width: 34,
                    height: height,
                    child: CustomPaint(
                      painter: _VTrackPainter(
                        materials: LookScope.of(context).materials,
                        fraction: _toFraction(gain),
                        hover: hover,
                      ),
                    ),
                  ),
                ),
              );
            },
          );
        },
      ),
    );
  }
}

class _VTrackPainter extends CustomPainter {
  const _VTrackPainter({
    required this.materials,
    required this.fraction,
    required this.hover,
  });

  final LookMaterials materials;
  final double fraction;
  final double hover;

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

    // Thumb (top = +12 → fraction 1).
    final thumbY = (1.0 - fraction.clamp(0.0, 1.0)) * size.height;

    // Bottom→thumb fill; spectrum gradient in track space (cyan low → magenta high).
    final fillBottom = size.height;
    final fillTop = thumbY.clamp(0.0, size.height);
    final fillH = fillBottom - fillTop;
    if (fillH > 0.5) {
      final fillRect = Rect.fromLTWH(trackLeft, fillTop, trackW, fillH);
      canvas.save();
      canvas.clipRRect(track);
      canvas.clipRect(fillRect);
      canvas.drawRect(
        fillRect,
        Paint()
          ..color = const Color(0x663DE7FF)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3.5),
      );
      // Shader spans the full track so rising values reveal more magenta.
      canvas.drawRect(
        track.outerRect,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.bottomCenter,
            end: Alignment.topCenter,
            colors: materials.spectrumStops,
            stops: materials.spectrumStops.length == 4
                ? const [0, 0.26, 0.62, 1]
                : null,
          ).createShader(track.outerRect),
      );
      canvas.restore();
    }

    // Zero notch.
    final midY = size.height / 2;
    canvas.drawRect(
      Rect.fromLTWH(trackLeft - 13, midY - 0.5, trackW + 26, 1),
      Paint()..color = const Color(0x24E2ECFF),
    );
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
    if (hover > 0.001) {
      canvas.drawRRect(
        thumb.inflate(1.5),
        Paint()
          ..color = Color.fromRGBO(61, 231, 255, 0.35 * hover)
          ..maskFilter = const MaskFilter.blur(BlurStyle.normal, 3),
      );
    }
    canvas.drawRRect(
      thumb,
      Paint()
        ..shader = LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            mockupHoverLift(const Color(0xFF757C8F), hover),
            mockupHoverLift(const Color(0xFF3D4350), hover),
            mockupHoverLift(const Color(0xFF1E222C), hover),
          ],
          stops: const [0, 0.42, 1],
        ).createShader(thumb.outerRect),
    );
    canvas.drawRRect(
      thumb,
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = Color.lerp(
          const Color(0x59ECF4FF),
          const Color(0xCC3DE7FF),
          hover,
        )!,
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
        ..shader = LinearGradient(
          colors: [
            materials.spectrumStops.isNotEmpty
                ? materials.spectrumStops.first
                : const Color(0xFFCBF9FF),
            materials.spectrumStops.length > 1
                ? materials.spectrumStops[1]
                : const Color(0xFF3DE7FF),
          ],
        ).createShader(line.outerRect),
    );
    canvas.drawRRect(
      line,
      Paint()
        ..color = Color.fromRGBO(61, 231, 255, 0.75 + 0.25 * hover)
        ..maskFilter = MaskFilter.blur(BlurStyle.normal, 4 + 2 * hover),
    );
  }

  @override
  bool shouldRepaint(covariant _VTrackPainter oldDelegate) =>
      oldDelegate.fraction != fraction ||
      oldDelegate.hover != hover ||
      oldDelegate.materials != materials;
}
