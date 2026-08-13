import 'package:flutter/material.dart';

import '../../domain/saved_playlist.dart';
import '../../theme/look_paint.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/mockup/mockup_button.dart';
import '../chrome/mockup/mockup_hover.dart';
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
    this.disabledPaths = const {},
    this.onCollapse,
    this.onSelect,
    this.onAdd,
    this.onCreate,
    this.onRemove,
  });

  /// Key on the empty state, so tests can tell "nothing kept yet" apart from
  /// "panel did not render".
  static const emptyKey = Key('pl-collection-empty');

  /// Key on a row's missing-file mark, per row index.
  static Key missingKeyFor(int index) => Key('pl-collection-missing-$index');

  /// Height of one collection row — compact beside the 37px track rows, so the
  /// two lists never read as one.
  static const double rowHeight = 26;

  /// The saved playlists to paint, in any order — the panel is alphabetical by
  /// display name and sorts them itself, so the order it reads in never depends
  /// on the order a host snapshot happened to arrive in.
  final List<SavedPlaylist> playlists;

  /// Normalized path of the loaded entry, so its row reads as such.
  final String? selectedPath;

  /// Normalized paths of the **disabled playlists**: entries whose file was
  /// missing at the last check, painted dimmed and marked. Not a field on the
  /// entry, because the state is derived rather than kept.
  final Set<String> disabledPaths;

  final VoidCallback? onCollapse;
  final ValueChanged<SavedPlaylist>? onSelect;
  final VoidCallback? onAdd;

  /// Makes a new saved playlist out of the current playlist's tracks. Null
  /// disables the control — which is how an empty current playlist is refused,
  /// since there is nothing there to keep.
  final VoidCallback? onCreate;

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
                          disabled: disabledPaths.contains(entry.path),
                          missingKey: missingKeyFor(index),
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
                key: const Key('pl-collection-create'),
                width: 30,
                height: 24,
                semanticLabel: 'Create playlist from current playlist',
                onPressed: onCreate,
                child: PlaylistCollectionCreateMark(
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
///
/// A **disabled playlist** — one whose file was missing at the last check —
/// keeps its figures, because a disabled playlist still counts toward the About
/// stats, but reads as unavailable: marked with a struck ring and dimmed by the
/// same [MockupHoverTokens.disabledOpacity] every disabled control in this
/// chrome uses. It stays tappable, because selecting it is how the panel's
/// remove control reaches it; what the tap means is the caller's to decide.
class _CollectionRow extends StatelessWidget {
  const _CollectionRow({
    super.key,
    required this.entry,
    required this.selected,
    required this.disabled,
    required this.missingKey,
    this.onTap,
  });

  final SavedPlaylist entry;
  final bool selected;
  final bool disabled;
  final Key missingKey;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final look = LookScope.of(context);
    final palette = look.palette;
    final ink = selected ? palette.phosphorHot : palette.inkDim;

    Widget content = Padding(
      padding: const EdgeInsets.symmetric(horizontal: 10),
      child: Row(
        children: [
          if (disabled) ...[
            PlaylistCollectionMissingMark(key: missingKey, color: ink),
            const SizedBox(width: 6),
          ],
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
              color: selected ? palette.phosphorDefault : palette.phosphorDim,
            ),
          ),
        ],
      ),
    );

    if (disabled) {
      // Only the contents dim. A disabled row can still be the highlighted one
      // — that is how it gets removed — and that highlight must stay readable.
      content = Opacity(
        opacity: MockupHoverTokens.disabledOpacity,
        child: content,
      );
    }

    return SizedBox(
      height: MockupPlaylistCollectionPane.rowHeight,
      child: Semantics(
        selected: selected,
        button: true,
        label: disabled
            ? '${entry.displayName}, ${entry.trackCount} tracks, file missing'
            : '${entry.displayName}, ${entry.trackCount} tracks',
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
            child: content,
          ),
        ),
      ),
    );
  }
}

/// The mark on a **disabled playlist**'s row: a struck ring, in the same
/// stroked idiom as [PlaylistCollectionChevron]. It says the file behind the
/// entry was not there at the last check — not that anything was deleted.
class PlaylistCollectionMissingMark extends StatelessWidget {
  const PlaylistCollectionMissingMark({
    super.key,
    required this.color,
    this.size = 9,
  });

  final Color color;
  final double size;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size.square(size),
      painter: _MissingMarkPainter(color: color),
    );
  }
}

class _MissingMarkPainter extends CustomPainter {
  const _MissingMarkPainter({required this.color});

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.4
      ..strokeCap = StrokeCap.round
      ..color = color;
    final centre = Offset(size.width / 2, size.height / 2);
    final radius = size.shortestSide / 2 - 0.7;
    canvas.drawCircle(centre, radius, paint);
    final reach = radius * 0.7;
    canvas.drawLine(
      centre + Offset(-reach, reach),
      centre + Offset(reach, -reach),
      paint,
    );
  }

  @override
  bool shouldRepaint(covariant _MissingMarkPainter oldDelegate) =>
      color != oldDelegate.color;
}

/// The mark on the panel's create control: a short stack of lines with a plus
/// beside it — a list being made, rather than a file being fetched, which is
/// what the neighbouring add control does.
///
/// Stroked in the same idiom as [PlaylistCollectionChevron], and drawn here
/// rather than in `MockupIcons` because it belongs to this panel: the mockup's
/// icon set has no glyph for an action the window did not used to have.
class PlaylistCollectionCreateMark extends StatelessWidget {
  const PlaylistCollectionCreateMark({
    super.key,
    required this.color,
    this.size = 12,
  });

  final Color color;
  final double size;

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      size: Size.square(size),
      painter: _CreateMarkPainter(color: color),
    );
  }
}

class _CreateMarkPainter extends CustomPainter {
  const _CreateMarkPainter({required this.color});

  final Color color;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.4
      ..strokeCap = StrokeCap.round
      ..color = color;
    // Three list rules down the left two-thirds…
    final ruleWidth = size.width * 0.62;
    for (var i = 0; i < 3; i++) {
      final y = size.height * (0.18 + i * 0.32);
      canvas.drawLine(Offset(0.7, y), Offset(ruleWidth, y), paint);
    }
    // …and a plus in the bottom-right corner.
    final cx = size.width - size.width * 0.16;
    final cy = size.height - size.height * 0.16;
    final reach = size.width * 0.2;
    canvas.drawLine(Offset(cx - reach, cy), Offset(cx + reach, cy), paint);
    canvas.drawLine(Offset(cx, cy - reach), Offset(cx, cy + reach), paint);
  }

  @override
  bool shouldRepaint(covariant _CreateMarkPainter oldDelegate) =>
      color != oldDelegate.color;
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
