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
}
