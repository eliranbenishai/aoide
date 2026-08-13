import 'package:flutter/material.dart';

import '../../domain/saved_playlist.dart';
import '../../theme/look_paint.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_icons.dart';
import '../chrome/mockup/mockup_screen.dart';

/// The playlist collection half of the Playlist Manager body: the playlists the
/// listener keeps, beside the current playlist.
///
/// Rows are references to files where the listener put them — selecting one
/// loads it, and removing one only ever drops the reference.
class MockupPlaylistCollectionPane extends StatelessWidget {
  const MockupPlaylistCollectionPane({
    super.key,
    this.playlists = const [],
    this.selectedPath,
    this.onCollapse,
    this.onSelect,
    this.onAdd,
    this.onRemove,
  });

  /// Key on the empty state, so tests can tell "nothing kept yet" apart from
  /// "panel did not render".
  static const emptyKey = Key('pl-collection-empty');

  /// Height of one collection row — compact beside the 37px track rows, so the
  /// two lists never read as one.
  static const double rowHeight = 26;

  /// The saved playlists to paint, in any order — the panel is alphabetical by
  /// display name and sorts them itself, so the order it reads in never depends
  /// on the order a host snapshot happened to arrive in.
  final List<SavedPlaylist> playlists;

  /// Normalized path of the loaded entry, so its row reads as such.
  final String? selectedPath;

  final VoidCallback? onCollapse;
  final ValueChanged<SavedPlaylist>? onSelect;
  final VoidCallback? onAdd;

  /// Removes the entry passed back — the selected one, matching how the
  /// footer's track remove acts on the track selection.
  final ValueChanged<SavedPlaylist>? onRemove;

  SavedPlaylist? get _selected {
    for (final entry in playlists) {
      if (entry.path == selectedPath) return entry;
    }
    return null;
  }

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final selected = _selected;
    final rows = List<SavedPlaylist>.of(playlists)
      ..sort(SavedPlaylist.compareByDisplayName);

