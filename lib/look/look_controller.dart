import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as p;

import '../domain/tramp_settings.dart';
import '../platform/settings_store.dart';
import 'builtin_look.dart';
import 'look_catalog.dart';
import 'look_font_loader.dart';
import 'look_installer.dart';
import 'look_manifest.dart';
import 'look_merger.dart';
import 'resolved_look.dart';

class LookController extends ChangeNotifier {
  LookController({
    required SettingsStore settingsStore,
    required Future<Directory> Function() supportDir,
    LookFontLoader? fontLoader,
  })  : _settingsStore = settingsStore,
        _supportDir = supportDir,
        _fontLoader = fontLoader ?? LookFontLoader();

  final SettingsStore _settingsStore;
  Future<Directory> Function() _supportDir;
  final LookFontLoader _fontLoader;
  final LookCatalog _catalog = const LookCatalog();

  ResolvedLook _resolved = LookMerger.resolve(
    activeId: 'builtin',
    installed: const {},
  );
  String _activeLookId = 'builtin';
  Directory? _looksDirectory;
  Map<String, LookManifest> _installed = const {};
  Map<String, String> _fontFiles = const {};
  String? _lastError;
  TrampSettings _settings = TrampSettings.defaults;

  ResolvedLook get resolved => _resolved;
  /// Absolute font paths for overridden roles (`chrome` / `lcd`).
  Map<String, String> get fontFiles => _fontFiles;
  String get activeLookId => _activeLookId;
  Directory get looksDirectory =>
      _looksDirectory ?? Directory(p.join('looks'));
  List<LookManifest> get installed => [
        BuiltinLook.manifest,
        ..._installed.values,
      ];
  String? get lastError => _lastError;

  Future<void> bootstrap({
    required TrampSettings settings,
    required Future<Directory> Function() supportDir,
  }) async {
    _supportDir = supportDir;
    _settings = settings;
    _looksDirectory = await _resolveLooksDirectory(settings.looksDirectory);
    await _rescan();

    final requested = settings.activeLookId;
    final available = requested == 'builtin' || _installed.containsKey(requested);
    await _activateInternal(available ? requested : 'builtin');
  }

  Future<void> setLooksDirectory(String? absolutePath) async {
    _settings = _settings.copyWith(
      looksDirectory: absolutePath,
      clearLooksDirectory: absolutePath == null,
    );
    _looksDirectory = await _resolveLooksDirectory(absolutePath);
    await _persistSettings();
    await _rescan();

    final active = _activeLookId;
    if (active != 'builtin' && !_installed.containsKey(active)) {
      await activate('builtin');
      return;
    }
    await _activateInternal(active);
  }

  Future<void> activate(String id) async {
    await _activateInternal(id);
  }

  Future<bool> installZip(
    File zip, {
    required Future<LookConflictChoice> Function(LookConflict) onConflict,
  }) async {
    try {
      final installer = LookInstaller(
        looksDir: looksDirectory,
        onConflict: onConflict,
      );
      final ok = await installer.installZip(zip);
      if (ok) {
        await _rescan();
        _lastError = null;
        notifyListeners();
      }
      return ok;
    } catch (e) {
      _lastError = _shortError(e);
      notifyListeners();
      return false;
    }
  }

  Future<bool> installDirectory(
    Directory dir, {
    required Future<LookConflictChoice> Function(LookConflict) onConflict,
  }) async {
    try {
      final installer = LookInstaller(
        looksDir: looksDirectory,
        onConflict: onConflict,
      );
      final ok = await installer.installDirectory(dir);
      if (ok) {
        await _rescan();
        _lastError = null;
        notifyListeners();
      }
      return ok;
    } catch (e) {
      _lastError = _shortError(e);
      notifyListeners();
      return false;
    }
  }

