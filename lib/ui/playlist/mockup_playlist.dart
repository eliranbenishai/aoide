import 'package:desktop_drop/desktop_drop.dart';
import 'package:flutter/material.dart';
import 'package:path/path.dart' as p;

import '../../domain/track.dart';
import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_sort.dart';
import '../../ui/format.dart';
import '../chrome/logo.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_icons.dart';
import '../chrome/mockup/mockup_popup_menu.dart';
import '../chrome/mockup/mockup_screen.dart';
import '../chrome/mockup/mockup_shell.dart';
import '../session/session_messages.dart';
import 'mockup_playlist_scrollbar.dart';
import '../../theme/look_paint.dart';
import '../../theme/look_scope.dart';

/// Mockup-faithful playlist body (grows with window; footer bottom-anchored).
///
/// Layout matches `player-mockup-2.html` `.win--pl .body`: list well + footer
/// strip (add/remove/sort/options · mini transport · TOTAL) + status line.
class MockupPlaylist extends StatelessWidget {
  const MockupPlaylist({
    super.key,
    required this.playlist,
    this.playingIndex,
    this.playing = false,
    this.onSessionCommand,
    this.onAddFiles,
    this.onLoadPlaylist,
    this.onSavePlaylist,
    this.onDropPaths,
  });

  final PlaylistController playlist;
  final int? playingIndex;
  final bool playing;
  final ValueChanged<SessionCommand>? onSessionCommand;
  final VoidCallback? onAddFiles;
  final VoidCallback? onLoadPlaylist;
  final VoidCallback? onSavePlaylist;
  final void Function(List<String> paths)? onDropPaths;

  void _emit(SessionCommand command) => onSessionCommand?.call(command);

  @override
  Widget build(BuildContext context) {
    Widget body = ListenableBuilder(
      listenable: playlist,
      builder: (context, _) {
        return Padding(
          padding: const EdgeInsets.all(12),
          child: Column(
            children: [
              Expanded(
                child: _PlaylistWell(
                  playlist: playlist,
                  playingIndex: playingIndex,
                  onSelect: (index) {
                    playlist.select(index);
                    _emit(PlaylistOpCommand('select', index: index));
                  },
                  onActivate: (index) {
                    playlist.select(index);
                    _emit(PlaylistOpCommand('playIndex', index: index));
                  },
                ),
              ),
              const SizedBox(height: 10),
              _PlaylistFooter(
                playlist: playlist,
                playing: playing,
                playingIndex: playingIndex,
                onAdd: onAddFiles,
                onRemove: () {
                  playlist.removeSelected();
                  _emit(const PlaylistOpCommand('removeSelected'));
                },
                onSort: (key) {
                  if (key == 'reverse') {
                    playlist.reverseTracks();
                    _emit(const PlaylistOpCommand('reverse'));
                    return;
                  }
                  final sortKey = PlaylistSortKey.values.asNameMap()[key];
                  if (sortKey != null) {
                    playlist.sortBy(sortKey);
                  }
                  _emit(PlaylistOpCommand('sort', sortKey: key));
                },
                onOption: (op) {
                  switch (op) {
                    case 'load':
                      onLoadPlaylist?.call();
                    case 'save':
                      onSavePlaylist?.call();
                    case 'clear':
                      playlist.clear();
                      _emit(const PlaylistOpCommand('clear'));
                    case 'selectAll':
                      playlist.selectAll();
                      _emit(const PlaylistOpCommand('selectAll'));
                    case 'invertSelection':
                      playlist.invertSelection();
                      _emit(const PlaylistOpCommand('invertSelection'));
                  }
                },
                onPrevious: () => _emit(const TransportCommand('previous')),
                onPlayPause: () => _emit(const TransportCommand('playPause')),
                onNext: () => _emit(const TransportCommand('next')),
              ),
            ],
          ),
        );
      },
    );

    if (onDropPaths != null) {
      body = DropTarget(
        onDragDone: (details) {
          final paths = details.files
              .map((f) => f.path)
              .where((path) => path.isNotEmpty)
              .toList();
          if (paths.isNotEmpty) onDropPaths!(paths);
        },
        child: body,
      );
    }

    return body;
  }
}

class _PlaylistWell extends StatefulWidget {
  const _PlaylistWell({
    required this.playlist,
    required this.playingIndex,
    required this.onSelect,
    required this.onActivate,
  });

