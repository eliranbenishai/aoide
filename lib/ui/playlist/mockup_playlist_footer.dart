import 'package:flutter/material.dart';
import 'package:path/path.dart' as p;

import '../../playlist/playlist_controller.dart';
import '../../theme/look_paint.dart';
import '../../theme/look_scope.dart';
import '../../ui/format.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_icons.dart';
import '../chrome/mockup/mockup_popup_menu.dart';
import '../chrome/mockup/mockup_screen.dart';
import '../chrome/mockup/mockup_shell.dart';

/// The button strip and status line under the playlist's track list.
///
/// Layout matches `player-mockup-2.html` `.win--pl` footer: add / remove / sort
/// / options, a mini transport, the TOTAL readout, and the status line beneath.
class MockupPlaylistFooter extends StatefulWidget {
  const MockupPlaylistFooter({
    super.key,
    required this.playlist,
    required this.playing,
    this.playingIndex,
    this.onAdd,
    this.onRemove,
    this.onSort,
    this.onOption,
    this.onPrevious,
    this.onPlayPause,
    this.onNext,
  });

  static const double _strip = 74;
  static const double _gap = 10;
  static const double _statusLine = 26;

  /// Bottom-anchored footer height — no spare band under the status row.
  static const double height = _strip + _gap + _statusLine;

  final PlaylistController playlist;
  final bool playing;
  final int? playingIndex;
  final VoidCallback? onAdd;
  final VoidCallback? onRemove;
  final ValueChanged<String>? onSort;
  final ValueChanged<String>? onOption;
  final VoidCallback? onPrevious;
  final VoidCallback? onPlayPause;
  final VoidCallback? onNext;

  @override
  State<MockupPlaylistFooter> createState() => _MockupPlaylistFooterState();
}

class _MockupPlaylistFooterState extends State<MockupPlaylistFooter> {
  bool _sortMenuOpen = false;
  bool _optionsMenuOpen = false;

  Duration get _total {
    var sum = Duration.zero;
    for (final t in widget.playlist.playlist.tracks) {
      final d = t.duration;
      if (d != null) sum += d;
    }
    return sum;
  }

  String get _statusName {
    final path = widget.playlist.playlist.sourcePath;
    if (path == null || path.isEmpty) return 'untitled playlist';
    return p.basename(path);
  }

  Future<void> _openSortMenu(BuildContext context) async {
    final box = context.findRenderObject() as RenderBox?;
    if (box == null) return;
    // Capture styles — popup routes sit above the local LookScope in tests.
    final look = LookScope.of(context);
    final labelStyle = TextStyle(
      color: look.palette.inkDefault,
      fontFamily: look.chromeFamily,
      fontWeight: FontWeight.w700,
      fontSize: 13,
      letterSpacing: 13 * 0.12,
    );
    setState(() => _sortMenuOpen = true);
    String? selected;
    try {
      selected = await showMockupMenu<String>(
        context: context,
        anchor: box,
        placement: MockupMenuPlacement.above,
        color: look.palette.shellMid,
        items: [
          PopupMenuItem(value: 'title', child: Text('Title', style: labelStyle)),
          PopupMenuItem(value: 'artist', child: Text('Artist', style: labelStyle)),
          PopupMenuItem(
            value: 'duration',
            child: Text('Duration', style: labelStyle),
          ),
          PopupMenuItem(value: 'path', child: Text('Path', style: labelStyle)),
          PopupMenuItem(
            value: 'reverse',
            child: Text('Reverse', style: labelStyle),
          ),
        ],
      );
    } finally {
      if (mounted) setState(() => _sortMenuOpen = false);
    }
    if (selected != null) widget.onSort?.call(selected);
  }

