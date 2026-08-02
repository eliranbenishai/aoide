import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/ui/classic_main_player.dart';
import 'package:tramp/ui/tramp_shell.dart';

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

  testWidgets('shell scales player to fit short window without overflow', (
    tester,
  ) async {
    final playlist = PlaylistController(store: _Mem());
    final playback =
        PlaybackController(playlist: playlist, engine: FakePlayerEngine());
    addTearDown(playback.dispose);
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.binding.setSurfaceSize(const Size(550, 160));

    await tester.pumpWidget(
      MaterialApp(
        home: TrampShell(
          playback: playback,
          playlistController: playlist,
          transport: ClassicMainPlayer(
            playback: playback,
            hasTracks: false,
          ),
          playlist: const ColoredBox(
            key: Key('playlist-pane'),
            color: Colors.black,
          ),
        ),
      ),
    );
    await tester.pump();

    expect(tester.takeException(), isNull);
    final host = tester.getSize(find.byKey(playerScaleHostKey));
    expect(host.height, lessThanOrEqualTo(160));
    expect(host.width / host.height, closeTo(550 / 232, 0.05));
  });
}
