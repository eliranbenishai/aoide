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

  final bool draggableTitle;

  @override
  Widget build(BuildContext context) {
    Widget title = MockupTitleBar(
      windowName: 'Equalizer',
      showZoom: false,
      onCollapse: onCollapse,
      onClose: onClose,
    );
    if (draggableTitle && onDockMove != null && dockLogicalTopLeft != null) {
      title = DockDragArea(
        zoom: zoom,
        logicalTopLeft: dockLogicalTopLeft!,
        onMove: onDockMove!,
        child: title,
      );
    }

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
              MockupEqualizer(
                settings: settings,
                onSessionCommand: onSessionCommand,
                presetNames: presetNames,
              ),
          ],
        ),
      ),
    );
  }
}
