import 'dart:convert';

import 'package:flutter_test/flutter_test.dart';
import 'package:flutter/painting.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/track.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/look/builtin_look.dart';
import 'package:tramp/look/look_materials.dart';
import 'package:tramp/look/look_palette.dart';
import 'package:tramp/look/resolved_look.dart';
import 'package:tramp/ui/session/session_messages.dart';

void main() {
  group('parseWindowRole / encodeWindowArguments', () {
    test('empty arguments are main', () {
      expect(parseWindowRole(''), WindowRole.main);
      expect(parseWindowRole('   '), WindowRole.main);
    });

    test('round-trips equalizer and playlist roles', () {
      for (final role in [WindowRole.equalizer, WindowRole.playlist]) {
        final encoded = encodeWindowArguments(role);
        expect(jsonDecode(encoded), {'role': role.name});
        expect(parseWindowRole(encoded), role);
      }
    });

    test('unknown role throws', () {
      expect(
        () => parseWindowRole(jsonEncode({'role': 'video'})),
        throwsFormatException,
      );
    });
  });

  group('parseMultiWindowRole', () {
    test('empty argv is main', () {
      expect(parseMultiWindowRole(const []), WindowRole.main);
    });

    test('finds JSON role among argv tokens', () {
      expect(
        parseMultiWindowRole([
          '1234',
          encodeWindowArguments(WindowRole.playlist),
        ]),
        WindowRole.playlist,
      );
    });
  });

  group('SessionEvent codec', () {
    test('ZoomChanged round-trip', () {
      const event = ZoomChangedEvent(150);
      final decoded = SessionEvent.fromJson(event.toEnvelope());
      expect(decoded, isA<ZoomChangedEvent>());
      expect((decoded as ZoomChangedEvent).zoomPercent, 150);
    });

    test('DockSnapshot round-trip', () {
      final event = DockSnapshotEvent(
        main: WindowFrameState.mainDefault,
        equalizer: WindowFrameState.equalizerDefault.copyWith(visible: true),
        playlist: WindowFrameState.playlistDefault.copyWith(
          width: 900,
          height: 500,
        ),
        settings: WindowFrameState.settingsDefault,
        about: WindowFrameState.aboutDefault,
        dockEdges: const [
          DockEdge(a: WindowId.main, b: WindowId.playlist, side: DockSide.bottom),
        ],
        zoomPercent: 125,
      );
      final decoded =
          SessionEvent.fromJson(event.toEnvelope()) as DockSnapshotEvent;
      expect(decoded.zoomPercent, 125);
      expect(decoded.equalizer.visible, isTrue);
      expect(decoded.playlist.width, 900);
      expect(decoded.dockEdges.single.side, DockSide.bottom);
    });

    test('EqSnapshot round-trip', () {
      final settings = EqualizerSettings.flat.withGain(2, 6).copyWith(
            enabled: true,
          );
      final event = EqSnapshotEvent(settings);
      final decoded =
          SessionEvent.fromJson(event.toEnvelope()) as EqSnapshotEvent;
      expect(decoded.settings.enabled, isTrue);
      expect(decoded.settings.gains[2], 6);
    });

    test('LevelsFrame round-trip via string envelope', () {
      const event = LevelsFrameEvent([0.1, 0.5, 0.9]);
      final raw = jsonEncode(event.toEnvelope());
      final decoded =
          SessionEvent.fromJson(SessionEvent.decodeEnvelope(raw))
              as LevelsFrameEvent;
      expect(decoded.bands, [0.1, 0.5, 0.9]);
    });

    test('LookSnapshot round-trip', () {
      final look = ResolvedLook(
        id: 'neon',
        name: 'Neon',
        author: 'Ada',
        palette: BuiltinLook.resolved.palette,
        materials: const LookMaterials(
          bevelLightOpacity: 0.2,
          bevelSoftOpacity: 0.08,
          spectrumStops: [
            Color(0xFFCBF9FF),
            Color(0xFF3DE7FF),
          ],
          railStops: [
            Color(0xFF1A7A88),
            Color(0xFF8A2258),
          ],
        ),
        chromeFamily: 'Look.neon.chrome',
        lcdFamily: 'Look.neon.lcd',
      );
      final event = LookSnapshotEvent.fromResolved(
        look,
        fontFiles: const {
          'chrome': r'D:\looks\neon\chrome.ttf',
          'lcd': r'D:\looks\neon\lcd.ttf',
        },
      );
      final decoded =
          SessionEvent.fromJson(event.toEnvelope()) as LookSnapshotEvent;
      expect(decoded.id, 'neon');
      expect(decoded.name, 'Neon');
      expect(decoded.author, 'Ada');
      expect(decoded.chromeFamily, 'Look.neon.chrome');
      expect(decoded.lcdFamily, 'Look.neon.lcd');
      expect(decoded.fontFiles, {
        'chrome': r'D:\looks\neon\chrome.ttf',
        'lcd': r'D:\looks\neon\lcd.ttf',
      });
      expect(decoded.palette.toJson(), look.palette.toJson());
      expect(decoded.materials.toJson(), look.materials.toJson());
      final resolved = decoded.toResolved();
      expect(resolved.id, look.id);
      expect(resolved.chromeFamily, look.chromeFamily);
      expect(resolved.materials.bevelLightOpacity, 0.2);
    });

    test('LookSnapshot omits empty fontFiles and author', () {
      final event = LookSnapshotEvent.fromResolved(BuiltinLook.resolved);
      final json = event.toJson();
      expect(json.containsKey('fontFiles'), isFalse);
      expect(json.containsKey('author'), isFalse);
      final decoded =
          SessionEvent.fromJson(event.toEnvelope()) as LookSnapshotEvent;
      expect(decoded.fontFiles, isNull);
      expect(decoded.author, isNull);
      expect(decoded.palette, isA<LookPalette>());
    });

    test('unknown event type throws', () {
      expect(
        () => SessionEvent.fromJson({'type': 'nope', 'payload': {}}),
        throwsFormatException,
      );
    });
  });

  group('SessionCommand codec', () {
    test('Transport / Seek / Volume / Mono round-trip', () {
      final cases = <SessionCommand>[
        const TransportCommand('playPause'),
        const SeekCommand(1234),
        const VolumeCommand(0.42),
        const MonoCommand(true),
      ];
      for (final command in cases) {
        final decoded = SessionCommand.fromJson(command.toEnvelope());
        expect(decoded.toEnvelope(), command.toEnvelope());
      }
    });

    test('ToggleWindow and ClientReady round-trip', () {
      const toggle = ToggleWindowCommand(
        window: WindowId.equalizer,
        visible: false,
      );
      final toggleDecoded =
          SessionCommand.fromJson(toggle.toEnvelope()) as ToggleWindowCommand;
      expect(toggleDecoded.window, WindowId.equalizer);
      expect(toggleDecoded.visible, isFalse);

      const ready = ClientReadyCommand(WindowRole.playlist);
      final readyDecoded =
          SessionCommand.fromJson(ready.toEnvelope()) as ClientReadyCommand;
      expect(readyDecoded.role, WindowRole.playlist);
    });

    test('Eq / shade / PlaylistOp / ZoomStep / AlwaysOnTop round-trip', () {
      final cases = <SessionCommand>[
        const EqGainCommand(band: 3, gain: -4.5),
        const EqPreampCommand(2.5),
        const EqEnabledCommand(true),
        const EqAutoCommand(true),
        const ApplyPresetCommand('Rock'),
        const SetShadedCommand(window: WindowId.equalizer, shaded: true),
        const PlaylistOpCommand('playIndex', index: 2),
        const PlaylistOpCommand(
          'addPaths',
          paths: ['/a.mp3', '/b.mp3'],
        ),
        const PlaylistOpCommand('sort', sortKey: 'title'),
        const ResizePlaylistCommand(width: 900, height: 500),
        const MoveWindowCommand(
          window: WindowId.playlist,
          left: 12.5,
          top: 348,
          shiftUndock: true,
          ended: true,
        ),
        const MoveWindowCommand(
          window: WindowId.equalizer,
          left: 0,
          top: 348,
          shiftUndock: false,
          ended: true,
          softEnd: true,
        ),
        const ZoomStepCommand(1),
        const AlwaysOnTopCommand(true),
      ];
      for (final command in cases) {
        expect(
          SessionCommand.fromJson(command.toEnvelope()).toEnvelope(),
          command.toEnvelope(),
        );
      }
    });
  });

  group('PlaylistSnapshotEvent', () {
    test('round-trips tracks and selection', () {
      const event = PlaylistSnapshotEvent(
        tracks: [
          Track(
            path: '/a.mp3',
            title: 'A',
            artist: 'X',
            duration: Duration(seconds: 12),
          ),
        ],
        selectedIndices: [0],
        selectedIndex: 0,
        sourcePath: '/list.m3u',
        playingIndex: 0,
        playing: true,
      );
      final decoded =
          SessionEvent.fromJson(event.toEnvelope()) as PlaylistSnapshotEvent;
      expect(decoded.trackCount, 1);
      expect(decoded.tracks.single.title, 'A');
      expect(decoded.selectedIndices, [0]);
      expect(decoded.playingIndex, 0);
      expect(decoded.playing, isTrue);
      expect(decoded.sourcePath, '/list.m3u');
    });
  });
}
