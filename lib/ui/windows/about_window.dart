import 'dart:async';

import 'package:flutter/material.dart';

import '../../app.dart';
import '../../platform/open_url.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_colors.dart';
import '../../theme/tramp_metrics.dart';
import '../../theme/tramp_text.dart';
import '../chrome/logo.dart';
import '../chrome/mockup/mockup_screen.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../chrome/mockup/mockup_title_bar.dart';
import '../chrome/proxima_logo.dart';
import '../docking/dock_drag_area.dart';

/// About OS window: mockup shell + title bar + credits.
///
/// Freestanding like settings (movable, not snappable, skip taskbar).
class AboutWindow extends StatelessWidget {
  const AboutWindow({
    super.key,
    this.version = trampAppVersion,
    this.copyrightYear = trampCopyrightYear,
    this.companyName = trampCompanyName,
    this.websiteUrl = trampWebsiteUrl,
    this.shaded = false,
    this.onCollapse,
    this.onClose,
    this.onOpenUrl,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.about;

  final String version;
  final int copyrightYear;
  final String companyName;
  final String websiteUrl;
  final bool shaded;
  final VoidCallback? onCollapse;
  final VoidCallback? onClose;
  final ValueChanged<Uri>? onOpenUrl;
  final double zoom;
  final ValueGetter<Offset>? dockLogicalTopLeft;
  final void Function(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  })? onDockMove;
  final VoidCallback? onNativeDragStarted;
  final bool draggableTitle;

  void _openWebsite() {
    final uri = Uri.parse(websiteUrl);
    final opener = onOpenUrl;
    if (opener != null) {
      opener(uri);
    } else {
      unawaited(openExternalUrl(uri));
    }
  }

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
        height: height,
        child: Column(
          children: [
            title,
            if (!shaded)
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.fromLTRB(18, 14, 18, 14),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      Row(
                        children: [
                          const TrampLogo(size: 64),
                          const SizedBox(width: 14),
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
                                const SizedBox(height: 6),
                                Text(
                                  'Version $version',
                                  style: TrampText.lcd(look)
                                      .copyWith(color: colors.labelDim),
                                ),
                                const SizedBox(height: 4),
                                Text(
                                  '© $copyrightYear $companyName',
                                  style: TrampText.lcd(look).copyWith(
                                    color: colors.labelDim,
                                    fontSize: 10,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      Text(
                        'A desktop music player — dense, playlist-centric, '
                        'with distinctive chrome.',
                        style: TrampText.lcd(look).copyWith(
                          color: colors.label,
                          height: 1.35,
                        ),
                      ),
                      const SizedBox(height: 12),
                      Expanded(
                        child: MockupScreen(
                          child: SizedBox.expand(
                            child: Padding(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 16,
                                vertical: 10,
                              ),
                              child: Column(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  const ProximaMagnificaLogo(size: 88),
                                  const SizedBox(height: 6),
                                  Text(
                                    companyName,
                                    style: TrampText.chromeLabel(look).copyWith(
                                      fontSize: 13,
                                      letterSpacing: 1.2,
                                      color: colors.label,
                                    ),
                                  ),
                                  const SizedBox(height: 8),
                                  _AboutLink(
                                    url: websiteUrl,
                                    onTap: _openWebsite,
                                  ),
                                ],
                              ),
                            ),
                          ),
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

class _AboutLink extends StatelessWidget {
  const _AboutLink({required this.url, required this.onTap});

  final String url;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final colors = TrampColors.of(look);
    return Semantics(
      button: true,
      label: url,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        child: GestureDetector(
          key: const Key('about-website'),
          onTap: onTap,
          child: Text(
            url,
            style: TrampText.lcd(look).copyWith(
              color: colors.phosphor,
              decoration: TextDecoration.underline,
              decorationColor: colors.phosphor,
            ),
          ),
        ),
      ),
    );
  }
}
