import 'dart:async';

import 'package:flutter/material.dart' hide RepeatMode;
import 'package:window_manager/window_manager.dart';

import '../../domain/repeat_mode.dart';
import '../../domain/tramp_settings.dart';
import '../../playback/playback_controller.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/lcd_text.dart';
import '../chrome/spectrum_visualizer.dart';
import '../chrome/tramp_mark.dart';
import '../chrome/transport_icons.dart';
import '../format.dart';
import '../skin/graphite_skin.dart';
import '../skin/skin_button.dart';
import '../skin/skin_image.dart';
import '../skin/skin_slider.dart';
import '../zoom/zoom_controller.dart';

/// The main player panel.
///
/// The look is the graphite skin PNG ([GraphiteSkin.mainFace]); everything on
/// top is a live overlay or a hit target aligned to the art. Control positions
/// are the mockup's own bezels, so a [SkinButton] sits exactly over the face
/// glyph it drives. Coordinates are the fixed 812x242 logical canvas.
class MainPlayerPanel extends StatefulWidget {
  const MainPlayerPanel({
    super.key,
    required this.playback,
    required this.zoom,
    required this.lowerRegion,
    required this.hasTracks,
    required this.onSelectRegion,
    this.onOpenFiles,
    this.onOpenMenu,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.mainPlayer;

  final PlaybackController playback;
  final ZoomController zoom;
  final LowerRegion lowerRegion;
  final bool hasTracks;
  final ValueChanged<LowerRegion> onSelectRegion;
  final VoidCallback? onOpenFiles;
  final VoidCallback? onOpenMenu;
  final bool draggableTitle;

  @override
  State<MainPlayerPanel> createState() => _MainPlayerPanelState();
}

class _MainPlayerPanelState extends State<MainPlayerPanel> {
  PlaybackController get playback => widget.playback;

  Future<void> _play() async {
    if (!widget.hasTracks) return;
    if (playback.playing) return;
    await playback.playPause();
  }

  Future<void> _pause() async {
    if (playback.playing) await playback.playPause();
  }

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: MainPlayerPanel.logicalSize.width,
      height: MainPlayerPanel.logicalSize.height,
      child: ListenableBuilder(
        listenable: Listenable.merge([playback, widget.zoom]),
        builder: (context, _) => _buildChrome(context),
      ),
    );
  }

  Widget _buildChrome(BuildContext context) {
    const well = GraphiteSkin.mainDisplayWell;

    return Stack(
      children: [
        // Dark LCD glass shows through the face's punched display well.
        Positioned.fromRect(
          rect: well,
          child: const ColoredBox(color: TrampColors.lcdGlass),
        ),
        const SkinImage(
          asset: GraphiteSkin.mainFace,
          logicalSize: MainPlayerPanel.logicalSize,
        ),

        ..._displayWell(),
        _titleBar(),
        ..._transport(),
        ..._rightControls(),
        _open(),
        _mute(),
      ],
    );
  }

  // ---------------------------------------------------------------------------
  // Display well overlays: spectrum, seek, LCD text, region indicators.
  // ---------------------------------------------------------------------------