  Future<void> _openOptionsMenu(BuildContext context) async {
    final box = context.findRenderObject() as RenderBox?;
    if (box == null) return;
    final look = LookScope.of(context);
    final labelStyle = TextStyle(
      color: look.palette.inkDefault,
      fontFamily: look.chromeFamily,
      fontWeight: FontWeight.w700,
      fontSize: 13,
      letterSpacing: 13 * 0.12,
    );
    setState(() => _optionsMenuOpen = true);
    String? selected;
    try {
      selected = await showMockupMenu<String>(
        context: context,
        anchor: box,
        placement: MockupMenuPlacement.above,
        color: look.palette.shellMid,
        items: [
          PopupMenuItem(
            value: 'load',
            child: Text('Load playlist…', style: labelStyle),
          ),
          PopupMenuItem(
            value: 'save',
            child: Text('Save playlist…', style: labelStyle),
          ),
          PopupMenuItem(value: 'clear', child: Text('Clear', style: labelStyle)),
          PopupMenuItem(
            value: 'selectAll',
            child: Text('Select all', style: labelStyle),
          ),
          PopupMenuItem(
            value: 'invertSelection',
            child: Text('Invert selection', style: labelStyle),
          ),
        ],
      );
    } finally {
      if (mounted) setState(() => _optionsMenuOpen = false);
    }
    if (selected != null) widget.onOption?.call(selected);
  }

