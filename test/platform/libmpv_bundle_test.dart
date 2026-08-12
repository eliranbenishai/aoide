import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:path/path.dart' as p;
import 'package:tramp/platform/libmpv_bundle.dart';

void main() {
  group('analyzeConfigString', () {
    test('rejects slim media_kit-style config with --disable-filters', () {
      const slim = '''
        --enable-lgpl --disable-debug --disable-filters --disable-libass
        equalizer aresample
      ''';
      expect(LibmpvBundle.analyzeConfigString(slim), isFalse);
    });

    test('accepts full build markers without --disable-filters', () {
      const full = '''
        lavfi equalizer aresample swresample
        --enable-libavfilter
      ''';
      expect(LibmpvBundle.analyzeConfigString(full), isTrue);
    });

    test('accepts distro lavfi bridge without embedded filter names', () {
      expect(LibmpvBundle.analyzeConfigString('lavfi f_lavfi bridge'), isTrue);
    });

    test('rejects strings with neither filter names nor lavfi', () {
      expect(
        LibmpvBundle.analyzeConfigString('aresample without eq name'),
        isFalse,
      );
      expect(LibmpvBundle.analyzeConfigString('no markers here'), isFalse);
    });
  });

  group('resolveLibraryPath', () {
    test('honors TRAMP_LIBMPV_PATH', () {
      final path = LibmpvBundle.resolveLibraryPath(
        environment: {'TRAMP_LIBMPV_PATH': r'D:\full\libmpv-2.dll'},
        overrideOs: 'windows',
      );
      expect(path, r'D:\full\libmpv-2.dll');
    });

    test('finds library beside executable on Windows', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_mpv_');
      addTearDown(() => dir.delete(recursive: true));
      final dll = File(p.join(dir.path, 'libmpv-2.dll'));
      await dll.writeAsBytes([0]);
      final exe = p.join(dir.path, 'tramp.exe');
      final path = LibmpvBundle.resolveLibraryPath(
        executablePath: exe,
        overrideOs: 'windows',
        environment: const {},
      );
      expect(path, dll.path);
    });

    test('falls back to third_party staging under repoRoot', () async {
      final root = await Directory.systemTemp.createTemp('tramp_root_');
      addTearDown(() => root.delete(recursive: true));
      final stagedDir = Directory(
        p.join(root.path, 'third_party', 'libmpv', 'windows', 'x86_64'),
      );
      await stagedDir.create(recursive: true);
      final dll = File(p.join(stagedDir.path, 'libmpv-2.dll'));
      await dll.writeAsBytes([0]);
      final path = LibmpvBundle.resolveLibraryPath(
        executablePath: p.join(root.path, 'missing', 'tramp.exe'),
        repoRoot: root.path,
        overrideOs: 'windows',
        environment: const {},
      );
      expect(path, dll.path);
    });
  });

  group('scanLibraryFile / verify', () {
    test('scanLibraryFile detects slim vs full fixture files', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_scan_');
      addTearDown(() => dir.delete(recursive: true));

      final slim = File(p.join(dir.path, 'slim.bin'));
      await slim.writeAsString(
        'cfg --disable-filters equalizer aresample padding',
      );
      final full = File(p.join(dir.path, 'full.bin'));
      await full.writeAsString('cfg equalizer aresample lavfi padding');

      expect(await LibmpvBundle.scanLibraryFile(slim.path), isFalse);
      expect(await LibmpvBundle.scanLibraryFile(full.path), isTrue);
    });

    test('verify enforce:false returns hasFilters for fixture', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_ver_');
      addTearDown(() => dir.delete(recursive: true));
      // Use the platform-native library filename so resolve prefers this
      // fixture over a distro libmpv on the host (e.g. /usr/lib64/libmpv.so).
      final name = LibmpvBundle.expectedLibraryFileName();
      final full = File(p.join(dir.path, name));
      await full.writeAsString('equalizer aresample ok');

      final info = await LibmpvBundle.verify(
        executablePath: p.join(dir.path, 'tramp'),
        enforce: false,
        environment: const {},
      );
      expect(info.hasFilters, isTrue);
      expect(info.path, full.path);
    });

    test('verify enforce:true throws on slim', () async {
      final dir = await Directory.systemTemp.createTemp('tramp_slim_');
      addTearDown(() => dir.delete(recursive: true));
      final name = LibmpvBundle.expectedLibraryFileName();
      final slim = File(p.join(dir.path, name));
      await slim.writeAsString('--disable-filters equalizer aresample');

      await expectLater(
        () => LibmpvBundle.verify(
          executablePath: p.join(dir.path, 'tramp'),
          enforce: true,
          environment: const {},
        ),
        throwsA(isA<StateError>()),
      );
    });
  });
}
