import 'dart:async';

import 'package:flutter/material.dart';

import '../../app.dart';
import '../../domain/about_stats.dart';
import '../../platform/open_url.dart';
import '../../theme/look_paint.dart';
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
///
/// Laid out as three registers of the same hardware the rest of the chrome
/// imitates: the product on the shell face (badge, wordmark, version readout),
/// the listener's own counters glowing in a CRT well, and the maker's brushed
/// nameplate along the bottom edge.
class AboutWindow extends StatelessWidget {
  const AboutWindow({
    super.key,
    this.version = trampAppVersion,
    this.copyrightYear = trampCopyrightYear,
    this.companyName = trampCompanyName,
    this.freePromise = trampFreePromise,
    this.websiteUrl = trampWebsiteUrl,
    this.stats = AboutStats.unmeasured,
    this.shaded = false,
    this.onCollapse,
    this.onClose,
    this.onOpenUrl,
    this.zoom = 1.0,
    this.dockLogicalTopLeft,
    this.onDockMove,
    this.onNativeDragStarted,
    this.startDragging,
    this.draggableTitle = true,
  });

  static const logicalSize = TrampMetrics.about;

  final String version;
  final int copyrightYear;
  final String companyName;

  /// Sits under the © year in place of the company name — see [trampFreePromise].
  final String freePromise;
  final String websiteUrl;

  /// Measured usage counters for the stats well, pushed by the host over the
  /// session bus. Zeros until that reading arrives — see [AboutStats].
  final AboutStats stats;
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
  final Future<void> Function()? startDragging;
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
        height: height,
        child: Column(
          children: [
            title,
            if (!shaded)
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.fromLTRB(16, 14, 16, 14),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      Row(
                        children: [
                          const _ProductBadge(size: 58),
                          const SizedBox(width: 15),
                          const Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                _Wordmark(size: 28),
                                SizedBox(height: 9),
                                _Backronym(trampBackronym),
                              ],
                            ),
                          ),
                          const SizedBox(width: 10),
                          _VersionWell(version: version),
                        ],
                      ),
                      const SizedBox(height: 12),
                      Text(
                        trampTagline,
                        style: TrampText.lcd(look).copyWith(
                          fontSize: 10.5,
                          height: 1.5,
                          color: colors.label.withValues(alpha: 0.84),
                        ),
                      ),
                      const SizedBox(height: 12),
                      Expanded(child: _StatsWell(stats: stats)),
                      const SizedBox(height: 12),
                      _MakerPlate(
                        companyName: companyName,
                        freePromise: freePromise,
                        copyrightYear: copyrightYear,
                        websiteUrl: websiteUrl,
                        onOpenWebsite: _openWebsite,
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

/// The Tramp logo on a lit disc, as the title bar wears it at control size.
///
/// The artwork is line art over transparency, so it needs the pale disc to
/// read — which also keeps it off the CRT wells, where scanlines would band it.
class _ProductBadge extends StatelessWidget {
  const _ProductBadge({required this.size});

  final double size;

  @override
  Widget build(BuildContext context) {
    final palette = LookScope.of(context).palette;
    return SizedBox.square(
      dimension: size,
      child: Stack(
        alignment: Alignment.center,
        children: [
          DecoratedBox(
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              boxShadow: [
                BoxShadow(
                  color: palette.accentDefault.withValues(alpha: 0.3),
                  blurRadius: size * 0.42,
                ),
                const BoxShadow(
                  color: Color(0x99000000),
                  offset: Offset(0, 3),
                  blurRadius: 7,
                ),
              ],
            ),
            child: SizedBox.square(dimension: size),
          ),
          ClipOval(
            child: ColoredBox(
              color: const Color(0xFFE9ECF4),
              child: Transform.scale(
                scale: 1.12,
                child: TrampLogo(size: size),
              ),
            ),
          ),
          IgnorePointer(
            child: DecoratedBox(
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                border: Border.all(color: const Color(0xA6000000)),
              ),
              child: SizedBox.square(dimension: size),
            ),
          ),
        ],
      ),
    );
  }
}

/// TRAMP set like the title-bar wordmark: cool ink, bevel, phosphor bloom.
class _Wordmark extends StatelessWidget {
  const _Wordmark({required this.size});