  @override
  Widget build(BuildContext context) {
    final count = widget.playlist.playlist.tracks.length;
    final statusPlaying = widget.playingIndex;

    return SizedBox(
      height: MockupPlaylistFooter.height,
      child: Column(
        children: [
          SizedBox(
            height: MockupPlaylistFooter._strip,
            child: MockupPlate(
              child: Padding(
                padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                child: Row(
                  children: [
                    MockupButton(
                      key: const Key('pl-add'),
                      width: 52,
                      height: 52,
                      semanticLabel: 'Add tracks',
                      onPressed: widget.onAdd,
                      child: MockupIcons.add(size: 21, color: MockupIcons.inkOf(context)),
                    ),
                    const SizedBox(width: 8),
                    MockupButton(
                      key: const Key('pl-remove'),
                      width: 52,
                      height: 52,
                      semanticLabel: 'Remove selected tracks',
                      onPressed: widget.onRemove,
                      child: MockupIcons.remove(size: 21, color: MockupIcons.inkOf(context)),
                    ),
                    const SizedBox(width: 14),
                    const _FooterSep(),
                    const SizedBox(width: 14),
                    Builder(
                      builder: (ctx) => MockupButton(
                        key: const Key('pl-sort'),
                        width: 52,
                        height: 52,
                        menu: true,
                        on: _sortMenuOpen,
                        semanticLabel: 'Sort playlist',
                        onPressed: () => _openSortMenu(ctx),
                        child: MockupIcons.sort(size: 21, color: MockupIcons.inkOf(context)),
                      ),
                    ),
                    const SizedBox(width: 8),
                    Builder(
                      builder: (ctx) => MockupButton(
                        key: const Key('pl-options'),
                        width: 52,
                        height: 52,
                        menu: true,
                        on: _optionsMenuOpen,
                        semanticLabel: 'Playlist options',
                        onPressed: () => _openOptionsMenu(ctx),
                        child: MockupIcons.options(size: 21, color: MockupIcons.inkOf(context)),
                      ),
                    ),
                    const SizedBox(width: 8),
                    // minWidth 0 so the rail can fully collapse before TOTAL
                    // / buttons are forced into overflow.
                    const Expanded(
                      child: MockupRail(height: 52, minWidth: 0),
                    ),
                    const SizedBox(width: 8),
                    MockupButton(
                      key: const Key('pl-prev'),
                      width: 52,
                      height: 52,
                      semanticLabel: 'Previous',
                      onPressed: widget.onPrevious,
                      child: MockupIcons.previous(size: 18, color: MockupIcons.inkOf(context)),
                    ),
                    const SizedBox(width: 8),
                    MockupButton(
                      key: const Key('pl-play'),
                      width: 52,
                      height: 52,
                      semanticLabel: widget.playing ? 'Pause' : 'Play',
                      on: widget.playing,
                      onPressed: widget.onPlayPause,
                      child: widget.playing
                          ? MockupIcons.pause(size: 18, color: MockupIcons.inkOf(context))
                          : MockupIcons.play(size: 18, color: MockupIcons.inkOf(context)),
                    ),
                    const SizedBox(width: 8),
                    MockupButton(
                      key: const Key('pl-next'),
                      width: 52,
                      height: 52,
                      semanticLabel: 'Next',
                      onPressed: widget.onNext,
                      child: MockupIcons.next(size: 18, color: MockupIcons.inkOf(context)),
                    ),
                    const SizedBox(width: 8),
                    MockupScreen(
                      child: Padding(
                        padding: const EdgeInsets.symmetric(
                          horizontal: 18,
                          vertical: 8,
                        ),
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          crossAxisAlignment: CrossAxisAlignment.baseline,
                          textBaseline: TextBaseline.alphabetic,
                          children: [
                            Text(
                              'TOTAL',
                              style: TextStyle(
                                fontFamily: LookScope.of(context).chromeFamily,
                                fontWeight: FontWeight.w700,
                                fontSize: 11,
                                letterSpacing: 11 * 0.2,
                                color: LookScope.of(context).palette.phosphorDim,
                              ),
                            ),
                            const SizedBox(width: 12),
                            Text(
                              formatDuration(_total),
                              style: TextStyle(
                                fontFamily: LookScope.of(context).lcdFamily,
                                fontWeight: FontWeight.w500,
                                fontSize: 18,
                                decoration: TextDecoration.none,
                                color: LookScope.of(context).palette.phosphorDefault,
                                shadows: [
                                  // `.glow` — keep tight vs Skia bloom
                                  Shadow(
                                    color: Color(0x733DE7FF),
                                    blurRadius: 4,
                                  ),
                                ],
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
          const SizedBox(height: MockupPlaylistFooter._gap),
          SizedBox(
            height: MockupPlaylistFooter._statusLine,
            child: Padding(
              // Trailing inset clears the window resize grip (bottom-right).
              padding: const EdgeInsets.fromLTRB(6, 0, 22, 0),
              child: Row(
                children: [
                  Flexible(
                    child: Text(
                      _statusName.toUpperCase(),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: _statusStyle(context),
                    ),
                  ),
                  const _StatusDot(),
                  Text(
                    '$count TRACK${count == 1 ? '' : 'S'}',
                    style: _statusStyle(context),
                  ),
                  const _StatusDot(),
                  Text(
                    statusPlaying == null
                        ? 'STOPPED'
                        : 'PLAYING ${statusPlaying + 1}',
                    style: _statusStyle(context),
                  ),
                  const _StatusDot(),
                  const Spacer(),
                  Text(
                    'DROP FILES HERE TO ENQUEUE',
                    style: _statusStyle(context),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _FooterSep extends StatelessWidget {
  const _FooterSep();

  @override
  Widget build(BuildContext context) {
    final sheen = LookPaint.coolSheen(LookScope.of(context).palette);
    return Container(
      width: 1,
      height: 40,
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topCenter,
          end: Alignment.bottomCenter,
          colors: [
            const Color(0xB3000000),
            sheen.withValues(alpha: 0x1F / 255),
            const Color(0xB3000000),
          ],
        ),
      ),
    );
  }
}

class _StatusDot extends StatelessWidget {
  const _StatusDot();

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 18),
      child: SizedBox(
        width: 5,
        height: 5,
        child: DecoratedBox(
          decoration: BoxDecoration(
            color: LookScope.of(context).palette.inkFaint,
            shape: BoxShape.circle,
          ),
        ),
      ),
    );
  }
}

TextStyle _statusStyle(BuildContext context) => TextStyle(
      fontFamily: LookScope.of(context).chromeFamily,
      fontWeight: FontWeight.w700,
      fontSize: 12,
      letterSpacing: 12 * 0.18,
      color: LookScope.of(context).palette.inkFaint,
    );
