import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/ui/chrome/tramp_logo.dart';
import 'package:tramp/ui/classic_main_player.dart';

class _Mem implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  testWidgets('TrampLogo paints via CustomPaint', (tester) async {
    await tester.pumpWidget(
      const MaterialApp(
        home: Scaffold(
          body: Center(child: TrampLogo()),
        ),
      ),
    );
    expect(find.byType(TrampLogo), findsOneWidget);
    expect(find.byType(CustomPaint), findsWidgets);
  });

  testWidgets('ClassicMainPlayer title bar includes TrampLogo', (tester) async {
    final playlist = PlaylistController(store: _Mem());
    final playback = PlaybackController(
      playlist: playlist,
      engine: FakePlayerEngine(),
    );
    addTearDown(playback.dispose);

    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: ClassicMainPlayer(playback: playback, hasTracks: false),
        ),
      ),
    );
    await tester.pump();

    expect(find.byType(TrampLogo), findsOneWidget);
    expect(
      find.descendant(
        of: find.byType(ClassicMainPlayer),
        matching: find.byType(TrampLogo),
      ),
      findsOneWidget,
    );
  });
}
