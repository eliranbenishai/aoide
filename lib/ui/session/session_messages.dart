import 'dart:convert';

import '../../domain/equalizer_settings.dart';
import '../../domain/saved_playlist.dart';
import '../../domain/track.dart';
import '../../domain/tramp_settings.dart';
import '../../look/look_materials.dart';
import '../../look/look_palette.dart';
import '../../look/resolved_look.dart';

/// Which Flutter engine / OS window this entrypoint owns.
enum WindowRole { main, equalizer, playlist, settings, about }

/// JSON arguments passed to [WindowController.create] / secondary engines.
String encodeWindowArguments(WindowRole role) =>
    jsonEncode(<String, dynamic>{'role': role.name});

/// Parses [WindowController.arguments] (empty → main).
WindowRole parseWindowRole(String arguments) {
  final trimmed = arguments.trim();
  if (trimmed.isEmpty) return WindowRole.main;
  final decoded = jsonDecode(trimmed);
  if (decoded is! Map) {
    throw FormatException('window arguments must be a JSON object');
  }
  final role = decoded['role'];
  if (role == null || role == 'main') return WindowRole.main;
  if (role == 'equalizer') return WindowRole.equalizer;
  if (role == 'playlist') return WindowRole.playlist;
  if (role == 'settings') return WindowRole.settings;
  if (role == 'about') return WindowRole.about;
  throw FormatException('unknown window role: $role');
}

/// Process-argv helper for hosts that do not yet have a [WindowController].
///
/// Secondary engines should prefer [parseWindowRole] on
/// `WindowController.arguments`. Empty / non-JSON argv → [WindowRole.main].
WindowRole parseMultiWindowRole(List<String> args) {
  for (final arg in args) {
    final trimmed = arg.trim();
    if (trimmed.isEmpty) continue;
    if (trimmed.startsWith('{')) {
      return parseWindowRole(trimmed);
    }
  }
  return WindowRole.main;
}

/// Host → client notifications.
sealed class SessionEvent {
  const SessionEvent();

  String get type;

  Map<String, dynamic> toJson();

  static SessionEvent fromJson(Map<String, dynamic> json) {
    final type = json['type'];
    final payload = _payload(json);
    switch (type) {
      case ZoomChangedEvent.typeName:
        return ZoomChangedEvent.fromPayload(payload);
      case PlaybackSnapshotEvent.typeName:
        return PlaybackSnapshotEvent.fromPayload(payload);
      case EqSnapshotEvent.typeName:
        return EqSnapshotEvent.fromPayload(payload);
      case PlaylistSnapshotEvent.typeName:
        return PlaylistSnapshotEvent.fromPayload(payload);
      case PlaylistCollectionSnapshotEvent.typeName:
        return PlaylistCollectionSnapshotEvent.fromPayload(payload);
      case DockSnapshotEvent.typeName:
        return DockSnapshotEvent.fromPayload(payload);
      case LookSnapshotEvent.typeName:
        return LookSnapshotEvent.fromPayload(payload);
      case SettingsSnapshotEvent.typeName:
        return SettingsSnapshotEvent.fromPayload(payload);
      case LevelsFrameEvent.typeName:
        return LevelsFrameEvent.fromPayload(payload);
      default:
        throw FormatException('unknown SessionEvent type: $type');
    }
  }

  Map<String, dynamic> toEnvelope() => {
        'type': type,
        'payload': toJson(),
      };

  static Map<String, dynamic> decodeEnvelope(Object? raw) {
    if (raw is String) {
      final decoded = jsonDecode(raw);
      if (decoded is Map<String, dynamic>) return decoded;
      if (decoded is Map) return Map<String, dynamic>.from(decoded);
    }
    if (raw is Map<String, dynamic>) return raw;
    if (raw is Map) return Map<String, dynamic>.from(raw);
    throw FormatException('invalid SessionEvent envelope');
  }
}

final class ZoomChangedEvent extends SessionEvent {
  const ZoomChangedEvent(this.zoomPercent);

  static const typeName = 'zoom_changed';

  final int zoomPercent;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'zoomPercent': zoomPercent};

  factory ZoomChangedEvent.fromPayload(Map<String, dynamic> json) {
    final percent = json['zoomPercent'];
    if (percent is! num) {
      throw const FormatException('ZoomChangedEvent.zoomPercent');
    }
    return ZoomChangedEvent(percent.toInt());
  }
}

final class PlaybackSnapshotEvent extends SessionEvent {
  const PlaybackSnapshotEvent({
    required this.playing,
    required this.positionMs,
    required this.durationMs,
    required this.volume,
    required this.muted,
    required this.shuffle,
    required this.repeatMode,
    this.playingPath,
  });

  static const typeName = 'playback_snapshot';

