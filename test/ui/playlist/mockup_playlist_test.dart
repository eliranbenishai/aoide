import 'package:flutter/foundation.dart' show debugDefaultTargetPlatformOverride;
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/saved_playlist.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/chrome/mockup/mockup_hover.dart';
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
    // Loaded, not built up track by track: in the client this mirror is only
    // ever filled from a host snapshot, and that leaves it unaltered.
    playlist.setTracks(const [
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
    List<SavedPlaylist> collection = const [],
    String? selectedCollectionPath,
    Set<String> disabledCollectionPaths = const {},
    VoidCallback? onAddSavedPlaylist,
    bool altered = false,
    Future<String?> Function()? pickSavePlaylistPath,
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
              collection: collection,
              selectedCollectionPath: selectedCollectionPath,
              disabledCollectionPaths: disabledCollectionPaths,
              onAddSavedPlaylist: onAddSavedPlaylist,
              altered: altered,
              pickSavePlaylistPath: pickSavePlaylistPath,
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

  group('playlist collection', () {
    final driving = SavedPlaylist(
      path: '/music/driving.m3u',
      trackCount: 12,
    );
    final sunday = SavedPlaylist(
      path: '/music/sun.m3u',
      name: 'Sunday Morning',
      trackCount: 7,
    );
    final work = SavedPlaylist(path: '/music/work.m3u', trackCount: 41);

    Finder rowText(int index, String text) => find.descendant(
          of: find.byKey(Key('pl-collection-row-$index')),
          matching: find.text(text),
        );

    testWidgets('rows show a display name and a track count', (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving, sunday],
      );

      expect(find.byKey(MockupPlaylistCollectionPane.emptyKey), findsNothing);
      expect(rowText(0, 'DRIVING'), findsOneWidget);
      expect(rowText(0, '12'), findsOneWidget);
      expect(rowText(1, 'SUNDAY MORNING'), findsOneWidget);
      expect(rowText(1, '7'), findsOneWidget);
    });

    testWidgets('rows read alphabetically by display name', (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        // Deliberately out of order: the panel is alphabetical whatever the
        // host sent.
        collection: [work, sunday, driving],
      );

      expect(rowText(0, 'DRIVING'), findsOneWidget);
      expect(rowText(1, 'SUNDAY MORNING'), findsOneWidget);
      expect(rowText(2, 'WORK'), findsOneWidget);
      expect(
        tester.getRect(find.byKey(const Key('pl-collection-row-0'))).top,
        lessThan(
          tester.getRect(find.byKey(const Key('pl-collection-row-2'))).top,
        ),
      );
    });

    testWidgets('tapping a row asks the host to load it', (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving, sunday],
      );

      await tester.tap(find.byKey(const Key('pl-collection-row-1')));
      await tester.pump();

      final load = commands.whereType<LoadSavedPlaylistCommand>().single;
      expect(load.path, sunday.path);
    });

    testWidgets('the loaded row reads as the loaded one', (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving, sunday],
        selectedCollectionPath: sunday.path,
      );

      BoxDecoration decorationOf(int index) => tester
          .widget<DecoratedBox>(
            find.descendant(
              of: find.byKey(Key('pl-collection-row-$index')),
              matching: find.byType(DecoratedBox),
            ),
          )
          .decoration as BoxDecoration;

      expect(decorationOf(1).gradient, isNotNull);
      expect(decorationOf(0).gradient, isNull);
      expect(
        tester.widget<Text>(rowText(1, 'SUNDAY MORNING')).style!.color,
        isNot(tester.widget<Text>(rowText(0, 'DRIVING')).style!.color),
      );
    });

    testWidgets('the panel remove control drops the selected entry',
        (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving, sunday],
        selectedCollectionPath: driving.path,
      );

      await tester.tap(find.byKey(const Key('pl-collection-remove')));
      await tester.pump();

      final remove = commands.whereType<RemoveSavedPlaylistCommand>().single;
      expect(remove.path, driving.path);
    });

    testWidgets('remove does nothing with no row selected', (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving, sunday],
      );

      await tester.tap(find.byKey(const Key('pl-collection-remove')));
      await tester.pump();

      expect(commands, isEmpty);
    });

    testWidgets('the panel add control opens the listener\'s file picker',
        (tester) async {
      final commands = <SessionCommand>[];
      var adds = 0;
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        onAddSavedPlaylist: () => adds++,
      );

      await tester.tap(find.byKey(const Key('pl-collection-add')));
      await tester.pump();

      expect(adds, 1);
    });

    testWidgets('panel controls are not the footer\'s track controls',
        (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving],
      );

      expect(find.byKey(const Key('pl-collection-add')), findsOneWidget);
      expect(find.byKey(const Key('pl-collection-remove')), findsOneWidget);
      expect(find.byKey(const Key('pl-add')), findsOneWidget);
      expect(find.byKey(const Key('pl-remove')), findsOneWidget);
      expect(
        find.bySemanticsLabel('Add playlist to collection'),
        findsOneWidget,
      );
      expect(find.bySemanticsLabel('Add tracks'), findsOneWidget);
      expect(
        find.bySemanticsLabel('Remove playlist from collection'),
        findsOneWidget,
      );
      expect(find.bySemanticsLabel('Remove selected tracks'), findsOneWidget);
    });

    testWidgets('the empty state shows only while nothing is kept',
        (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      expect(find.byKey(MockupPlaylistCollectionPane.emptyKey), findsOneWidget);
      expect(find.byKey(const Key('pl-collection-row-0')), findsNothing);

      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving],
      );

      expect(find.byKey(MockupPlaylistCollectionPane.emptyKey), findsNothing);
      expect(find.byKey(const Key('pl-collection-row-0')), findsOneWidget);
    });

    testWidgets('rows still fit at the minimum width with the panel shown',
        (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(
        tester,
        commands: commands,
        size: TrampMetrics.playlistMinWithCollection,
        collection: [driving, sunday, work],
        selectedCollectionPath: work.path,
      );

      expect(tester.takeException(), isNull);
      expect(find.byKey(const Key('pl-collection-row-0')), findsOneWidget);
      expect(find.text('TOTAL'), findsOneWidget);
    });

    group('disabled playlists', () {
      Finder markIn(int index) => find.descendant(
            of: find.byKey(Key('pl-collection-row-$index')),
            matching: find.byType(PlaylistCollectionMissingMark),
          );

      testWidgets('a row whose file is missing reads as disabled',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          disabledCollectionPaths: {driving.path},
        );

        expect(markIn(0), findsOneWidget);
        expect(markIn(1), findsNothing);
        expect(
          tester
              .widget<Opacity>(
                find.descendant(
                  of: find.byKey(const Key('pl-collection-row-0')),
                  matching: find.byType(Opacity),
                ),
              )
              .opacity,
          MockupHoverTokens.disabledOpacity,
        );
        expect(
          find.descendant(
            of: find.byKey(const Key('pl-collection-row-1')),
            matching: find.byType(Opacity),
          ),
          findsNothing,
        );
        // The listener can still read which playlist it is, and how much of it
        // there was — a disabled playlist still counts toward the About stats.
        expect(rowText(0, 'DRIVING'), findsOneWidget);
        expect(rowText(0, '12'), findsOneWidget);
        String labelOf(int index) => tester
            .getSemantics(find.byKey(Key('pl-collection-row-$index')))
            .label;
        expect(labelOf(0), contains('driving, 12 tracks, file missing'));
        expect(labelOf(1), contains('Sunday Morning, 7 tracks'));
        expect(labelOf(1), isNot(contains('missing')));
      });

      testWidgets('tapping a disabled row never asks the host to load it',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          disabledCollectionPaths: {driving.path},
        );

        await tester.tap(find.byKey(const Key('pl-collection-row-0')));
        await tester.pump();

        expect(commands.whereType<LoadSavedPlaylistCommand>(), isEmpty);
        // It is highlighted instead — how the remove control reaches it.
        expect(
          commands.whereType<SelectSavedPlaylistCommand>().single.path,
          driving.path,
        );

        await tester.tap(find.byKey(const Key('pl-collection-row-1')));
        await tester.pump();
        expect(
          commands.whereType<LoadSavedPlaylistCommand>().single.path,
          sunday.path,
        );
      });

      testWidgets('a disabled row can still be removed', (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          selectedCollectionPath: driving.path,
          disabledCollectionPaths: {driving.path},
        );

        await tester.tap(find.byKey(const Key('pl-collection-remove')));
        await tester.pump();

        expect(
          commands.whereType<RemoveSavedPlaylistCommand>().single.path,
          driving.path,
        );
      });

      testWidgets('every file missing still paints every row', (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday, work],
          disabledCollectionPaths: {driving.path, sunday.path, work.path},
        );

        expect(find.byKey(MockupPlaylistCollectionPane.emptyKey), findsNothing);
        for (var index = 0; index < 3; index++) {
          expect(markIn(index), findsOneWidget);
        }
        expect(rowText(2, 'WORK'), findsOneWidget);
        expect(tester.takeException(), isNull);
      });
    });

    group('altered current playlist', () {
      const dialogKey = Key('pl-altered-dialog');
      const cancelKey = Key('pl-altered-cancel');
      const discardKey = Key('pl-altered-discard');
      const saveKey = Key('pl-altered-save');

      Iterable<LoadSavedPlaylistCommand> loads(List<SessionCommand> commands) =>
          commands.whereType<LoadSavedPlaylistCommand>();

      Iterable<PlaylistOpCommand> saves(List<SessionCommand> commands) =>
          commands.whereType<PlaylistOpCommand>().where(
                (c) => c.op == 'savePlaylist',
              );

      Future<void> tapSunday(WidgetTester tester) async {
        await tester.tap(find.byKey(const Key('pl-collection-row-1')));
        await tester.pumpAndSettle();
      }

      testWidgets('an unaltered playlist is replaced without asking',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
        );

        await tapSunday(tester);

        expect(find.byKey(dialogKey), findsNothing);
        expect(loads(commands).single.path, sunday.path);
      });

      testWidgets('an altered playlist offers save, discard, and cancel',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
        );

        await tapSunday(tester);

        expect(find.byKey(dialogKey), findsOneWidget);
        expect(find.byKey(saveKey), findsOneWidget);
        expect(find.byKey(discardKey), findsOneWidget);
        expect(find.byKey(cancelKey), findsOneWidget);
        expect(find.text('Save and load'), findsOneWidget);
        expect(find.text('Discard and load'), findsOneWidget);
        expect(find.text('Cancel'), findsOneWidget);
        // Nothing has happened yet — the listener has not answered.
        expect(commands, isEmpty);
      });

      testWidgets('cancel keeps the current playlist and loads nothing',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
        );

        await tapSunday(tester);
        await tester.tap(find.byKey(cancelKey));
        await tester.pumpAndSettle();

        expect(find.byKey(dialogKey), findsNothing);
        expect(commands, isEmpty);
        expect(find.textContaining('Alpha'), findsOneWidget);
      });

      testWidgets('an idle Return keypress cannot discard anything',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
        );

        await tapSunday(tester);
        // Cancel holds the default focus, so Return answers with it.
        await tester.sendKeyEvent(LogicalKeyboardKey.enter);
        await tester.pumpAndSettle();

        expect(find.byKey(dialogKey), findsNothing);
        expect(commands, isEmpty);
      });

      testWidgets('discard loads the new playlist', (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
        );

        await tapSunday(tester);
        await tester.tap(find.byKey(discardKey));
        await tester.pumpAndSettle();

        expect(find.byKey(dialogKey), findsNothing);
        expect(loads(commands).single.path, sunday.path);
        expect(saves(commands), isEmpty);
      });

      testWidgets('save writes straight to the origin, then loads',
          (tester) async {
        final commands = <SessionCommand>[];
        var pickerOpened = 0;
        playlist.setTracks(
          playlist.playlist.tracks,
          sourcePath: '/music/current.m3u',
        );
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
          pickSavePlaylistPath: () async {
            pickerOpened++;
            return '/music/elsewhere.m3u';
          },
        );

        await tapSunday(tester);
        await tester.tap(find.byKey(saveKey));
        await tester.pumpAndSettle();

        // Straight to the origin: no save dialog, and the save lands first.
        expect(pickerOpened, 0);
        expect(commands, hasLength(2));
        expect((commands[0] as PlaylistOpCommand).op, 'savePlaylist');
        expect((commands[0] as PlaylistOpCommand).path, '/music/current.m3u');
        expect((commands[1] as LoadSavedPlaylistCommand).path, sunday.path);
      });

      testWidgets('save with no origin opens the save dialog, then loads',
          (tester) async {
        final commands = <SessionCommand>[];
        var pickerOpened = 0;
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
          pickSavePlaylistPath: () async {
            pickerOpened++;
            return '/music/kept.m3u';
          },
        );

        expect(playlist.playlist.sourcePath, isNull);

        await tapSunday(tester);
        await tester.tap(find.byKey(saveKey));
        await tester.pumpAndSettle();

        expect(pickerOpened, 1);
        expect(commands, hasLength(2));
        expect((commands[0] as PlaylistOpCommand).op, 'savePlaylist');
        expect((commands[0] as PlaylistOpCommand).path, '/music/kept.m3u');
        expect((commands[1] as LoadSavedPlaylistCommand).path, sunday.path);
      });

      testWidgets(
          'cancelling that save dialog keeps the current playlist, still '
          'altered', (tester) async {
        final commands = <SessionCommand>[];
        var pickerOpened = 0;
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
          altered: true,
          // The listener backs out of the save dialog.
          pickSavePlaylistPath: () async {
            pickerOpened++;
            return null;
          },
        );

        await tapSunday(tester);
        await tester.tap(find.byKey(saveKey));
        await tester.pumpAndSettle();

        // Neither half happened: nothing was written, and nothing was loaded.
        expect(pickerOpened, 1);
        expect(saves(commands), isEmpty);
        expect(loads(commands), isEmpty);
        expect(commands, isEmpty);
        expect(find.textContaining('Alpha'), findsOneWidget);

        // Still protected: the next click asks again rather than falling
        // through to the load.
        await tapSunday(tester);
        expect(find.byKey(dialogKey), findsOneWidget);
        await tester.tap(find.byKey(cancelKey));
        await tester.pumpAndSettle();
        expect(loads(commands), isEmpty);
      });

      testWidgets('a change the host has not confirmed yet still asks',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving, sunday],
        );

        // Sorting here mutates this window's own mirror; the host's snapshot
        // saying so has not arrived yet.
        await tester.tap(find.byKey(const Key('pl-sort')));
        await tester.pumpAndSettle();
        await tester.tap(find.text('Duration').last);
        await tester.pumpAndSettle();
        expect(playlist.altered, isTrue);

        await tapSunday(tester);

        expect(find.byKey(dialogKey), findsOneWidget);
        expect(loads(commands), isEmpty);
      });
    });

    group('creating a playlist from every track', () {
      Iterable<CreatePlaylistFromCurrentCommand> creates(
        List<SessionCommand> commands,
      ) =>
          commands.whereType<CreatePlaylistFromCurrentCommand>();

      testWidgets('asks where to save, then keeps what was written',
          (tester) async {
        final commands = <SessionCommand>[];
        var pickerOpened = 0;
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving],
          pickSavePlaylistPath: () async {
            pickerOpened++;
            return '/music/kept pile.m3u';
          },
        );

        await tester.tap(find.byKey(const Key('pl-collection-create')));
        await tester.pumpAndSettle();

        expect(pickerOpened, 1);
        // One command, because the write and the reference are one action —
        // the entry joins the collection with no separate "now add it" step.
        expect(commands, hasLength(1));
        expect(creates(commands).single.path, '/music/kept pile.m3u');
      });

      testWidgets('cancelling the save dialog changes nothing at all',
          (tester) async {
        final commands = <SessionCommand>[];
        var pickerOpened = 0;
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving],
          pickSavePlaylistPath: () async {
            pickerOpened++;
            return null;
          },
        );

        await tester.tap(find.byKey(const Key('pl-collection-create')));
        await tester.pumpAndSettle();

        expect(pickerOpened, 1);
        expect(commands, isEmpty);
        // The current playlist is still exactly where it was.
        expect(find.textContaining('Alpha'), findsOneWidget);
        expect(find.byKey(const Key('pl-collection-row-0')), findsOneWidget);
      });

      testWidgets('an empty current playlist is refused, not half-written',
          (tester) async {
        final commands = <SessionCommand>[];
        var pickerOpened = 0;
        playlist.setTracks(const []);
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          collection: [driving],
          pickSavePlaylistPath: () async {
            pickerOpened++;
            return '/music/nothing.m3u';
          },
        );

        await tester.tap(find.byKey(const Key('pl-collection-create')));
        await tester.pumpAndSettle();

        // Refused outright: no dialog to back out of, and nothing emitted.
        expect(pickerOpened, 0);
        expect(commands, isEmpty);
      });

      testWidgets('it wakes up as soon as there is something to keep',
          (tester) async {
        final commands = <SessionCommand>[];
        playlist.setTracks(const []);
        await pumpPl(
          tester,
          commands: commands,
          size: const Size(1000, 700),
          pickSavePlaylistPath: () async => '/music/kept.m3u',
        );

        // A drop lands in this window's own mirror before the host echoes it.
        playlist.addTracks(const [Track(path: '/dropped.mp3')]);
        await tester.pump();

        await tester.tap(find.byKey(const Key('pl-collection-create')));
        await tester.pumpAndSettle();

        expect(creates(commands).single.path, '/music/kept.m3u');
      });

      testWidgets('create is its own control, beside add and remove',
          (tester) async {
        final commands = <SessionCommand>[];
        await pumpPl(
          tester,
          commands: commands,
          size: TrampMetrics.playlistMinWithCollection,
          collection: [driving],
        );

        expect(find.byKey(const Key('pl-collection-create')), findsOneWidget);
        expect(
          find.bySemanticsLabel('Create playlist from current playlist'),
          findsOneWidget,
        );
        expect(
          tester.takeException(),
          isNull,
          reason: 'three controls still fit the narrowest panel',
        );
      });
    });

    testWidgets('a collection snapshot mid-drag does not undo the drag',
        (tester) async {
      final commands = <SessionCommand>[];
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      await tester.drag(
        find.byKey(const Key('pl-divider')),
        const Offset(60, 0),
      );
      await tester.pump();
      final dragged = collectionPaneWidth(tester);

      // Same width prop, new collection — the rebuild a host broadcast causes.
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [driving],
      );

      expect(collectionPaneWidth(tester), dragged);
    });
  });

  group('multi-select', () {
    /// Five rows, so a range has an inside as well as two ends.
    void fiveTracks() {
      playlist.setTracks(const [
        Track(path: '/a.mp3', title: 'Alpha'),
        Track(path: '/b.mp3', title: 'Bravo'),
        Track(path: '/c.mp3', title: 'Charlie'),
        Track(path: '/d.mp3', title: 'Delta'),
        Track(path: '/e.mp3', title: 'Echo'),
      ]);
    }

    /// Clicks a row with [holding] down, the way a listener holds a modifier
    /// while clicking. Released again, so no test leaves a key stuck down.
    Future<void> clickRow(
      WidgetTester tester,
      int index, {
      List<LogicalKeyboardKey> holding = const [],
    }) async {
      for (final key in holding) {
        await tester.sendKeyDownEvent(key);
      }
      await tester.tap(find.byKey(Key('pl-row-$index')));
      // Rows answer to double-tap as well, so the arena is held open and a
      // single tap only lands once that window has passed.
      await tester.pump(const Duration(milliseconds: 400));
      await tester.pumpAndSettle();
      for (final key in holding) {
        await tester.sendKeyUpEvent(key);
      }
    }

    /// What the row actually paints: selected rows carry the highlight
    /// gradient, unselected ones carry none.
    bool rowReadsSelected(WidgetTester tester, int index) {
      final decoration = tester
          .widget<DecoratedBox>(
            find
                .descendant(
                  of: find.byKey(Key('pl-row-$index')),
                  matching: find.byType(DecoratedBox),
                )
                .first,
          )
          .decoration as BoxDecoration;
      return decoration.gradient != null;
    }

    Set<int> rowsReadingSelected(WidgetTester tester) => {
          for (var i = 0; i < 5; i++)
            if (rowReadsSelected(tester, i)) i,
        };

    Iterable<String> selectionOps(List<SessionCommand> commands) => commands
        .whereType<PlaylistOpCommand>()
        .map((c) => '${c.op}:${c.index}');

    testWidgets('shift-click selects the range, and every row shows it',
        (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      await clickRow(tester, 1);
      await clickRow(tester, 3, holding: [LogicalKeyboardKey.shiftLeft]);

      expect(playlist.selectedIndices, {1, 2, 3});
      expect(rowsReadingSelected(tester), {1, 2, 3});
      expect(selectionOps(commands), ['select:1', 'selectRange:3']);
    });

    testWidgets('the platform modifier toggles single rows in and out',
        (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      await clickRow(tester, 0);
      await clickRow(tester, 2, holding: [LogicalKeyboardKey.controlLeft]);
      await clickRow(tester, 4, holding: [LogicalKeyboardKey.controlLeft]);

      expect(rowsReadingSelected(tester), {0, 2, 4});

      await clickRow(tester, 2, holding: [LogicalKeyboardKey.controlLeft]);

      expect(rowsReadingSelected(tester), {0, 4});
      expect(
        selectionOps(commands),
        ['select:0', 'toggleSelect:2', 'toggleSelect:4', 'toggleSelect:2'],
      );
    });

    testWidgets('on macOS it is Command that toggles, and Control that does not',
        (tester) async {
      // Reset inside the body: the framework checks for a leaked debug
      // override before tear-downs run.
      debugDefaultTargetPlatformOverride = TargetPlatform.macOS;
      try {
        final commands = <SessionCommand>[];
        fiveTracks();
        await pumpPl(tester, commands: commands, size: const Size(1000, 700));

        await clickRow(tester, 0);
        await clickRow(tester, 2, holding: [LogicalKeyboardKey.metaLeft]);

        expect(rowsReadingSelected(tester), {0, 2});

        // Control is not the Mac convention, so it reads as a plain tap.
        await clickRow(tester, 4, holding: [LogicalKeyboardKey.controlLeft]);

        expect(rowsReadingSelected(tester), {4});
      } finally {
        debugDefaultTargetPlatformOverride = null;
      }
    });

    testWidgets('a plain tap collapses the selection to one row',
        (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      await clickRow(tester, 0);
      await clickRow(tester, 4, holding: [LogicalKeyboardKey.shiftLeft]);
      expect(rowsReadingSelected(tester), {0, 1, 2, 3, 4});

      await clickRow(tester, 2);

      expect(rowsReadingSelected(tester), {2});
      expect(playlist.selectedIndices, {2});
    });

    testWidgets('shift-clicking with nothing selected selects just that row',
        (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));
      expect(playlist.selectedIndex, isNull);

      await clickRow(tester, 3, holding: [LogicalKeyboardKey.shiftLeft]);

      expect(rowsReadingSelected(tester), {3});
      expect(tester.takeException(), isNull);
    });

    testWidgets('removing tracks acts on the whole selection', (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      await clickRow(tester, 1);
      await clickRow(tester, 3, holding: [LogicalKeyboardKey.shiftLeft]);
      await tester.tap(find.byKey(const Key('pl-remove')));
      await tester.pump();

      expect(
        playlist.playlist.tracks.map((t) => t.title),
        ['Alpha', 'Echo'],
      );
      expect(
        commands.whereType<PlaylistOpCommand>().map((c) => c.op),
        contains('removeSelected'),
      );
    });

    testWidgets('selecting rows never marks the current playlist altered',
        (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(
        tester,
        commands: commands,
        size: const Size(1000, 700),
        collection: [SavedPlaylist(path: '/music/driving.m3u')],
      );

      await clickRow(tester, 0);
      await clickRow(tester, 3, holding: [LogicalKeyboardKey.shiftLeft]);
      await clickRow(tester, 2, holding: [LogicalKeyboardKey.controlLeft]);

      expect(playlist.altered, isFalse);
      // And the proof a listener would meet: clicking a saved playlist does
      // not stop to ask about work that was never done.
      await tester.tap(find.byKey(const Key('pl-collection-row-0')));
      await tester.pumpAndSettle();
      expect(find.byKey(const Key('pl-altered-dialog')), findsNothing);
      expect(commands.whereType<LoadSavedPlaylistCommand>(), hasLength(1));
    });

    testWidgets('double-tap still plays the row it landed on', (tester) async {
      final commands = <SessionCommand>[];
      fiveTracks();
      await pumpPl(tester, commands: commands, size: const Size(1000, 700));

      await tester.tap(find.byKey(const Key('pl-row-2')));
      await tester.pump(const Duration(milliseconds: 50));
      await tester.tap(find.byKey(const Key('pl-row-2')));
      await tester.pumpAndSettle();

      expect(
        commands.whereType<PlaylistOpCommand>().map((c) => c.op),
        contains('playIndex'),
      );
    });
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