  final PlaylistController playlist;
  final int? playingIndex;
  final ValueChanged<int> onSelect;
  final ValueChanged<int> onActivate;

  @override
  State<_PlaylistWell> createState() => _PlaylistWellState();
}

class _PlaylistWellState extends State<_PlaylistWell> {
  final _scrollController = ScrollController();

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final tracks = widget.playlist.playlist.tracks;
    final selected = widget.playlist.selectedIndices;

    // List well + 10px gutter + 14px scrollbar (mockup: track outside `.list`).
    return Row(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Expanded(
          child: ClipRRect(
            borderRadius: BorderRadius.circular(3),
            child: Stack(
              children: [
                const Positioned.fill(
                  child: IgnorePointer(
                    child: CustomPaint(painter: _ListWellPainter()),
                  ),
                ),
                if (tracks.isEmpty)
                  Center(
                    child: Text(
                      'DROP FILES TO ENQUEUE',
                      style: TextStyle(
                        fontFamily: LookScope.of(context).chromeFamily,
                        fontWeight: FontWeight.w700,
                        fontSize: 13,
                        letterSpacing: 13 * 0.18,
                        color: LookScope.of(context).palette.inkFaint,
                      ),
                    ),
                  )
                else
                  Positioned.fill(
                    child: ScrollConfiguration(
                      behavior: ScrollConfiguration.of(context).copyWith(
                        scrollbars: false,
                      ),
                      // Non-interactive transparent shell keeps desktop row
                      // hit-testing; MockupPlaylistScrollbar draws the track.
                      child: RawScrollbar(
                        controller: _scrollController,
                        interactive: false,
                        thickness: 14,
                        radius: const Radius.circular(999),
                        thumbColor: const Color(0x00000000),
                        trackColor: const Color(0x00000000),
                        trackVisibility: true,
                        thumbVisibility: true,
                        child: ListView.builder(
                          controller: _scrollController,
                          padding: const EdgeInsets.symmetric(vertical: 6),
                          itemCount: tracks.length,
                          itemExtent: 37,
                          itemBuilder: (context, index) {
                            final track = tracks[index];
                            return _PlaylistRow(
                              key: Key('pl-row-$index'),
                              index: index,
                              track: track,
                              selected: selected.contains(index),
                              playing: widget.playingIndex == index,
                              onSelect: () => widget.onSelect(index),
                              onActivate: () => widget.onActivate(index),
                            );
                          },
                        ),
                      ),
                    ),
                  ),
                const Positioned(
                  right: 26,
                  bottom: 8,
                  child: IgnorePointer(
                    child: Opacity(
                      opacity: 0.05,
                      child: TrampLogo(size: 178),
                    ),
                  ),
                ),
                const Positioned.fill(
                  child: IgnorePointer(
                    child: CustomPaint(painter: _ListScanlinePainter()),
                  ),
                ),
              ],
            ),
          ),
        ),
        if (tracks.isNotEmpty) ...[
          const SizedBox(width: 10),
          SizedBox(
            width: 14,
            child: MockupPlaylistScrollbar(controller: _scrollController),
          ),
        ],
      ],
    );
  }
}

class _PlaylistRow extends StatelessWidget {
  const _PlaylistRow({
    super.key,
    required this.index,
    required this.track,
    required this.selected,
    required this.playing,
    required this.onSelect,
    required this.onActivate,
  });

  final int index;
  final Track track;
  final bool selected;
  final bool playing;
  final VoidCallback onSelect;
  final VoidCallback onActivate;

  String get _label {
    final artist = track.artist?.trim();
    if (artist != null && artist.isNotEmpty) {
      return '$artist — ${track.displayTitle}';
    }
    return track.displayTitle;
  }

