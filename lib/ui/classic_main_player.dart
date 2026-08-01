import 'dart:async';

import 'package:flutter/material.dart' hide RepeatMode;
import 'package:window_manager/window_manager.dart';

import '../domain/repeat_mode.dart';
import '../playback/playback_controller.dart';
import '../theme/tramp_colors.dart';
import 'chrome/chrome_button.dart';
import 'chrome/chrome_slider.dart';
import 'chrome/metal_panel.dart';
import 'chrome/spectrum_visualizer.dart';
import 'chrome/tramp_logo.dart';
import 'chrome/transport_icons.dart';
import 'format.dart';

export 'format.dart' show formatDuration;

/// Fixed-aspect classic main player chrome, scaled by the shell via [FittedBox].
class ClassicMainPlayer extends StatefulWidget {
  const ClassicMainPlayer({
    super.key,
    required this.playback,
    required this.hasTracks,
    this.playlistFocusNode,
    this.onFocusPlaylist,
  });

  static const logicalSize = Size(550, 232);

  final PlaybackController playback;
  final bool hasTracks;
  final FocusNode? playlistFocusNode;
  final VoidCallback? onFocusPlaylist;

  @override
  State<ClassicMainPlayer> createState() => _ClassicMainPlayerState();
}

class _ClassicMainPlayerState extends State<ClassicMainPlayer> {
  double? _volumeFraction;

  PlaybackController get playback => widget.playback;

  Future<void> _play() async {
    if (!widget.hasTracks) return;
    if (playback.playing) return;
    await playback.playPause();
  }

  Future<void> _pause() async {
    if (playback.playing) {
      await playback.playPause();
    }
  }

  @override
  Widget build(BuildContext context) {
    final focusNode = widget.playlistFocusNode;

    return SizedBox(
      width: ClassicMainPlayer.logicalSize.width,
      height: ClassicMainPlayer.logicalSize.height,
      child: MetalPanel(
        style: MetalPanelStyle.raised,
        child: Padding(
          padding: const EdgeInsets.all(6),
          child: ListenableBuilder(
            listenable: playback,
            builder: (context, _) {
              Widget buildChrome({required bool playlistActive}) {
                final track = playback.currentTrack;
                final duration = playback.duration;
                final durationMs = duration.inMilliseconds;
                final positionFraction = durationMs > 0
                    ? playback.position.inMilliseconds / durationMs
                    : 0.0;
                final seekValue = positionFraction.clamp(0.0, 1.0);
                final volumeValue =
                    (_volumeFraction ?? playback.volume).clamp(0.0, 1.0);
                final canTransport = track != null || widget.hasTracks;
                final titleLine = track == null
                    ? 'No track'
                    : [
                        if (track.artist != null &&
                            track.artist!.trim().isNotEmpty)
                          track.artist!.trim(),
                        track.displayTitle,
                      ].join(' - ');

                return Column(
                  children: [
                    const _TitleBar(),
                    const SizedBox(height: 6),
                    Expanded(
                      child: _LcdRow(
                        positionLabel: formatDuration(playback.position),
                        titleLine: titleLine,
                        trackOpen: track != null,
                        playing: playback.playing,
                        volume: playback.muted ? 0 : playback.volume,
                        shuffle: playback.shuffle,
                        repeatMode: playback.repeatMode,
                        playlistActive: playlistActive,
                        onToggleShuffle: playback.toggleShuffle,
                        onCycleRepeat: playback.cycleRepeatMode,
                        onFocusPlaylist: widget.onFocusPlaylist,
                      ),
                    ),
                    const SizedBox(height: 6),
                    SizedBox(
                      height: 22,
                      child: ChromeSlider(
                        key: const Key('transport-seek'),
                        value: seekValue,
                        onChangeEnd: durationMs > 0
                            ? (value) {
                                unawaited(
                                  playback.seek(
                                    Duration(
                                      milliseconds:
                                          (value * durationMs).round(),
                                    ),
                                  ),
                                );
                              }
                            : null,
                      ),
                    ),
                    const SizedBox(height: 6),
                    _TransportRow(
                      volume: volumeValue,
                      muted: playback.muted,
                      onPrevious: canTransport
                          ? () => unawaited(playback.previous())
                          : null,
                      onPlay:
                          canTransport ? () => unawaited(_play()) : null,
                      onPause:
                          canTransport ? () => unawaited(_pause()) : null,
                      onStop: track != null
                          ? () => unawaited(playback.stop())
                          : null,
                      onNext: canTransport
                          ? () => unawaited(playback.next())
                          : null,
                      onVolumeChanged: (value) {
                        setState(() => _volumeFraction = value);
                        playback.setVolume(value);
                      },
                      onVolumeChangeEnd: (_) =>
                          setState(() => _volumeFraction = null),
                      onToggleMute: playback.toggleMute,
                    ),
                  ],
                );
              }

              if (focusNode == null) {
                return buildChrome(playlistActive: widget.hasTracks);
              }

              return ListenableBuilder(
                listenable: focusNode,
                builder: (context, _) {
                  return buildChrome(
                    playlistActive:
                        widget.hasTracks || focusNode.hasFocus,
                  );
                },
              );
            },
          ),
        ),
      ),
    );
  }
}

