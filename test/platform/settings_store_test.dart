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

  test('round-trips zoom and lower region', () async {
    const written = TrampSettings(
      zoomPercent: 200,
      lowerRegion: LowerRegion.equalizer,
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

  test('type-corrupt equalizer presetName does not crash read', () async {
    await File('${dir.path}/settings.json').writeAsString(
      '{"zoomPercent":150,"lowerRegion":"equalizer",'
      '"equalizer":{"enabled":true,"auto":false,"preamp":0,'
      '"gains":[0,0,0,0,0,0,0,0,0,0],"presetName":42}}',
    );
    final settings = await store().read();
    expect(settings.zoomPercent, 150);
    expect(settings.lowerRegion, LowerRegion.equalizer);
    expect(settings.equalizer.presetName, isNull);
    expect(settings.equalizer.enabled, isTrue);
  });

  test('non-map equalizer falls back to flat while other fields load', () async {
    await File('${dir.path}/settings.json').writeAsString(
      '{"zoomPercent":200,"lowerRegion":"playlist","equalizer":"garbage"}',
    );
    final settings = await store().read();
    expect(settings.zoomPercent, 200);
    expect(settings.lowerRegion, LowerRegion.playlist);
    expect(settings.equalizer, EqualizerSettings.flat);
  });

  test('round-trips equalizer state', () async {
    const written = TrampSettings(
      zoomPercent: 150,
      lowerRegion: LowerRegion.equalizer,
      equalizer: EqualizerSettings(
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

  test('round-trips playlist window size', () async {
    const written = TrampSettings(
      zoomPercent: 100,
      lowerRegion: LowerRegion.playlist,
      playlistWindowWidth: 900,
      playlistWindowHeight: 700,
    );
    await store().write(written);
    expect(await store().read(), written);
  });
}
