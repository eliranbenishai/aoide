import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/domain/tramp_settings.dart';
import 'package:tramp/platform/settings_store.dart';
import 'package:tramp/platform/usage_store.dart';

void main() {
  late Directory supportDir;
  late FileUsageStore store;

  setUp(() async {
    supportDir = await Directory.systemTemp.createTemp('tramp_support_');
    store = FileUsageStore(supportDir: () async => supportDir);
  });

  tearDown(() async {
    if (await supportDir.exists()) await supportDir.delete(recursive: true);
  });

  File usageFile() =>
      File(p.join(supportDir.path, FileUsageStore.fileName));

  test('reads zero before anything has been played', () async {
    expect((await store.read()).spins, 0);
  });

  test('the lifetime count round-trips through a real directory', () async {
    await store.write(const UsageCounters(spins: 4096));

    expect((await store.read()).spins, 4096);
  });

  test('a later session picks the count up where the last one left it',
      () async {
    await store.write(const UsageCounters(spins: 11));

    final nextLaunch = FileUsageStore(supportDir: () async => supportDir);

    expect((await nextLaunch.read()).spins, 11);
  });

  test('nothing is kept beside the last-session file', () async {
    await store.write(const UsageCounters(spins: 1));

    expect(await usageFile().exists(), isTrue);
    expect(
      await File(p.join(supportDir.path, 'session.json')).exists(),
      isFalse,
      reason: '"last session" is a different store, with no error handling',
    );
  });

  group('malformed files', () {
    test('a truncated file reads as zero rather than throwing', () async {
      await usageFile().writeAsString('{"spins":');

      expect((await store.read()).spins, 0);
    });

    test('a file that is not an object at all reads as zero', () async {
      await usageFile().writeAsString('[]');

      expect((await store.read()).spins, 0);
    });

    test('a directory where the file should be reads as zero', () async {
      await Directory(usageFile().path).create(recursive: true);

      expect((await store.read()).spins, 0);
    });

    test('a non-numeric or negative count reads as zero', () async {
      await usageFile().writeAsString(jsonEncode({'spins': 'lots'}));
      expect((await store.read()).spins, 0);

      await usageFile().writeAsString(jsonEncode({'spins': -5}));
      expect(
        (await store.read()).spins,
        0,
        reason: 'no machine has un-played music',
      );
    });
  });

  test('resetting settings leaves the spin count alone', () async {
    await store.write(const UsageCounters(spins: 837));

    // What Reset Settings does: rewrite settings.json with the defaults. It
    // touches nothing else, exactly as it already spares installed skins and
    // the playlist collection. A spin count is history, not a preference.
    final settings = FileSettingsStore(supportDir: () async => supportDir);
    await settings.write(TrampSettings.defaults.copyWith(zoomPercent: 200));
    await settings.write(TrampSettings.defaults);

    expect((await store.read()).spins, 837);
    expect(
      (await settings.read()).zoomPercent,
      TrampSettings.defaults.zoomPercent,
      reason: 'the preference did reset — only the history survived',
    );
  });
}
