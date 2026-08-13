import 'dart:convert';

import 'package:flutter_test/flutter_test.dart';
import 'package:flutter/painting.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/saved_playlist.dart';
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

    test('round-trips secondary roles as compact JSON', () {
      for (final role in [
        WindowRole.equalizer,
        WindowRole.playlist,
        WindowRole.settings,
        WindowRole.about,
      ]) {
        final encoded = encodeWindowArguments(role);
        expect(encoded, '{"role":"${role.name}"}');
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
        const ResizePlaylistCollectionCommand(width: 220.5, collapsed: false),
        const ResizePlaylistCollectionCommand(width: 240, collapsed: true),
        const AddSavedPlaylistCommand('/music/driving.m3u'),
        const RemoveSavedPlaylistCommand('/music/driving.m3u'),
        const SelectSavedPlaylistCommand('/music/driving.m3u'),
        const LoadSavedPlaylistCommand(r'D:\music\driving.m3u'),
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

    test('saved-playlist commands reject an empty path', () {
      for (final type in [
        AddSavedPlaylistCommand.typeName,
        RemoveSavedPlaylistCommand.typeName,
        SelectSavedPlaylistCommand.typeName,
        LoadSavedPlaylistCommand.typeName,
      ]) {
        expect(
          () => SessionCommand.fromJson({
            'type': type,
            'payload': {'path': ''},
          }),
          throwsFormatException,
          reason: '$type with no path',
        );
      }
    });
  });

  group('PlaylistCollectionSnapshotEvent', () {
    test('round-trips the collection and the loaded entry', () {
      final event = PlaylistCollectionSnapshotEvent(
        playlists: [
          SavedPlaylist(
            path: '/music/driving.m3u',
            trackCount: 12,
            totalDuration: const Duration(minutes: 47, seconds: 5),
            modified: DateTime.fromMillisecondsSinceEpoch(1723000000000),
          ),
          SavedPlaylist(
            path: '/music/sun.m3u',
            name: 'Sunday Morning',
            trackCount: 7,
          ),
        ],
        selectedPath: normalizePlaylistPath('/music/sun.m3u'),
      );

      final decoded = SessionEvent.fromJson(event.toEnvelope())
          as PlaylistCollectionSnapshotEvent;

      expect(decoded.playlists, event.playlists);
      expect(decoded.playlists.first.trackCount, 12);
      expect(
        decoded.playlists.first.totalDuration,
        const Duration(minutes: 47, seconds: 5),
      );
      expect(decoded.playlists.last.displayName, 'Sunday Morning');
      expect(decoded.selectedPath, normalizePlaylistPath('/music/sun.m3u'));
      expect(decoded.lastError, isNull);
    });

    test('an empty collection round-trips as empty, not as absent', () {
      const event = PlaylistCollectionSnapshotEvent(
        playlists: [],
        lastError: 'Could not read playlist: no such file',
      );
      final decoded = SessionEvent.fromJson(
        SessionEvent.decodeEnvelope(jsonEncode(event.toEnvelope())),
      ) as PlaylistCollectionSnapshotEvent;

      expect(decoded.playlists, isEmpty);
      expect(decoded.selectedPath, isNull);
      expect(decoded.disabledPaths, isEmpty);
      expect(decoded.lastError, 'Could not read playlist: no such file');
    });

    test('carries which entries are disabled playlists', () {
      final event = PlaylistCollectionSnapshotEvent(
        playlists: [
          SavedPlaylist(path: '/music/driving.m3u', trackCount: 12),
          SavedPlaylist(path: '/music/sun.m3u', trackCount: 7),
        ],
        disabledPaths: [normalizePlaylistPath('/music/sun.m3u')],
      );

      final decoded = SessionEvent.fromJson(
        SessionEvent.decodeEnvelope(jsonEncode(event.toEnvelope())),
      ) as PlaylistCollectionSnapshotEvent;

      expect(decoded.disabledPaths, [normalizePlaylistPath('/music/sun.m3u')]);
      expect(
        decoded.playlists.map((e) => e.toJson().keys).expand((keys) => keys),
        isNot(contains('disabled')),
        reason: 'disabled is derived per snapshot, never a field on the entry',
      );
    });

    test('a snapshot from before disabled paths existed decodes as none', () {
      final decoded = PlaylistCollectionSnapshotEvent.fromPayload({
        'playlists': [
          SavedPlaylist(path: '/music/driving.m3u').toJson(),
        ],
      });

      expect(decoded.playlists, hasLength(1));
      expect(decoded.disabledPaths, isEmpty);
    });
  });

  group('SettingsSnapshotEvent', () {
    test('carries the playlist collection layout across the bus', () {
      const event = SettingsSnapshotEvent(
        resumeLastSession: true,
        confirmBeforeQuit: false,
        scrollTitle: true,
        minimizeHidesSecondaries: true,
        dockSnapStrength: DockSnapStrength.strong,
        skins: [],
        activeSkinId: 'builtin',
        playlistCollectionWidth: 312.5,
        playlistCollectionCollapsed: true,
      );
      final decoded =
          SessionEvent.fromJson(event.toEnvelope()) as SettingsSnapshotEvent;
      expect(decoded.playlistCollectionWidth, 312.5);
      expect(decoded.playlistCollectionCollapsed, isTrue);
      expect(decoded.dockSnapStrength, DockSnapStrength.strong);
    });

    test('absent or unusable collection width falls back to the default', () {
      for (final raw in <Object?>[null, 0, -20, 'wide', double.nan]) {
        final decoded = SettingsSnapshotEvent.fromPayload({
          'activeSkinId': 'builtin',
          'playlistCollectionWidth': raw,
        });
        expect(
          decoded.playlistCollectionWidth,
          TrampSettings.defaultPlaylistCollectionWidth,
          reason: 'width $raw should fall back',
        );
        expect(decoded.playlistCollectionCollapsed, isFalse);
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

    test('altered rides the snapshot both ways', () {
      for (final altered in [true, false]) {
        final event = PlaylistSnapshotEvent(
          tracks: const [Track(path: '/a.mp3')],
          selectedIndices: const [0],
          sourcePath: '/list.m3u',
          altered: altered,
        );
        final decoded =
            SessionEvent.fromJson(event.toEnvelope()) as PlaylistSnapshotEvent;
        expect(decoded.altered, altered, reason: 'altered $altered');
      }
    });

    test('a snapshot without the field decodes as unaltered', () {
      final decoded = PlaylistSnapshotEvent.fromPayload({
        'tracks': [const Track(path: '/a.mp3').toJson()],
      });
      expect(decoded.altered, isFalse);
    });
  });
}
