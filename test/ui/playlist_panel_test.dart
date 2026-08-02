import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/chrome/metal_panel.dart';
import 'package:tramp/ui/playlist_panel.dart';

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

  testWidgets('wears the graphite panel and glass surfaces', (tester) async {
    await pump(tester);
    final surfaces = tester
        .widgetList<MetalPanel>(find.byType(MetalPanel))
        .map((p) => p.surface)
        .toList();
    expect(surfaces, contains(TrampSurface.raisedPanel));
    expect(surfaces, contains(TrampSurface.lcdGlass));
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
      if (colour != null && colour.alpha > 0) {
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
}