  final bool playing;
  final int positionMs;
  final int durationMs;
  final double volume;
  final bool muted;
  final bool shuffle;
  final String repeatMode;
  final String? playingPath;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'playing': playing,
        'positionMs': positionMs,
        'durationMs': durationMs,
        'volume': volume,
        'muted': muted,
        'shuffle': shuffle,
        'repeatMode': repeatMode,
        'playingPath': playingPath,
      };

  factory PlaybackSnapshotEvent.fromPayload(Map<String, dynamic> json) {
    return PlaybackSnapshotEvent(
      playing: json['playing'] == true,
      positionMs: (json['positionMs'] as num?)?.toInt() ?? 0,
      durationMs: (json['durationMs'] as num?)?.toInt() ?? 0,
      volume: (json['volume'] as num?)?.toDouble() ?? 1.0,
      muted: json['muted'] == true,
      shuffle: json['shuffle'] == true,
      repeatMode: json['repeatMode'] as String? ?? 'off',
      playingPath: json['playingPath'] as String?,
    );
  }
}

final class EqSnapshotEvent extends SessionEvent {
  const EqSnapshotEvent(this.settings);

  static const typeName = 'eq_snapshot';

  final EqualizerSettings settings;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'settings': settings.toJson()};

  factory EqSnapshotEvent.fromPayload(Map<String, dynamic> json) {
    final raw = json['settings'];
    if (raw is! Map) {
      throw const FormatException('EqSnapshotEvent.settings');
    }
    return EqSnapshotEvent(
      EqualizerSettings.fromJson(Map<String, dynamic>.from(raw)),
    );
  }
}

final class PlaylistSnapshotEvent extends SessionEvent {
  const PlaylistSnapshotEvent({
    required this.tracks,
    required this.selectedIndices,
    this.selectedIndex,
    this.sourcePath,
    this.playingIndex,
    this.playing = false,
    this.altered = false,
  });

  static const typeName = 'playlist_snapshot';

  final List<Track> tracks;
  final List<int> selectedIndices;
  final int? selectedIndex;
  final String? sourcePath;
  final int? playingIndex;
  final bool playing;

  /// Whether this is an **altered current playlist**. Rides the playlist
  /// snapshot because the altered state lives with the current playlist on the
  /// host, and the Playlist Manager that has to ask about it before replacing it
  /// is a separate engine.
  final bool altered;

  int get trackCount => tracks.length;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'tracks': [for (final t in tracks) t.toJson()],
        'trackCount': tracks.length,
        'selectedIndices': selectedIndices,
        'selectedIndex': selectedIndex,
        'sourcePath': sourcePath,
        'playingIndex': playingIndex,
        'playing': playing,
        'altered': altered,
      };

  factory PlaylistSnapshotEvent.fromPayload(Map<String, dynamic> json) {
    final rawTracks = json['tracks'];
    final tracks = <Track>[];
    if (rawTracks is List) {
      for (final item in rawTracks) {
        if (item is Map) {
          tracks.add(Track.fromJson(Map<String, dynamic>.from(item)));
        }
      }
    }
    final rawSelected = json['selectedIndices'];
    final selectedIndices = <int>[];
    if (rawSelected is List) {
      for (final item in rawSelected) {
        if (item is num) selectedIndices.add(item.toInt());
      }
    }
    final selectedIndex = (json['selectedIndex'] as num?)?.toInt();
    if (selectedIndices.isEmpty && selectedIndex != null) {
      selectedIndices.add(selectedIndex);
    }
    return PlaylistSnapshotEvent(
      tracks: tracks,
      selectedIndices: selectedIndices,
      selectedIndex: selectedIndex,
      sourcePath: json['sourcePath'] as String?,
      playingIndex: (json['playingIndex'] as num?)?.toInt(),
      playing: json['playing'] == true,
      // A snapshot from before this field decodes as unaltered, which is the
      // safe way round: it can only cost a prompt, never a playlist.
      altered: json['altered'] == true,
    );
  }
}

/// The listener's playlist collection, for the Playlist Manager's left panel.
///
/// Deliberately its own event rather than a widening of [SettingsSnapshotEvent]:
/// the collection is content the listener keeps, not a preference, and it
/// changes on a different cadence.
final class PlaylistCollectionSnapshotEvent extends SessionEvent {
  const PlaylistCollectionSnapshotEvent({
    required this.playlists,
    this.selectedPath,
    this.disabledPaths = const [],
    this.lastError,
  });

  static const typeName = 'playlist_collection_snapshot';

  /// Saved playlists in the order the panel paints them.
  final List<SavedPlaylist> playlists;

  /// Normalized path of the loaded / highlighted entry, if any.
  final String? selectedPath;

  /// Normalized paths of the **disabled playlists** — entries whose file was
  /// missing at the host's last check.
  ///
  /// Rides beside [playlists] rather than inside a [SavedPlaylist] field,
  /// because disabled is derived from that check and never stored: the entries
  /// here are exactly what the index on disk holds.
  final List<String> disabledPaths;
  final String? lastError;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'playlists': [for (final entry in playlists) entry.toJson()],
        'selectedPath': selectedPath,
        'disabledPaths': disabledPaths,
        'lastError': lastError,
      };

  factory PlaylistCollectionSnapshotEvent.fromPayload(
    Map<String, dynamic> json,
  ) {
    final raw = json['playlists'];
    final playlists = <SavedPlaylist>[];
    if (raw is List) {
      for (final item in raw) {
        if (item is Map) {
          playlists.add(
            SavedPlaylist.fromJson(Map<String, dynamic>.from(item)),
          );
        }
      }
    }
    final selected = json['selectedPath'];
    final disabled = json['disabledPaths'];
    return PlaylistCollectionSnapshotEvent(
      playlists: playlists,
      selectedPath:
          selected is String && selected.isNotEmpty ? selected : null,
      disabledPaths: [
        if (disabled is List)
          for (final path in disabled)
            if (path is String && path.isNotEmpty) path,
      ],
      lastError: json['lastError'] as String?,
    );
  }
}