    return Padding(
      padding: const EdgeInsets.fromLTRB(12, 12, 6, 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(
            children: [
              Expanded(
                child: Text(
                  'PLAYLISTS',
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: TextStyle(
                    fontFamily: look.chromeFamily,
                    fontWeight: FontWeight.w700,
                    fontSize: 11,
                    letterSpacing: 11 * 0.2,
                    color: palette.inkFaint,
                  ),
                ),
              ),
              const SizedBox(width: 6),
              MockupButton(
                key: const Key('pl-collection-collapse'),
                width: 24,
                height: 20,
                semanticLabel: 'Collapse playlist collection',
                onPressed: onCollapse,
                child: PlaylistCollectionChevron(
                  pointsLeft: true,
                  color: MockupIcons.inkOf(context),
                ),
              ),
            ],
          ),
          const SizedBox(height: 10),
          Expanded(
            child: MockupScreen(
              child: rows.isEmpty
                  ? _EmptyCollection(emptyKey: emptyKey)
                  : ListView.builder(
                      padding: const EdgeInsets.symmetric(vertical: 4),
                      itemCount: rows.length,
                      itemExtent: rowHeight,
                      itemBuilder: (context, index) {
                        final entry = rows[index];
                        return _CollectionRow(
                          key: Key('pl-collection-row-$index'),
                          entry: entry,
                          selected: entry.path == selectedPath,
                          onTap: onSelect == null
                              ? null
                              : () => onSelect!(entry),
                        );
                      },
                    ),
            ),
          ),
          const SizedBox(height: 8),
          // The panel's own controls. Kept in their own strip, at their own
          // size, with their own labels: the footer's add / remove act on
          // tracks and these act on whole playlists.
          Row(
            children: [
              MockupButton(
                key: const Key('pl-collection-add'),
                width: 30,
                height: 24,
                semanticLabel: 'Add playlist to collection',
                onPressed: onAdd,
                child: MockupIcons.add(
                  size: 13,
                  color: MockupIcons.inkOf(context),
                ),
              ),
              const SizedBox(width: 6),
              MockupButton(
                key: const Key('pl-collection-remove'),
                width: 30,
                height: 24,
                semanticLabel: 'Remove playlist from collection',
                onPressed: selected == null || onRemove == null
                    ? null
                    : () => onRemove!(selected),
                child: MockupIcons.remove(
                  size: 13,
                  color: MockupIcons.inkOf(context),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class _EmptyCollection extends StatelessWidget {
  const _EmptyCollection({required this.emptyKey});

  final Key emptyKey;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    return Center(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12),
        child: Column(
          key: emptyKey,
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              'NO SAVED PLAYLISTS',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontFamily: look.chromeFamily,
                fontWeight: FontWeight.w700,
                fontSize: 11,
                letterSpacing: 11 * 0.16,
                color: palette.inkFaint,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'ADD A PLAYLIST FILE WITH + BELOW',
              textAlign: TextAlign.center,
              style: TextStyle(
                fontFamily: look.lcdFamily,
                fontWeight: FontWeight.w500,
                fontSize: 10,
                height: 1.5,
                letterSpacing: 0.4,
                color: palette.phosphorDim,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

/// One saved playlist: the name the listener reads, and how many tracks it
/// holds. Styled off the track row but quieter, and uppercased like the
/// footer's status line so a Windows-case-folded path still reads as chrome.
class _CollectionRow extends StatelessWidget {
  const _CollectionRow({
    super.key,
    required this.entry,
    required this.selected,
    this.onTap,
  });

  final SavedPlaylist entry;
  final bool selected;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final ink = selected ? palette.phosphorHot : palette.inkDim;

    return SizedBox(
      height: MockupPlaylistCollectionPane.rowHeight,
      child: Semantics(
        selected: selected,
        button: true,
        label: '${entry.displayName}, ${entry.trackCount} tracks',
        child: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTap: onTap,
          child: DecoratedBox(
            decoration: BoxDecoration(
              gradient: selected
                  ? LinearGradient(
                      begin: Alignment.topCenter,
                      end: Alignment.bottomCenter,
                      colors: [
                        palette.phosphorDefault.withValues(alpha: 0.13),
                        palette.phosphorDefault.withValues(alpha: 0.04),
                      ],
                    )
                  : null,
            ),
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 10),
              child: Row(
                children: [
                  Expanded(
                    child: Text(
                      entry.displayName.toUpperCase(),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: TextStyle(
                        fontFamily: look.chromeFamily,
                        fontWeight: FontWeight.w700,
                        fontSize: 11,
                        letterSpacing: 11 * 0.1,
                        color: ink,
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Text(
                    '${entry.trackCount}',
                    style: TextStyle(
                      fontFamily: look.lcdFamily,
                      fontWeight: FontWeight.w500,
                      fontSize: 12,
                      color: selected
                          ? palette.phosphorDefault
                          : palette.phosphorDim,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}

/// The draggable divider between the collection panel and the track list.
///
/// Reports width deltas rather than absolute positions so the owner stays the
/// single place that clamps. Sits well inside the window's 6px
/// `DragToResizeArea` edges, so window resize and panel resize never compete.
class PlaylistCollectionDivider extends StatelessWidget {
  const PlaylistCollectionDivider({
    super.key,
    this.onDragDelta,
    this.onDragEnd,
  });

  final ValueChanged<double>? onDragDelta;
  final VoidCallback? onDragEnd;

  @override
  Widget build(BuildContext context) {
    final palette = LookScope.of(context).palette;
    return MouseRegion(
      cursor: SystemMouseCursors.resizeLeftRight,
      child: GestureDetector(
        behavior: HitTestBehavior.opaque,
        onHorizontalDragUpdate: (details) =>
            onDragDelta?.call(details.delta.dx),
        onHorizontalDragEnd: (_) => onDragEnd?.call(),
        child: SizedBox(
          width: TrampMetrics.playlistDividerWidth,
          child: Semantics(
            label: 'Resize playlist collection',
            child: CustomPaint(
              painter: _DividerGripPainter(
                sheen: LookPaint.coolSheen(palette),
              ),
              child: const SizedBox.expand(),
            ),
          ),
        ),
      ),
    );
  }
}

/// The affordance that brings a collapsed collection panel back.
///
/// Deliberately overlaid on the track pane's gutter rather than laid out beside
/// it: while the panel is collapsed the window must render exactly as it did
/// before the Playlist Manager gained its second panel, down to its minimum
/// width.
class PlaylistCollectionReopenTab extends StatelessWidget {
  const PlaylistCollectionReopenTab({super.key, this.onPressed});

  final VoidCallback? onPressed;

  @override
  Widget build(BuildContext context) {
    return MockupButton(
      key: const Key('pl-collection-reopen'),
      width: 14,
      height: 56,
      semanticLabel: 'Show playlist collection',
      onPressed: onPressed,
      child: PlaylistCollectionChevron(
        pointsLeft: false,
        color: MockupIcons.inkOf(context),
      ),
    );
  }
}

/// Stroked chevron for the collection panel's collapse / reopen controls.
class PlaylistCollectionChevron extends StatelessWidget {
  const PlaylistCollectionChevron({
    super.key,
    required this.pointsLeft,
    required this.color,
    this.size = 8,
  });

  final bool pointsLeft;
  final Color color;
  final double size;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size(size * 0.7, size),
      painter: _ChevronPainter(pointsLeft: pointsLeft, color: color),
    );
  }
}

class _ChevronPainter extends CustomPainter {
  const _ChevronPainter({required this.pointsLeft, required this.color});

  final bool pointsLeft;
  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.6
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round
      ..color = color;
    final tipX = pointsLeft ? 0.5 : size.width - 0.5;
    final backX = pointsLeft ? size.width - 0.5 : 0.5;
    canvas.drawPath(
      Path()
        ..moveTo(backX, 1)
        ..lineTo(tipX, size.height / 2)
        ..lineTo(backX, size.height - 1),
      paint,
    );
  }

  @override
  bool shouldRepaint(covariant _ChevronPainter oldDelegate) =>
      pointsLeft != oldDelegate.pointsLeft || color != oldDelegate.color;
}

class _DividerGripPainter extends CustomPainter {
  const _DividerGripPainter({required this.sheen});

  final Color sheen;

  @override
  void paint(Canvas canvas, Size size) {
    // Two hairlines reading as a groove, faded at both ends like `.rail`.
    final centre = (size.width / 2).floorToDouble();
    void hairline(double x, Color color) {
      final rect = Rect.fromLTWH(x, 0, 1, size.height);
      canvas.drawRect(
        rect,
        Paint()
          ..shader = LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [const Color(0x00000000), color, const Color(0x00000000)],
            stops: const [0, 0.5, 1],
          ).createShader(rect),
      );
    }

    hairline(centre - 1, const Color(0xB3000000));
    hairline(centre, sheen.withValues(alpha: 0x1F / 255));
  }

  @override
  bool shouldRepaint(covariant _DividerGripPainter oldDelegate) =>
      sheen != oldDelegate.sheen;
}
