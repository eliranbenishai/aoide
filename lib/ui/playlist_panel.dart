import 'package:flutter/material.dart';

import '../domain/track.dart';
import '../playback/playback_controller.dart';
import '../playlist/playlist_controller.dart';
import '../theme/tramp_colors.dart';
import 'chrome/chrome_button.dart';

String formatTrackDuration(Duration? duration) {
  if (duration == null) return '';
  final totalSeconds = duration.inSeconds;
  final minutes = totalSeconds ~/ 60;
  final seconds = totalSeconds % 60;
  return '$minutes:${seconds.toString().padLeft(2, '0')}';
}

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
    return DecoratedBox(
      decoration: const BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [TrampColors.metalHi, TrampColors.metalFace],
        ),
        border: Border(
          top: BorderSide(
            color: TrampColors.metalHi,
            width: TrampColors.borderWidth,
          ),
          left: BorderSide(
            color: TrampColors.metalHi,
            width: TrampColors.borderWidth,
          ),
          right: BorderSide(
            color: TrampColors.metalDeep,
            width: TrampColors.borderWidth,
          ),
          bottom: BorderSide(
            color: TrampColors.metalDeep,
            width: TrampColors.borderWidth,
          ),
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          _PlaylistToolbar(
            onOpen: onOpen,
            onSave: onSave,
            onAddFiles: onAddFiles,
          ),
          Expanded(
            child: ListenableBuilder(
              listenable: playlist,
              builder: (context, _) {
                final tracks = playlist.playlist.tracks;
                if (tracks.isEmpty) {
                  return const Center(
                    child: Text(
                      'No tracks',
                      style: TextStyle(color: TrampColors.metalShadow),
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
        ],
      ),
    );
  }
}

class _PlaylistToolbar extends StatelessWidget {
  const _PlaylistToolbar({
    this.onOpen,
    this.onSave,
    this.onAddFiles,
  });

  final VoidCallback? onOpen;
  final VoidCallback? onSave;
  final VoidCallback? onAddFiles;

  @override
  Widget build(BuildContext context) {
    return DecoratedBox(
      decoration: const BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [TrampColors.metalHi, TrampColors.metalMid],
        ),
        border: Border(
          bottom: BorderSide(
            color: TrampColors.groove,
            width: TrampColors.borderWidth,
          ),
        ),
      ),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        child: Wrap(
          spacing: 8,
          children: [
            ChromeButton(
              onPressed: onOpen,
              child: const Padding(
                padding: EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                child: Text('Open…'),
              ),
            ),
            ChromeButton(
              onPressed: onSave,
              child: const Padding(
                padding: EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                child: Text('Save…'),
              ),
            ),
            ChromeButton(
              onPressed: onAddFiles,
              child: const Padding(
                padding: EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                child: Text('Add files…'),
              ),
            ),
          ],
        ),
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
    final foreground =
        active ? TrampColors.lcdPhosphor : TrampColors.metalDeep;
    final muted =
        active ? TrampColors.lcdPhosphorDim : TrampColors.metalShadow;
    final indexLabel = (index + 1).toString().padLeft(2, '0');
    final artist = track.artist?.trim();

    final semanticLabel = artist != null && artist.isNotEmpty
        ? '${track.displayTitle}, $artist'
        : track.displayTitle;

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
                color: active ? TrampColors.lcdBackground : null,
                border: const Border(
                  bottom: BorderSide(
                    color: TrampColors.groove,
                    width: TrampColors.borderWidth,
                  ),
                ),
              ),
              child: Padding(
                padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
                child: Row(
                  children: [
                    SizedBox(
                      width: 28,
                      child: Text(
                        indexLabel,
                        style: TextStyle(color: muted, fontSize: 13),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Text.rich(
                        TextSpan(
                          children: [
                            TextSpan(
                              text: track.displayTitle,
                              style: TextStyle(
                                color: foreground,
                                fontWeight: FontWeight.w600,
                                fontSize: 13,
                              ),
                            ),
                            if (artist != null && artist.isNotEmpty)
                              TextSpan(
                                text: ' — $artist',
                                style: TextStyle(color: muted, fontSize: 13),
                              ),
                          ],
                        ),
                        maxLines: 1,
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                    const SizedBox(width: 10),
                    Text(
                      formatTrackDuration(track.duration),
                      style: TextStyle(
                        color: muted,
                        fontSize: 13,
                        fontFeatures: const [FontFeature.tabularFigures()],
                      ),
                    ),
                    const SizedBox(width: 8),
                    Semantics(
                      label: 'Reorder',
                      child: Icon(
                        Icons.drag_handle,
                        size: 16,
                        color: muted,
                      ),
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
