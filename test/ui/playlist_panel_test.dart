import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/playlist_panel.dart';
import 'package:tramp/ui/skin/nine_slice_skin.dart';

import '../support/test_fonts.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  late PlaylistController playlist;
  late PlaybackController playback;

  setUpAll(loadTrampFonts);

  setUp(() {
    playlist = PlaylistController(store: MemoryStore());
    playback = PlaybackController(
      playlist: playlist,
      engine: FakePlayerEngine(),
    );
    playlist.addTracks(const [
      Track(path: 'a.mp3', title: 'Alpha'),
      Track(path: 'b.mp3', title: 'Beta', artist: 'Someone'),
    ]);
  });

  tearDown(() async => playback.dispose());

  Future<void> pump(WidgetTester tester) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: PlaylistPanel(playlist: playlist, playback: playback),
        ),
      ),
    );
  }

  testWidgets('lists every track and its artist', (tester) async {
    await pump(tester);
    expect(find.text('Alpha'), findsOneWidget);
    expect(find.text('Beta'), findsOneWidget);
    expect(find.text('Someone'), findsOneWidget);
  });

  testWidgets('wears the graphite 9-slice playlist chrome', (tester) async {
    await pump(tester);
    final skin = tester.widget<NineSliceSkin>(find.byType(NineSliceSkin));
    expect(skin.slices, PlaylistSlices.graphite);
  });

  testWidgets('the selected row title is phosphor, others are label',
      (tester) async {
    playlist.select(1);
    await pump(tester);
    expect(
      tester.widget<Text>(find.text('Beta')).style!.color,
      TrampColors.phosphor,
    );
    expect(
      tester.widget<Text>(find.text('Alpha')).style!.color,
      TrampColors.label,
    );
  });

  testWidgets('toolbar buttons are wired to their callbacks', (tester) async {
    var opens = 0;
    var saves = 0;
    var adds = 0;
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: PlaylistPanel(
            playlist: playlist,
            playback: playback,
            onOpen: () => opens++,
            onSave: () => saves++,
            onAddFiles: () => adds++,
          ),
        ),
      ),
    );
    await tester.tap(find.byKey(const Key('playlist-open')));
    await tester.tap(find.byKey(const Key('playlist-save')));
    await tester.tap(find.byKey(const Key('playlist-add')));
    expect([opens, saves, adds], [1, 1, 1]);
  });

  testWidgets('tapping selects and double-tapping starts playback',
      (tester) async {
    await pump(tester);
    await tester.tap(find.text('Beta'));
    // onTap+onDoubleTap: the recognizer waits for the double-tap window.
    await tester.pump(kDoubleTapTimeout);
    expect(playlist.selectedIndex, 1);

    await tester.tap(find.text('Alpha'));
    await tester.pump(const Duration(milliseconds: 50));
    await tester.tap(find.text('Alpha'));
    await tester.pump();
    await tester.pumpAndSettle();
    expect(playback.playingIndex, 0);
  });

  testWidgets('an empty playlist says so', (tester) async {
    // PlaylistController.clear() exists; use it to empty the list.
    playlist.clear();
    await pump(tester);
    expect(find.text('No tracks'), findsOneWidget);
  });

  testWidgets('no light surface survives anywhere in the tree',
      (tester) async {
    await pump(tester);
    for (final box in tester.widgetList<DecoratedBox>(
      find.byType(DecoratedBox),
    )) {
      final decoration = box.decoration as BoxDecoration;
      final colour = decoration.color;
      if (colour != null && colour.a > 0) {
        expect(colour.computeLuminance(), lessThan(0.5),
            reason: 'playlist chrome must stay dark');
      }
      final gradient = decoration.gradient;
      if (gradient is LinearGradient) {
        for (final c in gradient.colors) {
          expect(c.computeLuminance(), lessThan(0.5),
              reason: 'playlist gradients must stay dark');
        }
      }
    }
  });

  testWidgets('never uses Material Icon fonts', (tester) async {
    await pump(tester);
    expect(find.byType(Icon), findsNothing);
  });

  testWidgets(
      'a large list scrolls and the 9-slice fills a resized well',
      (tester) async {
    await tester.binding.setSurfaceSize(const Size(900, 1200));
    addTearDown(() => tester.binding.setSurfaceSize(null));

    playlist.clear();
    playlist.addTracks([
      for (var i = 0; i < 2000; i++) Track(path: '$i.mp3', title: 'Track $i'),
    ]);

    // Grow, then shrink, the hosting well: the skin must track the parent both
    // ways without overflowing or distorting.
    for (final size in const [Size(900, 1200), Size(420, 300)]) {
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: Center(
              child: SizedBox(
                width: size.width,
                height: size.height,
                child: PlaylistPanel(playlist: playlist, playback: playback),
              ),
            ),
          ),
        ),
      );
      await tester.pump();
      expect(tester.takeException(), isNull);
      expect(tester.getSize(find.byType(NineSliceSkin)), size);
    }

    // The list virtualises: the first rows are built but the 2000th is not, so
    // a huge playlist stays cheap to render in the grown well.
    expect(find.text('Track 0'), findsOneWidget);
    expect(find.text('Track 1999'), findsNothing);

    // Jumping the scroll position reveals later rows and the chrome still fills
    // the well exactly (no distortion from the large list or the resize).
    final position = tester.state<ScrollableState>(find.byType(Scrollable));
    position.position.jumpTo(1000);
    await tester.pump();
    expect(find.text('Track 0'), findsNothing);
    expect(tester.getSize(find.byType(NineSliceSkin)), const Size(420, 300));
  });

  testWidgets('drag handles align across rows with varying content',
      (tester) async {
    playlist.clear();
    playlist.addTracks(const [
      Track(
        path: 'a.mp3',
        title: 'A',
        duration: Duration(minutes: 3, seconds: 45),
      ),
      Track(
        path: 'b.mp3',
        title: 'A Very Long Title That Must Not Push The Trailing Controls',
        artist: 'Someone',
        duration: Duration(minutes: 12, seconds: 34),
      ),
      Track(
        path: 'c.mp3',
        title: 'Short',
        artist: 'Someone With A Long Artist Name',
      ),
    ]);
    await pump(tester);
    await tester.pumpAndSettle();

    final handles = find.byWidgetPredicate(
      (widget) =>
          widget is SizedBox &&
          widget.width == 12 &&
          widget.height == 10 &&
          widget.child is CustomPaint,
    );
    expect(handles, findsNWidgets(3));

    final xPositions = <double>[
      for (var i = 0; i < 3; i++) tester.getTopLeft(handles.at(i)).dx,
    ];
    expect(
      xPositions.toSet().length,
      1,
      reason: 'drag handles must share the same x; got $xPositions',
    );
  });
}
