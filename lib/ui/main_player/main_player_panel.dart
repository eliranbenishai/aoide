import 'dart:async';

import 'package:flutter/material.dart' hide RepeatMode;
import 'package:window_manager/window_manager.dart';

import '../../domain/repeat_mode.dart';
import '../../domain/tramp_settings.dart';
import '../../playback/playback_controller.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/chrome_button.dart';
import '../chrome/chrome_slider.dart';
import '../chrome/lcd_text.dart';
import '../chrome/metal_panel.dart';
import '../chrome/spectrum_visualizer.dart';
import '../chrome/title_bar.dart';
import '../chrome/tramp_mark.dart';
import '../chrome/transport_icons.dart';
import '../format.dart';
import '../zoom/zoom_controller.dart';

/// The main player panel.
///
/// Authored against the fixed 812x242 canvas with absolute positions from the
/// design spec's geometry table.
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
  double? _volumePreview;

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
      child: MetalPanel(
        surface: TrampSurface.raisedPanel,
        child: ListenableBuilder(
          listenable: Listenable.merge([playback, widget.zoom]),
          builder: (context, _) => _buildChrome(context),
        ),
      ),
    );
  }

  Widget _buildChrome(BuildContext context) {
    final track = playback.currentTrack;
    final durationMs = playback.duration.inMilliseconds;
    final seek = durationMs > 0
        ? (playback.position.inMilliseconds / durationMs).clamp(0.0, 1.0)
        : 0.0;
    final volume = (_volumePreview ?? playback.volume).clamp(0.0, 1.0);
    final canTransport = track != null || widget.hasTracks;
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

    return Stack(
      children: [
        Positioned(
          left: 0,
          right: 0,
          top: 0,
          child: TrampTitleBar(
            title: 'TRAMP',
            draggable: widget.draggableTitle,
            // Tramp's compact mark. The mockup shows Winamp's lightning bolt
            // here — that is their brand, not a generic icon. The full colour
            // logo is illegible at this size; see the spec's Brand assets.
            leading: ChromeButton.icon(
              key: const Key('player-menu'),
              icon: const TrampMark(size: 19),
              onPressed: widget.onOpenMenu,
              semanticLabel: 'Tramp menu',
              size: const Size(27, 27),
            ),
            trailing: [
              ChromeButton.icon(
                key: const Key('window-minimize'),
                icon: TransportIcons.minimize(),
                onPressed: () => unawaited(windowManager.minimize()),
                semanticLabel: 'Minimize',
                size: const Size(27, 22),
              ),
              ChromeButton.icon(
                key: const Key('window-maximize'),
                icon: TransportIcons.maximize(),
                onPressed: () => unawaited(_toggleMaximize()),
                semanticLabel: 'Maximize',
                size: const Size(27, 22),
              ),
              ChromeButton.icon(
                key: const Key('window-close'),
                icon: TransportIcons.close(),
                onPressed: () => unawaited(windowManager.close()),
                semanticLabel: 'Close',
                size: const Size(27, 22),
              ),
            ],
          ),
        ),

        // Display well.
        Positioned(
          left: 41,
          top: 41,
          width: 527,
          height: 137,
          child: MetalPanel(
            surface: TrampSurface.lcdGlass,
            child: Stack(
              children: [
                Positioned(
                  left: 6,
                  top: 6,
                  width: 227,
                  height: 105,
                  child: SpectrumVisualizer(levels: playback.levelsStream),
                ),
                Positioned(
                  left: 6,
                  top: 117,
                  width: 227,
                  height: 4,
                  child: ChromeSlider(
                    key: const Key('transport-seek'),
                    value: seek,
                    axis: Axis.horizontal,
                    thumbExtent: 4,
                    thumbThickness: 3,
                    semanticLabel: 'Seek',
                    onChangeEnd: durationMs > 0
                        ? (v) => unawaited(playback.seek(
                              Duration(milliseconds: (v * durationMs).round()),
                            ))
                        : null,
                  ),
                ),
                Positioned(
                  left: 245,
                  top: 8,
                  width: 274,
                  child: LcdText(title, lit: track != null),
                ),
                Positioned(
                  left: 245,
                  top: 34,
                  child: Row(
                    crossAxisAlignment: CrossAxisAlignment.end,
                    children: [
                      LcdText(
                        formatDuration(playback.position),
                        size: LcdSize.large,
                      ),
                      const SizedBox(width: 10),
                      Padding(
                        padding: const EdgeInsets.only(bottom: 4),
                        child: Text(
                          formatDuration(playback.duration),
                          style: TrampText.lcd
                              .copyWith(color: TrampColors.labelDim),
                        ),
                      ),
                    ],
                  ),
                ),
                Positioned(
                  left: 245,
                  top: 74,
                  width: 274,
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
                  left: 245,
                  top: 100,
                  child: Row(
                    children: [
                      LcdIndicator(
                        'EQ',
                        lit: widget.lowerRegion == LowerRegion.equalizer,
                        onTap: () =>
                            widget.onSelectRegion(LowerRegion.equalizer),
                      ),
                      const SizedBox(width: 6),
                      LcdIndicator(
                        'PL',
                        lit: widget.lowerRegion == LowerRegion.playlist,
                        onTap: () =>
                            widget.onSelectRegion(LowerRegion.playlist),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ),

        // Right block: shuffle, repeat, volume, region buttons.
        Positioned(
          left: 665,
          top: 52,
          child: ChromeButton.icon(
            key: const Key('player-shuffle'),
            icon: TransportIcons.shuffle(
              colour: playback.shuffle
                  ? TrampColors.phosphor
                  : TrampColors.label,
            ),
            active: playback.shuffle,
            onPressed: playback.toggleShuffle,
            semanticLabel: 'Shuffle',
            size: const Size(52, 26),
          ),
        ),
        Positioned(
          left: 727,
          top: 52,
          child: ChromeButton.icon(
            key: const Key('player-repeat'),
            icon: TransportIcons.repeat(
              colour: playback.repeatMode == RepeatMode.off
                  ? TrampColors.label
                  : TrampColors.phosphor,
              one: playback.repeatMode == RepeatMode.one,
            ),
            active: playback.repeatMode != RepeatMode.off,
            onPressed: playback.cycleRepeatMode,
            semanticLabel: 'Repeat',
            size: const Size(52, 26),
          ),
        ),
        Positioned(
          left: 607,
          top: 106,
          width: 148,
          height: 16,
          child: ChromeSlider(
            key: const Key('transport-volume'),
            value: playback.muted ? 0 : volume,
            axis: Axis.horizontal,
            dimmed: playback.muted,
            semanticLabel: 'Volume',
            onChanged: (v) {
              setState(() => _volumePreview = v);
              playback.setVolume(v);
            },
            onChangeEnd: (_) => setState(() => _volumePreview = null),
          ),
        ),
        Positioned(
          left: 759,
          top: 104,
          child: ChromeButton.icon(
            key: const Key('transport-mute'),
            icon: TransportIcons.speaker(
              colour: playback.muted
                  ? TrampColors.labelDim
                  : TrampColors.label,
              muted: playback.muted,
            ),
            onPressed: playback.toggleMute,
            semanticLabel: playback.muted ? 'Unmute' : 'Mute',
            size: const Size(20, 20),
          ),
        ),
        Positioned(
          left: 610,
          top: 149,
          child: ChromeButton.label(
            key: const Key('player-eq'),
            text: 'EQ',
            active: widget.lowerRegion == LowerRegion.equalizer,
            onPressed: () => widget.onSelectRegion(LowerRegion.equalizer),
            size: const Size(52, 26),
          ),
        ),
        Positioned(
          left: 663,
          top: 149,
          child: ChromeButton.label(
            key: const Key('player-pl'),
            text: 'PL',
            active: widget.lowerRegion == LowerRegion.playlist,
            onPressed: () => widget.onSelectRegion(LowerRegion.playlist),
            size: const Size(52, 26),
          ),
        ),

        // Transport row.
        for (final button in _transportButtons(canTransport, track != null))
          button,

        Positioned(
          left: 609,
          top: 194,
          child: _ZoomButton(zoom: widget.zoom),
        ),
        Positioned(
          left: 726,
          top: 194,
          child: ChromeButton.label(
            key: const Key('player-open'),
            text: 'OPEN',
            leading: TransportIcons.eject(),
            onPressed: widget.onOpenFiles,
            size: const Size(54, 26),
          ),
        ),
      ],
    );
  }

  List<Widget> _transportButtons(bool canTransport, bool trackOpen) {
    final specs = <(String, Widget, VoidCallback?)>[
      (
        'transport-prev',
        TransportIcons.prev(),
        canTransport ? () => unawaited(playback.previous()) : null,
      ),
      (
        'transport-play',
        TransportIcons.play(),
        canTransport ? () => unawaited(_play()) : null,
      ),
      (
        'transport-pause',
        TransportIcons.pause(),
        canTransport ? () => unawaited(_pause()) : null,
      ),
      (
        'transport-stop',
        TransportIcons.stop(),
        trackOpen ? () => unawaited(playback.stop()) : null,
      ),
      (
        'transport-next',
        TransportIcons.next(),
        canTransport ? () => unawaited(playback.next()) : null,
      ),
    ];

    const lefts = [43.0, 116.0, 190.0, 264.0, 338.0];

    return [
      for (var i = 0; i < specs.length; i++)
        Positioned(
          left: lefts[i],
          top: 182,
          child: ChromeButton.icon(
            key: Key(specs[i].$1),
            icon: specs[i].$2,
            onPressed: specs[i].$3,
            semanticLabel: specs[i].$1,
            size: const Size(69, 40),
          ),
        ),
    ];
  }

  Future<void> _toggleMaximize() async {
    if (await windowManager.isMaximized()) {
      await windowManager.unmaximize();
    } else {
      await windowManager.maximize();
    }
  }
}

/// Shows the current step and offers the ones the display can host.
class _ZoomButton extends StatelessWidget {
  const _ZoomButton({required this.zoom});

  final ZoomController zoom;

  @override
  Widget build(BuildContext context) {
    return ChromeButton.dropdown(
      key: const Key('player-zoom'),
      text: 'ZOOM ${zoom.percent}%',
      size: const Size(108, 26),
      semanticLabel: 'Zoom level',
      onPressed: () async {
        final box = context.findRenderObject()! as RenderBox;
        final origin = box.localToGlobal(Offset.zero);
        final chosen = await showMenu<int>(
          context: context,
          position: RelativeRect.fromLTRB(
            origin.dx,
            origin.dy + box.size.height,
            origin.dx,
            origin.dy,
          ),
          items: [
            for (final step in zoom.enabledSteps)
              PopupMenuItem<int>(value: step, child: Text('$step%')),
          ],
        );
        if (chosen != null) zoom.setPercent(chosen);
      },
    );
  }
}