final class DockSnapshotEvent extends SessionEvent {
  const DockSnapshotEvent({
    required this.main,
    required this.equalizer,
    required this.playlist,
    required this.settings,
    required this.about,
    required this.dockEdges,
    required this.zoomPercent,
  });

  static const typeName = 'dock_snapshot';

  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final WindowFrameState settings;
  final WindowFrameState about;
  final List<DockEdge> dockEdges;
  final int zoomPercent;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'main': main.toJson(),
        'equalizer': equalizer.toJson(),
        'playlist': playlist.toJson(),
        'settings': settings.toJson(),
        'about': about.toJson(),
        'dockEdges': dockEdges.map((e) => e.toJson()).toList(),
        'zoomPercent': zoomPercent,
      };

  factory DockSnapshotEvent.fromPayload(Map<String, dynamic> json) {
    return DockSnapshotEvent(
      main: WindowFrameState.fromJson(
        Map<String, dynamic>.from(json['main'] as Map? ?? const {}),
        fallback: WindowFrameState.mainDefault,
      ),
      equalizer: WindowFrameState.fromJson(
        Map<String, dynamic>.from(json['equalizer'] as Map? ?? const {}),
        fallback: WindowFrameState.equalizerDefault,
      ),
      playlist: WindowFrameState.fromJson(
        Map<String, dynamic>.from(json['playlist'] as Map? ?? const {}),
        fallback: WindowFrameState.playlistDefault,
      ),
      settings: WindowFrameState.fromJson(
        Map<String, dynamic>.from(json['settings'] as Map? ?? const {}),
        fallback: WindowFrameState.settingsDefault,
      ),
      about: WindowFrameState.fromJson(
        Map<String, dynamic>.from(json['about'] as Map? ?? const {}),
        fallback: WindowFrameState.aboutDefault,
      ),
      dockEdges: [
        for (final edge in (json['dockEdges'] as List? ?? const []))
          DockEdge.fromJson(Map<String, dynamic>.from(edge as Map)),
      ],
      zoomPercent: (json['zoomPercent'] as num?)?.toInt() ?? 100,
    );
  }
}

/// Catalog entry for a skin (look pack) in settings UI.
final class SkinCatalogEntry {
  const SkinCatalogEntry({
    required this.id,
    required this.name,
    this.author,
  });

  final String id;
  final String name;
  final String? author;

  Map<String, dynamic> toJson() => {
        'id': id,
        'name': name,
        if (author != null) 'author': author,
      };

  factory SkinCatalogEntry.fromJson(Map<String, dynamic> json) {
    final id = json['id'];
    final name = json['name'];
    if (id is! String || id.isEmpty) {
      throw const FormatException('SkinCatalogEntry.id');
    }
    if (name is! String || name.isEmpty) {
      throw const FormatException('SkinCatalogEntry.name');
    }
    final author = json['author'];
    return SkinCatalogEntry(
      id: id,
      name: name,
      author: author is String && author.isNotEmpty ? author : null,
    );
  }
}

/// General prefs + skin catalog for the settings window.
final class SettingsSnapshotEvent extends SessionEvent {
  const SettingsSnapshotEvent({
    required this.resumeLastSession,
    required this.confirmBeforeQuit,
    required this.scrollTitle,
    required this.minimizeHidesSecondaries,
    required this.dockSnapStrength,
    required this.skins,
    required this.activeSkinId,
    this.lastSkinError,
    this.playlistCollectionWidth = TrampSettings.defaultPlaylistCollectionWidth,
    this.playlistCollectionCollapsed = false,
  });

  static const typeName = 'settings_snapshot';

  final bool resumeLastSession;
  final bool confirmBeforeQuit;
  final bool scrollTitle;
  final bool minimizeHidesSecondaries;
  final DockSnapStrength dockSnapStrength;
  final List<SkinCatalogEntry> skins;
  final String activeSkinId;
  final String? lastSkinError;

