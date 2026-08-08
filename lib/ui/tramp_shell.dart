import 'dart:async';

import 'package:desktop_drop/desktop_drop.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

import '../playback/playback_controller.dart';
import '../playlist/playlist_controller.dart';
import '../theme/tramp_colors.dart';
import '../theme/tramp_metrics.dart';
import 'lower_region.dart';
import 'window_layout.dart';
import 'zoom/zoom_controller.dart';
import 'zoom/zoom_scope.dart';

/// The scaled stack of panels. Named so layout tests can measure it.
const Key panelStackKey = Key('panel-stack');

class PlayPauseIntent extends Intent {
  const PlayPauseIntent();
}

class NextTrackIntent extends Intent {
  const NextTrackIntent();
}

class PreviousTrackIntent extends Intent {
  const PreviousTrackIntent();
}

class StopPlaybackIntent extends Intent {
  const StopPlaybackIntent();
}

class SelectPreviousIntent extends Intent {
  const SelectPreviousIntent();
}

class SelectNextIntent extends Intent {
  const SelectNextIntent();
}

class PlaySelectedIntent extends Intent {
  const PlaySelectedIntent();
}

class RemoveSelectedIntent extends Intent {
  const RemoveSelectedIntent();
}

class OpenFilesIntent extends Intent {
  const OpenFilesIntent();
}

class SavePlaylistIntent extends Intent {
  const SavePlaylistIntent();
}

class ZoomInIntent extends Intent {
  const ZoomInIntent();
}

class ZoomOutIntent extends Intent {
  const ZoomOutIntent();
}

class ZoomResetIntent extends Intent {
  const ZoomResetIntent();
}

class TrampShell extends StatelessWidget {
  const TrampShell({
    super.key,
    required this.mainPlayer,
    required this.equalizer,
    required this.playlist,
    required this.playback,
    required this.playlistController,
    required this.zoom,
    required this.lowerRegion,
    this.equalizerCollapsed = false,
    this.hasTracks = false,
    this.playlistFocusNode,
    this.onDropPaths,
    this.onOpenFiles,
    this.onSavePlaylist,
  });

  final Widget mainPlayer;
  final Widget equalizer;
  final Widget playlist;
  final PlaybackController playback;
  final PlaylistController playlistController;
  final ZoomController zoom;
  final LowerRegion lowerRegion;
  final bool equalizerCollapsed;
  final bool hasTracks;
  final FocusNode? playlistFocusNode;
  final void Function(List<String> paths)? onDropPaths;
  final Future<void> Function()? onOpenFiles;
  final Future<void> Function()? onSavePlaylist;

  static const _shortcuts = <ShortcutActivator, Intent>{
    SingleActivator(LogicalKeyboardKey.space): PlayPauseIntent(),
    SingleActivator(LogicalKeyboardKey.mediaPlayPause): PlayPauseIntent(),
    SingleActivator(LogicalKeyboardKey.mediaTrackNext): NextTrackIntent(),
    SingleActivator(LogicalKeyboardKey.mediaTrackPrevious): PreviousTrackIntent(),
    SingleActivator(LogicalKeyboardKey.mediaStop): StopPlaybackIntent(),
    SingleActivator(LogicalKeyboardKey.arrowUp): SelectPreviousIntent(),
    SingleActivator(LogicalKeyboardKey.arrowDown): SelectNextIntent(),
    SingleActivator(LogicalKeyboardKey.enter): PlaySelectedIntent(),
    SingleActivator(LogicalKeyboardKey.delete): RemoveSelectedIntent(),
    SingleActivator(LogicalKeyboardKey.keyO, control: true): OpenFilesIntent(),
    SingleActivator(LogicalKeyboardKey.keyO, meta: true): OpenFilesIntent(),
    SingleActivator(LogicalKeyboardKey.keyS, control: true): SavePlaylistIntent(),
    SingleActivator(LogicalKeyboardKey.keyS, meta: true): SavePlaylistIntent(),
    SingleActivator(LogicalKeyboardKey.equal, control: true): ZoomInIntent(),
    SingleActivator(LogicalKeyboardKey.equal, meta: true): ZoomInIntent(),
    SingleActivator(LogicalKeyboardKey.minus, control: true): ZoomOutIntent(),
    SingleActivator(LogicalKeyboardKey.minus, meta: true): ZoomOutIntent(),
    SingleActivator(LogicalKeyboardKey.digit0, control: true): ZoomResetIntent(),
    SingleActivator(LogicalKeyboardKey.digit0, meta: true): ZoomResetIntent(),
  };

