import 'package:flutter/material.dart';

import '../../playlist/playlist_controller.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../docking/dock_drag_area.dart';
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
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
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

  /// Global zoom factor for docking drag → logical conversion.
  final double zoom;

  /// Logical top-left at drag start (from host dock snapshot / apply_frame).
  final ValueGetter<Offset>? dockLogicalTopLeft;

  /// Title-bar dock drag (logical coords, Shift undock, pan-end).
  final void Function(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  })? onDockMove;

  /// Native OS title-bar drag began (sibling sync via onWindowMove).
  final VoidCallback? onNativeDragStarted;

  final bool draggableTitle;

  @override
  Widget build(BuildContext context) {
    Widget title = MockupTitleBar(
      windowName: 'Playlist Editor',
      showBrand: false,
      showZoom: false,
      onCollapse: onCollapse,
      onClose: onClose,
    );
    if (draggableTitle && onDockMove != null && dockLogicalTopLeft != null) {
      title = DockDragArea(
        zoom: zoom,
        logicalTopLeft: dockLogicalTopLeft!,
        onMove: onDockMove!,
        onNativeDragStarted: onNativeDragStarted,
        child: title,
      );
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