  /// Playlist Manager collection panel layout, so the playlist window paints
  /// the persisted divider position instead of the default.
  final double playlistCollectionWidth;
  final bool playlistCollectionCollapsed;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'resumeLastSession': resumeLastSession,
        'confirmBeforeQuit': confirmBeforeQuit,
        'scrollTitle': scrollTitle,
        'minimizeHidesSecondaries': minimizeHidesSecondaries,
        'dockSnapStrength': dockSnapStrength.name,
        'skins': [for (final s in skins) s.toJson()],
        'activeSkinId': activeSkinId,
        'lastSkinError': lastSkinError,
        'playlistCollectionWidth': playlistCollectionWidth,
        'playlistCollectionCollapsed': playlistCollectionCollapsed,
      };

  factory SettingsSnapshotEvent.fromPayload(Map<String, dynamic> json) {
    final rawSkins = json['skins'];
    final skins = <SkinCatalogEntry>[];
    if (rawSkins is List) {
      for (final item in rawSkins) {
        if (item is Map) {
          skins.add(SkinCatalogEntry.fromJson(Map<String, dynamic>.from(item)));
        }
      }
    }
    final active = json['activeSkinId'];
    return SettingsSnapshotEvent(
      resumeLastSession: json['resumeLastSession'] == true,
      confirmBeforeQuit: json['confirmBeforeQuit'] == true,
      scrollTitle: json['scrollTitle'] != false,
      minimizeHidesSecondaries: json['minimizeHidesSecondaries'] != false,
      dockSnapStrength:
          DockSnapStrength.tryParse(json['dockSnapStrength']) ??
              DockSnapStrength.normal,
      skins: skins,
      activeSkinId: active is String && active.isNotEmpty ? active : 'builtin',
      lastSkinError: json['lastSkinError'] as String?,
      playlistCollectionWidth: _positiveWidth(json['playlistCollectionWidth']) ??
          TrampSettings.defaultPlaylistCollectionWidth,
      playlistCollectionCollapsed: json['playlistCollectionCollapsed'] == true,
    );
  }

  static double? _positiveWidth(Object? raw) {
    if (raw is! num) return null;
    final value = raw.toDouble();
    if (!value.isFinite || value <= 0) return null;
    return value;
  }
}

final class LookSnapshotEvent extends SessionEvent {
  const LookSnapshotEvent({
    required this.id,
    required this.name,
    required this.palette,
    required this.materials,
    required this.chromeFamily,
    required this.lcdFamily,
    this.author,
    this.fontFiles,
  });

  static const typeName = 'look_snapshot';

  final String id;
  final String name;
  final String? author;
  final LookPalette palette;
  final LookMaterials materials;
  final String chromeFamily;
  final String lcdFamily;
  final Map<String, String>? fontFiles;

  @override
  String get type => typeName;

  factory LookSnapshotEvent.fromResolved(
    ResolvedLook look, {
    Map<String, String>? fontFiles,
  }) {
    final files = fontFiles == null || fontFiles.isEmpty
        ? null
        : Map<String, String>.from(fontFiles);
    return LookSnapshotEvent(
      id: look.id,
      name: look.name,
      author: look.author,
      palette: look.palette,
      materials: look.materials,
      chromeFamily: look.chromeFamily,
      lcdFamily: look.lcdFamily,
      fontFiles: files,
    );
  }

  ResolvedLook toResolved() => ResolvedLook(
        id: id,
        name: name,
        author: author,
        palette: palette,
        materials: materials,
        chromeFamily: chromeFamily,
        lcdFamily: lcdFamily,
      );

  @override
  Map<String, dynamic> toJson() => {
        'id': id,
        'name': name,
        if (author != null) 'author': author,
        'palette': palette.toJson(),
        'materials': materials.toJson(),
        'chromeFamily': chromeFamily,
        'lcdFamily': lcdFamily,
        if (fontFiles != null && fontFiles!.isNotEmpty) 'fontFiles': fontFiles,
      };

  factory LookSnapshotEvent.fromPayload(Map<String, dynamic> json) {
    final id = json['id'];
    final name = json['name'];
    if (id is! String || id.isEmpty) {
      throw const FormatException('LookSnapshotEvent.id');
    }
    if (name is! String || name.isEmpty) {
      throw const FormatException('LookSnapshotEvent.name');
    }
    final chromeFamily = json['chromeFamily'];
    final lcdFamily = json['lcdFamily'];
    if (chromeFamily is! String || chromeFamily.isEmpty) {
      throw const FormatException('LookSnapshotEvent.chromeFamily');
    }
    if (lcdFamily is! String || lcdFamily.isEmpty) {
      throw const FormatException('LookSnapshotEvent.lcdFamily');
    }
    final paletteRaw = json['palette'];
    final materialsRaw = json['materials'];
    if (paletteRaw is! Map) {
      throw const FormatException('LookSnapshotEvent.palette');
    }
    if (materialsRaw is! Map) {
      throw const FormatException('LookSnapshotEvent.materials');
    }
    Map<String, String>? fontFiles;
    final fontFilesRaw = json['fontFiles'];
    if (fontFilesRaw is Map && fontFilesRaw.isNotEmpty) {
      fontFiles = {
        for (final entry in fontFilesRaw.entries)
          if (entry.key is String && entry.value is String)
            entry.key as String: entry.value as String,
      };
      if (fontFiles.isEmpty) fontFiles = null;
    }
    final author = json['author'];
    return LookSnapshotEvent(
      id: id,
      name: name,
      author: author is String && author.isNotEmpty ? author : null,
      palette: LookPalette.fromJson(Map<String, dynamic>.from(paletteRaw)),
      materials:
          LookMaterials.fromJson(Map<String, dynamic>.from(materialsRaw)),
      chromeFamily: chromeFamily,
      lcdFamily: lcdFamily,
      fontFiles: fontFiles,
    );
  }
}

final class LevelsFrameEvent extends SessionEvent {
  const LevelsFrameEvent(this.bands);

  static const typeName = 'levels_frame';

