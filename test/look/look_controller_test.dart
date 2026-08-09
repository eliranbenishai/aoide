import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/painting.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/look/look_controller.dart';
import 'package:tramp/look/look_font_loader.dart';
import 'package:tramp/platform/settings_store.dart';

class MemorySettingsStore implements SettingsStore {
  MemorySettingsStore([this.stored = TrampSettings.defaults]);

  TrampSettings stored;
  int writes = 0;

  @override
  Future<TrampSettings> read() async => stored;

  @override
  Future<void> write(TrampSettings settings) async {
    stored = settings;
    writes++;
  }
}

void main() {
  late Directory supportDir;
  late Directory looksDir;
  late MemorySettingsStore store;
  late List<({String family, Uint8List bytes, int weight})> fontLoads;

  Future<Directory> supportDirFn() async => supportDir;

  LookFontLoader fakeFontLoader() => LookFontLoader(
        load: ({
          required String family,
          required Uint8List bytes,
          required int weight,
        }) async {
          fontLoads.add((family: family, bytes: bytes, weight: weight));
        },
      );

  LookController controller() => LookController(
        settingsStore: store,
        supportDir: supportDirFn,
        fontLoader: fakeFontLoader(),
      );

  setUp(() async {
    supportDir = Directory.systemTemp.createTempSync('tramp-look-support');
    looksDir = Directory(p.join(supportDir.path, 'looks'));
    await looksDir.create(recursive: true);
    store = MemorySettingsStore();
    fontLoads = [];
  });

  tearDown(() async {
    if (supportDir.existsSync()) {
      await supportDir.delete(recursive: true);
    }
  });

  Future<void> writeNeonCyanPack({required Directory targetLooksDir}) async {
    final packDir = Directory(p.join(targetLooksDir.path, 'neon-cyan'));
    await packDir.create(recursive: true);
    await File(p.join(packDir.path, 'look.json')).writeAsString('''
{
  "formatVersion": 1,
  "id": "neon-cyan",
  "name": "Neon Cyan",
  "author": "Example",
  "extends": "builtin",
  "colors": {
    "phosphor": {
      "default": "#112233",
      "hot": "#b8f6ff"
    }
  },
  "fonts": {
    "lcd": { "file": "fonts/lcd.ttf", "weight": 500 }
  }
}
''');
    final fontsDir = Directory(p.join(packDir.path, 'fonts'));
    await fontsDir.create(recursive: true);
    await File(p.join(fontsDir.path, 'lcd.ttf')).writeAsBytes([1, 2, 3, 4]);
  }

  test('bootstrap with missing active id resolves builtin and persists it',
      () async {
    final c = controller();
    await c.bootstrap(
      settings: TrampSettings.defaults.copyWith(activeLookId: 'missing-pack'),
      supportDir: supportDirFn,
    );

    expect(c.activeLookId, 'builtin');
    expect(c.resolved.id, 'builtin');
    expect(store.writes, greaterThan(0));
    expect(store.stored.activeLookId, 'builtin');
  });

  test('setLooksDirectory to empty temp falls back to builtin when active missing',
      () async {
    await writeNeonCyanPack(targetLooksDir: looksDir);

    final c = controller();
    await c.bootstrap(
      settings: TrampSettings.defaults.copyWith(activeLookId: 'neon-cyan'),
      supportDir: supportDirFn,
    );
    expect(c.activeLookId, 'neon-cyan');

    final emptyLooks = Directory.systemTemp.createTempSync('tramp-empty-looks');
    addTearDown(() {
      if (emptyLooks.existsSync()) emptyLooks.deleteSync(recursive: true);
    });

    await c.setLooksDirectory(emptyLooks.path);

    expect(c.activeLookId, 'builtin');
    expect(c.resolved.id, 'builtin');
    expect(store.stored.looksDirectory, emptyLooks.path);
    expect(store.stored.activeLookId, 'builtin');
  });

  test('activate neon-cyan applies palette overlay and loads fonts', () async {
    await writeNeonCyanPack(targetLooksDir: looksDir);

    final c = controller();
    await c.bootstrap(
      settings: TrampSettings.defaults,
      supportDir: supportDirFn,
    );

    await c.activate('neon-cyan');

    expect(c.activeLookId, 'neon-cyan');
    expect(c.resolved.id, 'neon-cyan');
    expect(c.resolved.palette.phosphorDefault, const Color(0xFF112233));
    expect(c.resolved.lcdFamily, 'Look.neon-cyan.lcd');
    expect(fontLoads, hasLength(1));
    expect(fontLoads.single.family, 'Look.neon-cyan.lcd');
    expect(fontLoads.single.weight, 500);
    expect(fontLoads.single.bytes, [1, 2, 3, 4]);
    expect(store.stored.activeLookId, 'neon-cyan');
  });
}
