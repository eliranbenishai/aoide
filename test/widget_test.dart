import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/ui/windows/main_player_window.dart';

import 'support/test_fonts.dart';
import 'support/look_harness.dart';

class _MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  setUpAll(loadTrampFonts);

  testWidgets('mockup main chrome shows TRAMP brand', (tester) async {
    final engine = FakePlayerEngine();
    final playlist = PlaylistController(store: _MemoryStore());
    final playback = PlaybackController(playlist: playlist, engine: engine);
    addTearDown(playback.dispose);

    playlist.setTracks([
      const Track(path: 'a.mp3', title: 'Demo', artist: 'Tramp'),
    ]);

    await tester.pumpWidget(
      MaterialApp(
        builder: (context, child) => wrapWithLook(child ?? const SizedBox.shrink()),
        home: ColoredBox(
          color: MockupTokens.shellDeep,
          child: MainPlayerWindow(
            playback: playback,
            trackCount: 1,
            draggableTitle: false,
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    expect(find.textContaining('TRAMP'), findsWidgets);
    expect(find.byKey(const Key('player-mono')), findsOneWidget);
  });
}