  final List<double> bands;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'bands': bands};

  factory LevelsFrameEvent.fromPayload(Map<String, dynamic> json) {
    final raw = json['bands'];
    if (raw is! List) {
      throw const FormatException('LevelsFrameEvent.bands');
    }
    return LevelsFrameEvent([
      for (final value in raw) (value as num).toDouble(),
    ]);
  }
}

/// Client → host requests.
sealed class SessionCommand {
  const SessionCommand();

  String get type;

  Map<String, dynamic> toJson();

  static SessionCommand fromJson(Map<String, dynamic> json) {
    final type = json['type'];
    final payload = _payload(json);
    switch (type) {
      case TransportCommand.typeName:
        return TransportCommand.fromPayload(payload);
      case SeekCommand.typeName:
        return SeekCommand.fromPayload(payload);
      case VolumeCommand.typeName:
        return VolumeCommand.fromPayload(payload);
      case MonoCommand.typeName:
        return MonoCommand.fromPayload(payload);
      case ToggleWindowCommand.typeName:
        return ToggleWindowCommand.fromPayload(payload);
      case EqGainCommand.typeName:
        return EqGainCommand.fromPayload(payload);
      case EqPreampCommand.typeName:
        return EqPreampCommand.fromPayload(payload);
      case EqEnabledCommand.typeName:
        return EqEnabledCommand.fromPayload(payload);
      case EqAutoCommand.typeName:
        return EqAutoCommand.fromPayload(payload);
      case ApplyPresetCommand.typeName:
        return ApplyPresetCommand.fromPayload(payload);
      case SetShadedCommand.typeName:
        return SetShadedCommand.fromPayload(payload);
      case PlaylistOpCommand.typeName:
        return PlaylistOpCommand.fromPayload(payload);
      case ResizePlaylistCommand.typeName:
        return ResizePlaylistCommand.fromPayload(payload);
      case ResizePlaylistCollectionCommand.typeName:
        return ResizePlaylistCollectionCommand.fromPayload(payload);
      case AddSavedPlaylistCommand.typeName:
        return AddSavedPlaylistCommand.fromPayload(payload);
      case RemoveSavedPlaylistCommand.typeName:
        return RemoveSavedPlaylistCommand.fromPayload(payload);
      case SelectSavedPlaylistCommand.typeName:
        return SelectSavedPlaylistCommand.fromPayload(payload);
      case LoadSavedPlaylistCommand.typeName:
        return LoadSavedPlaylistCommand.fromPayload(payload);
      case MoveWindowCommand.typeName:
        return MoveWindowCommand.fromPayload(payload);
      case ZoomStepCommand.typeName:
        return ZoomStepCommand.fromPayload(payload);
      case AlwaysOnTopCommand.typeName:
        return AlwaysOnTopCommand.fromPayload(payload);
      case ClientReadyCommand.typeName:
        return ClientReadyCommand.fromPayload(payload);
      case UpdateGeneralSettingsCommand.typeName:
        return UpdateGeneralSettingsCommand.fromPayload(payload);
      case ActivateSkinCommand.typeName:
        return ActivateSkinCommand.fromPayload(payload);
      case InstallSkinPathCommand.typeName:
        return InstallSkinPathCommand.fromPayload(payload);
      case SetSkinsDirectoryCommand.typeName:
        return SetSkinsDirectoryCommand.fromPayload(payload);
      case ResetSettingsCommand.typeName:
        return const ResetSettingsCommand();
      default:
        throw FormatException('unknown SessionCommand type: $type');
    }
  }

  Map<String, dynamic> toEnvelope() => {
        'type': type,
        'payload': toJson(),
      };

  static Map<String, dynamic> decodeEnvelope(Object? raw) {
    if (raw is String) {
      final decoded = jsonDecode(raw);
      if (decoded is Map<String, dynamic>) return decoded;
      if (decoded is Map) return Map<String, dynamic>.from(decoded);
    }
    if (raw is Map<String, dynamic>) return raw;
    if (raw is Map) return Map<String, dynamic>.from(raw);
    throw FormatException('invalid SessionCommand envelope');
  }
}

final class TransportCommand extends SessionCommand {
  const TransportCommand(this.action);

  static const typeName = 'transport';

  /// One of: playPause, stop, next, previous.
  final String action;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'action': action};

  factory TransportCommand.fromPayload(Map<String, dynamic> json) {
    final action = json['action'];
    if (action is! String || action.isEmpty) {
      throw const FormatException('TransportCommand.action');
    }
    return TransportCommand(action);
  }
}

final class SeekCommand extends SessionCommand {
  const SeekCommand(this.positionMs);

  static const typeName = 'seek';

  final int positionMs;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'positionMs': positionMs};

  factory SeekCommand.fromPayload(Map<String, dynamic> json) {
    final ms = json['positionMs'];
    if (ms is! num) throw const FormatException('SeekCommand.positionMs');
    return SeekCommand(ms.toInt());
  }
}

final class VolumeCommand extends SessionCommand {
  const VolumeCommand(this.volume);

  static const typeName = 'volume';

  final double volume;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'volume': volume};

  factory VolumeCommand.fromPayload(Map<String, dynamic> json) {
    final volume = json['volume'];
    if (volume is! num) throw const FormatException('VolumeCommand.volume');
    return VolumeCommand(volume.toDouble());
  }
}

