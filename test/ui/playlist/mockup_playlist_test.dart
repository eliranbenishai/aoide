import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/playlist/mockup_playlist.dart';
import 'package:tramp/ui/playlist/mockup_playlist_collection_pane.dart';
import 'package:tramp/ui/session/session_messages.dart';
import 'package:tramp/ui/windows/playlist_window.dart';

import '../../support/look_harness.dart';
import '../../support/test_fonts.dart';

class MemoryStore implements PlaylistStore {
  String? last;

  @override
  Future<String?> readLastPlaylistPath() async => last;

  @override
  Future<void> writeLastPlaylistPath(String? path) async => last = path;
}

void main() {
  late PlaylistController playlist;

  setUpAll(loadTrampFonts);

  setUp(() {
    playlist = PlaylistController(store: MemoryStore());
    playlist.addTracks(const [
      Track(
        path: '/a.mp3',
        title: 'Alpha',
        artist: 'Artist A',
        duration: Duration(seconds: 65),
      ),
      Track(
        path: '/b.mp3',
        title: 'Bravo',
        artist: 'Artist B',
        duration: Duration(seconds: 125),
      ),
      Track(
        path: '/c.mp3',
        title: 'Charlie',
        artist: 'Artist C',
        duration: Duration(seconds: 30),
      ),
    ]);
  });

  Future<void> pumpPl(
    WidgetTester tester, {
    required List<SessionCommand> commands,
    bool shaded = false,
    Size size = TrampMetrics.playlistDefault,
    int? playingIndex,
    bool playing = false,
    double collectionWidth = TrampSettings.defaultPlaylistCollectionWidth,
    bool collectionCollapsed = false,
    ValueChanged<double>? onCollectionWidthChanged,
    ValueChanged<bool>? onCollectionCollapsedChanged,
  }) async {
    await tester.binding.setSurfaceSize(Size(size.width + 40, size.height + 40));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: wrapWithLook(
            PlaylistWindow(
              playlist: playlist,
              size: size,
              shaded: shaded,
              playingIndex: playingIndex,
              playing: playing,
              draggableTitle: false,
              onSessionCommand: commands.add,
              collectionWidth: collectionWidth,
              collectionCollapsed: collectionCollapsed,
              onCollectionWidthChanged: onCollectionWidthChanged,
              onCollectionCollapsedChanged: onCollectionCollapsedChanged,
            ),
          ),
        ),
      ),
    );
  }

  double collectionPaneWidth(WidgetTester tester) =>
      tester.getSize(find.byType(MockupPlaylistCollectionPane)).width;

  double trackPaneWidth(WidgetTester tester) =>
      tester.getSize(find.byType(MockupPlaylist)).width;

  testWidgets('holds default canvas size', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands);

    expect(
      tester.getSize(find.byType(PlaylistWindow)),
      TrampMetrics.playlistDefault,
    );
    expect(find.byType(MockupPlaylist), findsOneWidget);
    expect(tester.takeException(), isNull);
  });

  testWidgets('shade collapses to title bar height', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, shaded: true);

    expect(
      tester.getSize(find.byType(PlaylistWindow)).height,
      TrampMetrics.titleBar,
    );
    expect(find.byType(MockupPlaylist), findsNothing);
  });

  testWidgets('list grows when window is taller', (tester) async {
    final commands = <SessionCommand>[];
    const tall = Size(825, 900);
    await pumpPl(tester, commands: commands, size: tall);

    expect(tester.getSize(find.byType(PlaylistWindow)), tall);
    expect(find.textContaining('Alpha'), findsOneWidget);
  });

  testWidgets('narrow window does not overflow the footer strip', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(
      tester,
      commands: commands,
      size: TrampMetrics.playlistMin,
    );

    expect(tester.takeException(), isNull);
    expect(find.text('TOTAL'), findsOneWidget);
  });

  testWidgets('row tap selects and emits select op', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands);

    await tester.ensureVisible(find.byKey(const Key('pl-row-1')));
    await tester.tap(find.byKey(const Key('pl-row-1')));
    await tester.pumpAndSettle();

    expect(playlist.selectedIndex, 1);
    expect(commands.whereType<PlaylistOpCommand>().single.op, 'select');
    expect(commands.whereType<PlaylistOpCommand>().single.index, 1);
  });

  testWidgets('select all / invert from options menu', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands);

    await tester.tap(find.byKey(const Key('pl-options')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Select all').last);
    await tester.pumpAndSettle();

    expect(playlist.selectedIndices, {0, 1, 2});
    expect(
      commands.whereType<PlaylistOpCommand>().map((c) => c.op),
      contains('selectAll'),
    );

    await tester.tap(find.byKey(const Key('pl-options')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Invert selection').last);
    await tester.pumpAndSettle();

    expect(playlist.selectedIndices, isEmpty);
  });

  testWidgets('sort menu emits sort and reorders rows', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands);

    await tester.tap(find.byKey(const Key('pl-sort')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Duration').last);
    await tester.pumpAndSettle();

    expect(
      playlist.playlist.tracks.map((t) => t.title),
      ['Charlie', 'Alpha', 'Bravo'],
    );
    expect(
      commands.whereType<PlaylistOpCommand>().single.sortKey,
      'duration',
    );
  });

  testWidgets('mini transport emits TransportCommand', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, playing: true, playingIndex: 0);

    await tester.tap(find.byKey(const Key('pl-play')));
    await tester.pump();
    expect(commands.single, isA<TransportCommand>());
    expect((commands.single as TransportCommand).action, 'playPause');
  });

  testWidgets('TOTAL sums track durations', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands);
    // 65+125+30 = 220s → 3:40
    expect(find.text('3:40'), findsOneWidget);
    expect(find.text('TOTAL'), findsOneWidget);
  });

  testWidgets('window is titled Playlist Manager', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands);

    expect(find.text('PLAYLIST MANAGER'), findsOneWidget);
    expect(find.text('PLAYLIST EDITOR'), findsNothing);
  });

  testWidgets('collection panel and divider sit left of the track list',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, size: const Size(1000, 700));

    expect(find.byType(MockupPlaylistCollectionPane), findsOneWidget);
    expect(find.byKey(const Key('pl-divider')), findsOneWidget);

    final pane = tester.getRect(find.byType(MockupPlaylistCollectionPane));
    final divider = tester.getRect(find.byKey(const Key('pl-divider')));
    final tracks = tester.getRect(find.byType(MockupPlaylist));
    expect(pane.right, lessThanOrEqualTo(divider.left));
    expect(divider.right, lessThanOrEqualTo(tracks.left));
    expect(divider.width, TrampMetrics.playlistDividerWidth);
  });

  testWidgets('shade hides the collection panel and divider too',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, shaded: true);

    expect(find.byType(MockupPlaylistCollectionPane), findsNothing);
    expect(find.byKey(const Key('pl-divider')), findsNothing);
  });

  testWidgets('empty collection tells the listener how to add a playlist',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, size: const Size(1000, 700));

    expect(
      find.byKey(MockupPlaylistCollectionPane.emptyKey),
      findsOneWidget,
    );
    expect(find.textContaining('ADD A PLAYLIST FILE'), findsOneWidget);
  });

  testWidgets('dragging the divider resizes both panels', (tester) async {
    final commands = <SessionCommand>[];
    final reported = <double>[];
    await pumpPl(
      tester,
      commands: commands,
      size: const Size(1000, 700),
      onCollectionWidthChanged: reported.add,
    );

    final before = collectionPaneWidth(tester);
    final tracksBefore = trackPaneWidth(tester);
    expect(before, TrampSettings.defaultPlaylistCollectionWidth);

    await tester.drag(find.byKey(const Key('pl-divider')), const Offset(60, 0));
    await tester.pump();

    expect(collectionPaneWidth(tester), before + 60);
    expect(trackPaneWidth(tester), tracksBefore - 60);
    expect(reported.last, before + 60);
    expect(tester.takeException(), isNull);
  });

  testWidgets('divider cannot squeeze either panel past its minimum',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, size: const Size(1000, 700));

    await tester.drag(
      find.byKey(const Key('pl-divider')),
      const Offset(-400, 0),
    );
    await tester.pump();
    expect(
      collectionPaneWidth(tester),
      TrampMetrics.playlistCollectionMinWidth,
    );

    await tester.drag(find.byKey(const Key('pl-divider')), const Offset(400, 0));
    await tester.pump();
    expect(trackPaneWidth(tester), TrampMetrics.playlistMin.width);
    expect(tester.takeException(), isNull);
  });

  testWidgets('collapsing hides the panel and reopening restores its width',
      (tester) async {
    final commands = <SessionCommand>[];
    final collapses = <bool>[];
    await pumpPl(
      tester,
      commands: commands,
      size: const Size(1000, 700),
      onCollectionCollapsedChanged: collapses.add,
    );

    await tester.drag(find.byKey(const Key('pl-divider')), const Offset(60, 0));
    await tester.pump();
    final dragged = collectionPaneWidth(tester);
    expect(dragged, isNot(TrampSettings.defaultPlaylistCollectionWidth));

    await tester.tap(find.byKey(const Key('pl-collection-collapse')));
    await tester.pump();
    expect(find.byType(MockupPlaylistCollectionPane), findsNothing);
    expect(find.byKey(const Key('pl-divider')), findsNothing);
    expect(collapses, [true]);
    expect(trackPaneWidth(tester), 1000);

    await tester.tap(find.byKey(const Key('pl-collection-reopen')));
    await tester.pump();
    expect(collectionPaneWidth(tester), dragged);
    expect(collapses, [true, false]);
  });

  testWidgets('minimum width with the panel shown still fits the footer',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(
      tester,
      commands: commands,
      size: TrampMetrics.playlistMinWithCollection,
    );

    expect(tester.takeException(), isNull);
    expect(find.text('TOTAL'), findsOneWidget);
    expect(
      collectionPaneWidth(tester),
      TrampMetrics.playlistCollectionMinWidth,
    );
    expect(trackPaneWidth(tester), TrampMetrics.playlistMin.width);
  });

  testWidgets('resizing the window gives all slack to the track list',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPl(tester, commands: commands, size: const Size(900, 700));

    final paneWidth = collectionPaneWidth(tester);
    final tracksBefore = tester.getSize(find.byType(MockupPlaylist));

    await pumpPl(tester, commands: commands, size: const Size(1100, 820));

    expect(collectionPaneWidth(tester), paneWidth);
    final tracksAfter = tester.getSize(find.byType(MockupPlaylist));
    expect(tracksAfter.width, tracksBefore.width + 200);
    expect(tracksAfter.height, tracksBefore.height + 120);
  });

  testWidgets('collapse invokes shade callback', (tester) async {
    var collapsed = 0;
    await tester.binding.setSurfaceSize(const Size(900, 750));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: wrapWithLook(
            PlaylistWindow(
              playlist: playlist,
              draggableTitle: false,
              onCollapse: () => collapsed++,
            ),
          ),
        ),
      ),
    );

    await tester.tap(find.bySemanticsLabel('Collapse'));
    await tester.pump();
    expect(collapsed, 1);
  });
}
