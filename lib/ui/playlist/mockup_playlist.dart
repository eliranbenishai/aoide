import 'package:desktop_drop/desktop_drop.dart';
import 'package:flutter/material.dart';

import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_sort.dart';
import '../session/session_messages.dart';
import 'mockup_playlist_footer.dart';
import 'mockup_playlist_track_pane.dart';

/// Mockup-faithful playlist body (grows with window; footer bottom-anchored).
///
/// Layout matches `player-mockup-2.html` `.win--pl .body`: list well + footer
/// strip (add/remove/sort/options · mini transport · TOTAL) + status line.
///
/// This widget only composes and wires; the two halves live in
/// [MockupPlaylistTrackPane] and [MockupPlaylistFooter].
class MockupPlaylist extends StatelessWidget {
  const MockupPlaylist({
    super.key,
    required this.playlist,
    this.playingIndex,
    this.playing = false,
    this.onSessionCommand,
    this.onAddFiles,
    this.onLoadPlaylist,
    this.onSavePlaylist,
    this.onDropPaths,
  });

  final PlaylistController playlist;
  final int? playingIndex;
  final bool playing;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final VoidCallback? onAddFiles;
  final VoidCallback? onLoadPlaylist;
  final VoidCallback? onSavePlaylist;
  final void Function(List<String> paths)? onDropPaths;

  void _emit(SessionCommand command) => onSessionCommand?.call(command);

  /// Selects every track, from the options menu or the keyboard chord.
  ///
  /// Both routes go through here so the two cannot drift into applying the same
  /// op differently — and neither guards on an empty list, because selecting
  /// nothing is already a no-op at both ends of the bus.
  void _selectAll() {
    playlist.selectAll();
    _emit(const PlaylistOpCommand('selectAll'));
  }

  @override
  Widget build(BuildContext context) {
    Widget body = ListenableBuilder(
      listenable: playlist,
      builder: (context, _) {
        return Padding(
          padding: const EdgeInsets.all(12),
          child: Column(
            children: [
              Expanded(
                child: MockupPlaylistTrackPane(
                  playlist: playlist,
                  playingIndex: playingIndex,
                  onSelect: (index, how) {
                    // Applied here and echoed to the host, so this window
                    // repaints on the frame the listener clicked and the two
                    // engines' selections stay the same selection.
                    switch (how) {
                      case TrackRowSelection.replace:
                        playlist.select(index);
                        _emit(PlaylistOpCommand('select', index: index));
                      case TrackRowSelection.range:
                        playlist.selectRange(index);
                        _emit(PlaylistOpCommand('selectRange', index: index));
                      case TrackRowSelection.toggle:
                        playlist.toggleSelection(index);
                        _emit(PlaylistOpCommand('toggleSelect', index: index));
                    }
                  },
                  onActivate: (index) {
                    playlist.select(index);
                    _emit(PlaylistOpCommand('playIndex', index: index));
                  },
                  onReorder: (oldIndex, newIndex) {
                    // A drag carries the row it started on and nothing else,
                    // even out of a multi-selection — the list can only show
                    // one row in flight, so a preview of several moving would
                    // be a promise it cannot keep. `move` remaps the selection
                    // across the reorder, so every highlighted row stays on
                    // the track it was highlighting.
                    playlist.move(oldIndex, newIndex);
                    // Emitted whichever way it lands: the controller is the
                    // single place that judges what counts as a reorder, and
                    // it makes that judgement identically at both ends, so a
                    // row dropped back where it came from is a no-op twice
                    // over rather than a rule written down twice.
                    _emit(PlaylistOpCommand(
                      'move',
                      index: oldIndex,
                      toIndex: newIndex,
                    ));
                  },
                ),
              ),
              const SizedBox(height: 10),
              MockupPlaylistFooter(
                playlist: playlist,
                playing: playing,
                playingIndex: playingIndex,
                onAdd: onAddFiles,
                onRemove: () {
                  playlist.removeSelected();
                  _emit(const PlaylistOpCommand('removeSelected'));
                },
                onSort: (key) {
                  if (key == 'reverse') {
                    playlist.reverseTracks();
                    _emit(const PlaylistOpCommand('reverse'));
                    return;
                  }
                  final sortKey = PlaylistSortKey.values.asNameMap()[key];
                  if (sortKey != null) {
                    playlist.sortBy(sortKey);
                  }
                  _emit(PlaylistOpCommand('sort', sortKey: key));
                },
                onOption: (op) {
                  switch (op) {
                    case 'load':
                      onLoadPlaylist?.call();
                    case 'save':
                      onSavePlaylist?.call();
                    case 'clear':
                      playlist.clear();
                      _emit(const PlaylistOpCommand('clear'));
                    case 'selectAll':
                      _selectAll();
                    case 'invertSelection':
                      playlist.invertSelection();
                      _emit(const PlaylistOpCommand('invertSelection'));
                  }
                },
                onPrevious: () => _emit(const TransportCommand('previous')),
                onPlayPause: () => _emit(const TransportCommand('playPause')),
                onNext: () => _emit(const TransportCommand('next')),
              ),
            ],
          ),
        );
      },
    );

    if (onDropPaths != null) {
      body = DropTarget(
        onDragDone: (details) {
          final paths = details.files
              .map((f) => f.path)
              .where((path) => path.isNotEmpty)
              .toList();
          if (paths.isNotEmpty) onDropPaths!(paths);
        },
        child: body,
      );
    }

    // The chord is bound to the current-playlist panel rather than the window,
    // so it always means "select all *tracks*" — the collection panel beside it
    // holds a different kind of row and is free to claim the chord for its own
    // later. `CallbackShortcuts` cannot take focus itself, hence the node: the
    // panel is this window's subject, so it is the right thing to focus on open.
    return CallbackShortcuts(
      bindings: {selectAllActivator(): _selectAll},
      child: Focus(autofocus: true, child: body),
    );
  }
}