final class MonoCommand extends SessionCommand {
  const MonoCommand(this.enabled);

  static const typeName = 'mono';

  final bool enabled;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'enabled': enabled};

  factory MonoCommand.fromPayload(Map<String, dynamic> json) =>
      MonoCommand(json['enabled'] == true);
}

final class ToggleWindowCommand extends SessionCommand {
  const ToggleWindowCommand({required this.window, required this.visible});

  static const typeName = 'toggle_window';

  final WindowId window;
  final bool visible;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'window': window.name,
        'visible': visible,
      };

  factory ToggleWindowCommand.fromPayload(Map<String, dynamic> json) {
    final name = json['window'];
    final window = WindowId.values.asNameMap()[name];
    if (window == null) {
      throw FormatException('ToggleWindowCommand.window: $name');
    }
    return ToggleWindowCommand(
      window: window,
      visible: json['visible'] == true,
    );
  }
}

/// SetEqGain — band index `0..9` (see [EqualizerSettings.bandFrequencies]).
final class EqGainCommand extends SessionCommand {
  const EqGainCommand({required this.band, required this.gain});

  static const typeName = 'eq_gain';

  final int band;
  final double gain;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'band': band, 'gain': gain};

  factory EqGainCommand.fromPayload(Map<String, dynamic> json) {
    final band = json['band'];
    final gain = json['gain'];
    if (band is! num || gain is! num) {
      throw const FormatException('EqGainCommand');
    }
    return EqGainCommand(band: band.toInt(), gain: gain.toDouble());
  }
}

/// SetPreamp — ±[EqualizerSettings.gainLimit] dB.
final class EqPreampCommand extends SessionCommand {
  const EqPreampCommand(this.preamp);

  static const typeName = 'eq_preamp';

  final double preamp;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'preamp': preamp};

  factory EqPreampCommand.fromPayload(Map<String, dynamic> json) {
    final preamp = json['preamp'];
    if (preamp is! num) throw const FormatException('EqPreampCommand.preamp');
    return EqPreampCommand(preamp.toDouble());
  }
}

/// SetEqEnabled.
final class EqEnabledCommand extends SessionCommand {
  const EqEnabledCommand(this.enabled);

  static const typeName = 'eq_enabled';

  final bool enabled;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'enabled': enabled};

  factory EqEnabledCommand.fromPayload(Map<String, dynamic> json) =>
      EqEnabledCommand(json['enabled'] == true);
}

/// SetEqAuto.
final class EqAutoCommand extends SessionCommand {
  const EqAutoCommand(this.enabled);

  static const typeName = 'eq_auto';

  final bool enabled;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'enabled': enabled};

  factory EqAutoCommand.fromPayload(Map<String, dynamic> json) =>
      EqAutoCommand(json['enabled'] == true);
}

/// ApplyPreset — name from [EqualizerPresets.builtIn].
final class ApplyPresetCommand extends SessionCommand {
  const ApplyPresetCommand(this.name);

  static const typeName = 'apply_preset';

  final String name;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'name': name};

  factory ApplyPresetCommand.fromPayload(Map<String, dynamic> json) {
    final name = json['name'];
    if (name is! String || name.isEmpty) {
      throw const FormatException('ApplyPresetCommand.name');
    }
    return ApplyPresetCommand(name);
  }
}

/// Windowshade collapse / expand for EQ or playlist.
final class SetShadedCommand extends SessionCommand {
  const SetShadedCommand({required this.window, required this.shaded});

  static const typeName = 'set_shaded';

  final WindowId window;
  final bool shaded;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'window': window.name,
        'shaded': shaded,
      };

  factory SetShadedCommand.fromPayload(Map<String, dynamic> json) {
    final name = json['window'];
    final window = WindowId.values.asNameMap()[name];
    if (window == null || window == WindowId.main) {
      throw FormatException('SetShadedCommand.window: $name');
    }
    return SetShadedCommand(
      window: window,
      shaded: json['shaded'] == true,
    );
  }
}

/// Partial update for general preferences (null fields leave host unchanged).
final class UpdateGeneralSettingsCommand extends SessionCommand {
  const UpdateGeneralSettingsCommand({
    this.resumeLastSession,
    this.confirmBeforeQuit,
    this.scrollTitle,
    this.minimizeHidesSecondaries,
    this.dockSnapStrength,
  });

  static const typeName = 'update_general_settings';

  final bool? resumeLastSession;
  final bool? confirmBeforeQuit;
  final bool? scrollTitle;
  final bool? minimizeHidesSecondaries;
  final DockSnapStrength? dockSnapStrength;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        if (resumeLastSession != null) 'resumeLastSession': resumeLastSession,
        if (confirmBeforeQuit != null) 'confirmBeforeQuit': confirmBeforeQuit,
        if (scrollTitle != null) 'scrollTitle': scrollTitle,
        if (minimizeHidesSecondaries != null)
          'minimizeHidesSecondaries': minimizeHidesSecondaries,
        if (dockSnapStrength != null)
          'dockSnapStrength': dockSnapStrength!.name,
      };

  factory UpdateGeneralSettingsCommand.fromPayload(Map<String, dynamic> json) {
    return UpdateGeneralSettingsCommand(
      resumeLastSession: json.containsKey('resumeLastSession')
          ? json['resumeLastSession'] == true
          : null,
      confirmBeforeQuit: json.containsKey('confirmBeforeQuit')
          ? json['confirmBeforeQuit'] == true
          : null,
      scrollTitle: json.containsKey('scrollTitle')
          ? json['scrollTitle'] == true
          : null,
      minimizeHidesSecondaries: json.containsKey('minimizeHidesSecondaries')
          ? json['minimizeHidesSecondaries'] == true
          : null,
      dockSnapStrength: DockSnapStrength.tryParse(json['dockSnapStrength']),
    );
  }
}

