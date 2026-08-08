import 'dart:convert';

import '../../domain/equalizer_settings.dart';
import '../../domain/track.dart';
import '../../domain/tramp_settings.dart';

/// Which Flutter engine / OS window this entrypoint owns.
enum WindowRole { main, equalizer, playlist }

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
      case DockSnapshotEvent.typeName:
        return DockSnapshotEvent.fromPayload(payload);
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
  });

  static const typeName = 'playlist_snapshot';

  final List<Track> tracks;
  final List<int> selectedIndices;
  final int? selectedIndex;
  final String? sourcePath;
  final int? playingIndex;
  final bool playing;

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
    );
  }
}

final class DockSnapshotEvent extends SessionEvent {
  const DockSnapshotEvent({
    required this.main,
    required this.equalizer,
    required this.playlist,
    required this.dockEdges,
    required this.zoomPercent,
  });

  static const typeName = 'dock_snapshot';

  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final List<DockEdge> dockEdges;
  final int zoomPercent;

  @override
  String get type => typeName;

  @override
  Map<String, dynamic> toJson() => {
        'main': main.toJson(),
        'equalizer': equalizer.toJson(),
        'playlist': playlist.toJson(),
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
      dockEdges: [
        for (final edge in (json['dockEdges'] as List? ?? const []))
          DockEdge.fromJson(Map<String, dynamic>.from(edge as Map)),
      ],
      zoomPercent: (json['zoomPercent'] as num?)?.toInt() ?? 100,
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
      case ZoomStepCommand.typeName:
        return ZoomStepCommand.fromPayload(payload);
      case AlwaysOnTopCommand.typeName:
        return AlwaysOnTopCommand.fromPayload(payload);
      case ClientReadyCommand.typeName:
        return ClientReadyCommand.fromPayload(payload);
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