  final double size;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return Text(
      'TRAMP',
      style: TextStyle(
        fontFamily: look.chromeFamily,
        fontWeight: FontWeight.w700,
        fontSize: size,
        height: 1,
        letterSpacing: size * 0.22,
        decoration: TextDecoration.none,
        color: LookPaint.wordmark(palette),
        shadows: [
          Shadow(
            offset: const Offset(0, -1),
            color: LookPaint.hoverLiftTarget(palette).withValues(alpha: 0.3),
          ),
          const Shadow(offset: Offset(0, 2), color: Color(0xD9000000)),
          Shadow(
            color: palette.phosphorDefault.withValues(alpha: 0.34),
            blurRadius: 10,
          ),
        ],
      ),
    );
  }
}

/// The backronym with each word's initial lit, so TRAMP reads out of the line.
class _Backronym extends StatelessWidget {
  const _Backronym(this.text);

  final String text;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final base = TrampText.chromeLabel(look).copyWith(
      fontSize: 10,
      letterSpacing: 1.9,
      color: palette.inkDim,
    );
    final initial = base.copyWith(
      color: palette.phosphorDefault,
      shadows: [
        Shadow(
          color: palette.phosphorDefault.withValues(alpha: 0.45),
          blurRadius: 7,
        ),
      ],
    );

    final words = text.toUpperCase().split(' ');
    return Text.rich(
      TextSpan(
        children: [
          for (final (index, word) in words.indexed) ...[
            if (index > 0) const TextSpan(text: ' '),
            TextSpan(text: word.substring(0, 1), style: initial),
            TextSpan(text: word.substring(1)),
          ],
        ],
      ),
      style: base,
      semanticsLabel: text,
    );
  }
}

/// Version as a panel-mounted readout rather than a line of body text.
class _VersionWell extends StatelessWidget {
  const _VersionWell({required this.version});

  final String version;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return DecoratedBox(
      decoration: BoxDecoration(
        borderRadius: BorderRadius.circular(2),
        color: palette.well,
        border: Border.all(color: const Color(0xB3000000)),
        boxShadow: [
          BoxShadow(
            color: LookPaint.coolSheen(palette).withValues(alpha: 0.1),
            offset: const Offset(0, 1),
          ),
        ],
      ),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(7, 4, 7, 4),
        child: Text(
          'V $version',
          style: TrampText.lcd(look).copyWith(
            fontSize: 10,
            letterSpacing: 1,
            color: palette.phosphorDefault,
            shadows: [
              Shadow(
                color: palette.phosphorDefault.withValues(alpha: 0.55),
                blurRadius: 7,
              ),
            ],
          ),
        ),
      ),
    );
  }
}

/// Usage counters in a CRT well — the hour meter on the front of the machine.
class _StatsWell extends StatelessWidget {
  const _StatsWell({required this.stats});

  final AboutStats stats;

  @override
  Widget build(BuildContext context) {
    return MockupScreen(
      child: SizedBox.expand(
        child: Padding(
          padding: const EdgeInsets.fromLTRB(18, 11, 18, 11),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              const Center(child: _Kicker('ON THIS MACHINE')),
              const SizedBox(height: 9),
              _StatRow('PLAYLISTS', stats.playlistsLabel),
              _StatRow('TRACKS', stats.tracksLabel),
              _StatRow('TOTAL TIME', stats.totalTimeLabel),
              _StatRow('SPINS', stats.spinsLabel),
            ],
          ),
        ),
      ),
    );
  }
}

