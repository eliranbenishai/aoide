import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../domain/track.dart';
import '../playback/playback_controller.dart';
import '../playlist/playlist_controller.dart';
import '../theme/tramp_colors.dart';
import 'widgets/tramp_button.dart';

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
          colors: [TrampColors.playlistTop, TrampColors.playlistBottom],
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
            child: Focus(
              autofocus: true,
              onKeyEvent: (node, event) {
                if (event is! KeyDownEvent ||
                    event.logicalKey != LogicalKeyboardKey.delete) {
                  return KeyEventResult.ignored;
                }
                final index = playlist.selectedIndex;
                if (index == null) return KeyEventResult.ignored;
                playlist.removeAt(index);
                return KeyEventResult.handled;
              },
              child: ListenableBuilder(
                listenable: playlist,
                builder: (context, _) {
                  final tracks = playlist.playlist.tracks;
                  if (tracks.isEmpty) {
                    return const Center(
                      child: Text(
                        'No tracks',
                        style: TextStyle(color: TrampColors.muted),
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
      decoration: BoxDecoration(
        border: Border(
          bottom: BorderSide(
            color: TrampColors.ink.withValues(alpha: 0.12),
          ),
        ),
      ),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
        child: Wrap(
          spacing: 8,
          children: [
            TrampButton(onPressed: onOpen, child: const Text('Open…')),
            TrampButton(onPressed: onSave, child: const Text('Save…')),
            TrampButton(onPressed: onAddFiles, child: const Text('Add files…')),
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
    final foreground = active ? TrampColors.surface : TrampColors.ink;
    final muted = active
        ? TrampColors.surface.withValues(alpha: 0.55)
        : TrampColors.muted;
    final indexLabel = (index + 1).toString().padLeft(2, '0');
    final artist = track.artist?.trim();

    return ReorderableDragStartListener(
      index: index,
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
              color: active ? TrampColors.ink : null,
              border: Border(
                bottom: BorderSide(
                  color: TrampColors.ink.withValues(alpha: 0.12),
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
                  Icon(
                    Icons.drag_handle,
                    size: 16,
                    color: muted,
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}
