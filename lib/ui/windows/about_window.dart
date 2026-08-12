import 'package:flutter/material.dart';

import '../../theme/look_scope.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/logo.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../docking/dock_drag_area.dart';

/// About OS window: mockup shell + title bar + brand blurb.
///
/// Freestanding like settings (movable, not snappable, skip taskbar).
class AboutWindow extends StatelessWidget {
  const AboutWindow({
    super.key,
    this.version = '0.1.0',
    this.shaded = false,
    this.onCollapse,
    this.onClose,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.about;

  final String version;
  final bool shaded;
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
    final look = LookScope.of(context);
    final colors = TrampColors.of(look);
    final draggable =
        draggableTitle && onDockMove != null && dockLogicalTopLeft != null;
    final title = MockupTitleBar(
      windowName: 'About',
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
                child: Padding(
                  padding: const EdgeInsets.fromLTRB(20, 16, 20, 16),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          const TrampLogo(size: 56),
                          const SizedBox(width: 16),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  'TRAMP',
                                  style: TrampText.chromeLabel(look).copyWith(
                                    fontSize: 22,
                                    letterSpacing: 4,
                                    color: colors.railAccent,
                                  ),
                                ),
                                const SizedBox(height: 4),
                                Text(
                                  'Version $version',
                                  style: TrampText.lcd(look)
                                      .copyWith(color: colors.labelDim),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                      Text(
                        'A desktop music player — dense, playlist-centric, '
                        'with distinctive chrome.',
                        style: TrampText.lcd(look).copyWith(
                          color: colors.label,
                          height: 1.35,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