  Map<Type, Action<Intent>> _actions() {
    return {
      PlayPauseIntent: CallbackAction<PlayPauseIntent>(
        onInvoke: (_) {
          unawaited(_playPause());
          return null;
        },
      ),
      NextTrackIntent: CallbackAction<NextTrackIntent>(
        onInvoke: (_) {
          unawaited(playback.next());
          return null;
        },
      ),
      PreviousTrackIntent: CallbackAction<PreviousTrackIntent>(
        onInvoke: (_) {
          unawaited(playback.previous());
          return null;
        },
      ),
      StopPlaybackIntent: CallbackAction<StopPlaybackIntent>(
        onInvoke: (_) {
          unawaited(playback.stop());
          return null;
        },
      ),
      SelectPreviousIntent: CallbackAction<SelectPreviousIntent>(
        onInvoke: (_) {
          _selectPrevious();
          return null;
        },
      ),
      SelectNextIntent: CallbackAction<SelectNextIntent>(
        onInvoke: (_) {
          _selectNext();
          return null;
        },
      ),
      PlaySelectedIntent: CallbackAction<PlaySelectedIntent>(
        onInvoke: (_) {
          unawaited(_playSelected());
          return null;
        },
      ),
      RemoveSelectedIntent: CallbackAction<RemoveSelectedIntent>(
        onInvoke: (_) {
          final index = playlistController.selectedIndex;
          if (index != null) {
            playlistController.removeAt(index);
          }
          return null;
        },
      ),
      OpenFilesIntent: CallbackAction<OpenFilesIntent>(
        onInvoke: (_) {
          unawaited(onOpenFiles?.call());
          return null;
        },
      ),
      SavePlaylistIntent: CallbackAction<SavePlaylistIntent>(
        onInvoke: (_) {
          unawaited(onSavePlaylist?.call());
          return null;
        },
      ),
      ZoomInIntent: CallbackAction<ZoomInIntent>(
        onInvoke: (_) {
          zoom.stepUp();
          return null;
        },
      ),
      ZoomOutIntent: CallbackAction<ZoomOutIntent>(
        onInvoke: (_) {
          zoom.stepDown();
          return null;
        },
      ),
      ZoomResetIntent: CallbackAction<ZoomResetIntent>(
        onInvoke: (_) {
          zoom.reset();
          return null;
        },
      ),
    };
  }

  Future<void> _playPause() async {
    if (!hasTracks) return;
    await playback.playPause();
  }

  Future<void> _playSelected() async {
    final index = playlistController.selectedIndex;
    if (index == null) return;
    await playback.playIndex(index);
  }

  void _selectPrevious() {
    final tracks = playlistController.playlist.tracks;
    if (tracks.isEmpty) return;

    final current = playlistController.selectedIndex;
    if (current == null) {
      playlistController.select(tracks.length - 1);
    } else if (current > 0) {
      playlistController.select(current - 1);
    }
  }

  void _selectNext() {
    final tracks = playlistController.playlist.tracks;
    if (tracks.isEmpty) return;

    final current = playlistController.selectedIndex;
    if (current == null) {
      playlistController.select(0);
    } else if (current < tracks.length - 1) {
      playlistController.select(current + 1);
    }
  }

  @override
  Widget build(BuildContext context) {
    // Material supplies the ink/text theme ancestor. MaterialApp alone is not
    // enough without a Scaffold; without this, debug builds underline every Text.
    final content = Material(
      color: TrampColors.frame,
      child: ListenableBuilder(
        listenable: zoom,
        builder: (context, _) {
          final factor = zoom.factor;
          final ratio = MediaQuery.devicePixelRatioOf(context);

          final focusedPlaylist = playlistFocusNode == null
              ? playlist
              : Focus(focusNode: playlistFocusNode, child: playlist);

          // Transform.scale paints larger but does not change layout size, and
          // it forwards its (tight) constraints to the child — which would
          // clamp the "logical" stack up to the scaled host size. OverflowBox
          // re-imposes logical constraints so the stack lays out at its true
          // logical size while the keyed host observes the scaled size.
          return ZoomScope(
            factor: factor,
            devicePixelRatio: ratio,
            child: LayoutBuilder(
              builder: (context, constraints) {
                final layout = panelStackLayout(
                  lowerRegion: lowerRegion,
                  factor: factor,
                  equalizerCollapsed: equalizerCollapsed,
                  contentSize: Size(
                    constraints.maxWidth - TrampMetrics.frame * 2,
                    constraints.maxHeight - TrampMetrics.frame * 2,
                  ),
                );
                final host = layout.hostSize(factor);
                return Padding(
                  padding: const EdgeInsets.all(TrampMetrics.frame),
                  child: Align(
                    alignment: Alignment.topLeft,
                    child: SizedBox(
                      key: panelStackKey,
                      width: host.width,
                      height: host.height,
                      child: Transform.scale(
                        scale: factor,
                        alignment: Alignment.topLeft,
                        child: OverflowBox(
                          alignment: Alignment.topLeft,
                          minWidth: 0,
                          maxWidth: layout.logicalWidth,
                          minHeight: 0,
                          maxHeight: layout.logicalHeight,
                          child: SizedBox(
                            width: layout.logicalWidth,
                            height: layout.logicalHeight,
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                mainPlayer,
                                const SizedBox(height: TrampMetrics.gutter),
                                if (lowerRegion == LowerRegion.equalizer)
                                  equalizer
                                else
                                  SizedBox(
                                    width: layout.logicalWidth,
                                    height: layout.lowerHeight,
                                    child: focusedPlaylist,
                                  ),
                              ],
                            ),
                          ),
                        ),
                      ),
                    ),
                  ),
                );
              },
            ),
          );
        },
      ),
    );

    // Resize edges exist only in playlist mode (ADR 0003); in equalizer mode
    // the window is snapped to the fixed stack and must not edge-resize.
    final shell = lowerRegion == LowerRegion.playlist
        ? DragToResizeArea(resizeEdgeSize: 6, child: content)
        : content;

    final focusedShell = Shortcuts(
      shortcuts: _shortcuts,
      child: Actions(
        actions: _actions(),
        child: Focus(
          autofocus: true,
          child: shell,
        ),
      ),
    );

    if (onDropPaths == null) {
      return focusedShell;
    }

    return DropTarget(
      onDragDone: (details) {
        final paths = details.files
            .map((file) => file.path)
            .where((path) => path.isNotEmpty)
            .toList();

        if (paths.isNotEmpty) {
          onDropPaths!(paths);
        }
      },
      child: focusedShell,
    );
  }
}
