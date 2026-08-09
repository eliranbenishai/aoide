// Golden: mockup-faithful main player at 100% (825×348).
@Tags(['golden'])
library;

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/mockup_tokens.dart';
import 'package:tramp/ui/chrome/mockup/mockup_shell.dart';
import 'package:tramp/ui/windows/main_player_window.dart';

import '../../support/test_fonts.dart';

class MemoryStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

/// Spectrum heights / peaks from `player-mockup-2.html` `.viz i` demo.
const _mockupBars = <double>[
  0.26, 0.52, 0.71, 0.88, 0.64, 0.47, 0.58, 0.39, 0.31, 0.44,
  0.35, 0.24, 0.29, 0.19, 0.22, 0.14, 0.17, 0.10, 0.12, 0.07,
];
const _mockupPeaks = <double>[
  0.44, 0.70, 0.88, 0.96, 0.80, 0.66, 0.74, 0.57, 0.52, 0.61,
  0.55, 0.42, 0.47, 0.36, 0.40, 0.30, 0.33, 0.24, 0.27, 0.19,
];

void main() {
  late FakePlayerEngine engine;
  late PlaylistController playlist;
  late PlaybackController playback;

  setUpAll(() async {
    await loadTrampFonts();
    await MockupShell.ensureNoiseReady();
  });

  setUp(() {
    engine = FakePlayerEngine(
      trackDuration: const Duration(minutes: 5, seconds: 47),
    );
    playlist = PlaylistController(store: MemoryStore());
    playback = PlaybackController(playlist: playlist, engine: engine);
  });

  tearDown(() async => playback.dispose());

  testWidgets('main player window matches mockup layout at 100%', (tester) async {
    // Seed display to match the static mockup demo content.
    final tracks = List<Track>.generate(
      12,
      (i) => Track(
        path: 'track_$i.mp3',
        title: i == 2
            ? 'Neon Boulevard (Extended Mix)'
            : 'Track ${i + 1}',
        artist: i == 2 ? 'Velvet Static' : null,
        album: i == 2 ? 'Copper Rain EP' : null,
        duration: const Duration(minutes: 5, seconds: 47),
      ),
    );
    playlist.setTracks(tracks);
    await playback.playIndex(2);
    await playback.seek(const Duration(minutes: 2, seconds: 41));
    await playback.playPause(); // pause — static mockup is not mid-transport
    engine.emitFormat(
      const AudioFormatInfo(
        bitrateKbps: 192,
        sampleRateHz: 44100,
        channels: 2,
      ),
    );
    playback.setVolume(0.66);
    playback.toggleShuffle();
    // Repeat all so LED is lit like the mockup.
    playback.cycleRepeatMode();

    const size = Size(825, 348);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: MainPlayerWindow(
              playback: playback,
              trackCount: tracks.length,
              forceMono: false,
              alwaysOnTop: false,
              equalizerVisible: true,
              playlistVisible: true,
              draggableTitle: false,
              spectrumBars: _mockupBars,
              spectrumPeaks: _mockupPeaks,
            ),
          ),
        ),
      ),
    );
    await tester.pump(); // layout + post-frame marquee measure
    await tester.pump(); // marquee controller starts (never settles — looping)

    await expectLater(
      find.byType(MainPlayerWindow),
      matchesGoldenFile('goldens/main_player_window.png'),
    );
  });

  testWidgets('main display quiet/silent measured frames', (tester) async {
    // Real analyser silence: synthetic:false zeros — not AudioLevels.synthesised.
    const quietBars = <double>[
      0.04, 0.06, 0.05, 0.03, 0.02, 0.04, 0.03, 0.02, 0.01, 0.02,
      0.015, 0.01, 0.012, 0.008, 0.01, 0.006, 0.008, 0.004, 0.005, 0.003,
    ];

    playlist.setTracks([
      const Track(
        path: 'quiet.mp3',
        title: 'Quiet Room',
        artist: 'Tramp',
        duration: Duration(minutes: 1),
      ),
    ]);
    await playback.playIndex(0);
    await playback.playPause();
    engine.emitFormat(
      const AudioFormatInfo(
        bitrateKbps: 128,
        sampleRateHz: 44100,
        channels: 2,
      ),
    );

    const size = Size(825, 348);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topLeft,
          child: ColoredBox(
            color: MockupTokens.shellDeep,
            child: MainPlayerWindow(
              playback: playback,
              trackCount: 1,
              forceMono: false,
              alwaysOnTop: false,
              equalizerVisible: true,
              playlistVisible: true,
              draggableTitle: false,
              spectrumBars: quietBars,
              spectrumPeaks: quietBars,
            ),
          ),
        ),
      ),
    );
    await tester.pump();
    await tester.pump();

    await expectLater(
      find.byType(MainPlayerWindow),
      matchesGoldenFile('goldens/main_player_window_quiet.png'),
    );
  });
}
