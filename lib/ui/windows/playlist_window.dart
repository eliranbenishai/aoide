import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:window_manager/window_manager.dart';

import '../../playlist/playlist_controller.dart';
import '../../domain/saved_playlist.dart';
import '../../domain/tramp_settings.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../chrome/window_resize_grip.dart';
import '../docking/dock_drag_area.dart';
import '../playlist/altered_playlist_dialog.dart';
import '../playlist/mockup_playlist.dart';
import '../playlist/mockup_playlist_collection_pane.dart';
import '../playlist/rename_playlist_dialog.dart';
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
    this.collection = const [],
    this.selectedCollectionPath,
    this.disabledCollectionPaths = const {},
    this.onAddSavedPlaylist,
    this.altered = false,
    this.pickSavePlaylistPath,
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

  /// The listener's playlist collection, from the host snapshot. Held as a prop
  /// rather than in state: the host owns it, and mirroring it here would put a
  /// second copy behind the divider's `didUpdateWidget` rule.
  final List<SavedPlaylist> collection;
  final String? selectedCollectionPath;

  /// Normalized paths of the **disabled playlists**, from the same snapshot —
  /// the host derives them from its last check rather than storing them.
  final Set<String> disabledCollectionPaths;

  /// Opens the playlist-file picker; the client sends the add command.
  final VoidCallback? onAddSavedPlaylist;

  /// Whether the host's current playlist is an **altered current playlist**,
  /// from the playlist snapshot. Navigating to a saved playlist asks first.
  final bool altered;

  /// Opens the save dialog and answers with the chosen path, or null when the
  /// listener cancelled it. Only reached by the confirmation's save when the
  /// current playlist has no origin to write straight to.
  final Future<String?> Function()? pickSavePlaylistPath;

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

  void _emit(SessionCommand command) => widget.onSessionCommand?.call(command);

  /// True when the **altered current playlist** needs protecting: what the host
  /// broadcast, or a mutation this window has made that the host has not echoed
  /// back yet, so a click in that gap still asks. Applying any snapshot resets
  /// the mirror's own flag, so this can never read stale-true.
  bool get _altered => widget.altered || widget.playlist.altered;

  /// A row tap loads the playlist — unless it is a **disabled playlist**, which
  /// is only highlighted, so the panel's remove control can reach it while the
  /// load that would fail never starts.
  ///
  /// While the current playlist is altered the load is put to the listener
  /// first, and every answer but discard can end with no load at all.
  Future<void> _selectSavedPlaylist(SavedPlaylist entry) async {
    if (widget.disabledCollectionPaths.contains(entry.path)) {
      _emit(SelectSavedPlaylistCommand(entry.path));
      return;
    }
    if (!_altered) {
      _emit(LoadSavedPlaylistCommand(entry.path));
      return;
    }
    final choice = await showAlteredPlaylistDialog(context);
    if (!mounted) return;
    switch (choice) {
      case AlteredPlaylistChoice.cancel:
        // Everything stays exactly as it was: no save, no load, still altered.
        return;
      case AlteredPlaylistChoice.discard:
        _emit(LoadSavedPlaylistCommand(entry.path));
      case AlteredPlaylistChoice.save:
        if (!await _saveWholeCurrentPlaylist()) return;
        _emit(LoadSavedPlaylistCommand(entry.path));
    }
  }

  /// Keeps the pile the listener has built: they say where it goes, the file is
  /// written there, and a reference to it joins the collection — one action,
  /// with no separate "now add it" step.
  ///
  /// Cancelling the save dialog changes nothing at all: no command leaves the
  /// window, so there is no file, no entry, and the altered state stays exactly
  /// where it was.
  ///
  /// An **empty** current playlist never reaches here — the control that calls
  /// this is disabled while there is nothing to keep.
  Future<void> _createPlaylistFromCurrentTracks() async {
    final path = await widget.pickSavePlaylistPath?.call();
    if (!mounted || path == null || path.isEmpty) return;
    _emit(CreatePlaylistFromCurrentCommand(path));
  }

  /// Pulls the selected tracks out into a playlist of their own.
  ///
  /// Deliberately **not** the same action as create-from-current. Only some of
  /// the current tracks are being kept, so the rest are still unsaved: the
  /// current playlist's tracks, its origin, and its altered state all stay
  /// exactly as they are, and nothing is loaded. Navigating would either
  /// discard the current playlist or raise the very prompt that avoids — so
  /// this creates the entry, the host highlights it, and the listener does not
  /// move.
  ///
  /// Cancelling the save dialog changes nothing at all: no command leaves the
  /// window, so there is no file and no entry.
  ///
  /// An **empty selection** never reaches here — the menu item that calls this
  /// is greyed while there is nothing selected to pull out.
  Future<void> _createPlaylistFromSelectedTracks() async {
    final path = await widget.pickSavePlaylistPath?.call();
    if (!mounted || path == null || path.isEmpty) return;
    _emit(CreatePlaylistFromSelectionCommand(path));
  }

  /// Retitles a saved playlist to whatever the listener types, or back to its
  /// filename when they clear the field.
  ///
  /// The name lives in Tramp's index and nowhere else: the playlist file keeps
  /// the name the listener gave it in their own file manager. Backing out of
  /// the dialog emits nothing.
  Future<void> _renameSavedPlaylist(SavedPlaylist entry) async {
    final name = await showRenamePlaylistDialog(
      context,
      currentName: entry.displayName,
    );
    if (!mounted || name == null) return;
    _emit(RenameSavedPlaylistCommand(entry.path, name));
  }

  /// Writes the whole current playlist to the file that becomes its origin:
  /// straight to the origin it already has, or wherever the save dialog says.
  ///
  /// Returns false when nothing was written — which is the sharp edge of the
  /// confirmation. A cancelled save dialog goes back to the current playlist
  /// with the altered state still raised; it must not fall through to the load.
  Future<bool> _saveWholeCurrentPlaylist() async {
    final origin = widget.playlist.playlist.sourcePath;
    if (origin != null && origin.isNotEmpty) {
      _emit(PlaylistOpCommand('savePlaylist', path: origin));
      return true;
    }
    final path = await widget.pickSavePlaylistPath?.call();
    if (!mounted || path == null || path.isEmpty) return false;
    _emit(PlaylistOpCommand('savePlaylist', path: path));
    return true;
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
            // Listens to the current playlist as well as the collection: the
            // create control has to go dead the moment the last track leaves —
            // and its from-selection half the moment the selection empties —
            // not when the host's next snapshot says so.
            child: ListenableBuilder(
              listenable: widget.playlist,
              builder: (context, _) => MockupPlaylistCollectionPane(
                playlists: widget.collection,
                selectedPath: widget.selectedCollectionPath,
                disabledPaths: widget.disabledCollectionPaths,
                onCollapse: () => _setCollapsed(true),
                onSelect: (entry) => unawaited(_selectSavedPlaylist(entry)),
                onAdd: widget.onAddSavedPlaylist,
                onCreateFromCurrent: widget.playlist.playlist.tracks.isEmpty
                    ? null
                    : () => unawaited(_createPlaylistFromCurrentTracks()),
                onCreateFromSelection:
                    widget.playlist.selectedIndices.isEmpty
                        ? null
                        : () => unawaited(_createPlaylistFromSelectedTracks()),
                onRename: (entry) => unawaited(_renameSavedPlaylist(entry)),
                onRemove: (entry) =>
                    _emit(RemoveSavedPlaylistCommand(entry.path)),
              ),
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
