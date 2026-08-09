import 'builtin_look.dart';
import 'look_manifest.dart';
import 'look_materials.dart';
import 'look_palette.dart';
import 'resolved_look.dart';

const _maxChainLength = 8;
const _chromeFamily = 'TrampCondensed';
const _lcdFamily = 'TrampMono';

class LookMerger {
  static ResolvedLook resolve({
    required String activeId,
    required Map<String, LookManifest> installed,
  }) {
    final chain = _collectChain(activeId, installed);
    final active = chain.first;

    var mergedColors = <String, dynamic>{};
    var mergedMaterials = <String, dynamic>{};
    for (final manifest in chain.reversed) {
      mergedColors = _deepMerge(mergedColors, manifest.colors);
      mergedMaterials = _deepMerge(mergedMaterials, manifest.materials);
    }

    return ResolvedLook(
      id: active.id,
      name: active.name,
      author: active.author,
      palette: LookPalette.fromMergedColors(mergedColors),
      materials: LookMaterials.fromMergedMaterials(mergedMaterials),
      chromeFamily: _chromeFamily,
      lcdFamily: _lcdFamily,
    );
  }

  static List<LookManifest> _collectChain(
    String activeId,
    Map<String, LookManifest> installed,
  ) {
    final chain = <LookManifest>[];
    final visited = <String>{};
    var currentId = activeId;

    while (true) {
      if (visited.contains(currentId)) {
        throw FormatException('look extends cycle detected at $currentId');
      }
      if (chain.length >= _maxChainLength) {
        throw FormatException('look extends chain exceeds $_maxChainLength');
      }
      visited.add(currentId);

      final manifest = currentId == 'builtin'
          ? BuiltinLook.manifest
          : installed[currentId];
      if (manifest == null) {
        throw FormatException('look pack not found: $currentId');
      }

      chain.add(manifest);
      if (currentId == 'builtin') {
        break;
      }
      currentId = manifest.extendsId;
    }

    return chain;
  }

  static Map<String, dynamic> _deepMerge(
    Map<String, dynamic> base,
    Map<String, dynamic> overlay,
  ) {
    final result = Map<String, dynamic>.from(base);
    for (final entry in overlay.entries) {
      final key = entry.key;
      final value = entry.value;
      final existing = result[key];
      if (value is Map && existing is Map) {
        result[key] = _deepMerge(
          Map<String, dynamic>.from(existing),
          Map<String, dynamic>.from(value),
        );
      } else {
        result[key] = value;
      }
    }
    return result;
  }
}