  Future<void> _activateInternal(String id) async {
    try {
      final (next, fontFiles) = await _resolveWithFonts(id);
      _resolved = next;
      _fontFiles = fontFiles;
      _activeLookId = id;
      _lastError = null;
      _settings = _settings.copyWith(activeLookId: id);
      await _persistSettings();
      notifyListeners();
    } catch (e) {
      _lastError = _shortError(e);
      notifyListeners();
    }
  }

  String _shortError(Object e) {
    if (e is FormatException) {
      final message = e.message;
      return message.length > 160 ? '${message.substring(0, 157)}...' : message;
    }
    if (e is FileSystemException) {
      final message = e.message;
      if (message.isNotEmpty) {
        return message.length > 160
            ? '${message.substring(0, 157)}...'
            : message;
      }
    }
    final text = e.toString();
    return text.length > 160 ? '${text.substring(0, 157)}...' : text;
  }

  Future<(ResolvedLook, Map<String, String>)> _resolveWithFonts(
    String id,
  ) async {
    final base = LookMerger.resolve(activeId: id, installed: _installed);
    final chain = _collectChain(id);
    var chromeFamily = base.chromeFamily;
    var lcdFamily = base.lcdFamily;
    final fontFiles = <String, String>{};

    for (final role in const ['chrome', 'lcd']) {
      final nearest = _nearestFont(chain, role);
      if (nearest == null) continue;

      final (manifest, fileRel, weight) = nearest;
      if (manifest.id == 'builtin') continue;

      final packRoot = p.canonicalize(
        p.join(_looksDirectory!.path, manifest.id),
      );
      final file = File(p.normalize(p.join(packRoot, fileRel)));
      final canonicalFile = p.canonicalize(file.path);
      if (!p.isWithin(packRoot, canonicalFile)) {
        throw FormatException('font file escapes pack root: $fileRel');
      }
      if (!await file.exists()) {
        throw FileSystemException('font file missing', file.path);
      }

      final family = await _fontLoader.ensureFamily(
        packId: id,
        role: role,
        file: file,
        weight: weight,
      );
      fontFiles[role] = file.absolute.path;
      if (role == 'chrome') {
        chromeFamily = family;
      } else {
        lcdFamily = family;
      }
    }

    return (
      ResolvedLook(
        id: base.id,
        name: base.name,
        author: base.author,
        palette: base.palette,
        materials: base.materials,
        chromeFamily: chromeFamily,
        lcdFamily: lcdFamily,
      ),
      Map<String, String>.unmodifiable(fontFiles),
    );
  }

  (LookManifest, String, int)? _nearestFont(
    List<LookManifest> chain,
    String role,
  ) {
    for (final manifest in chain) {
      final roleMap = manifest.fonts[role];
      if (roleMap is! Map) continue;
      final file = roleMap['file'];
      if (file is! String) continue;
      final weight = roleMap['weight'];
      return (manifest, file, weight is int ? weight : 400);
    }
    return null;
  }

  List<LookManifest> _collectChain(String activeId) {
    final chain = <LookManifest>[];
    final visited = <String>{};
    var currentId = activeId;

    while (true) {
      if (visited.contains(currentId)) {
        throw FormatException('look extends cycle detected at $currentId');
      }
      if (chain.length >= 8) {
        throw FormatException('look extends chain exceeds 8');
      }
      visited.add(currentId);

      final manifest = currentId == 'builtin'
          ? BuiltinLook.manifest
          : _installed[currentId];
      if (manifest == null) {
        throw FormatException('look pack not found: $currentId');
      }

      chain.add(manifest);
      if (currentId == 'builtin') break;
      currentId = manifest.extendsId;
    }
    return chain;
  }

  Future<void> _rescan() async {
    final dir = _looksDirectory!;
    final result = await _catalog.scan(dir);
    _installed = result.manifests;
  }

  Future<Directory> _resolveLooksDirectory(String? absolutePath) async {
    if (absolutePath != null && absolutePath.isNotEmpty) {
      return Directory(absolutePath);
    }
    return LookCatalog.defaultLooksDirectory(_supportDir);
  }

  Future<void> _persistSettings() async {
    await _settingsStore.write(_settings);
  }
}