final class ActivateSkinCommand extends SessionCommand {
  const ActivateSkinCommand(this.id);

  static const typeName = 'activate_skin';

  final String id;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'id': id};

  factory ActivateSkinCommand.fromPayload(Map<String, dynamic> json) {
    final id = json['id'];
    if (id is! String || id.isEmpty) {
      throw const FormatException('ActivateSkinCommand.id');
    }
    return ActivateSkinCommand(id);
  }
}

final class InstallSkinPathCommand extends SessionCommand {
  const InstallSkinPathCommand({required this.path, required this.isDirectory});

  static const typeName = 'install_skin_path';

  final String path;
  final bool isDirectory;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'path': path,
        'isDirectory': isDirectory,
      };

  factory InstallSkinPathCommand.fromPayload(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('InstallSkinPathCommand.path');
    }
    return InstallSkinPathCommand(
      path: path,
      isDirectory: json['isDirectory'] == true,
    );
  }
}

/// Set skins catalog directory; null path resets to the default support dir.
final class SetSkinsDirectoryCommand extends SessionCommand {
  const SetSkinsDirectoryCommand(this.path);

  static const typeName = 'set_skins_directory';

  final String? path;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'path': path};

  factory SetSkinsDirectoryCommand.fromPayload(Map<String, dynamic> json) {
    final path = json['path'];
    if (path == null) return const SetSkinsDirectoryCommand(null);
    if (path is! String) {
      throw const FormatException('SetSkinsDirectoryCommand.path');
    }
    return SetSkinsDirectoryCommand(path.isEmpty ? null : path);
  }
}

/// Reset all TrampSettings to defaults and re-bootstrap looks/skins.
final class ResetSettingsCommand extends SessionCommand {
  const ResetSettingsCommand();

  static const typeName = 'reset_settings';

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => const {};
}

final class PlaylistOpCommand extends SessionCommand {
  const PlaylistOpCommand(
    this.op, {
    this.index,
    this.path,
    this.paths,
    this.sortKey,
  });

  static const typeName = 'playlist_op';

  /// Ops: playIndex, select, removeSelected, clear, selectAll, invertSelection,
  /// addPaths, openPlaylist, savePlaylist, sort, reverse.
  final String op;
  final int? index;
  final String? path;
  final List<String>? paths;
  final String? sortKey;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'op': op,
        'index': index,
        'path': path,
        'paths': paths,
        'sortKey': sortKey,
      };

  factory PlaylistOpCommand.fromPayload(Map<String, dynamic> json) {
    final op = json['op'];
    if (op is! String || op.isEmpty) {
      throw const FormatException('PlaylistOpCommand.op');
    }
    final rawPaths = json['paths'];
    List<String>? paths;
    if (rawPaths is List) {
      paths = [
        for (final p in rawPaths)
          if (p is String && p.isNotEmpty) p,
      ];
    }
    return PlaylistOpCommand(
      op,
      index: (json['index'] as num?)?.toInt(),
      path: json['path'] as String?,
      paths: paths,
      sortKey: json['sortKey'] as String?,
    );
  }
}

/// User resized the playlist OS window — logical size before zoom.
final class ResizePlaylistCommand extends SessionCommand {
  const ResizePlaylistCommand({required this.width, required this.height});

  static const typeName = 'resize_playlist';

  final double width;
  final double height;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'width': width, 'height': height};

  factory ResizePlaylistCommand.fromPayload(Map<String, dynamic> json) {
    final width = json['width'];
    final height = json['height'];
    if (width is! num || height is! num) {
      throw const FormatException('ResizePlaylistCommand');
    }
    return ResizePlaylistCommand(
      width: width.toDouble(),
      height: height.toDouble(),
    );
  }
}

/// Listener moved the Playlist Manager divider or collapsed the collection
/// panel — width is logical, so global zoom does not distort it.
final class ResizePlaylistCollectionCommand extends SessionCommand {
  const ResizePlaylistCollectionCommand({
    required this.width,
    required this.collapsed,
  });

  static const typeName = 'resize_playlist_collection';

  final double width;
  final bool collapsed;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'width': width,
        'collapsed': collapsed,
      };

  factory ResizePlaylistCollectionCommand.fromPayload(
    Map<String, dynamic> json,
  ) {
    final width = json['width'];
    if (width is! num) {
      throw const FormatException('ResizePlaylistCollectionCommand.width');
    }
    return ResizePlaylistCollectionCommand(
      width: width.toDouble(),
      collapsed: json['collapsed'] == true,
    );
  }
}

