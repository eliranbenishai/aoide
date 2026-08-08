import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../../playlist/playlist_controller.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../playlist/mockup_playlist.dart';
import '../session/session_messages.dart';

/// Full playlist window: mockup shell + title bar + [MockupPlaylist] body.
///
/// Title: Collapse (shade) · Close (hide). Freely resizable; no per-window zoom.
class PlaylistWindow extends StatelessWidget {
  const PlaylistWindow({
    super.key,
    required this.playlist,
    this.size = TrampMetrics.playlistDefault,
    this.shaded = false,
    this.playingIndex,
    this.playing = false,
    this.onSessionCommand,
    this.onAddFiles,
    this.onLoadPlaylist,
    this.onSavePlaylist,
    this.onDropPaths,
    this.onCollapse,
    this.onClose,
    this.draggableTitle = true,
  });

  static const logicalDefault = TrampMetrics.playlistDefault;

  final PlaylistController playlist;
  final Size size;
  final bool shaded;
  final int? playingIndex;
  final bool playing;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final VoidCallback? onAddFiles;
  final VoidCallback? onLoadPlaylist;
  final VoidCallback? onSavePlaylist;
  final void Function(List<String> paths)? onDropPaths;
  final VoidCallback? onCollapse;
  final VoidCallback? onClose;
  final bool draggableTitle;

  @override
  Widget build(BuildContext context) {
    Widget title = MockupTitleBar(
      windowName: 'Playlist Editor',
      wordmarkSize: 19,
      showVersion: false,
      showZoom: false,
      onCollapse: onCollapse,
      onClose: onClose,
    );
    if (draggableTitle) {
      title = DragToMoveArea(child: title);
    }

    final height = shaded ? TrampMetrics.titleBar : size.height;

    return SizedBox(
      width: size.width,
      height: height,
      child: MockupShell(
        width: size.width,
        child: Column(
          children: [
            title,
            if (!shaded)
              Expanded(
                child: MockupPlaylist(
                  playlist: playlist,
                  playingIndex: playingIndex,
                  playing: playing,
                  onSessionCommand: onSessionCommand,
                  onAddFiles: onAddFiles,
                  onLoadPlaylist: onLoadPlaylist,
                  onSavePlaylist: onSavePlaylist,
                  onDropPaths: onDropPaths,
                ),
              ),
          ],
        ),
      ),
    );
  }
}
