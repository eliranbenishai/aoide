import 'package:flutter/material.dart';

import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../docking/dock_drag_area.dart';
import '../session/session_messages.dart';
import '../settings/mockup_settings.dart';

/// Settings OS window: mockup shell + title bar + [MockupSettings] body.
///
/// Title: Collapse (shade) · Close (hide). Not snappable; freestanding.
class SettingsWindow extends StatelessWidget {
  const SettingsWindow({
    super.key,
    required this.snapshot,
    this.shaded = false,
    this.onSessionCommand,
    this.onCollapse,
    this.onClose,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.settings;

  final SettingsSnapshotEvent snapshot;
  final bool shaded;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final VoidCallback? onCollapse;
  final VoidCallback? onClose;
  final double zoom;
  final ValueGetter<Offset>? dockLogicalTopLeft;
  final void Function(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  })? onDockMove;
  final VoidCallback? onNativeDragStarted;
  final bool draggableTitle;

  @override
  Widget build(BuildContext context) {
    final draggable =
        draggableTitle && onDockMove != null && dockLogicalTopLeft != null;
    final title = MockupTitleBar(
      windowName: 'Settings',
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
              Expanded(
                child: ClipRect(
                  child: MockupSettings(
                    snapshot: snapshot,
                    onSessionCommand: onSessionCommand,
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