  @override
  Widget build(BuildContext context) {
    final Color color;
    if (playing) {
      color = LookScope.of(context).palette.phosphorHot;
    } else if (selected) {
      color = const Color(0xE0D6F4FF);
    } else {
      color = const Color(0x739AE2F0);
    }

    return SizedBox(
      height: 37,
      width: double.infinity,
      child: Semantics(
        selected: selected,
        button: true,
        label: _label,
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTap: onSelect,
          onDoubleTap: onActivate,
          child: DecoratedBox(
              decoration: BoxDecoration(
                gradient: selected
                    ? LinearGradient(
                        begin: Alignment.topCenter,
                        end: Alignment.bottomCenter,
                        colors: [
                          Color(0x213DE7FF),
                          Color(0x0A3DE7FF),
                        ],
                      )
                    : null,
              ),
              child: Stack(
                fit: StackFit.expand,
                children: [
                  if (playing)
                    Positioned(
                      left: 0,
                      top: 6,
                      bottom: 6,
                      child: DecoratedBox(
                        decoration: BoxDecoration(
                          color: LookScope.of(context).palette.accentDefault,
                          borderRadius: const BorderRadius.horizontal(
                            right: Radius.circular(2),
                          ),
                          boxShadow: const [
                            BoxShadow(
                              color: Color(0xE6FF3D9A),
                              blurRadius: 12,
                            ),
                          ],
                        ),
                        child: const SizedBox(width: 3),
                      ),
                    ),
                  Padding(
                    padding: const EdgeInsets.fromLTRB(16, 0, 52, 0),
                    child: Row(
                      children: [
                        SizedBox(
                          width: 34,
                          child: Text(
                            '${index + 1}.',
                            textAlign: TextAlign.right,
                            style: TextStyle(
                              fontFamily: LookScope.of(context).lcdFamily,
                              fontWeight: FontWeight.w500,
                              fontSize: 15,
                              color: playing
                                  ? LookScope.of(context).palette.phosphorDefault
                                  : color.withValues(alpha: 0.7),
                              shadows: playing
                                  ? const [
                                      Shadow(
                                        color: Color(0x803DE7FF),
                                        blurRadius: 10,
                                      ),
                                    ]
                                  : null,
                            ),
                          ),
                        ),
                        const SizedBox(width: 14),
                        Expanded(
                          child: Text(
                            _label,
                            maxLines: 1,
                            overflow: TextOverflow.ellipsis,
                            style: TextStyle(
                              fontFamily: LookScope.of(context).lcdFamily,
                              fontWeight: FontWeight.w500,
                              fontSize: 15,
                              letterSpacing: 0.15,
                              color: color,
                              shadows: playing
                                  ? const [
                                      Shadow(
                                        color: Color(0x803DE7FF),
                                        blurRadius: 10,
                                      ),
                                    ]
                                  : null,
                            ),
                          ),
                        ),
                        Text(
                          track.duration == null
                              ? ''
                              : formatDuration(track.duration!),
                          style: TextStyle(
                            fontFamily: LookScope.of(context).lcdFamily,
                            fontWeight: FontWeight.w500,
                            fontSize: 15,
                            color: color.withValues(alpha: 0.8),
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      );
  }
}

class _PlaylistFooter extends StatefulWidget {
  const _PlaylistFooter({
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
  State<_PlaylistFooter> createState() => _PlaylistFooterState();
}

class _PlaylistFooterState extends State<_PlaylistFooter> {
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

    // Strip 74 + gap 10 + status 26 — no spare band under the status row.
    return SizedBox(
      height: 110,
      child: Column(
        children: [
          SizedBox(
            height: 74,
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
          const SizedBox(height: 10),
          SizedBox(
            height: 26,
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

class _ListWellPainter extends CustomPainter {
  const _ListWellPainter();

  @override
  void paint(Canvas canvas, Size size) {
    final rect = Offset.zero & size;
    // Stripe + radial wash from mockup `.list`.
    canvas.drawRect(
      rect,
      Paint()
        ..shader = const RadialGradient(
          center: Alignment(-0.6, -1.2),
          radius: 1.35,
          colors: [Color(0xFF0D1622), Color(0xFF05070C)],
          stops: [0, 0.7],
        ).createShader(rect),
    );
    const row = 37.0;
    for (var y = 0.0; y < size.height; y += row * 2) {
      canvas.drawRect(
        Rect.fromLTWH(0, y, size.width, row),
        Paint()..color = const Color(0x04E2ECFF),
      );
      canvas.drawRect(
        Rect.fromLTWH(0, y + row, size.width, row),
        Paint()..color = const Color(0x1F000000),
      );
    }
    canvas.drawRect(
      rect.deflate(0.5),
      Paint()
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1
        ..color = const Color(0x143DE7FF),
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}

class _ListScanlinePainter extends CustomPainter {
  const _ListScanlinePainter();

  @override
  void paint(Canvas canvas, Size size) {
    for (var y = 0.0; y < size.height; y += 3) {
      canvas.drawRect(
        Rect.fromLTWH(0, y, size.width, 1),
        Paint()..color = const Color(0x38000000),
      );
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => false;
}