  List<Widget> _displayWell() {
    final track = playback.currentTrack;
    final durationMs = playback.duration.inMilliseconds;
    final seek = durationMs > 0
        ? (playback.position.inMilliseconds / durationMs).clamp(0.0, 1.0)
        : 0.0;
    final format = playback.formatInfo;
    final index = playback.playingIndex;

    final title = track == null
        ? 'No track'
        : [
            if (index != null) '${index + 1}.',
            if (track.artist != null && track.artist!.trim().isNotEmpty)
              '${track.artist!.trim()} -',
            track.displayTitle,
          ].join(' ');

    return [
      Positioned(
        left: 44,
        top: 45,
        width: 200,
        height: 96,
        child: SpectrumVisualizer(levels: playback.levelsStream),
      ),
      Positioned(
        left: 44,
        top: 150,
        width: 200,
        height: 5,
        child: SkinSlider(
          key: const Key('transport-seek'),
          axis: Axis.horizontal,
          value: seek,
          trackSize: const Size(200, 5),
          thumbAsset: GraphiteSkin.seekThumb,
          thumbSize: const Size(9, 6),
          semanticLabel: 'Seek',
          onChangeEnd: durationMs > 0
              ? (v) => unawaited(playback.seek(
                    Duration(milliseconds: (v * durationMs).round()),
                  ))
              : null,
        ),
      ),
      Positioned(
        left: 258,
        top: 45,
        width: 296,
        child: LcdText(title, lit: track != null),
      ),
      Positioned(
        left: 258,
        top: 70,
        child: Row(
          crossAxisAlignment: CrossAxisAlignment.end,
          children: [
            LcdText(formatDuration(playback.position), size: LcdSize.large),
            const SizedBox(width: 10),
            Padding(
              padding: const EdgeInsets.only(bottom: 4),
              child: Text(
                formatDuration(playback.duration),
                style: TrampText.lcd.copyWith(color: TrampColors.labelDim),
              ),
            ),
          ],
        ),
      ),
      Positioned(
        left: 258,
        top: 112,
        width: 296,
        child: Row(
          children: [
            LcdText(format.bitrateLabel),
            const SizedBox(width: 12),
            LcdText(format.sampleRateLabel),
            const SizedBox(width: 12),
            LcdText(format.channelLabel),
          ],
        ),
      ),
      Positioned(
        left: 258,
        top: 138,
        child: Row(
          children: [
            LcdIndicator(
              'EQ',
              lit: widget.lowerRegion == LowerRegion.equalizer,
              onTap: () => widget.onSelectRegion(LowerRegion.equalizer),
            ),
            const SizedBox(width: 6),
            LcdIndicator(
              'PL',
              lit: widget.lowerRegion == LowerRegion.playlist,
              onTap: () => widget.onSelectRegion(LowerRegion.playlist),
            ),
          ],
        ),
      ),
    ];
  }

  // ---------------------------------------------------------------------------
  // Title bar: mark (menu), draggable rail region, and the window controls
  // minimize / zoom- / zoom+ / close. The rails and TRAMP wordmark are face art.
  // ---------------------------------------------------------------------------

  Widget _titleBar() {
    Widget drag = const SizedBox(
      width: 610,
      height: TrampMetrics.titleBar,
    );
    if (widget.draggableTitle) drag = DragToMoveArea(child: drag);

    return Positioned(
      left: 0,
      top: 0,
      right: 0,
      height: TrampMetrics.titleBar,
      child: Stack(
        children: [
          Positioned(left: 50, top: 0, child: drag),
          Positioned(
            left: 15,
            top: 4,
            child: _MarkButton(onPressed: widget.onOpenMenu),
          ),
          Positioned(
            left: 669,
            top: 7,
            child: _WindowButton(
              buttonKey: const Key('window-minimize'),
              idleAsset: GraphiteSkin.winMinimizeIdle,
              pressedAsset: GraphiteSkin.winMinimizePressed,
              semanticLabel: 'Minimize',
              onPressed: () => unawaited(windowManager.minimize()),
            ),
          ),
          Positioned(
            left: 704,
            top: 7,
            child: _WindowButton(
              buttonKey: const Key('window-zoom-out'),
              idleAsset: GraphiteSkin.winBlankIdle,
              pressedAsset: GraphiteSkin.winBlankPressed,
              glyph: const _SignGlyph(plus: false),
              semanticLabel: 'Zoom out',
              onPressed: widget.zoom.stepDown,
            ),
          ),
          Positioned(
            left: 739,
            top: 7,
            child: _WindowButton(
              buttonKey: const Key('window-zoom-in'),
              idleAsset: GraphiteSkin.winBlankIdle,
              pressedAsset: GraphiteSkin.winBlankPressed,
              glyph: const _SignGlyph(plus: true),
              semanticLabel: 'Zoom in',
              onPressed: widget.zoom.stepUp,
            ),
          ),
          Positioned(
            left: 774,
            top: 7,
            child: _WindowButton(
              buttonKey: const Key('window-close'),
              idleAsset: GraphiteSkin.winCloseIdle,
              pressedAsset: GraphiteSkin.winClosePressed,
              semanticLabel: 'Close',
              onPressed: () => unawaited(windowManager.close()),
            ),
          ),
        ],
      ),
    );
  }

