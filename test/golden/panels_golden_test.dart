// Golden images for the graphite chrome panels.
//
// Platform note: Flutter goldens are platform-specific. Text rasterisation
// differs between Windows, macOS and Linux, so these images (generated on
// Windows) will not match byte-for-byte on another OS. CI on a different
// platform needs either its own golden set or a tolerance-based comparison.
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/eq/equalizer_controller.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/playback/audio_format_info.dart';
import 'package:tramp/playback/audio_levels.dart';
import 'package:tramp/playback/fake_player_engine.dart';
import 'package:tramp/playback/playback_controller.dart';
import 'package:tramp/playlist/playlist_controller.dart';
import 'package:tramp/playlist/playlist_store.dart';
import 'package:tramp/theme/tramp_metrics.dart';
import 'package:tramp/ui/equalizer/equalizer_panel.dart';
import 'package:tramp/ui/main_player/main_player_panel.dart';
import 'package:tramp/ui/zoom/zoom_controller.dart';
import 'package:tramp/ui/zoom/zoom_scope.dart';

import '../support/test_fonts.dart';

class MemorySettingsStore implements SettingsStore {
  TrampSettings stored = TrampSettings.defaults;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async => stored = settings;
}

class MemoryPlaylistStore implements PlaylistStore {
  @override
  Future<String?> readLastPlaylistPath() async => null;

  @override
  Future<void> writeLastPlaylistPath(String? path) async {}
}

/// Wraps a fixed-size panel at a zoom factor for capture.
///
/// The transparent [Material] matches production (`TrampShell` supplies one)
/// so text is not painted with Flutter's missing-Material underline decoration.
Widget frame(Widget panel, Size logical, double factor) {
  return MaterialApp(
    debugShowCheckedModeBanner: false,
    home: ZoomScope(
      factor: factor,
      devicePixelRatio: 1,
      child: Align(
        alignment: Alignment.topLeft,
        child: Transform.scale(
          scale: factor,
          alignment: Alignment.topLeft,
          child: SizedBox(
            width: logical.width,
            height: logical.height,
            child: Material(
              color: Colors.transparent,
              child: panel,
            ),
          ),
        ),
      ),
    ),
  );
}

/// Decodes every [Image] in the tree so goldens capture skin PNGs.
///
/// Widget tests don't await asynchronous asset decoding, so a freshly pumped
/// [SkinImage] paints nothing. Precaching each provider inside [runAsync] then
/// pumping again lets the real pixels land before the golden is captured.
Future<void> precacheSkin(WidgetTester tester) async {
  await tester.runAsync(() async {
    for (final element in find.byType(Image).evaluate()) {
      final image = element.widget as Image;
      await precacheImage(image.image, element);
    }
  });
  await tester.pumpAndSettle();
}

void main() {
  setUpAll(loadTrampFonts);

  for (final factor in [1.0, 2.0]) {
    final tag = '${(factor * 100).round()}';

    testWidgets('main player golden at $tag%', (tester) async {
      final engine = FakePlayerEngine();
      final playlist = PlaylistController(store: MemoryPlaylistStore());
      playlist.addTracks(const [
        // Neutral fixture. Do not use Winamp's bundled demo track here.
        Track(path: 'a.mp3', title: 'Night Ferry', artist: 'The Sleepless'),
      ]);
      final playback = PlaybackController(playlist: playlist, engine: engine);
      addTearDown(playback.dispose);

      await playback.playIndex(0);
      engine.emitFormat(const AudioFormatInfo(
        bitrateKbps: 128,
        sampleRateHz: 44100,
        channels: 2,
      ));

      final size = TrampMetrics.mainPlayer * factor;
      await tester.binding.setSurfaceSize(size);
      addTearDown(() => tester.binding.setSurfaceSize(null));

      await tester.pumpWidget(frame(
        MainPlayerPanel(
          playback: playback,
          zoom: ZoomController(workArea: const Size(6000, 4000)),
          lowerRegion: LowerRegion.playlist,
          hasTracks: true,
          draggableTitle: false,
          onSelectRegion: (_) {},
        ),
        TrampMetrics.mainPlayer,
        factor,
      ));
      // Subscribe first, then emit — levels before pump are dropped.
      await tester.pump();
      engine.emitLevels(
        AudioLevels.synthesised(intensity: 0.75, seed: 42),
      );
      await tester.pumpAndSettle();
      await precacheSkin(tester);

      await expectLater(
        find.byType(MainPlayerPanel),
        matchesGoldenFile('goldens/main_player_$tag.png'),
      );
    });

    testWidgets('equalizer golden at $tag%', (tester) async {
      final controller = EqualizerController(
        store: MemorySettingsStore(),
        sink: const NoopEqualizerSink(),
      );
      controller.applyPreset('Rock');

      final size = TrampMetrics.equalizer * factor;
      await tester.binding.setSurfaceSize(size);
      addTearDown(() => tester.binding.setSurfaceSize(null));

      await tester.pumpWidget(frame(
        EqualizerPanel(
          controller: controller,
          draggableTitle: false,
          onCollapse: () {},
          onClose: () {},
        ),
        TrampMetrics.equalizer,
        factor,
      ));
      await tester.pumpAndSettle();
      await precacheSkin(tester);

      await expectLater(
        find.byType(EqualizerPanel),
        matchesGoldenFile('goldens/equalizer_$tag.png'),
      );
    });
  }

  testWidgets('equalizer windowshade golden', (tester) async {
    final controller = EqualizerController(
      store: MemorySettingsStore(),
      sink: const NoopEqualizerSink(),
    );

    final size = Size(TrampMetrics.equalizer.width, TrampMetrics.titleBar);
    await tester.binding.setSurfaceSize(size);
    addTearDown(() => tester.binding.setSurfaceSize(null));

    await tester.pumpWidget(frame(
      EqualizerPanel(
        controller: controller,
        collapsed: true,
        draggableTitle: false,
        onCollapse: () {},
        onClose: () {},
      ),
      size,
      1.0,
    ));
    await tester.pumpAndSettle();
    await precacheSkin(tester);

    await expectLater(
      find.byType(EqualizerPanel),
      matchesGoldenFile('goldens/equalizer_shade.png'),
    );
  });
}
