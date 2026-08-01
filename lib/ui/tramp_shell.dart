import 'dart:async';
import 'dart:math' as math;

import 'package:desktop_drop/desktop_drop.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

import '../playback/playback_controller.dart';
import '../playlist/playlist_controller.dart';
import '../theme/tramp_colors.dart';
import 'classic_main_player.dart';

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

class TrampShell extends StatelessWidget {
  const TrampShell({
    super.key,
    required this.transport,
    required this.playlist,
    required this.playback,
    required this.playlistController,
    this.hasTracks = false,
    this.playlistFocusNode,
    this.onDropPaths,
    this.onOpenFiles,
    this.onSavePlaylist,
  });

  final Widget transport;
  final Widget playlist;
  final PlaybackController playback;
  final PlaylistController playlistController;
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
    final shell = DragToResizeArea(
      resizeEdgeSize: 6,
      child: ColoredBox(
        color: TrampColors.metalMid,
        child: DecoratedBox(
          decoration: BoxDecoration(
            border: Border.all(
              color: TrampColors.skinBorder,
              width: TrampColors.borderWidth,
            ),
          ),
          child: LayoutBuilder(
            builder: (context, constraints) {
              final logical = ClassicMainPlayer.logicalSize;
              final widthScale = constraints.maxWidth / logical.width;
              final heightScale = constraints.maxHeight.isFinite
                  ? constraints.maxHeight / logical.height
                  : widthScale;
              final scale = math.min(widthScale, heightScale);
              final playerWidth = logical.width * scale;
              final playerHeight = logical.height * scale;

              return Column(
                children: [
                  Align(
                    alignment: Alignment.topCenter,
                    child: SizedBox(
                      width: playerWidth,
                      height: playerHeight,
                      child: FittedBox(
                        fit: BoxFit.contain,
                        child: transport,
                      ),
                    ),
                  ),
                  Expanded(
                    child: playlistFocusNode == null
                        ? playlist
                        : Focus(
                            focusNode: playlistFocusNode,
                            child: playlist,
                          ),
                  ),
                ],
              );
            },
          ),
        ),
      ),
    );

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
