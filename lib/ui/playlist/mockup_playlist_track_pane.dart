import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../../domain/track.dart';
import '../../look/resolved_look.dart';
import '../../playlist/playlist_controller.dart';
import '../../theme/look_scope.dart';
import '../../ui/format.dart';
import '../chrome/logo.dart';
import 'mockup_playlist_scrollbar.dart';

/// What a tap on a track row asks the selection to do, once the modifiers the
/// listener was holding have been read.
enum TrackRowSelection {
  /// A plain tap: collapse the selection to the row that was clicked.
  replace,

  /// Shift-click: select the contiguous range from the anchor.
  range,

  /// The platform modifier click: add or drop just this row.
  toggle,
}

/// Whether the platform's own toggle-a-row modifier is down: **Command** on
/// macOS, **Control** on Windows and Linux.
///
/// The convention is read from [defaultTargetPlatform] rather than `dart:io`'s
/// `Platform`, so it follows the same notion of the host every other Flutter
/// widget behaves by — and so a test can pin either desktop's convention
/// without pretending to run on it. The key state comes from
/// [HardwareKeyboard], the way the title bar already reads Shift for undock.
bool trackRowToggleModifierPressed() {
  final keyboard = HardwareKeyboard.instance;
  return defaultTargetPlatform == TargetPlatform.macOS
      ? keyboard.isMetaPressed
      : keyboard.isControlPressed;
}

/// What the modifiers held at this moment mean for a row tap.
///
/// Shift wins over the toggle modifier when both are down: a range is the more
/// specific request, and it is the one the listener can see the result of.
TrackRowSelection trackRowSelectionFromKeyboard() {
  if (HardwareKeyboard.instance.isShiftPressed) return TrackRowSelection.range;
  if (trackRowToggleModifierPressed()) return TrackRowSelection.toggle;
  return TrackRowSelection.replace;
}

/// Turns the index a reordered row comes to rest at into the insert-before
/// index `PlaylistController.move` speaks.
///
/// Flutter's `onReorderItem` answers with the row's final position, having
/// already accounted for the hole it left behind; `move` accounts for that
/// hole itself. A row that travelled *down* the list is therefore one greater
/// in insert-before terms, and one that travelled up is unchanged. Converted
/// once, here, so the controller keeps one convention on both sides of the
/// session bus.
int insertBeforeIndex(int oldIndex, int restingIndex) =>
    oldIndex < restingIndex ? restingIndex + 1 : restingIndex;

/// The track list half of the playlist body: list well, rows, and the custom
/// scrollbar beside it.
///
/// Layout matches `player-mockup-2.html` `.win--pl .list` — recessed well with
/// alternating row stripes, scanline overlay, and a watermark in the gap.
class MockupPlaylistTrackPane extends StatefulWidget {
  const MockupPlaylistTrackPane({
    super.key,
    required this.playlist,
    required this.playingIndex,
    required this.onSelect,
    required this.onActivate,
    required this.onReorder,
  });

  /// Height of one track row. The stripe pattern painted behind the list has to
  /// land on the same rhythm, so this is the one place it is defined.
  static const double rowHeight = 37;

  final PlaylistController playlist;
  final int? playingIndex;

  /// A row tap, with what the modifiers made of it. The pane reads the
  /// keyboard so the owner only ever decides what each gesture *means*.
  final void Function(int index, TrackRowSelection how) onSelect;
  final ValueChanged<int> onActivate;

  /// A row dragged from `oldIndex` and dropped before `newIndex` — Flutter's
  /// own reorder convention, which is also `PlaylistController.move`'s, so
  /// nothing between the gesture and the host has to translate it.
  final void Function(int oldIndex, int newIndex) onReorder;

  @override
  State<MockupPlaylistTrackPane> createState() =>
      _MockupPlaylistTrackPaneState();
}

class _MockupPlaylistTrackPaneState extends State<MockupPlaylistTrackPane> {
  final _scrollController = ScrollController();

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  /// The row the listener is carrying, painted over the gap it will drop into.
  ///
  /// Two things have to be put back that the framework takes away. The proxy is
  /// built in the app's [Overlay], which is above the window's [LookScope], so
  /// the row is handed the look again or it cannot paint at all. And Material's
  /// own decorator floats the row on an elevated card, which is the wrong idiom
  /// here — the row has the list well behind it and no surface of its own — so
  /// a transparent Material replaces it and the row in flight looks exactly
  /// like the row that will land.
  Widget _rowInFlight(ResolvedLook look, Widget child) => LookScope(
        look: look,
        child: Material(type: MaterialType.transparency, child: child),
      );

  @override
  Widget build(BuildContext context) {
    final tracks = widget.playlist.playlist.tracks;
    final selected = widget.playlist.selectedIndices;
    final look = LookScope.of(context);

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
                        child: ReorderableListView.builder(
                          scrollController: _scrollController,
                          padding: const EdgeInsets.symmetric(vertical: 6),
                          itemCount: tracks.length,
                          itemExtent: MockupPlaylistTrackPane.rowHeight,
                          onReorderItem: (oldIndex, restingIndex) {
                            widget.onReorder(
                              oldIndex,
                              insertBeforeIndex(oldIndex, restingIndex),
                            );
                          },
                          // No handle column: the whole row is the grip, the
                          // way the classic playlist behaved. A tap has to
                          // survive that, which it does — the drag recognizer
                          // only claims the pointer once it has moved past the
                          // hit slop, so a click (and its modifiers) still
                          // reaches the row's own tap handler.
                          buildDefaultDragHandles: false,
                          proxyDecorator: (child, _, __) =>
                              _rowInFlight(look, child),
                          itemBuilder: (context, index) {
                            final track = tracks[index];
                            return ReorderableDragStartListener(
                              key: Key('pl-row-$index'),
                              index: index,
                              child: _PlaylistRow(
                                index: index,
                                track: track,
                                selected: selected.contains(index),
                                playing: widget.playingIndex == index,
                                onSelect: () => widget.onSelect(
                                  index,
                                  trackRowSelectionFromKeyboard(),
                                ),
                                onActivate: () => widget.onActivate(index),
                              ),
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
      height: MockupPlaylistTrackPane.rowHeight,
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
                  ? const LinearGradient(
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
    const row = MockupPlaylistTrackPane.rowHeight;
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
