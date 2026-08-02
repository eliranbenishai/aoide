import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/equalizer_settings.dart';
import '../../eq/equalizer_controller.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../skin/graphite_skin.dart';
import '../skin/skin_button.dart';
import '../skin/skin_image.dart';
import '../skin/skin_slider.dart';

/// The ten-band equalizer panel.
///
/// The look is the graphite skin PNG ([GraphiteSkin.equalizerFace]); the baked
/// mockup thumbs, LED fills and gain numbers were cleaned from that art (see
/// `.scratch/graphite-skin/slice_mockup.py`) so the live overlays below own
/// them: a phosphor fill and a [SkinSlider] fader ride each groove, and the
/// gain readouts are code text. Coordinates are the fixed 812x206 logical
/// canvas, measured from the face's own grooves. Collapsed, the panel is the
/// [GraphiteSkin.equalizerShadeFace] title strip.
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

  /// Logical x of each band groove's centre (measured from the face).
  static const List<double> bandCentres = [
    186.5, 235.5, 284.5, 333.5, 382.5, 431, 480, 529, 578, 627,
  ];

  static const List<String> bandLabels = [
    '60', '170', '310', '600', '1K', '3K', '6K', '12K', '14K', '16K',
  ];

  static const double _preampCentre = 63.5;

  // Band faders share one vertical scale so a flat curve reads level; the
  // preamp is shorter but its 0 dB lands on the same row (114.75).
  static const double _bandTrackTop = 57;
  static const double _bandTrackHeight = 115.5;
  static const double _bandFillBottom = 182;
  static const double _preampTrackTop = 69.5;
  static const double _preampTrackHeight = 90.5;
  static const double _preampFillBottom = 160;

  static const Size _thumbSize = Size(34, 23);
  static const double _fillWidth = 5;

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

  static String _format(double gain) {
    final value = double.parse(gain.toStringAsFixed(1));
    if (value == 0) return '0.0';
    return '${value > 0 ? '+' : '-'}${value.abs().toStringAsFixed(1)}';
  }

  @override
  Widget build(BuildContext context) {
    if (collapsed) {
      return SizedBox(
        width: logicalSize.width,
        height: TrampMetrics.titleBar,
        child: Stack(
          children: [
            const SkinImage(
              asset: GraphiteSkin.equalizerShadeFace,
              // logicalSize.width (812) x the title-bar height.
              logicalSize: Size(812, TrampMetrics.titleBar),
            ),
            ..._titleControls(),
          ],
        ),
      );
    }

    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: ListenableBuilder(
        listenable: controller,
        builder: (context, _) {
          final settings = controller.settings;

          return Stack(
            children: [
              const SkinImage(
                asset: GraphiteSkin.equalizerFace,
                logicalSize: logicalSize,
              ),

              ..._titleControls(),

              Positioned(
                left: 31,
                top: 42,
                child: SkinButton(
                  key: const Key('eq-on'),
                  size: const Size(43, 18),
                  idleAsset: GraphiteSkin.eqOnIdle,
                  activeAsset: GraphiteSkin.eqOnActive,
                  active: settings.enabled,
                  onPressed: () => controller.setEnabled(!settings.enabled),
                  semanticLabel: 'Equalizer on',
                ),
              ),
              Positioned(
                left: 78.5,
                top: 42,
                child: SkinButton(
                  key: const Key('eq-auto'),
                  size: const Size(45, 18),
                  idleAsset: GraphiteSkin.eqAutoIdle,
                  activeAsset: GraphiteSkin.eqAutoActive,
                  active: settings.auto,
                  onPressed: () => controller.setAuto(!settings.auto),
                  semanticLabel: 'Auto preamp',
                ),
              ),
              Positioned(
                left: 666.5,
                top: 48,
                child: _PresetsButton(controller: controller),
              ),

              ..._fader(
                key: const Key('eq-preamp'),
                centreX: _preampCentre,
                trackTop: _preampTrackTop,
                trackHeight: _preampTrackHeight,
                fillBottom: _preampFillBottom,
                gain: settings.preamp,
                onChanged: (f) => controller.setPreamp(_toGain(f)),
                semanticLabel: 'Preamp',
              ),
              for (var i = 0; i < bandCentres.length; i++)
                ..._fader(
                  key: Key('eq-band-$i'),
                  centreX: bandCentres[i],
                  trackTop: _bandTrackTop,
                  trackHeight: _bandTrackHeight,
                  fillBottom: _bandFillBottom,
                  gain: settings.gains[i],
                  onChanged: (f) => controller.setGain(i, _toGain(f)),
                  semanticLabel: '${bandLabels[i]} hertz',
                ),
            ],
          );
        },
      ),
    );
  }

  /// The title-bar collapse / close controls and the draggable rail, shared by
  /// the full panel and the collapsed windowshade.
  List<Widget> _titleControls() {
    Widget drag = const SizedBox(width: 700, height: TrampMetrics.titleBar);
    if (draggableTitle) drag = DragToMoveArea(child: drag);

    return [
      Positioned(left: 50, top: 0, child: drag),
      Positioned(
        left: 7,
        top: 5,
        child: SkinButton(
          key: const Key('eq-collapse'),
          size: const Size(39, 22),
          idleAsset: GraphiteSkin.eqCollapseIdle,
          pressedAsset: GraphiteSkin.eqCollapsePressed,
          onPressed: onCollapse,
          semanticLabel: 'Collapse equalizer',
        ),
      ),
      Positioned(
        left: 770,
        top: 5,
        child: SkinButton(
          key: const Key('eq-close'),
          size: const Size(38, 22),
          idleAsset: GraphiteSkin.eqCloseIdle,
          pressedAsset: GraphiteSkin.eqClosePressed,
          onPressed: onClose,
          semanticLabel: 'Close equalizer',
        ),
      ),
    ];
  }

  /// A single fader: a phosphor fill from the groove bottom up to the grip, the
  /// [SkinSlider] grip on top, and the gain readout below.
  List<Widget> _fader({
    required Key key,
    required double centreX,
    required double trackTop,
    required double trackHeight,
    required double fillBottom,
    required double gain,
    required ValueChanged<double> onChanged,
    required String semanticLabel,
  }) {
    final value = _toFraction(gain);
    final thumbCentreY =
        trackTop + (1 - value) * (trackHeight - _thumbSize.height) +
            _thumbSize.height / 2;

    return [
      Positioned(
        left: centreX - _fillWidth / 2,
        top: thumbCentreY,
        width: _fillWidth,
        height: (fillBottom - thumbCentreY).clamp(0.0, double.infinity),
        child: const _EqFill(),
      ),
      Positioned(
        left: centreX - _thumbSize.width / 2,
        top: trackTop,
        width: _thumbSize.width,
        height: trackHeight,
        child: SkinSlider(
          key: key,
          axis: Axis.vertical,
          value: value,
          trackSize: Size(_thumbSize.width, trackHeight),
          thumbAsset: GraphiteSkin.eqThumb,
          thumbSize: _thumbSize,
          semanticLabel: semanticLabel,
          onChanged: onChanged,
          onChangeEnd: onChanged,
        ),
      ),
      Positioned(
        left: centreX - 24,
        top: 176,
        width: 48,
        child: Text(
          _format(gain),
          textAlign: TextAlign.center,
          style: TrampText.eqScale.copyWith(color: TrampColors.phosphor),
        ),
      ),
    ];
  }
}

