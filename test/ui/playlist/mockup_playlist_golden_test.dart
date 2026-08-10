// Golden: mockup-faithful playlist at 100% (825×696) + shade.
@Tags(['golden'])
library;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/ui/chrome/mockup/mockup_shell.dart';
import 'package:tramp/ui/windows/playlist_window.dart';

import '../../support/test_fonts.dart';
import '../../support/look_harness.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

PlaylistController _mockupPlaylist() {
  final c = PlaylistController(store: MemoryStore());
  c.setTracks(
    const [
      Track(
        path: '/1.mp3',
        title: 'Low Orbit Lullaby',
        artist: 'Cassette Mirage',
        duration: Duration(minutes: 4, seconds: 12),
      ),
      Track(
        path: '/2.mp3',
        title: 'Slow Dial',
        artist: 'The Brass Cassini',
        duration: Duration(minutes: 3, seconds: 38),
      ),
      Track(
        path: '/3.mp3',
        title: 'Neon Boulevard (Extended Mix)',
        artist: 'Velvet Static',
        duration: Duration(minutes: 5, seconds: 47),
      ),
      Track(
        path: '/4.mp3',
        title: 'Parking Garage Sunset',
        artist: 'Halogen Youth',
        duration: Duration(minutes: 4, seconds: 3),
      ),
      Track(
        path: '/5.mp3',
        title: 'Analogue Ghosts',
        artist: 'Moth & Marrow',
        duration: Duration(minutes: 6, seconds: 21),
      ),
      Track(
        path: '/6.mp3',
        title: 'Bakelite Heart',
        artist: 'Ruby Transit',
        duration: Duration(minutes: 3, seconds: 55),
      ),
      Track(
        path: '/7.mp3',
        title: 'Copper Rain',
        artist: 'Slow Signal',
        duration: Duration(minutes: 4, seconds: 44),
      ),
      Track(
        path: '/8.mp3',
        title: 'Departure Lounge B',
        artist: 'Aurora Kiosk',
        duration: Duration(minutes: 5, seconds: 9),
      ),
      Track(
        path: '/9.mp3',
        title: 'Tramp Theme (Demo)',
        artist: 'Pale Antenna',
        duration: Duration(minutes: 2, seconds: 58),
      ),
      Track(
        path: '/10.mp3',
        title: 'Fluorescent Hymn',
        artist: 'Nightbus Choir',
        duration: Duration(minutes: 6, seconds: 2),
      ),
      Track(
        path: '/11.mp3',
        title: 'Static Blonde',
        artist: 'Second Cassette',
        duration: Duration(minutes: 3, seconds: 27),
      ),
      Track(
        path: '/12.mp3',
        title: 'Neon Boulevard (Reprise)',
        artist: 'Velvet Static',
        duration: Duration(minutes: 2, seconds: 2),
      ),
      Track(
        path: '/13.mp3',
        title: 'Untitled Sketch',
        artist: 'Long Wave Motel',
        duration: Duration(minutes: 3, seconds: 16),
      ),
    ],
    sourcePath: r'copper rain — night set.m3u8',
  );
  c.select(2);
  return c;
}

void main() {
  setUpAll(() async {
    await loadTrampFonts();
    await MockupShell.ensureNoiseReady();
  });

  testWidgets('playlist window matches mockup layout at 100%', (tester) async {
    const size = Size(825, 696);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: wrapWithLook(
              PlaylistWindow(
                playlist: _mockupPlaylist(),
                playingIndex: 2,
                // Mockup transport shows Play (not Pause); row 3 still uses play style.
                playing: false,
                draggableTitle: false,
              ),
            ),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await expectLater(
      find.byType(PlaylistWindow),
      matchesGoldenFile('goldens/playlist_window.png'),
    );
  });

  testWidgets('playlist shade matches title-bar chrome', (tester) async {
    const size = Size(825, 42);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: wrapWithLook(
              PlaylistWindow(
                playlist: _mockupPlaylist(),
                shaded: true,
                draggableTitle: false,
              ),
            ),
          ),
        ),
      ),
    );
    await tester.pumpAndSettle();

    await expectLater(
      find.byType(PlaylistWindow),
      matchesGoldenFile('goldens/playlist_window_shade.png'),
    );
  });
}
