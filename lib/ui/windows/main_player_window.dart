import 'package:flutter/material.dart';

import '../../playback/playback_controller.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../docking/dock_drag_area.dart';
import '../main_player/mockup_main_player.dart';
import '../session/session_messages.dart';

/// Full main player window: mockup shell + title bar + [MockupMainPlayer] body.
class MainPlayerWindow extends StatelessWidget {
  const MainPlayerWindow({
    super.key,
    required this.playback,
    required this.trackCount,
    this.forceMono = false,
    this.alwaysOnTop = false,
    this.equalizerVisible = true,
    this.playlistVisible = true,
    this.onSessionCommand,
    this.onOpenFiles,
    this.onOpenOptions,
    this.onShowTrackInfo,
    this.onMinimize,
    this.onZoomOut,
    this.onZoomIn,
    this.onClose,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.draggableTitle = true,
    this.spectrumBars,
    this.spectrumPeaks,
  });

  static const logicalSize = TrampMetrics.mainPlayer;

  final PlaybackController playback;
  final int trackCount;
  final bool forceMono;
  final bool alwaysOnTop;
  final bool equalizerVisible;
  final bool playlistVisible;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final VoidCallback? onOpenFiles;
  final VoidCallback? onOpenOptions;
  final VoidCallback? onShowTrackInfo;
  final VoidCallback? onMinimize;
  final VoidCallback? onZoomOut;
  final VoidCallback? onZoomIn;
  final VoidCallback? onClose;

  /// Global zoom factor for docking drag → logical conversion.
  final double zoom;

  /// Logical top-left at drag start (host [DockingCoordinator] frame).
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
  final List<double>? spectrumBars;
  final List<double>? spectrumPeaks;

  @override
  Widget build(BuildContext context) {
    final draggable = draggableTitle &&
        onDockMove != null &&
        dockLogicalTopLeft != null;
    final title = MockupTitleBar(
      windowName: 'Main Player',
      onMinimize: onMinimize,
      onZoomOut: onZoomOut,
      onZoomIn: onZoomIn,
      onClose: onClose,
      wrapDragRegion: draggable
          ? (region) => DockDragArea(
                zoom: zoom,
                logicalTopLeft: dockLogicalTopLeft!,
                onMove: onDockMove!,
                onNativeDragStarted: onNativeDragStarted,
                child: region,
              )
          : null,
    );

    return SizedBox(
      width: logicalSize.width,
      height: logicalSize.height,
      child: MockupShell(
        width: logicalSize.width,
        child: Column(
          children: [
            title,
            MockupMainPlayer(
              playback: playback,
              trackCount: trackCount,
              forceMono: forceMono,
              alwaysOnTop: alwaysOnTop,
              equalizerVisible: equalizerVisible,
              playlistVisible: playlistVisible,
              onSessionCommand: onSessionCommand,
              onOpenFiles: onOpenFiles,
              onOpenOptions: onOpenOptions,
              onShowTrackInfo: onShowTrackInfo,
              spectrumBars: spectrumBars,
              spectrumPeaks: spectrumPeaks,
            ),
          ],
        ),
      ),
    );
  }
}
