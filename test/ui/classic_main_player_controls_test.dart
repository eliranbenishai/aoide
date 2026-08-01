import 'package:flutter/material.dart' hide RepeatMode;
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/repeat_mode.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_colors.dart';
import 'package:tramp/ui/classic_main_player.dart';
import 'package:tramp/ui/tramp_shell.dart';

class _Mem implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  late PlaylistController playlist;
  late FakePlayerEngine engine;
  late PlaybackController playback;

  setUp(() {
    playlist = PlaylistController(store: _Mem());
    playlist.addTracks(const [
      Track(path: '/song.mp3', title: 'Neon Dreams', artist: 'Volt'),
    ]);
    engine = FakePlayerEngine(trackDuration: const Duration(minutes: 3));
    playback = PlaybackController(playlist: playlist, engine: engine);
    addTearDown(playback.dispose);
  });

  Future<void> pumpPlayer(
    WidgetTester tester, {
    VoidCallback? onFocusPlaylist,
  }) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Center(
          child: ClassicMainPlayer(
            playback: playback,
            hasTracks: true,
            onFocusPlaylist: onFocusPlaylist,
          ),
        ),
      ),
    );
    await tester.pump();
  }

  testWidgets('play and stop transport buttons drive playback', (tester) async {
    await pumpPlayer(tester);

    expect(find.textContaining('TRAMP'), findsOneWidget);

    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pump();
    expect(playback.playing, isTrue);

    await tester.tap(find.byKey(const Key('transport-stop')));
    await tester.pump();
    expect(playback.playing, isFalse);
  });

  testWidgets('LCD shows title, unknown rates, and STEREO when track open', (
    tester,
  ) async {
    await playback.playIndex(0);
    await pumpPlayer(tester);

    expect(find.textContaining('Neon Dreams'), findsOneWidget);
    expect(find.textContaining('— kbps'), findsOneWidget);
    expect(find.textContaining('— kHz'), findsOneWidget);
    expect(find.text('STEREO'), findsOneWidget);
    expect(find.textContaining('0:00'), findsWidgets);
  });

  testWidgets('SHUF and REP toggle playback modes', (tester) async {
    await pumpPlayer(tester);

    expect(playback.shuffle, isFalse);
    await tester.tap(find.byKey(const Key('lcd-shuffle')));
    await tester.pump();
    expect(playback.shuffle, isTrue);

    expect(playback.repeatMode, RepeatMode.off);
    await tester.tap(find.byKey(const Key('lcd-repeat')));
    await tester.pump();
    expect(playback.repeatMode, RepeatMode.all);
  });

  testWidgets('PL focuses playlist via shell FocusNode', (tester) async {
    final playlistFocus = FocusNode();
    addTearDown(playlistFocus.dispose);

    await tester.pumpWidget(
      MaterialApp(
        home: TrampShell(
          playback: playback,
          playlistController: playlist,
          hasTracks: true,
          playlistFocusNode: playlistFocus,
          transport: ClassicMainPlayer(
            playback: playback,
            hasTracks: true,
            playlistFocusNode: playlistFocus,
            onFocusPlaylist: playlistFocus.requestFocus,
          ),
          playlist: const SizedBox(
            key: Key('playlist-target'),
            height: 80,
            child: Text('playlist'),
          ),
        ),
      ),
    );
    await tester.pump();

    expect(playlistFocus.hasFocus, isFalse);
    await tester.tap(find.byKey(const Key('lcd-playlist')));
    await tester.pump();
    expect(playlistFocus.hasFocus, isTrue);
  });

  testWidgets('PL lit when tracks present or playlist focused', (tester) async {
    final playlistFocus = FocusNode();
    addTearDown(playlistFocus.dispose);

    Color? plColor() {
      final text = tester.widget<Text>(
        find.descendant(
          of: find.byKey(const Key('lcd-playlist')),
          matching: find.text('PL'),
        ),
      );
      return text.style?.color;
    }

    Future<void> pump({required bool hasTracks}) async {
      await tester.pumpWidget(
        MaterialApp(
          home: TrampShell(
            playback: playback,
            playlistController: playlist,
            hasTracks: hasTracks,
            playlistFocusNode: playlistFocus,
            transport: ClassicMainPlayer(
              playback: playback,
              hasTracks: hasTracks,
              playlistFocusNode: playlistFocus,
              onFocusPlaylist: playlistFocus.requestFocus,
            ),
            playlist: const SizedBox(height: 80, child: Text('playlist')),
          ),
        ),
      );
      await tester.pump();
    }

    await pump(hasTracks: false);
    expect(playlistFocus.hasFocus, isFalse);
    expect(plColor(), isNot(TrampColors.lcdPhosphor));

    await tester.tap(find.byKey(const Key('lcd-playlist')));
    await tester.pumpAndSettle();
    expect(playlistFocus.hasFocus, isTrue);
    expect(plColor(), TrampColors.lcdPhosphor);

    playlistFocus.unfocus();
    await tester.pumpAndSettle();
    expect(playlistFocus.hasFocus, isFalse);
    expect(plColor(), isNot(TrampColors.lcdPhosphor));

    await pump(hasTracks: true);
    expect(plColor(), TrampColors.lcdPhosphor);
  });

  testWidgets('volume speaker toggles mute', (tester) async {
    await pumpPlayer(tester);
    expect(playback.muted, isFalse);

    await tester.tap(find.byKey(const Key('transport-mute')));
    await tester.pump();
    expect(playback.muted, isTrue);
  });
}