/// One counter: engraved label, dotted leader, phosphor value.
class _StatRow extends StatelessWidget {
  const _StatRow(this.label, this.value);

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return Expanded(
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          Text(
            label,
            style: TrampText.chromeLabel(look).copyWith(
              fontSize: 9.5,
              letterSpacing: 1.8,
              color: palette.inkDim,
            ),
          ),
          const Expanded(child: _Leader()),
          Text(
            value,
            style: TrampText.lcd(look).copyWith(
              fontSize: 10.5,
              color: palette.phosphorDefault.withValues(alpha: 0.9),
              shadows: [
                Shadow(
                  color: palette.phosphorDefault.withValues(alpha: 0.3),
                  blurRadius: 6,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

/// Dotted leader between a label and its value.
class _Leader extends StatelessWidget {
  const _Leader();

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 9),
      child: CustomPaint(
        painter: _LeaderPainter(colour: LookScope.of(context).palette.inkDim),
        child: const SizedBox(height: 1, width: double.infinity),
      ),
    );
  }
}

class _LeaderPainter extends CustomPainter {
  const _LeaderPainter({required this.colour});

  final Color colour;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()..color = colour.withValues(alpha: 0.4);
    final y = size.height / 2;
    // Right-aligned so the dots always meet the value, whatever the label.
    for (var x = size.width; x >= 0; x -= 4) {
      canvas.drawRect(Rect.fromLTWH(x, y, 1, 1), paint);
    }
  }

  @override
  bool shouldRepaint(covariant _LeaderPainter oldDelegate) =>
      colour != oldDelegate.colour;
}

/// Spaced small-caps line, used for kickers inside the wells.
class _Kicker extends StatelessWidget {
  const _Kicker(this.text);

  final String text;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    return Text(
      text,
      style: TrampText.chromeLabel(look).copyWith(
        fontSize: 9,
        letterSpacing: 2.6,
        color: look.palette.phosphorDefault.withValues(alpha: 0.6),
      ),
    );
  }
}

/// The maker's plate: company mark, name, copyright, website.
///
/// Bolted along the bottom edge, where the plate sits on the real hardware.
class _MakerPlate extends StatelessWidget {
  const _MakerPlate({
    required this.companyName,
    required this.freePromise,
    required this.copyrightYear,
    required this.websiteUrl,
    required this.onOpenWebsite,
  });

  final String companyName;
  final String freePromise;
  final int copyrightYear;
  final String websiteUrl;
  final VoidCallback onOpenWebsite;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return SizedBox(
      height: 48,
      child: DecoratedBox(
        decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(4),
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [LookPaint.plateFace(palette), palette.shellLow],
          ),
          border: Border.all(color: const Color(0x99000000)),
          boxShadow: [
            BoxShadow(
              color: LookPaint.coolSheen(palette).withValues(alpha: 0.06),
              offset: const Offset(0, -1),
            ),
          ],
        ),
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 13),
          child: Row(
            children: [
              const ProximaMagnificaMark(height: 27),
              const SizedBox(width: 12),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Text(
                      companyName.toUpperCase(),
                      style: TrampText.chromeLabel(look).copyWith(
                        fontSize: 11.5,
                        letterSpacing: 2.4,
                        color: palette.inkDefault.withValues(alpha: 0.92),
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      '© $copyrightYear $freePromise',
                      style: TrampText.lcd(look).copyWith(
                        fontSize: 9,
                        color: palette.inkDim,
                      ),
                    ),
                  ],
                ),
              ),
              _WebsiteChip(url: websiteUrl, onTap: onOpenWebsite),
            ],
          ),
        ),
      ),
    );
  }
}

/// Website link as a lit phosphor chip on the plate.
class _WebsiteChip extends StatefulWidget {
  const _WebsiteChip({required this.url, required this.onTap});

  final String url;
  final VoidCallback onTap;

  @override
  State<_WebsiteChip> createState() => _WebsiteChipState();
}

class _WebsiteChipState extends State<_WebsiteChip> {
  bool _hover = false;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final lit = _hover ? 1.0 : 0.0;
    return Semantics(
      button: true,
      label: widget.url,
      child: MouseRegion(
        cursor: SystemMouseCursors.click,
        onEnter: (_) => setState(() => _hover = true),
        onExit: (_) => setState(() => _hover = false),
        child: GestureDetector(
          key: const Key('about-website'),
          onTap: widget.onTap,
          child: DecoratedBox(
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(3),
              color: palette.well.withValues(alpha: 0.85),
              border: Border.all(
                color: palette.phosphorDefault
                    .withValues(alpha: 0.28 + 0.34 * lit),
              ),
              boxShadow: [
                BoxShadow(
                  color: palette.phosphorDefault
                      .withValues(alpha: 0.1 + 0.14 * lit),
                  blurRadius: 8,
                ),
              ],
            ),
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 9, vertical: 5),
              child: Text(
                _displayUrl(widget.url),
                style: TrampText.lcd(look).copyWith(
                  fontSize: 10.5,
                  letterSpacing: 0.4,
                  color: palette.phosphorDefault,
                  shadows: [
                    Shadow(
                      color: palette.phosphorDefault
                          .withValues(alpha: 0.5 + 0.3 * lit),
                      blurRadius: 7,
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }

  /// The scheme is noise on a chip this small; the tap still opens the full URL.
  static String _displayUrl(String url) =>
      url.replaceFirst(RegExp(r'^https?://'), '');
}
