import 'package:flutter/material.dart' hide RepeatMode;

import '../domain/repeat_mode.dart';
import '../domain/track.dart';
import '../playback/playback_controller.dart';
import '../theme/tramp_colors.dart';
import 'widgets/ink_slider.dart';
import 'widgets/tramp_button.dart';

String formatDuration(Duration duration) {
  final totalSeconds = duration.inSeconds;
  final minutes = totalSeconds ~/ 60;
  final seconds = totalSeconds % 60;
  return '$minutes:${seconds.toString().padLeft(2, '0')}';
}

String trackSubtitle(Track? track) {
  if (track == null) return '';
  final parts = <String>[];
  final artist = track.artist?.trim();
  final album = track.album?.trim();
  if (artist != null && artist.isNotEmpty) parts.add(artist);
  if (album != null && album.isNotEmpty) parts.add(album);
  return parts.join(' · ');
}

String repeatLabel(RepeatMode mode) {
  return switch (mode) {
    RepeatMode.off => 'Off',
    RepeatMode.all => 'All',
    RepeatMode.one => 'One',
  };
}

class TransportPanel extends StatefulWidget {
  const TransportPanel({
    super.key,
    required this.playback,
    required this.hasTracks,
  });

  final PlaybackController playback;
  final bool hasTracks;

  @override
  State<TransportPanel> createState() => _TransportPanelState();
}

class _TransportPanelState extends State<TransportPanel> {
  double? _seekFraction;
  double? _volumeFraction;

  PlaybackController get playback => widget.playback;

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: playback,
      builder: (context, _) {
        final track = playback.currentTrack;
        final duration = playback.duration;
        final durationMs = duration.inMilliseconds;
        final positionFraction = durationMs > 0
            ? playback.position.inMilliseconds / durationMs
            : 0.0;
        final seekValue = _seekFraction ?? positionFraction.clamp(0.0, 1.0);
        final volumeValue = _volumeFraction ?? playback.volume;
        final canTransport = track != null || widget.hasTracks;

        return DecoratedBox(
          decoration: BoxDecoration(
            color: TrampColors.transportWash,
            border: Border(
              bottom: BorderSide(
                color: TrampColors.ink,
                width: TrampColors.borderWidth,
              ),
            ),
            gradient: RadialGradient(
              center: const Alignment(0.9, -1),
              radius: 0.8,
              colors: [
                const Color(0xFFFFD7A8),
                TrampColors.transportWash.withValues(alpha: 0),
              ],
            ),
          ),
          child: Padding(
            padding: const EdgeInsets.fromLTRB(14, 14, 14, 12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Row(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            track?.displayTitle ?? 'No track',
                            style: const TextStyle(
                              fontWeight: FontWeight.w600,
                              fontSize: 14,
                            ),
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                          ),
                          if (track != null && trackSubtitle(track).isNotEmpty)
                            Text(
                              trackSubtitle(track),
                              style: const TextStyle(
                                color: TrampColors.muted,
                                fontSize: 11,
                              ),
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                            ),
                        ],
                      ),
                    ),
                    const SizedBox(width: 12),
                    Text(
                      '${formatDuration(playback.position)} / ${formatDuration(duration)}',
                      style: const TextStyle(
                        color: TrampColors.muted,
                        fontSize: 11,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 10),
                IgnorePointer(
                  ignoring: durationMs <= 0,
                  child: InkSlider(
                    value: seekValue,
                    onChanged: (value) => setState(() => _seekFraction = value),
                    onChangeEnd: durationMs > 0
                        ? (value) {
                            setState(() => _seekFraction = null);
                            final target = Duration(
                              milliseconds: (value * durationMs).round(),
                            );
                            playback.seek(target);
                          }
                        : null,
                  ),
                ),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    TrampButton(
                      onPressed:
                          canTransport ? () => playback.previous() : null,
                      child: const Text('Prev'),
                    ),
                    TrampButton(
                      primary: true,
                      onPressed:
                          canTransport ? () => playback.playPause() : null,
                      child: Text(playback.playing ? 'Pause' : 'Play'),
                    ),
                    TrampButton(
                      onPressed: track != null ? () => playback.stop() : null,
                      child: const Text('Stop'),
                    ),
                    TrampButton(
                      onPressed: canTransport ? () => playback.next() : null,
                      child: const Text('Next'),
                    ),
                    TrampButton(
                      primary: playback.shuffle,
                      onPressed: () => playback.toggleShuffle(),
                      child: const Text('Shuffle'),
                    ),
                    TrampButton(
                      onPressed: () => playback.cycleRepeatMode(),
                      child: Text('Repeat ${repeatLabel(playback.repeatMode)}'),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                Row(
                  children: [
                    const Text(
                      'Vol',
                      style: TextStyle(
                        color: TrampColors.muted,
                        fontSize: 11,
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: InkSlider(
                        value: volumeValue,
                        onChanged: (value) {
                          setState(() => _volumeFraction = value);
                          playback.setVolume(value);
                        },
                        onChangeEnd: (_) => setState(() => _volumeFraction = null),
                      ),
                    ),
                    const SizedBox(width: 8),
                    TrampButton(
                      primary: playback.muted,
                      onPressed: () => playback.toggleMute(),
                      child: const Text('Mute'),
                    ),
                  ],
                ),
              ],
            ),
          ),
        );
      },
    );
  }
}
