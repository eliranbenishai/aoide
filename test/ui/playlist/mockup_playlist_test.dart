import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/playlist/mockup_playlist.dart';
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
            ),
          ),
        ),
      ),
    );
  }

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