  // ---------------------------------------------------------------------------
  // Transport row: five skinned buttons over the face bezels.
  // ---------------------------------------------------------------------------

  List<Widget> _transport() {
    final track = playback.currentTrack;
    final canTransport = track != null || widget.hasTracks;
    final trackOpen = track != null;

    final specs = <(String, String, VoidCallback?)>[
      (
        'prev',
        'transport-prev',
        canTransport ? () => unawaited(playback.previous()) : null,
      ),
      ('play', 'transport-play', canTransport ? () => unawaited(_play()) : null),
      (
        'pause',
        'transport-pause',
        canTransport ? () => unawaited(_pause()) : null,
      ),
      ('stop', 'transport-stop', trackOpen ? () => unawaited(playback.stop()) : null),
      ('next', 'transport-next', canTransport ? () => unawaited(playback.next()) : null),
    ];
    const lefts = [39.5, 109.5, 179.5, 249.5, 319.5];

    return [
      for (var i = 0; i < specs.length; i++)
        Positioned(
          left: lefts[i],
          top: 181,
          child: SkinButton(
            key: Key(specs[i].$2),
            size: const Size(69, 40),
            idleAsset: GraphiteSkin.transportIdle[specs[i].$1]!,
            pressedAsset: GraphiteSkin.transportPressed[specs[i].$1],
            // The play glyph lights green while playing; every other transport
            // icon stays idle grey (the mockup's baked green is this state).
            activeAsset: specs[i].$1 == 'play'
                ? GraphiteSkin.transportPlayActive
                : null,
            active: specs[i].$1 == 'play' && playback.playing,
            onPressed: specs[i].$3,
            semanticLabel: specs[i].$2,
          ),
        ),
    ];
  }

  // ---------------------------------------------------------------------------
  // Right block: shuffle / repeat toggles, volume + mute, EQ / PL buttons.
  // ---------------------------------------------------------------------------

  List<Widget> _rightControls() {
    return [
      Positioned(
        left: 648.5,
        top: 49,
        child: SkinButton(
          key: const Key('player-shuffle'),
          size: const Size(76, 29),
          idleAsset: GraphiteSkin.shuffleIdle,
          activeAsset: GraphiteSkin.shuffleActive,
          active: playback.shuffle,
          onPressed: playback.toggleShuffle,
          semanticLabel: 'Shuffle',
        ),
      ),
      Positioned(
        left: 728.5,
        top: 49,
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            SkinButton(
              key: const Key('player-repeat'),
              size: const Size(76, 29),
              idleAsset: GraphiteSkin.repeatIdle,
              activeAsset: GraphiteSkin.repeatActive,
              active: playback.repeatMode != RepeatMode.off,
              onPressed: playback.cycleRepeatMode,
              semanticLabel: 'Repeat',
            ),
            // Repeat-one gets a small lit "1" badge over the arrows so the
            // three repeat states (off / all / one) read apart at a glance.
            if (playback.repeatMode == RepeatMode.one)
              const Positioned(
                right: 6,
                bottom: 3,
                child: _RepeatOneBadge(),
              ),
          ],
        ),
      ),

      // The L/R meter chrome is the volume slider: two live segmented phosphor
      // bars fill to the volume (both to 0 when muted), a metal grip rides the
      // gap between them, and a horizontal drag sets the volume.
      Positioned(
        left: 603,
        top: 98,
        width: 119,
        height: 32,
        child: _VolumeMeters(playback: playback),
      ),

