import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/theme/tramp_metrics.dart';

void main() {
  group('TrampSettings multi-window round-trip', () {
    test('visibility, shade, and positions round-trip', () {
      const settings = TrampSettings(
        zoomPercent: 150,
        alwaysOnTop: true,
        forceMono: false,
        main: WindowFrameState(
          visible: true,
          shaded: false,
          left: 12,
          top: 34,
        ),
        equalizer: WindowFrameState(
          visible: true,
          shaded: true,
          left: 12,
          top: 382,
        ),
        playlist: WindowFrameState(
          visible: false,
          shaded: false,
          left: 40,
          top: 100,
        ),
        settings: WindowFrameState.settingsDefault,
        dockEdges: [],
        equalizerCurve: EqualizerSettings.flat,
      );

      final again = TrampSettings.fromJson(settings.toJson());
      expect(again.main.visible, isTrue);
      expect(again.main.shaded, isFalse);
      expect(again.main.left, 12);
      expect(again.main.top, 34);
      expect(again.equalizer.visible, isTrue);
      expect(again.equalizer.shaded, isTrue);
      expect(again.equalizer.left, 12);
      expect(again.equalizer.top, 382);
      expect(again.playlist.visible, isFalse);
      expect(again.playlist.shaded, isFalse);
      expect(again.playlist.left, 40);
      expect(again.playlist.top, 100);
    });

    test('dock edges round-trip', () {
      const settings = TrampSettings(
        zoomPercent: 100,
        alwaysOnTop: false,
        forceMono: false,
        main: WindowFrameState.mainDefault,
        equalizer: WindowFrameState.equalizerDefault,
        playlist: WindowFrameState.playlistDefault,
        settings: WindowFrameState.settingsDefault,
        dockEdges: [
          DockEdge(
            a: WindowId.main,
            b: WindowId.playlist,
            side: DockSide.bottom,
          ),
          DockEdge(
            a: WindowId.playlist,
            b: WindowId.equalizer,
            side: DockSide.right,
          ),
        ],
        equalizerCurve: EqualizerSettings.flat,
      );

      final again = TrampSettings.fromJson(settings.toJson());
      expect(again.dockEdges, hasLength(2));
      expect(again.dockEdges[0].a, WindowId.main);
      expect(again.dockEdges[0].b, WindowId.playlist);
      expect(again.dockEdges[0].side, DockSide.bottom);
      expect(again.dockEdges[1].a, WindowId.playlist);
      expect(again.dockEdges[1].b, WindowId.equalizer);
      expect(again.dockEdges[1].side, DockSide.right);
    });

    test('forceMono and alwaysOnTop round-trip', () {
      final settings = TrampSettings.defaults.copyWith(
        forceMono: true,
        alwaysOnTop: true,
      );
      final again = TrampSettings.fromJson(settings.toJson());
      expect(again.forceMono, isTrue);
      expect(again.alwaysOnTop, isTrue);
    });

    test('playlist size round-trips on playlist frame', () {
      final settings = TrampSettings.defaults.copyWith(
        playlist: WindowFrameState.playlistDefault.copyWith(
          width: 900,
          height: 700,
        ),
      );
      final again = TrampSettings.fromJson(settings.toJson());
      expect(again.playlist.width, 900);
      expect(again.playlist.height, 700);
    });

    test('defaults omit optional playlist size from JSON', () {
      final json = TrampSettings.defaults.toJson();
      final playlist = json['playlist'] as Map<String, dynamic>;
      expect(playlist.containsKey('width'), isFalse);
      expect(playlist.containsKey('height'), isFalse);
      expect(json.containsKey('playlistWindowWidth'), isFalse);
      expect(json.containsKey('playlistWindowHeight'), isFalse);
      expect(json.containsKey('lowerRegion'), isFalse);
    });

    test('defaults stack main / EQ / playlist for multi-window', () {
      final d = TrampSettings.defaults;
      expect(d.zoomPercent, 75);
      expect(d.alwaysOnTop, isFalse);
      expect(d.forceMono, isFalse);
      expect(d.main.visible, isTrue);
      expect(d.main.shaded, isFalse);
      expect(d.main.left, 0);
      expect(d.main.top, 0);
      expect(d.equalizer.visible, isTrue);
      expect(d.equalizer.shaded, isFalse);
      expect(d.equalizer.top, TrampMetrics.mainPlayer.height);
      expect(d.playlist.visible, isTrue);
      expect(d.playlist.shaded, isFalse);
      expect(d.playlist.top, TrampMetrics.mainPlayer.height * 2);
      expect(d.playlist.width, isNull);
      expect(d.playlist.height, isNull);
      expect(d.dockEdges, isEmpty);
      expect(d.equalizerCurve, EqualizerSettings.flat);
    });
  });

  group('TrampSettings fromJson migration', () {
    test('old lowerRegion equalizer maps to EQ visible / PL hidden', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'lowerRegion': 'equalizer',
      });
      expect(settings.equalizer.visible, isTrue);
      expect(settings.playlist.visible, isFalse);
      expect(settings.main.visible, isTrue);
    });

    test('old lowerRegion playlist maps to PL visible / EQ hidden', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 125,
        'lowerRegion': 'playlist',
      });
      expect(settings.playlist.visible, isTrue);
      expect(settings.equalizer.visible, isFalse);
      expect(settings.zoomPercent, 125);
    });

    test('migrates top-level playlistWindowWidth/Height onto playlist frame', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'lowerRegion': 'playlist',
        'playlistWindowWidth': 900,
        'playlistWindowHeight': 700.5,
      });
      expect(settings.playlist.width, 900);
      expect(settings.playlist.height, 700.5);
    });

    test('migrates legacy equalizer curve object under equalizer key', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'lowerRegion': 'playlist',
        'equalizer': {
          'enabled': true,
          'auto': false,
          'preamp': 2,
          'gains': [1, 0, -1, 0, 0, 0, 0, 0, 0, 3],
          'presetName': 'Rock',
        },
      });
      expect(settings.equalizerCurve.enabled, isTrue);
      expect(settings.equalizerCurve.presetName, 'Rock');
      expect(settings.equalizerCurve.gains[0], 1);
      // Frame defaults still apply when legacy curve occupied the key.
      expect(settings.equalizer.visible, isFalse);
    });

    test('ignores unknown keys safely', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 200,
        'futureFeature': {'x': 1},
        'lowerRegion': 'playlist',
      });
      expect(settings.zoomPercent, 200);
      expect(settings.playlist.visible, isTrue);
    });

    test('fromJson ignores invalid or non-positive playlist sizes', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'playlist': {
          'visible': true,
          'shaded': false,
          'left': 0,
          'top': 348,
          'width': -1,
          'height': 0,
        },
      });
      expect(settings.playlist.width, isNull);
      expect(settings.playlist.height, isNull);

      final fromLegacy = TrampSettings.fromJson({
        'zoomPercent': 100,
        'lowerRegion': 'playlist',
        'playlistWindowWidth': '900',
        'playlistWindowHeight': 'garbage',
      });
      expect(fromLegacy.playlist.width, isNull);
      expect(fromLegacy.playlist.height, isNull);
    });
  });

  group('TrampSettings skin fields', () {
    test('activeSkinId and skinsDirectory round-trip', () {
      final settings = TrampSettings.defaults.copyWith(
        activeSkinId: 'neon-cyan',
        skinsDirectory: '/path/to/skins',
      );
      final again = TrampSettings.fromJson(settings.toJson());
      expect(again.activeSkinId, 'neon-cyan');
      expect(again.skinsDirectory, '/path/to/skins');
    });

    test('defaults use builtin and null skinsDirectory', () {
      expect(TrampSettings.defaults.activeSkinId, 'builtin');
      expect(TrampSettings.defaults.skinsDirectory, isNull);
    });

    test('missing activeSkinId defaults to builtin', () {
      final settings = TrampSettings.fromJson({'zoomPercent': 100});
      expect(settings.activeSkinId, 'builtin');
    });

    test('invalid activeSkinId defaults to builtin', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'activeSkinId': 'com.example.bad',
      });
      expect(settings.activeSkinId, 'builtin');
    });

    test('empty skinsDirectory becomes null', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'skinsDirectory': '',
      });
      expect(settings.skinsDirectory, isNull);
    });

    test('reads legacy activeLookId / looksDirectory keys', () {
      final settings = TrampSettings.fromJson({
        'zoomPercent': 100,
        'activeLookId': 'neon-cyan',
        'looksDirectory': '/legacy/looks',
      });
      expect(settings.activeSkinId, 'neon-cyan');
      expect(settings.skinsDirectory, '/legacy/looks');
      final json = settings.toJson();
      expect(json['activeSkinId'], 'neon-cyan');
      expect(json.containsKey('activeLookId'), isFalse);
      expect(json['skinsDirectory'], '/legacy/looks');
      expect(json.containsKey('looksDirectory'), isFalse);
    });

    test('defaults include activeSkinId and omit skinsDirectory in JSON', () {
      final json = TrampSettings.defaults.toJson();
      expect(json['activeSkinId'], 'builtin');
      expect(json.containsKey('skinsDirectory'), isFalse);
    });
  });

  group('TrampSettings general prefs', () {
    test('defaults for settings frame and general prefs', () {
      final d = TrampSettings.defaults;
      expect(d.settings.visible, isFalse);
      expect(d.settings.left, 860);
      expect(d.settings.top, 40);
      expect(d.settings.shaded, isFalse);
      expect(d.resumeLastSession, isTrue);
      expect(d.confirmBeforeQuit, isFalse);
      expect(d.scrollTitle, isTrue);
      expect(d.minimizeHidesSecondaries, isTrue);
      expect(d.dockSnapStrength, DockSnapStrength.normal);
      expect(d.dockSnapStrength.snapPixels, 20);
    });

    test('general prefs and settings frame round-trip', () {
      final settings = TrampSettings.defaults.copyWith(
        settings: WindowFrameState.settingsDefault.copyWith(
          visible: true,
          left: 100,
          top: 50,
        ),
        resumeLastSession: false,
        confirmBeforeQuit: true,
        scrollTitle: false,
        minimizeHidesSecondaries: false,
        dockSnapStrength: DockSnapStrength.strong,
      );
      final again = TrampSettings.fromJson(settings.toJson());
      expect(again.settings.visible, isTrue);
      expect(again.settings.left, 100);
      expect(again.resumeLastSession, isFalse);
      expect(again.confirmBeforeQuit, isTrue);
      expect(again.scrollTitle, isFalse);
      expect(again.minimizeHidesSecondaries, isFalse);
      expect(again.dockSnapStrength, DockSnapStrength.strong);
      expect(again.dockSnapStrength.snapPixels, 40);
      expect(DockSnapStrength.off.snapPixels, 0);
    });
  });

  group('TrampSettings value semantics', () {
    test('copyWith updates layout and flags', () {
      final updated = TrampSettings.defaults.copyWith(
        alwaysOnTop: true,
        forceMono: true,
        equalizer: WindowFrameState.equalizerDefault.copyWith(visible: true),
        playlist: WindowFrameState.playlistDefault.copyWith(
          visible: false,
          width: 800,
          height: 600,
        ),
        dockEdges: const [
          DockEdge(
            a: WindowId.main,
            b: WindowId.equalizer,
            side: DockSide.bottom,
          ),
        ],
      );
      expect(updated.alwaysOnTop, isTrue);
      expect(updated.forceMono, isTrue);
      expect(updated.equalizer.visible, isTrue);
      expect(updated.playlist.visible, isFalse);
      expect(updated.playlist.width, 800);
      expect(updated.playlist.height, 600);
      expect(updated.dockEdges, hasLength(1));
      expect(updated.zoomPercent, 75);
    });

    test('equality and hashCode include multi-window fields', () {
      final a = TrampSettings.defaults.copyWith(
        forceMono: true,
        playlist: WindowFrameState.playlistDefault.copyWith(width: 900),
      );
      final b = TrampSettings.defaults.copyWith(
        forceMono: true,
        playlist: WindowFrameState.playlistDefault.copyWith(width: 900),
      );
      final c = TrampSettings.defaults.copyWith(
        forceMono: true,
        playlist: WindowFrameState.playlistDefault.copyWith(width: 901),
      );
      expect(a, b);
      expect(a.hashCode, b.hashCode);
      expect(a, isNot(c));
    });
  });
}
