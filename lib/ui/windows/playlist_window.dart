import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../../playlist/playlist_controller.dart';
import '../../domain/tramp_settings.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../chrome/window_resize_grip.dart';
import '../docking/dock_drag_area.dart';
import '../playlist/mockup_playlist.dart';
import '../playlist/mockup_playlist_collection_pane.dart';
import '../session/session_messages.dart';

/// Full Playlist Manager window: mockup shell + title bar + two-panel body —
/// the playlist collection on the left, the current playlist on the right.
///
/// Title: Collapse (shade) · Close (hide). Freely resizable; no per-window zoom.
class PlaylistWindow extends StatefulWidget {
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
    this.collectionWidth = TrampSettings.defaultPlaylistCollectionWidth,
    this.collectionCollapsed = false,
    this.onCollectionWidthChanged,
    this.onCollectionCollapsedChanged,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.draggableTitle = true,
    this.showResizeGrip = true,
    this.startResizing,
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

  /// Logical width of the collection panel; the divider drags it live and
  /// reports the result through [onCollectionWidthChanged] for persistence.
  final double collectionWidth;
  final bool collectionCollapsed;
  final ValueChanged<double>? onCollectionWidthChanged;
  final ValueChanged<bool>? onCollectionCollapsedChanged;

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

  /// Bottom-right size grip (hidden when shaded / in goldens that opt out).
  final bool showResizeGrip;

  /// Override for tests; defaults to [windowManager.startResizing].
  final Future<void> Function(ResizeEdge edge)? startResizing;

  @override
  State<PlaylistWindow> createState() => _PlaylistWindowState();
}

class _PlaylistWindowState extends State<PlaylistWindow> {
  /// Held locally so a divider drag repaints both panels on the frame it
  /// happens, instead of waiting for the host to echo the width back.
  late double _collectionWidth = widget.collectionWidth;
  late bool _collectionCollapsed = widget.collectionCollapsed;

  @override
  void didUpdateWidget(covariant PlaylistWindow oldWidget) {
    super.didUpdateWidget(oldWidget);
    // Adopt only real changes: unrelated rebuilds (a playlist snapshot, say)
    // must not undo a drag that has not been reported yet.
    if (widget.collectionWidth != oldWidget.collectionWidth) {
      _collectionWidth = widget.collectionWidth;
    }
    if (widget.collectionCollapsed != oldWidget.collectionCollapsed) {
      _collectionCollapsed = widget.collectionCollapsed;
    }
  }

  /// Widest the collection panel may be at this window width. The track list
  /// keeps [TrampMetrics.playlistMin] width whatever happens, because below it
  /// the footer controls overflow.
  double get _widestCollection =>
      widget.size.width -
      TrampMetrics.playlistDividerWidth -
      TrampMetrics.playlistMin.width;

  double get _renderedCollectionWidth => math.min(
        math.max(_collectionWidth, TrampMetrics.playlistCollectionMinWidth),
        _widestCollection,
      );

  /// False below [TrampMetrics.playlistMinWithCollection] — a width the OS
  /// minimum keeps the listener out of while the panel is shown, but which
  /// tests and stale persisted frames can still ask for. Hiding the panel there
  /// beats rendering it as an unusable sliver.
  bool get _showCollection =>
      !_collectionCollapsed &&
      _widestCollection >= TrampMetrics.playlistCollectionMinWidth;

  void _dragDivider(double delta) {
    final double widest =
        math.max(TrampMetrics.playlistCollectionMinWidth, _widestCollection);
    // Measured from what is on screen, not from the stored width, so a drag
    // always answers the edge the listener grabbed.
    final double next = (_renderedCollectionWidth + delta)
        .clamp(TrampMetrics.playlistCollectionMinWidth, widest);
    if (next == _collectionWidth) return;
    setState(() => _collectionWidth = next);
    widget.onCollectionWidthChanged?.call(next);
  }

  void _setCollapsed(bool collapsed) {
    if (_collectionCollapsed == collapsed) return;
    setState(() => _collectionCollapsed = collapsed);
    widget.onCollectionCollapsedChanged?.call(collapsed);
  }

  @override
  Widget build(BuildContext context) {
    final draggable = widget.draggableTitle &&
        widget.onDockMove != null &&
        widget.dockLogicalTopLeft != null;
    final title = MockupTitleBar(
      windowName: 'Playlist Manager',
      showBrand: false,
      showZoom: false,
      onCollapse: widget.onCollapse,
      onClose: widget.onClose,
      wrapDragRegion: draggable
          ? (region) => DockDragArea(
                zoom: widget.zoom,
                logicalTopLeft: widget.dockLogicalTopLeft!,
                onMove: widget.onDockMove!,
                onNativeDragStarted: widget.onNativeDragStarted,
                child: region,
              )
          : null,
    );

    final height = widget.shaded ? TrampMetrics.titleBar : widget.size.height;
    final resizable = !widget.shaded;

    Widget shell = MockupShell(
      width: widget.size.width,
      child: Column(
        children: [
          title,
          if (!widget.shaded) Expanded(child: _buildBody()),
        ],
      ),
    );

    if (resizable) {
      // Edges except top* — top strip stays free for title-bar dock drag.
      shell = DragToResizeArea(
        resizeEdgeSize: 6,
        enableResizeEdges: const [
          ResizeEdge.left,
          ResizeEdge.right,
          ResizeEdge.bottom,
          ResizeEdge.bottomLeft,
          ResizeEdge.bottomRight,
        ],
        child: shell,
      );
    }

    return SizedBox(
      width: widget.size.width,
      height: height,
      child: Stack(
        clipBehavior: Clip.none,
        children: [
          Positioned.fill(child: shell),
          if (resizable && widget.showResizeGrip)
            Positioned(
              right: 6,
              bottom: 6,
              child: WindowResizeGrip(startResizing: widget.startResizing),
            ),
        ],
      ),
    );
  }

  Widget _buildBody() {
    final trackPane = MockupPlaylist(
      playlist: widget.playlist,
      playingIndex: widget.playingIndex,
      playing: widget.playing,
      onSessionCommand: widget.onSessionCommand,
      onAddFiles: widget.onAddFiles,
      onLoadPlaylist: widget.onLoadPlaylist,
      onSavePlaylist: widget.onSavePlaylist,
      onDropPaths: widget.onDropPaths,
    );

    if (_showCollection) {
      return Row(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          SizedBox(
            width: _renderedCollectionWidth,
            child: MockupPlaylistCollectionPane(
              onCollapse: () => _setCollapsed(true),
            ),
          ),
          PlaylistCollectionDivider(
            key: const Key('pl-divider'),
            onDragDelta: _dragDivider,
          ),
          Expanded(child: trackPane),
        ],
      );
    }

    if (!_collectionCollapsed || _widestCollection <= 0) return trackPane;

    // The reopen tab floats over the track pane's gutter so a collapsed
    // Playlist Manager keeps today's layout and today's minimum width exactly.
    return Stack(
      children: [
        Positioned.fill(child: trackPane),
        Positioned(
          left: 6,
          top: 0,
          bottom: 0,
          child: Center(
            child: PlaylistCollectionReopenTab(
              onPressed: () => _setCollapsed(false),
            ),
          ),
        ),
      ],
    );
  }
}