/// Keep a playlist file the listener already has: adds a **reference** to it at
/// the path given. A path already in the collection selects its existing entry.
final class AddSavedPlaylistCommand extends SessionCommand {
  const AddSavedPlaylistCommand(this.path);

  static const typeName = 'add_saved_playlist';

  final String path;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'path': path};

  factory AddSavedPlaylistCommand.fromPayload(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('AddSavedPlaylistCommand.path');
    }
    return AddSavedPlaylistCommand(path);
  }
}

/// Drop a saved playlist from the collection. The file on disk is never touched.
final class RemoveSavedPlaylistCommand extends SessionCommand {
  const RemoveSavedPlaylistCommand(this.path);

  static const typeName = 'remove_saved_playlist';

  final String path;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'path': path};

  factory RemoveSavedPlaylistCommand.fromPayload(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('RemoveSavedPlaylistCommand.path');
    }
    return RemoveSavedPlaylistCommand(path);
  }
}

/// Highlight a saved playlist without loading it.
///
/// What the panel sends for a **disabled playlist**: the listener can still
/// reach the row — so the panel's remove control can act on it — while the load
/// that would fail never starts.
final class SelectSavedPlaylistCommand extends SessionCommand {
  const SelectSavedPlaylistCommand(this.path);

  static const typeName = 'select_saved_playlist';

  final String path;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'path': path};

  factory SelectSavedPlaylistCommand.fromPayload(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('SelectSavedPlaylistCommand.path');
    }
    return SelectSavedPlaylistCommand(path);
  }
}

/// Load a saved playlist's tracks into the current playlist.
final class LoadSavedPlaylistCommand extends SessionCommand {
  const LoadSavedPlaylistCommand(this.path);

  static const typeName = 'load_saved_playlist';

  final String path;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'path': path};

  factory LoadSavedPlaylistCommand.fromPayload(Map<String, dynamic> json) {
    final path = json['path'];
    if (path is! String || path.isEmpty) {
      throw const FormatException('LoadSavedPlaylistCommand.path');
    }
    return LoadSavedPlaylistCommand(path);
  }
}

/// Title-bar drag update — logical top-left for [DockingCoordinator.move].
///
/// [ended] is true on pan-end so the host can persist layout once per gesture.
/// [softEnd] is a quiet-timeout end (Linux never emits `onWindowMoved`); the
/// host still snaps/persists but must not fight the OS-owned HWND.
final class MoveWindowCommand extends SessionCommand {
  const MoveWindowCommand({
    required this.window,
    required this.left,
    required this.top,
    required this.shiftUndock,
    this.ended = false,
    this.softEnd = false,
  });

  static const typeName = 'move_window';

  final WindowId window;
  final double left;
  final double top;
  final bool shiftUndock;
  final bool ended;
  final bool softEnd;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'window': window.name,
        'left': left,
        'top': top,
        'shiftUndock': shiftUndock,
        'ended': ended,
        'softEnd': softEnd,
      };

  factory MoveWindowCommand.fromPayload(Map<String, dynamic> json) {
    final name = json['window'];
    final window = WindowId.values.asNameMap()[name];
    final left = json['left'];
    final top = json['top'];
    if (window == null || left is! num || top is! num) {
      throw FormatException('MoveWindowCommand: $json');
    }
    return MoveWindowCommand(
      window: window,
      left: left.toDouble(),
      top: top.toDouble(),
      shiftUndock: json['shiftUndock'] == true,
      ended: json['ended'] == true,
      softEnd: json['softEnd'] == true,
    );
  }
}

final class ZoomStepCommand extends SessionCommand {
  const ZoomStepCommand(this.delta);

  static const typeName = 'zoom_step';

  /// +1 / −1 step through discrete zoom percents.
  final int delta;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'delta': delta};

  factory ZoomStepCommand.fromPayload(Map<String, dynamic> json) {
    final delta = json['delta'];
    if (delta is! num) throw const FormatException('ZoomStepCommand.delta');
    return ZoomStepCommand(delta.toInt());
  }
}

final class AlwaysOnTopCommand extends SessionCommand {
  const AlwaysOnTopCommand(this.enabled);

  static const typeName = 'always_on_top';

  final bool enabled;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'enabled': enabled};

  factory AlwaysOnTopCommand.fromPayload(Map<String, dynamic> json) =>
      AlwaysOnTopCommand(json['enabled'] == true);
}

/// Secondary engine finished registering window handlers.
final class ClientReadyCommand extends SessionCommand {
  const ClientReadyCommand(this.role);

  static const typeName = 'client_ready';

  final WindowRole role;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {'role': role.name};

  factory ClientReadyCommand.fromPayload(Map<String, dynamic> json) {
    final role = parseWindowRole(
      jsonEncode(<String, dynamic>{'role': json['role']}),
    );
    if (role == WindowRole.main) {
      throw const FormatException('ClientReadyCommand.role');
    }
    return ClientReadyCommand(role);
  }
}

Map<String, dynamic> _payload(Map<String, dynamic> json) {
  final payload = json['payload'];
  if (payload == null) return const {};
  if (payload is Map<String, dynamic>) return payload;
  if (payload is Map) return Map<String, dynamic>.from(payload);
  throw const FormatException('envelope payload must be a map');
}
