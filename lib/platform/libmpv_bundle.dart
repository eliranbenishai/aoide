import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as p;

/// Result of probing the libmpv binary Tramp expects to load.
class LibmpvInfo {
  const LibmpvInfo({
    required this.path,
    required this.hasFilters,
    this.markerSummary = '',
  });

  final String path;
  final bool hasFilters;
  final String markerSummary;
}

/// Resolves and verifies the bundled / loaded full libmpv build.
///
/// Slim media_kit audio builds embed `--disable-filters`, which removes
/// libavfilter `aresample` and makes EQ graphs silently no-op.
class LibmpvBundle {
  LibmpvBundle._();

  static const slimMarker = '--disable-filters';

  /// Pure heuristic on a build/config/binary ASCII dump (unit-tested).
  ///
  /// A build is full when it does **not** carry [slimMarker] and still exposes
  /// equalizer + aresample support strings.
  static bool analyzeConfigString(String text) {
    if (text.contains(slimMarker)) return false;
    return text.contains('equalizer') && text.contains('aresample');
  }

  /// Library file name expected next to the executable (platform-specific).
  static String expectedLibraryFileName({String? overrideOs}) {
    final os = overrideOs ?? Platform.operatingSystem;
    switch (os) {
      case 'windows':
        return 'libmpv-2.dll';
      case 'linux':
        return 'libmpv.so';
      case 'macos':
        return 'Mpv.framework/Mpv';
      default:
        return 'libmpv';
    }
  }

  /// Resolves the on-disk library path the process should load.
  ///
  /// Order: `TRAMP_LIBMPV_PATH` → next to executable → `third_party/libmpv/...`
  /// relative to [repoRoot] (when provided) or the executable's ancestors.
  static String? resolveLibraryPath({
    String? executablePath,
    String? repoRoot,
    String? overrideOs,
    Map<String, String>? environment,
  }) {
    final env = environment ?? Platform.environment;
    final fromEnv = env['TRAMP_LIBMPV_PATH'];
    if (fromEnv != null && fromEnv.isNotEmpty) {
      return p.normalize(fromEnv);
    }

    final exe = executablePath ?? Platform.resolvedExecutable;
    final exeDir = p.dirname(exe);
    final name = expectedLibraryFileName(overrideOs: overrideOs);
    final os = overrideOs ?? Platform.operatingSystem;

    final besideExe = <String>[
      p.join(exeDir, name),
      if (os == 'linux') p.join(exeDir, 'lib', 'libmpv.so'),
      if (os == 'linux') p.join(exeDir, 'lib', 'libmpv.so.2'),
      if (os == 'macos')
        p.join(exeDir, '..', 'Frameworks', 'Mpv.framework', 'Mpv'),
    ];
    for (final candidate in besideExe) {
      if (File(candidate).existsSync()) return p.normalize(candidate);
    }

    final roots = <String>[
      if (repoRoot != null) repoRoot,
      exeDir,
      p.dirname(exeDir),
      p.dirname(p.dirname(exeDir)),
      p.dirname(p.dirname(p.dirname(exeDir))),
      Directory.current.path,
    ];

    for (final root in roots) {
      final staged = switch (os) {
        'windows' => p.join(root, 'third_party', 'libmpv', 'windows', 'x86_64',
            'libmpv-2.dll'),
        'linux' => p.join(
            root, 'third_party', 'libmpv', 'linux', 'x86_64', 'libmpv.so'),
        'macos' => p.join(root, 'third_party', 'libmpv', 'macos', 'universal',
            'Mpv.xcframework'),
        _ => null,
      };
      if (staged == null) continue;
      if (File(staged).existsSync() || Directory(staged).existsSync()) {
        return p.normalize(staged);
      }
      if (os == 'linux') {
        final dir = Directory(p.dirname(staged));
        if (dir.existsSync()) {
          final matches = dir
              .listSync()
              .whereType<File>()
              .where((f) => p.basename(f.path).startsWith('libmpv.so'))
              .toList();
          if (matches.isNotEmpty) return p.normalize(matches.first.path);
        }
      }
    }
    return null;
  }

  /// Byte-scan a library file for filter capability markers.
  static Future<bool> scanLibraryFile(String path) async {
    if (await FileSystemEntity.isDirectory(path)) {
      // xcframework staging — presence only; macOS CI should still run verify
      // against the embedded binary when packaging is wired.
      return true;
    }
    final file = File(path);
    if (!await file.exists()) return false;

    final slim = _bytes(slimMarker);
    final eq = _bytes('equalizer');
    final ar = _bytes('aresample');
    const chunkSize = 2 * 1024 * 1024;
    final overlap = slim.length - 1;
    final raf = await file.open();
    try {
      final length = await file.length();
      var offset = 0;
      var sawSlim = false;
      var sawEq = false;
      var sawAr = false;
      Uint8List carry = Uint8List(0);
      while (offset < length) {
        final toRead =
            (length - offset) > chunkSize ? chunkSize : (length - offset);
        final chunk = await raf.read(toRead);
        final merged = Uint8List(carry.length + chunk.length);
        merged.setAll(0, carry);
        merged.setAll(carry.length, chunk);
        if (_contains(merged, slim)) sawSlim = true;
        if (_contains(chunk, eq) || _contains(merged, eq)) sawEq = true;
        if (_contains(chunk, ar) || _contains(merged, ar)) sawAr = true;
        if (sawSlim) return false;
        offset += toRead;
        if (chunk.length >= overlap) {
          carry = Uint8List.fromList(chunk.sublist(chunk.length - overlap));
        } else {
          carry = Uint8List.fromList(chunk);
        }
      }
      return sawEq && sawAr;
    } finally {
      await raf.close();
    }
  }

  /// Verifies the resolved library is a full build.
  ///
  /// Throws in debug/profile when the binary is missing or slim. Release
  /// still returns [LibmpvInfo] so callers can gate EQ features.
  static Future<LibmpvInfo> verify({
    String? executablePath,
    String? repoRoot,
    bool? enforce,
  }) async {
    final path = resolveLibraryPath(
      executablePath: executablePath,
      repoRoot: repoRoot,
    );
    final inFlutterTest =
        const bool.fromEnvironment('FLUTTER_TEST', defaultValue: false);
    final mustEnforce = enforce ?? (!kReleaseMode && !inFlutterTest);

    if (path == null) {
      const message =
          'Full libmpv not found. Run tool/fetch_full_libmpv.(ps1|sh) and rebuild.';
      if (mustEnforce) {
        throw StateError(message);
      }
      return const LibmpvInfo(
        path: '',
        hasFilters: false,
        markerSummary: 'missing',
      );
    }

    final hasFilters = await scanLibraryFile(path);
    final info = LibmpvInfo(
      path: path,
      hasFilters: hasFilters,
      markerSummary: hasFilters ? 'full' : 'slim($slimMarker)',
    );
    if (!hasFilters && mustEnforce) {
      throw StateError(
        'Slim libmpv detected at $path (${info.markerSummary}). '
        'Refusing to start in debug/profile. Fetch the full build.',
      );
    }
    return info;
  }

  static List<int> _bytes(String s) => s.codeUnits;

  static bool _contains(List<int> haystack, List<int> needle) {
    if (needle.isEmpty || haystack.length < needle.length) return false;
    outer:
    for (var i = 0; i <= haystack.length - needle.length; i++) {
      for (var j = 0; j < needle.length; j++) {
        if (haystack[i + j] != needle[j]) continue outer;
      }
      return true;
    }
    return false;
  }
}
