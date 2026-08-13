import 'dart:ui';

import 'package:flutter/material.dart' hide RepeatMode;
import 'package:flutter/rendering.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/repeat_mode.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/ui/chrome/mockup/mockup_button.dart';
import 'package:tramp/ui/main_player/mockup_main_player.dart';
import 'package:tramp/ui/session/session_messages.dart';
import 'package:tramp/ui/windows/main_player_window.dart';

import '../../support/test_fonts.dart';
import '../../support/look_harness.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

void main() {
  late FakePlayerEngine engine;
  late PlaylistController playlist;
  late PlaybackController playback;

  setUpAll(loadTrampFonts);

  setUp(() {
    engine = FakePlayerEngine();
    playlist = PlaylistController(store: MemoryStore());
    playback = PlaybackController(playlist: playlist, engine: engine);
  });

  tearDown(() async => playback.dispose());

  Future<void> pumpPlayer(
    WidgetTester tester, {
    required List<SessionCommand> commands,
    bool forceMono = false,
    bool equalizerVisible = true,
    bool playlistVisible = true,
    bool alwaysOnTop = false,
  }) async {
    await tester.binding.setSurfaceSize(const Size(900, 400));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: wrapWithLook(
            MainPlayerWindow(
              playback: playback,
              trackCount: playlist.playlist.tracks.length,
              forceMono: forceMono,
              alwaysOnTop: alwaysOnTop,
              equalizerVisible: equalizerVisible,
              playlistVisible: playlistVisible,
              draggableTitle: false,
              onSessionCommand: commands.add,
            ),
          ),
        ),
      ),
    );
  }

  testWidgets('EQ toggle sends ToggleWindow for equalizer', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands, equalizerVisible: true);

    await tester.tap(find.byKey(const Key('player-eq')));
    await tester.pump();

    expect(commands, hasLength(1));
    final cmd = commands.single as ToggleWindowCommand;
    expect(cmd.window, WindowId.equalizer);
    expect(cmd.visible, isFalse);
  });

  testWidgets('PL toggle sends ToggleWindow for playlist', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands, playlistVisible: false);

    await tester.tap(find.byKey(const Key('player-pl')));
    await tester.pump();

    expect(commands, hasLength(1));
    final cmd = commands.single as ToggleWindowCommand;
    expect(cmd.window, WindowId.playlist);
    expect(cmd.visible, isTrue);
  });

  testWidgets('Mono sends MonoCommand (SetMono)', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands, forceMono: false);

    await tester.tap(find.byKey(const Key('player-mono')));
    await tester.pump();

    expect(commands, hasLength(1));
    expect(commands.single, isA<MonoCommand>());
    expect((commands.single as MonoCommand).enabled, isTrue);
  });

  testWidgets('the abbreviated buttons spell themselves out on hover',
      (tester) async {
    // EQ, PL and Mono are the faces that say least, so their tooltips carry
    // the whole explanation — including which way the toggle will go.
    await pumpPlayer(
      tester,
      commands: <SessionCommand>[],
      equalizerVisible: false,
      playlistVisible: true,
      forceMono: false,
    );

    // One pointer, moved between targets — adding a second without removing
    // the first trips the mouse tracker.
    final gesture = await tester.createGesture(kind: PointerDeviceKind.mouse);
    await gesture.addPointer(location: Offset.zero);
    addTearDown(gesture.removePointer);
    await tester.pump();

    Future<void> hover(Key key) async {
      await gesture.moveTo(tester.getCenter(find.byKey(key)));
      await tester.pump();
      await tester.pump(const Duration(seconds: 1));
    }

    await hover(const Key('player-eq'));
    expect(find.text('Show equalizer'), findsOneWidget);

    await hover(const Key('player-pl'));
    expect(find.text('Hide Playlist Manager'), findsOneWidget);

    await hover(const Key('player-mono'));
    expect(find.text('Fold both channels to mono'), findsOneWidget);
  });

  testWidgets('options menu Always on top sends AlwaysOnTopCommand',
      (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands, alwaysOnTop: false);

    await tester.tap(find.byKey(const Key('player-options')));
    await tester.pump();
    expect(
      tester.widget<MockupButton>(find.byKey(const Key('player-options'))).on,
      isTrue,
    );
    await tester.pumpAndSettle();
    await tester.tap(find.text('Always on top'));
    await tester.pumpAndSettle();

    expect(commands.single, isA<AlwaysOnTopCommand>());
    expect((commands.single as AlwaysOnTopCommand).enabled, isTrue);
    expect(
      tester.widget<MockupButton>(find.byKey(const Key('player-options'))).on,
      isFalse,
    );
  });

  testWidgets('separate play and pause drive the controller', (tester) async {
    playlist.addTracks([
      const Track(path: 'a.mp3'),
      const Track(path: 'b.mp3'),
    ]);
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands);

    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pumpAndSettle();
    expect(playback.playing, isTrue);

    await tester.tap(find.byKey(const Key('transport-pause')));
    await tester.pumpAndSettle();
    expect(playback.playing, isFalse);
  });

  testWidgets('play/pause/mute use btn--on like PL and Mono', (tester) async {
    playlist.addTracks([const Track(path: 'a.mp3')]);
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands);

    MockupButton btn(Key key) => tester.widget<MockupButton>(find.byKey(key));

    expect(btn(const Key('transport-play')).on, isFalse);
    expect(btn(const Key('transport-pause')).on, isFalse);
    expect(btn(const Key('transport-mute')).on, isFalse);

    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pumpAndSettle();
    expect(btn(const Key('transport-play')).on, isTrue);
    expect(btn(const Key('transport-pause')).on, isFalse);

    await tester.tap(find.byKey(const Key('transport-pause')));
    await tester.pumpAndSettle();
    expect(btn(const Key('transport-play')).on, isFalse);
    expect(btn(const Key('transport-pause')).on, isTrue);

    await tester.tap(find.byKey(const Key('transport-mute')));
    await tester.pumpAndSettle();
    expect(btn(const Key('transport-mute')).on, isTrue);
  });

  testWidgets('transport looks disabled with an empty playlist', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands);

    for (final key in const [
      Key('transport-prev'),
      Key('transport-play'),
      Key('transport-pause'),
      Key('transport-stop'),
      Key('transport-next'),
    ]) {
      final semantics = tester.getSemantics(find.byKey(key));
      expect(semantics.hasFlag(SemanticsFlag.isEnabled), isFalse, reason: '$key');
      expect(
        find.descendant(
          of: find.byKey(key),
          matching: find.byWidgetPredicate(
            (w) => w is Opacity && w.opacity < 1.0,
          ),
        ),
        findsOneWidget,
        reason: '$key should be visually dimmed',
      );
    }

    // Play must not keep a permanent hot glow when there is nothing to play.
    expect(
      find.descendant(
        of: find.byKey(const Key('transport-play')),
        matching: find.byType(ImageFiltered),
      ),
      findsNothing,
    );
  });

  testWidgets('options cog opens menu with clutter actions', (tester) async {
    final commands = <SessionCommand>[];
    final actions = <String>[];
    await tester.binding.setSurfaceSize(const Size(900, 400));
    addTearDown(() => tester.binding.setSurfaceSize(null));
    await tester.pumpWidget(
      MaterialApp(
        home: Align(
          alignment: Alignment.topLeft,
          child: wrapWithLook(
            MainPlayerWindow(
              playback: playback,
              trackCount: playlist.playlist.tracks.length,
              draggableTitle: false,
              onSessionCommand: commands.add,
              onOptionsAction: (context, action) => actions.add(action),
            ),
          ),
        ),
      ),
    );

    expect(find.byKey(const Key('player-options')), findsOneWidget);
    expect(find.byKey(const Key('clutter-o')), findsNothing);

    await tester.tap(find.byKey(const Key('player-options')));
    await tester.pumpAndSettle();

    expect(find.text('Always on top'), findsOneWidget);
    expect(find.text('Settings…'), findsOneWidget);
    expect(find.text('Track info'), findsOneWidget);
    expect(find.text('About Tramp'), findsOneWidget);
    expect(find.text('Quit'), findsOneWidget);

    await tester.tap(find.text('Track info'));
    await tester.pumpAndSettle();
    expect(actions, ['info']);

    final body = tester.getSize(find.byType(MockupMainPlayer));
    expect(body, MockupMainPlayer.bodySize);
  });

  testWidgets('window is 825×348', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands);
    expect(
      tester.getSize(find.byType(MainPlayerWindow)),
      MainPlayerWindow.logicalSize,
    );
  });

  testWidgets('display repeat chip cycles OFF → PLAYLIST → TRACK', (tester) async {
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands);

    final chip = find.byKey(const Key('player-display-repeat'));
    expect(chip, findsOneWidget);
    expect(
      find.descendant(of: chip, matching: find.text('OFF')),
      findsOneWidget,
    );
    expect(playback.repeatMode, RepeatMode.off);

    await tester.tap(chip);
    await tester.pump();
    expect(playback.repeatMode, RepeatMode.all);
    expect(
      find.descendant(of: chip, matching: find.text('PLAYLIST')),
      findsOneWidget,
    );

    await tester.tap(chip);
    await tester.pump();
    expect(playback.repeatMode, RepeatMode.one);
    expect(
      find.descendant(of: chip, matching: find.text('TRACK')),
      findsOneWidget,
    );

    await tester.tap(chip);
    await tester.pump();
    expect(playback.repeatMode, RepeatMode.off);
    expect(
      find.descendant(of: chip, matching: find.text('OFF')),
      findsOneWidget,
    );
  });

  testWidgets('long title and album do not overflow the display with PLAYLIST',
      (tester) async {
    playlist.addTracks([
      const Track(
        path: 'aion.mp3',
        title: 'Aion – kingdom.',
        artist: 'Very Long Artist Name That Keeps Going',
        album: 'IXION: ORIGINAL SOUNDTRACK — DELUXE EDITION VOLUME ONE',
        year: 2022,
      ),
      const Track(path: 'b.mp3', title: 'B'),
      const Track(path: 'c.mp3', title: 'C'),
    ]);
    playback.cycleRepeatMode(); // all → PLAYLIST chip visible
    final commands = <SessionCommand>[];
    await pumpPlayer(tester, commands: commands);

    await tester.tap(find.byKey(const Key('transport-play')));
    await tester.pump();
    // Marquee schedules a post-frame restart; advance past layout + a tick.
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 100));

    expect(tester.takeException(), isNull);
    expect(find.text('PLAYLIST'), findsOneWidget);
  });
}
