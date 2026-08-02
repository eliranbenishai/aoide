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
          thumbAsset: GraphiteSkin.sliderThumb,
          thumbSize: const Size(7, 5),
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
        child: SkinButton(
          key: const Key('player-repeat'),
          size: const Size(76, 29),
          idleAsset: GraphiteSkin.repeatIdle,
          activeAsset: GraphiteSkin.repeatActive,
          active: playback.repeatMode != RepeatMode.off,
          onPressed: playback.cycleRepeatMode,
          semanticLabel: 'Repeat',
        ),
      ),

      // Volume rides the L/R meter chrome (that chrome is the volume slider).
      Positioned(
        left: 604.5,
        top: 96,
        width: 117,
        height: 34,
        child: SkinSlider(
          key: const Key('transport-volume'),
          axis: Axis.horizontal,
          value: playback.muted ? 0 : playback.volume.clamp(0.0, 1.0),
          trackSize: const Size(117, 34),
          thumbAsset: GraphiteSkin.sliderThumb,
          thumbSize: const Size(16, 23),
          semanticLabel: 'Volume',
          onChanged: playback.setVolume,
          onChangeEnd: playback.setVolume,
        ),
      ),
      Positioned(
        left: 724,
        top: 102,
        child: _MuteButton(playback: playback),
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
      size: const Size(30, 22),
      idleAsset: idleAsset,
      pressedAsset: pressedAsset,
      overlay: glyph,
      onPressed: onPressed,
      semanticLabel: semanticLabel,
    );
  }
}

/// Mute toggle: the blank window bezel (PNG metal) with a speaker glyph stamped
/// on top. The mockup carries no mute art, so the bezel is borrowed from the
/// title-bar buttons and only the speaker itself is code-drawn.
class _MuteButton extends StatelessWidget {
  const _MuteButton({required this.playback});

  final PlaybackController playback;

  @override
  Widget build(BuildContext context) {
    return SkinButton(
      key: const Key('transport-mute'),
      size: const Size(26, 20),
      idleAsset: GraphiteSkin.winBlankIdle,
      pressedAsset: GraphiteSkin.winBlankPressed,
      overlay: TransportIcons.speaker(
        colour: playback.muted ? TrampColors.labelDim : TrampColors.label,
        muted: playback.muted,
      ),
      onPressed: playback.toggleMute,
      semanticLabel: playback.muted ? 'Unmute' : 'Mute',
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
      width: 10,
      height: 10,
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
