import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/ui/classic_main_player.dart';

class _Mem implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;
  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  testWidgets('ClassicMainPlayer keeps logical aspect under wide parent', (tester) async {
    final playlist = PlaylistController(store: _Mem());
    final playback = PlaybackController(playlist: playlist, engine: FakePlayerEngine());
    addTearDown(playback.dispose);
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: SizedBox(
            width: 900,
            height: 400,
            child: FittedBox(
              fit: BoxFit.contain,
              child: ClassicMainPlayer(playback: playback, hasTracks: false),
            ),
          ),
        ),
      ),
    );
    await tester.pump();
    final size = tester.getSize(find.byType(ClassicMainPlayer));
    expect(size.width / size.height, closeTo(550 / 232, 0.05));
  });
}