      Positioned(
        left: 593.5,
        top: 147.5,
        child: SkinButton(
          key: const Key('player-eq'),
          size: const Size(57, 20),
          idleAsset: GraphiteSkin.eqIdle,
          activeAsset: GraphiteSkin.eqActive,
          active: widget.lowerRegion == LowerRegion.equalizer,
          onPressed: () => widget.onSelectRegion(LowerRegion.equalizer),
          semanticLabel: 'Equalizer',
        ),
      ),
      Positioned(
        left: 651.5,
        top: 147.5,
        child: SkinButton(
          key: const Key('player-pl'),
          size: const Size(57, 20),
          idleAsset: GraphiteSkin.plIdle,
          activeAsset: GraphiteSkin.plActive,
          active: widget.lowerRegion == LowerRegion.playlist,
          onPressed: () => widget.onSelectRegion(LowerRegion.playlist),
          semanticLabel: 'Playlist',
        ),
      ),
    ];
  }

  Widget _open() {
    return Positioned(
      left: 588,
      top: 191,
      child: _HitTarget(
        buttonKey: const Key('player-open'),
        size: const Size(148, 28),
        semanticLabel: 'Open files',
        onPressed: widget.onOpenFiles,
      ),
    );
  }

  /// Mute rides the small bezel right of OPEN (the mockup's old bolt slot) as a
  /// practical control: a speaker glyph over the face's own metal bezel, with a
  /// slash when muted. Kept off the L/R meters so those read like the mockup.
  Widget _mute() {
    return Positioned(
      left: 734,
      top: 192,
      child: Semantics(
        button: true,
        label: playback.muted ? 'Unmute' : 'Mute',
        child: MouseRegion(
          cursor: SystemMouseCursors.click,
          child: GestureDetector(
            key: const Key('transport-mute'),
            behavior: HitTestBehavior.opaque,
            onTap: playback.toggleMute,
            child: SizedBox(
              width: 66,
              height: 26,
              child: Center(
                child: TransportIcons.speaker(
                  colour:
                      playback.muted ? TrampColors.labelDim : TrampColors.label,
                  muted: playback.muted,
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}

/// Transparent tap area over a control the face already paints (e.g. OPEN).
class _HitTarget extends StatelessWidget {
  const _HitTarget({
    required this.buttonKey,
    required this.size,
    required this.semanticLabel,
    required this.onPressed,
  });

  final Key buttonKey;
  final Size size;
  final String semanticLabel;
  final VoidCallback? onPressed;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      enabled: onPressed != null,
      label: semanticLabel,
      child: MouseRegion(
        cursor: onPressed != null
            ? SystemMouseCursors.click
            : SystemMouseCursors.basic,
        child: GestureDetector(
          key: buttonKey,
          behavior: HitTestBehavior.opaque,
          onTap: onPressed,
          child: SizedBox.fromSize(size: size),
        ),
      ),
    );
  }
}

/// The Tramp mark in the title bar's leading slot; opens the app menu.
class _MarkButton extends StatelessWidget {
  const _MarkButton({required this.onPressed});

  final VoidCallback? onPressed;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      label: 'Tramp menu',
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          key: const Key('player-menu'),
          behavior: HitTestBehavior.opaque,
          onTap: onPressed,
          child: const SizedBox(
            width: 30,
            height: 26,
            child: Center(child: TrampMark(size: 19)),
          ),
        ),
      ),
    );
  }
}

/// A title-bar window control: a PNG metal bezel with an optional glyph.
///
/// The bezel is real mockup art ([SkinButton] over a cropped sprite), never a
/// code-painted bevel. Minimize and close carry their mockup glyphs baked into
/// the sprite; zoom- / zoom+ ride the blank bezel and stamp a code [_SignGlyph]
/// on top, since the mockup paints no zoom art (it had a maximize square that
/// Tramp does not use).
class _WindowButton extends StatelessWidget {
  const _WindowButton({
    required this.buttonKey,
    required this.idleAsset,
    required this.pressedAsset,
    required this.semanticLabel,
    required this.onPressed,
    this.glyph,
  });

  final Key buttonKey;
  final String idleAsset;
  final String pressedAsset;
  final String semanticLabel;
  final VoidCallback? onPressed;
  final Widget? glyph;

  @override
  Widget build(BuildContext context) {
    return SkinButton(
      key: buttonKey,
      size: const Size(32, 24),
      idleAsset: idleAsset,
      pressedAsset: pressedAsset,
      overlay: glyph,
      onPressed: onPressed,
      semanticLabel: semanticLabel,
    );
  }
}

/// The live L/R volume meters: two segmented phosphor bars that fill to the
/// volume (both to 0 when muted), a metal grip riding the gap between them, and
/// a horizontal drag that sets the volume. The baked green meter fills were
/// cleared from the face (`slice_mockup.py`), leaving the recessed wells this
/// paints into.
class _VolumeMeters extends StatelessWidget {
  const _VolumeMeters({required this.playback});

  final PlaybackController playback;

  // Bar geometry within the 119 x 32 box, measured from the cleared wells.
  static const double _barLeft = 3;
  static const double _barWidth = 113;
  static const double _lTop = 1;
  static const double _rTop = 24.5;
  static const double _barHeight = 7;
  static const Size _thumb = Size(10, 12);

  double _fractionFor(double dx) =>
      ((dx - _barLeft) / _barWidth).clamp(0.0, 1.0);

  @override
  Widget build(BuildContext context) {
    final level = playback.muted ? 0.0 : playback.volume.clamp(0.0, 1.0);
    final thumbX =
        _barLeft + level * (_barWidth - _thumb.width) + _thumb.width / 2;

    return Semantics(
      slider: true,
      label: 'Volume',
      value: '${(level * 100).round()}%',
      child: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onTapDown: (d) => playback.setVolume(_fractionFor(d.localPosition.dx)),
        onHorizontalDragUpdate: (d) =>
            playback.setVolume(_fractionFor(d.localPosition.dx)),
        child: Stack(
          clipBehavior: Clip.none,
          children: [
            CustomPaint(
              size: const Size(119, 32),
              painter: _MeterPainter(level),
            ),
            Positioned(
              left: thumbX - _thumb.width / 2,
              top: (32 - _thumb.height) / 2,
              child: const SkinImage(
                asset: GraphiteSkin.volumeThumb,
                logicalSize: _thumb,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// Paints the two meter bars as segmented phosphor: lit cells up to [level],
/// faint cells beyond, mirroring the mockup's L/R VU segments.
class _MeterPainter extends CustomPainter {
  const _MeterPainter(this.level);

  final double level;

  static const double _cell = 4.3;
  static const double _seg = 3.1;

  @override
  void paint(Canvas canvas, Size size) {
    _bar(canvas, _VolumeMeters._lTop);
    _bar(canvas, _VolumeMeters._rTop);
  }

  void _bar(Canvas canvas, double top) {
    const left = _VolumeMeters._barLeft;
    const width = _VolumeMeters._barWidth;
    final count = (width / _cell).floor();
    final litUntil = (level * count).round();
    final lit = Paint()..color = TrampColors.phosphor;
    final dim = Paint()..color = TrampColors.phosphorDim.withValues(alpha: 0.45);
    for (var i = 0; i < count; i++) {
      final x = left + i * _cell;
      canvas.drawRect(
        Rect.fromLTWH(x, top, _seg, _VolumeMeters._barHeight),
        i < litUntil ? lit : dim,
      );
    }
  }

  @override
  bool shouldRepaint(_MeterPainter old) => old.level != level;
}

/// A small lit "1" badge marking [RepeatMode.one] on the repeat button.
class _RepeatOneBadge extends StatelessWidget {
  const _RepeatOneBadge();

  @override
  Widget build(BuildContext context) {
    return Text(
      '1',
      style: TrampText.lcd.copyWith(
        color: TrampColors.phosphor,
        fontSize: 11,
        height: 1,
        fontWeight: FontWeight.w700,
      ),
    );
  }
}

/// Minus / plus glyph for the zoom-out / zoom-in window controls.
class _SignGlyph extends StatelessWidget {
  const _SignGlyph({required this.plus});

  final bool plus;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 12,
      height: 12,
      child: CustomPaint(painter: _SignPainter(plus: plus)),
    );
  }
}

class _SignPainter extends CustomPainter {
  const _SignPainter({required this.plus});

  final bool plus;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = TrampColors.label
      ..strokeWidth = 1.6
      ..strokeCap = StrokeCap.square;
    final cy = size.height / 2;
    final cx = size.width / 2;
    canvas.drawLine(Offset(size.width * 0.1, cy), Offset(size.width * 0.9, cy),
        paint);
    if (plus) {
      canvas.drawLine(Offset(cx, size.height * 0.1),
          Offset(cx, size.height * 0.9), paint);
    }
  }

  @override
  bool shouldRepaint(_SignPainter old) => old.plus != plus;
}