/// The lit segmented column below a fader grip: phosphor LED dashes painted
/// from the groove bottom up, mirroring the mockup's fader fill.
class _EqFill extends StatelessWidget {
  const _EqFill();

  @override
  Widget build(BuildContext context) {
    return const CustomPaint(size: Size.infinite, painter: _FillPainter());
  }
}

class _FillPainter extends CustomPainter {
  const _FillPainter();

  @override
  void paint(Canvas canvas, Size size) {
    const segment = 1.8;
    const gap = 0.9;
    final paint = Paint()..color = TrampColors.phosphor;
    for (var y = size.height - segment; y >= 0; y -= segment + gap) {
      canvas.drawRect(Rect.fromLTWH(0, y, size.width, segment), paint);
    }
  }

  @override
  bool shouldRepaint(_FillPainter oldDelegate) => false;
}

class _PresetsButton extends StatelessWidget {
  const _PresetsButton({required this.controller});

  final EqualizerController controller;

  @override
  Widget build(BuildContext context) {
    return SkinButton(
      key: const Key('eq-presets'),
      size: const Size(113, 17),
      idleAsset: GraphiteSkin.eqPresetsIdle,
      pressedAsset: GraphiteSkin.eqPresetsPressed,
      semanticLabel: 'Presets',
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
