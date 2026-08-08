import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:tramp/domain/equalizer_settings.dart';
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/platform/settings_store.dart';

void main() {
  late Directory dir;

  setUp(() async {
    dir = await Directory.systemTemp.createTemp('tramp-settings-test');
  });

  tearDown(() async {
    if (dir.existsSync()) await dir.delete(recursive: true);
  });

  FileSettingsStore store() =>
      FileSettingsStore(supportDir: () async => dir);

  test('reading with no file on disk yields the defaults', () async {
    final settings = await store().read();
    expect(settings, TrampSettings.defaults);
  });

  test('round-trips multi-window layout flags', () async {
    final written = TrampSettings.defaults.copyWith(
      zoomPercent: 200,
      alwaysOnTop: true,
      forceMono: true,
      equalizer: WindowFrameState.equalizerDefault.copyWith(visible: true),
      playlist: WindowFrameState.playlistDefault.copyWith(visible: false),
      dockEdges: const [
        DockEdge(
          a: WindowId.main,
          b: WindowId.equalizer,
          side: DockSide.bottom,
        ),
      ],
    );
    await store().write(written);
    expect(await store().read(), written);
  });

  test('unparseable json falls back to defaults instead of throwing', () async {
    await File('${dir.path}/settings.json').writeAsString('{not json');
    expect(await store().read(), TrampSettings.defaults);
  });

  test('an unknown zoom step falls back to the default', () async {
    await File('${dir.path}/settings.json')
        .writeAsString('{"zoomPercent":137,"lowerRegion":"playlist"}');
    final settings = await store().read();
    expect(settings.zoomPercent, TrampSettings.defaults.zoomPercent);
  });

  test('migrates legacy lowerRegion equalizer on read', () async {
    await File('${dir.path}/settings.json').writeAsString(
      '{"zoomPercent":150,"lowerRegion":"equalizer",'
      '"equalizer":{"enabled":true,"auto":false,"preamp":0,'
      '"gains":[0,0,0,0,0,0,0,0,0,0],"presetName":42}}',
    );
    final settings = await store().read();
    expect(settings.zoomPercent, 150);
    expect(settings.equalizer.visible, isTrue);
    expect(settings.playlist.visible, isFalse);
    expect(settings.equalizerCurve.presetName, isNull);
    expect(settings.equalizerCurve.enabled, isTrue);
  });

  test('non-map equalizer falls back to flat while other fields load', () async {
    await File('${dir.path}/settings.json').writeAsString(
      '{"zoomPercent":200,"lowerRegion":"playlist","equalizer":"garbage"}',
    );
    final settings = await store().read();
    expect(settings.zoomPercent, 200);
    expect(settings.playlist.visible, isTrue);
    expect(settings.equalizer.visible, isFalse);
    expect(settings.equalizerCurve, EqualizerSettings.flat);
  });

  test('round-trips equalizer curve state', () async {
    final written = TrampSettings.defaults.copyWith(
      zoomPercent: 150,
      equalizer: WindowFrameState.equalizerDefault.copyWith(visible: true),
      playlist: WindowFrameState.playlistDefault.copyWith(visible: false),
      equalizerCurve: const EqualizerSettings(
        enabled: true,
        auto: false,
        preamp: 2,
        gains: [1, 0, -1, 0, 0, 0, 0, 0, 0, 3],
        presetName: 'Rock',
      ),
    );
    await store().write(written);
    expect(await store().read(), written);
  });

  test('round-trips playlist window size on playlist frame', () async {
    final written = TrampSettings.defaults.copyWith(
      playlist: WindowFrameState.playlistDefault.copyWith(
        width: 900,
        height: 700,
      ),
    );
    await store().write(written);
    expect(await store().read(), written);
  });
}
