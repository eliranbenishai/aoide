import 'package:path/path.dart' as p;

import 'look_id.dart';
import 'look_manifest.dart';

final _colorHexPattern = RegExp(r'^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$');
final _fontFilePattern = RegExp(r'\.(ttf|otf)$', caseSensitive: false);

const _knownColorGroups = {
  'shell': {'highlight', 'base', 'mid', 'low', 'deep'},
  'ink': {'default', 'dim', 'faint'},
  'phosphor': {'default', 'hot', 'dim', 'deep'},
  'accent': {'default', 'dim'},
};

const _knownColorScalars = {'well'};

const _knownMaterialGroups = {
  'bevel': {'lightOpacity', 'softOpacity'},
  'spectrum': {'stops'},
  'rail': {'stops'},
};

const _knownFontRoles = {'chrome', 'lcd'};

class LookParser {
  static LookManifest parse(
    Map<String, dynamic> json, {
    bool allowBuiltin = false,
  }) {
    final formatVersion = json['formatVersion'];
    if (formatVersion != 1) {
      throw FormatException('formatVersion must be 1');
    }

    final id = json['id'];
    final idValid = id is String &&
        (allowBuiltin && id == 'builtin' ? isReservedLookId(id) : isValidLookId(id));
    if (!idValid) {
      throw FormatException('invalid look pack id: $id');
    }

    final name = json['name'];
    if (name is! String || name.isEmpty) {
      throw FormatException('name is required');
    }

    final extendsId = json['extends'];
    if (extendsId is! String || extendsId.isEmpty) {
      throw FormatException('extends is required');
    }

    final author = json['author'];
    if (author != null && author is! String) {
      throw FormatException('author must be a string');
    }

    final colors = _parseColors(json['colors']);
    final materials = _parseMaterials(json['materials']);
    final fonts = _parseFonts(json['fonts']);

    return LookManifest(
      formatVersion: formatVersion,
      id: id,
      name: name,
      author: author,
      extendsId: extendsId,
      colors: colors,
      materials: materials,
      fonts: fonts,
    );
  }

  static Map<String, dynamic> _parseColors(Object? value) {
    if (value == null) return const {};
    if (value is! Map) {
      throw FormatException('colors must be an object');
    }

    final result = <String, dynamic>{};
    for (final entry in value.entries) {
      final key = entry.key;
      if (key is! String) {
        throw FormatException('colors keys must be strings');
      }

      if (_knownColorScalars.contains(key)) {
        result[key] = _parseColorValue(entry.value, 'colors.$key');
        continue;
      }

      final knownKeys = _knownColorGroups[key];
      if (knownKeys == null) {
        throw FormatException('unknown color key: $key');
      }

      final group = entry.value;
      if (group is! Map) {
        throw FormatException('colors.$key must be an object');
      }

      final parsedGroup = <String, dynamic>{};
      for (final groupEntry in group.entries) {
        final subKey = groupEntry.key;
        if (subKey is! String) {
          throw FormatException('colors.$key keys must be strings');
        }
        if (!knownKeys.contains(subKey)) {
          throw FormatException('unknown color key: $key.$subKey');
        }
        parsedGroup[subKey] =
            _parseColorValue(groupEntry.value, 'colors.$key.$subKey');
      }
      result[key] = parsedGroup;
    }
    return result;
  }

  static Map<String, dynamic> _parseMaterials(Object? value) {
    if (value == null) return const {};
    if (value is! Map) {
      throw FormatException('materials must be an object');
    }

    final result = <String, dynamic>{};
    for (final entry in value.entries) {
      final key = entry.key;
      if (key is! String) {
        throw FormatException('materials keys must be strings');
      }

      final knownKeys = _knownMaterialGroups[key];
      if (knownKeys == null) {
        throw FormatException('unknown material key: $key');
      }

      final group = entry.value;
      if (group is! Map) {
        throw FormatException('materials.$key must be an object');
      }

      final parsedGroup = <String, dynamic>{};
      for (final groupEntry in group.entries) {
        final subKey = groupEntry.key;
        if (subKey is! String) {
          throw FormatException('materials.$key keys must be strings');
        }
        if (!knownKeys.contains(subKey)) {
          throw FormatException('unknown material key: $key.$subKey');
        }

        final path = 'materials.$key.$subKey';
        if (subKey == 'lightOpacity' || subKey == 'softOpacity') {
          parsedGroup[subKey] = _parseOpacity(groupEntry.value, path);
        } else if (subKey == 'stops') {
          parsedGroup[subKey] = _parseStops(groupEntry.value, path);
        }
      }
      result[key] = parsedGroup;
    }
    return result;
  }

  static Map<String, dynamic> _parseFonts(Object? value) {
    if (value == null) return const {};
    if (value is! Map) {
      throw FormatException('fonts must be an object');
    }

    final result = <String, dynamic>{};
    for (final entry in value.entries) {
      final key = entry.key;
      if (key is! String) {
        throw FormatException('fonts keys must be strings');
      }
      if (!_knownFontRoles.contains(key)) {
        throw FormatException('unknown font role: $key');
      }

      final role = entry.value;
      if (role is! Map) {
        throw FormatException('fonts.$key must be an object');
      }

      final parsedRole = <String, dynamic>{};
      for (final roleEntry in role.entries) {
        final subKey = roleEntry.key;
        if (subKey is! String) {
          throw FormatException('fonts.$key keys must be strings');
        }
        if (subKey == 'file') {
          parsedRole[subKey] = _parseFontFile(roleEntry.value, 'fonts.$key.file');
        } else if (subKey == 'weight') {
          parsedRole[subKey] = _parseFontWeight(roleEntry.value, 'fonts.$key.weight');
        } else {
          throw FormatException('unknown font key: $key.$subKey');
        }
      }

      if (!parsedRole.containsKey('file')) {
        throw FormatException('fonts.$key.file is required');
      }
      result[key] = parsedRole;
    }
    return result;
  }

  static String _parseColorValue(Object? value, String path) {
    if (value is! String || !_colorHexPattern.hasMatch(value)) {
      throw FormatException('$path must be #RRGGBB or #RRGGBBAA');
    }
    return value;
  }

  static num _parseOpacity(Object? value, String path) {
    if (value is! num || value < 0 || value > 1) {
      throw FormatException('$path must be a number from 0 to 1');
    }
    return value;
  }

  static List<String> _parseStops(Object? value, String path) {
    if (value is! List || value.isEmpty) {
      throw FormatException('$path must be a non-empty list of colors');
    }
    return [
      for (var i = 0; i < value.length; i++)
        _parseColorValue(value[i], '$path[$i]'),
    ];
  }

  static String _parseFontFile(Object? value, String path) {
    if (value is! String || !_fontFilePattern.hasMatch(value)) {
      throw FormatException('$path must be a pack-relative .ttf or .otf path');
    }
    final normalized = value.replaceAll('\\', '/');
    if (p.isAbsolute(normalized) ||
        normalized.startsWith('/') ||
        RegExp(r'^[a-zA-Z]:').hasMatch(normalized)) {
      throw FormatException('$path must not be an absolute path');
    }
    for (final segment in normalized.split('/')) {
      if (segment == '..') {
        throw FormatException('$path must not contain .. segments');
      }
    }
    return value;
  }

  static int _parseFontWeight(Object? value, String path) {
    if (value is! int) {
      throw FormatException('$path must be an integer');
    }
    return value;
  }
}
