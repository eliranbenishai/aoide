import 'dart:ui' show FontFeature;

import 'package:flutter/material.dart';

import '../domain/track.dart';
import '../playback/playback_controller.dart';
import '../playlist/playlist_controller.dart';
import '../theme/tramp_colors.dart';
import '../theme/tramp_text.dart';
import 'chrome/chrome_button.dart';
import 'chrome/metal_panel.dart';
import 'chrome/transport_icons.dart';
import 'zoom/zoom_scope.dart';

String formatTrackDuration(Duration? duration) {
  if (duration == null) return '';
  final totalSeconds = duration.inSeconds;
  final minutes = totalSeconds ~/ 60;
  final seconds = totalSeconds % 60;
  return '$minutes:${seconds.toString().padLeft(2, '0')}';
}

/// Width reserved for the right-aligned duration column (tabular m:ss).
/// Fits `999:59` at [TrampText.lcd] (~39.6 logical px measured).
const _kDurationColumnWidth = 40.0;

class PlaylistPanel extends StatelessWidget {
  const PlaylistPanel({
    super.key,
    required this.playlist,
    required this.playback,
    this.onOpen,
    this.onSave,
    this.onAddFiles,
  });

  final PlaylistController playlist;
  final PlaybackController playback;
  final VoidCallback? onOpen;
  final VoidCallback? onSave;
  final VoidCallback? onAddFiles;

  @override
  Widget build(BuildContext context) {
    return MetalPanel(
      surface: TrampSurface.raisedPanel,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _PlaylistToolbar(
            onOpen: onOpen,
            onSave: onSave,
            onAddFiles: onAddFiles,
          ),
          Expanded(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(6, 0, 6, 6),
              child: MetalPanel(
                surface: TrampSurface.lcdGlass,
                child: ListenableBuilder(
                  listenable: playlist,
                  builder: (context, _) {
                    final tracks = playlist.playlist.tracks;
                    if (tracks.isEmpty) {
                      return Center(
                        child: Text(
                          'No tracks',
                          style: TrampText.lcdDim,
                        ),
                      );
                    }

                    return ReorderableListView.builder(
                      buildDefaultDragHandles: false,
                      itemCount: tracks.length,
                      onReorder: playlist.move,
                      itemBuilder: (context, index) {
                        final track = tracks[index];
                        final active = playlist.selectedIndex == index;
                        return _PlaylistRow(
                          key: ValueKey(track.path),
                          index: index,
                          track: track,
                          active: active,
                          onActivate: () => playback.playIndex(index),
                          onSelect: () => playlist.select(index),
                        );
                      },
                    );
                  },
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _PlaylistToolbar extends StatelessWidget {
  const _PlaylistToolbar({this.onOpen, this.onSave, this.onAddFiles});

  final VoidCallback? onOpen;
  final VoidCallback? onSave;
  final VoidCallback? onAddFiles;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(6, 6, 6, 6),
      child: Row(
        children: [
          ChromeButton.label(
            key: const Key('playlist-open'),
            text: 'OPEN',
            onPressed: onOpen,
            size: const Size(54, 22),
          ),
          const SizedBox(width: 5),
          ChromeButton.label(
            key: const Key('playlist-save'),
            text: 'SAVE',
            onPressed: onSave,
            size: const Size(54, 22),
          ),
          const SizedBox(width: 5),
          ChromeButton.label(
            key: const Key('playlist-add'),
            text: 'ADD',
            onPressed: onAddFiles,
            size: const Size(48, 22),
          ),
        ],
      ),
    );
  }
}

class _PlaylistRow extends StatelessWidget {
  const _PlaylistRow({
    super.key,
    required this.index,
    required this.track,
    required this.active,
    required this.onActivate,
    required this.onSelect,
  });

  final int index;
  final Track track;
  final bool active;
  final VoidCallback onActivate;
  final VoidCallback onSelect;

  @override
  Widget build(BuildContext context) {
    final hairline = ZoomScope.hairlineFor(context);
    final foreground = active ? TrampColors.phosphor : TrampColors.label;
    final muted = active ? TrampColors.phosphorDim : TrampColors.labelDim;
    final indexLabel = (index + 1).toString().padLeft(2, '0');
    final artist = track.artist?.trim();
    final hasArtist = artist != null && artist.isNotEmpty;

    final semanticLabel =
        hasArtist ? '${track.displayTitle}, $artist' : track.displayTitle;

    return ReorderableDragStartListener(
      index: index,
      child: Semantics(
        selected: active,
        button: true,
        label: semanticLabel,
        child: Actions(
          actions: {
            ActivateIntent: CallbackAction<ActivateIntent>(
              onInvoke: (_) {
                onActivate();
                return null;
              },
            ),
          },
          child: GestureDetector(
            onTap: onSelect,
            onDoubleTap: onActivate,
            child: DecoratedBox(
              decoration: BoxDecoration(
                border: Border(
                  bottom: BorderSide(
                    color: TrampColors.bevelLo,
                    width: hairline,
                  ),
                ),
              ),
              child: Padding(
                padding:
                    const EdgeInsets.symmetric(horizontal: 8, vertical: 5),
                child: Row(
                  children: [
                    SizedBox(
                      width: 24,
                      child: Text(
                        indexLabel,
                        style: TrampText.lcd.copyWith(color: muted),
                      ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Row(
                        children: [
                          // Title and artist are separate Text widgets rather
                          // than one Text.rich so each is directly findable and
                          // assertable.
                          Flexible(
                            child: Text(
                              track.displayTitle,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                              style: TrampText.lcd.copyWith(color: foreground),
                            ),
                          ),
                          if (hasArtist) ...[
                            Text(
                              ' — ',
                              style: TrampText.lcd.copyWith(color: muted),
                            ),
                            Flexible(
                              child: Text(
                                artist,
                                maxLines: 1,
                                overflow: TextOverflow.ellipsis,
                                style: TrampText.lcd.copyWith(color: muted),
                              ),
                            ),
                          ],
                        ],
                      ),
                    ),
                    const SizedBox(width: 8),
                    SizedBox(
                      width: _kDurationColumnWidth,
                      child: Align(
                        alignment: Alignment.centerRight,
                        child: Text(
                          formatTrackDuration(track.duration),
                          style: TrampText.lcd.copyWith(
                            color: muted,
                            fontFeatures: const [FontFeature.tabularFigures()],
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(width: 6),
                    Semantics(
                      label: 'Reorder',
                      child: TransportIcons.dragHandle(colour: muted),
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