class _TitleBar extends StatelessWidget {
  const _TitleBar();

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 28,
      child: Row(
        children: [
          const TrampLogo(size: 24),
          const SizedBox(width: 6),
          Expanded(
            child: DragToMoveArea(
              child: const Align(
                alignment: Alignment.centerLeft,
                child: Text(
                  'TRAMP',
                  style: TextStyle(
                    color: TrampColors.metalDeep,
                    fontSize: 16,
                    fontWeight: FontWeight.w800,
                    letterSpacing: 1.2,
                    height: 1,
                  ),
                ),
              ),
            ),
          ),
          _WindowControlButton(
            label: 'Minimize',
            color: TrampColors.metalShadow,
            onPressed: windowManager.minimize,
          ),
          const SizedBox(width: 6),
          _WindowControlButton(
            label: 'Close',
            color: TrampColors.windowClose,
            onPressed: windowManager.close,
          ),
        ],
      ),
    );
  }
}

class _WindowControlButton extends StatelessWidget {
  const _WindowControlButton({
    required this.label,
    required this.color,
    required this.onPressed,
  });

  final String label;
  final Color color;
  final Future<void> Function() onPressed;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      button: true,
      label: label,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          onTap: () => onPressed(),
          child: SizedBox(
            width: 12,
            height: 12,
            child: ColoredBox(color: color),
          ),
        ),
      ),
    );
  }
}

class _LcdRow extends StatelessWidget {
  const _LcdRow({
    required this.positionLabel,
    required this.titleLine,
    required this.trackOpen,
    required this.playing,
    required this.volume,
    required this.shuffle,
    required this.repeatMode,
    required this.playlistActive,
    required this.onToggleShuffle,
    required this.onCycleRepeat,
    this.onFocusPlaylist,
  });

  final String positionLabel;
  final String titleLine;
  final bool trackOpen;
  final bool playing;
  final double volume;
  final bool shuffle;
  final RepeatMode repeatMode;
  final bool playlistActive;
  final VoidCallback onToggleShuffle;
  final VoidCallback onCycleRepeat;
  final VoidCallback? onFocusPlaylist;

  static const _lcdText = TextStyle(
    color: TrampColors.lcdPhosphor,
    fontSize: 12,
    fontWeight: FontWeight.w600,
    fontFamily: 'IBMPlexMono',
    height: 1.1,
  );

  static const _lcdDim = TextStyle(
    color: TrampColors.lcdPhosphorDim,
    fontSize: 10,
    fontWeight: FontWeight.w600,
    fontFamily: 'IBMPlexMono',
    height: 1.1,
  );

  @override
  Widget build(BuildContext context) {
    final repeatOn = repeatMode != RepeatMode.off;

    return Row(
      children: [
        Expanded(
          flex: 2,
          child: MetalPanel(
            style: MetalPanelStyle.insetLcd,
            child: Padding(
              padding: const EdgeInsets.fromLTRB(6, 4, 6, 4),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Expanded(
                    child: SpectrumVisualizer(
                      playing: playing,
                      volume: volume,
                    ),
                  ),
                  Text(
                    positionLabel,
                    textAlign: TextAlign.center,
                    style: _lcdText.copyWith(fontSize: 14),
                  ),
                ],
              ),
            ),
          ),
        ),
        const SizedBox(width: 6),
        Expanded(
          flex: 3,
          child: MetalPanel(
            style: MetalPanelStyle.insetLcd,
            child: Padding(
              padding: const EdgeInsets.fromLTRB(8, 6, 8, 4),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Expanded(
                    child: Align(
                      alignment: Alignment.centerLeft,
                      child: SingleChildScrollView(
                        scrollDirection: Axis.horizontal,
                        child: Text(titleLine, style: _lcdText),
                      ),
                    ),
                  ),
                  Row(
                    children: [
                      Text('— kbps', style: _lcdDim),
                      const Spacer(),
                      Text('— kHz', style: _lcdDim),
                      const Spacer(),
                      Text(
                        'STEREO',
                        style: trackOpen ? _lcdText.copyWith(fontSize: 10) : _lcdDim,
                      ),
                    ],
                  ),
                  const SizedBox(height: 4),
                  Row(
                    children: [
                      _LcdToggle(
                        key: const Key('lcd-shuffle'),
                        label: 'SHUF',
                        active: shuffle,
                        onTap: onToggleShuffle,
                      ),
                      const SizedBox(width: 8),
                      _LcdToggle(
                        key: const Key('lcd-repeat'),
                        label: 'REP',
                        active: repeatOn,
                        onTap: onCycleRepeat,
                      ),
                      const Spacer(),
                      if (onFocusPlaylist != null)
                        _LcdToggle(
                          key: const Key('lcd-playlist'),
                          label: 'PL',
                          active: playlistActive,
                          onTap: onFocusPlaylist!,
                          chrome: true,
                        ),
                    ],
                  ),
                ],
              ),
            ),
          ),
        ),
      ],
    );
  }
}

