import 'package:flutter/material.dart';

import '../../domain/equalizer_settings.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../docking/dock_drag_area.dart';
import '../equalizer/mockup_equalizer.dart';
import '../session/session_messages.dart';

/// Full equalizer window: mockup shell + title bar + [MockupEqualizer] body.
///
/// Title: Collapse (shade) · Close (hide). No per-window zoom.
class EqualizerWindow extends StatelessWidget {
  const EqualizerWindow({
    super.key,
    required this.settings,
    this.shaded = false,
    this.onSessionCommand,
    this.presetNames = const [],
    this.onCollapse,
    this.onClose,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.startDragging,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.equalizer;

  final EqualizerSettings settings;
  final bool shaded;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final List<String> presetNames;
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

  /// Override for this window's HWND; defaults to [windowManager.startDragging].
  final Future<void> Function()? startDragging;

  final bool draggableTitle;

  @override
  Widget build(BuildContext context) {
    final draggable =
        draggableTitle && onDockMove != null && dockLogicalTopLeft != null;
    final title = MockupTitleBar(
      windowName: 'Equalizer',
      showBrand: false,
      showZoom: false,
      onCollapse: onCollapse,
      onClose: onClose,
      wrapDragRegion: draggable
          ? (region) => DockDragArea(
                zoom: zoom,
                logicalTopLeft: dockLogicalTopLeft!,
                onMove: onDockMove!,
                onNativeDragStarted: onNativeDragStarted,
                startDragging: startDragging,
                child: region,
              )
          : null,
    );

    final height = shaded ? TrampMetrics.titleBar : logicalSize.height;

    return SizedBox(
      width: logicalSize.width,
      height: height,
      child: MockupShell(
        width: logicalSize.width,
        child: Column(
          children: [
            title,
            if (!shaded)
              // Expanded + clip: OS frame height can be a fraction of a logical
              // pixel short of 348 after snap/DPI rounding (overflowed by ~0.67px).
              Expanded(
                child: ClipRect(
                  child: Align(
                    alignment: Alignment.topCenter,
                    child: MockupEqualizer(
                      settings: settings,
                      onSessionCommand: onSessionCommand,
                      presetNames: presetNames,
                    ),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