class _LcdToggle extends StatelessWidget {
  const _LcdToggle({
    super.key,
    required this.label,
    required this.active,
    required this.onTap,
    this.chrome = false,
  });

  final String label;
  final bool active;
  final VoidCallback onTap;
  final bool chrome;

  @override
  Widget build(BuildContext context) {
    final text = Text(
      label,
      style: TextStyle(
        color: active ? TrampColors.lcdPhosphor : TrampColors.lcdPhosphorDim,
        fontSize: 10,
        fontWeight: FontWeight.w700,
        fontFamily: 'IBMPlexMono',
        height: 1,
      ),
    );

    final child = chrome
        ? DecoratedBox(
            decoration: BoxDecoration(
              color: TrampColors.metalMid,
              border: Border.all(color: TrampColors.metalDeep),
            ),
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 2),
              child: Text(
                label,
                style: TextStyle(
                  color: active
                      ? TrampColors.lcdPhosphor
                      : TrampColors.metalShadow,
                  fontSize: 10,
                  fontWeight: FontWeight.w700,
                  height: 1,
                ),
              ),
            ),
          )
        : text;

    return Semantics(
      button: true,
      label: label,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          onTap: onTap,
          behavior: HitTestBehavior.opaque,
          child: child,
        ),
      ),
    );
  }
}

class _TransportRow extends StatelessWidget {
  const _TransportRow({
    required this.volume,
    required this.muted,
    required this.onPrevious,
    required this.onPlay,
    required this.onPause,
    required this.onStop,
    required this.onNext,
    required this.onVolumeChanged,
    required this.onVolumeChangeEnd,
    required this.onToggleMute,
  });

  final double volume;
  final bool muted;
  final VoidCallback? onPrevious;
  final VoidCallback? onPlay;
  final VoidCallback? onPause;
  final VoidCallback? onStop;
  final VoidCallback? onNext;
  final ValueChanged<double> onVolumeChanged;
  final ValueChanged<double> onVolumeChangeEnd;
  final VoidCallback onToggleMute;

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      height: 36,
      child: Row(
        children: [
          ChromeButton(
            key: const Key('transport-prev'),
            onPressed: onPrevious,
            child: TransportIcons.prev(),
          ),
          const SizedBox(width: 4),
          ChromeButton(
            key: const Key('transport-play'),
            onPressed: onPlay,
            primary: true,
            child: TransportIcons.play(),
          ),
          const SizedBox(width: 4),
          ChromeButton(
            key: const Key('transport-pause'),
            onPressed: onPause,
            child: TransportIcons.pause(),
          ),
          const SizedBox(width: 4),
          ChromeButton(
            key: const Key('transport-stop'),
            onPressed: onStop,
            child: TransportIcons.stop(),
          ),
          const SizedBox(width: 4),
          ChromeButton(
            key: const Key('transport-next'),
            onPressed: onNext,
            child: TransportIcons.next(),
          ),
          const SizedBox(width: 12),
          Semantics(
            button: true,
            label: muted ? 'Unmute' : 'Mute',
            child: MouseRegion(
              cursor: SystemMouseCursors.click,
              child: GestureDetector(
                key: const Key('transport-mute'),
                onTap: onToggleMute,
                child: Icon(
                  muted ? Icons.volume_off : Icons.volume_up,
                  size: 18,
                  color: TrampColors.metalDeep,
                ),
              ),
            ),
          ),
          const SizedBox(width: 6),
          Expanded(
            child: SizedBox(
              height: 22,
              child: ChromeSlider(
                key: const Key('transport-volume'),
                value: muted ? 0 : volume,
                onChanged: onVolumeChanged,
                onChangeEnd: onVolumeChangeEnd,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
