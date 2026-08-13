import 'equalizer_settings.dart';
import '../look/look_id.dart';

/// Default Y for EQ stacked under main (classic×3 main height = 348).
const double _defaultStackTop = 348;

/// Default Y for playlist stacked under EQ (main + EQ = 696).
const double _defaultPlaylistTop = _defaultStackTop + 348;

/// Identifies one of the product windows in dock / settings graphs.
enum WindowId {
  main,
  equalizer,
  playlist,
  settings,
  about;

  /// Freestanding windows move alone and never snap / never are snap targets.
  bool get freestanding => this == settings || this == about;
}

/// Which edge of [DockEdge.a] touches [DockEdge.b].
enum DockSide { left, right, top, bottom }

/// Snap distance strength for docking (logical px).
enum DockSnapStrength {
  off,
  normal,
  strong;

  /// Mapped snap threshold in logical pixels.
  double get snapPixels => switch (this) {
        DockSnapStrength.off => 0,
        DockSnapStrength.normal => 20,
        DockSnapStrength.strong => 40,
      };

  static DockSnapStrength? tryParse(Object? raw) {
    if (raw is! String) return null;
    for (final value in DockSnapStrength.values) {
      if (value.name == raw) return value;
    }
    return null;
  }
}

/// Persisted frame for one of the product windows.
class WindowFrameState {
  const WindowFrameState({
    required this.visible,
    required this.shaded,
    required this.left,
    required this.top,
    this.width,
    this.height,
  });

  static const mainDefault = WindowFrameState(
    visible: true,
    shaded: false,
    left: 0,
    top: 0,
  );

  static const equalizerDefault = WindowFrameState(
    visible: true,
    shaded: false,
    left: 0,
    top: _defaultStackTop,
  );

  static const playlistDefault = WindowFrameState(
    visible: true,
    shaded: false,
    left: 0,
    top: _defaultPlaylistTop,
  );

  static const settingsDefault = WindowFrameState(
    visible: false,
    shaded: false,
    left: 860,
    top: 40,
  );

  static const aboutDefault = WindowFrameState(
    visible: false,
    shaded: false,
    left: 860,
    top: 480,
  );

  final bool visible;
  final bool shaded;
  final double left;
  final double top;

  /// Playlist only; null → default playlist canvas size.
  final double? width;
  final double? height;

  WindowFrameState copyWith({
    bool? visible,
    bool? shaded,
    double? left,
    double? top,
    double? width,
    double? height,
    bool clearWidth = false,
    bool clearHeight = false,
  }) {
    return WindowFrameState(
      visible: visible ?? this.visible,
      shaded: shaded ?? this.shaded,
      left: left ?? this.left,
      top: top ?? this.top,
      width: clearWidth ? null : (width ?? this.width),
      height: clearHeight ? null : (height ?? this.height),
    );
  }

  Map<String, dynamic> toJson() {
    final json = <String, dynamic>{
      'visible': visible,
      'shaded': shaded,
      'left': left,
      'top': top,
    };
    if (width != null) json['width'] = width;
    if (height != null) json['height'] = height;
    return json;
  }

  factory WindowFrameState.fromJson(
    Map<String, dynamic> json, {
    required WindowFrameState fallback,
  }) {
    return WindowFrameState(
      visible: json['visible'] is bool ? json['visible'] as bool : fallback.visible,
      shaded: json['shaded'] is bool ? json['shaded'] as bool : fallback.shaded,
      left: _finiteDouble(json['left']) ?? fallback.left,
      top: _finiteDouble(json['top']) ?? fallback.top,
      width: _positiveDouble(json['width']),
      height: _positiveDouble(json['height']),
    );
  }

  @override
  bool operator ==(Object other) =>
      other is WindowFrameState &&
      other.visible == visible &&
      other.shaded == shaded &&
      other.left == left &&
      other.top == top &&
      other.width == width &&
      other.height == height;

  @override
  int get hashCode => Object.hash(visible, shaded, left, top, width, height);

  @override
  String toString() =>
      'WindowFrameState(visible: $visible, shaded: $shaded, left: $left, '
      'top: $top, width: $width, height: $height)';
}

/// A sticky dock link between two windows.
class DockEdge {
  const DockEdge({
    required this.a,
    required this.b,
    required this.side,
  });

  final WindowId a;
  final WindowId b;

  /// Edge of [a] that touches [b].
  final DockSide side;

  Map<String, dynamic> toJson() => {
        'a': a.name,
        'b': b.name,
        'side': side.name,
      };

  factory DockEdge.fromJson(Map<String, dynamic> json) {
    final a = _windowId(json['a']);
    final b = _windowId(json['b']);
    final side = _dockSide(json['side']);
    if (a == null || b == null || side == null) {
      throw const FormatException('invalid DockEdge');
    }
    return DockEdge(a: a, b: b, side: side);
  }

  @override
  bool operator ==(Object other) =>
      other is DockEdge && other.a == a && other.b == b && other.side == side;

  @override
  int get hashCode => Object.hash(a, b, side);

  @override
  String toString() => 'DockEdge(a: $a, b: $b, side: $side)';
}

/// Persisted UI state for multi-window layout and global chrome flags.
class TrampSettings {
  const TrampSettings({
    required this.zoomPercent,
    required this.alwaysOnTop,
    required this.forceMono,
    required this.main,
    required this.equalizer,
    required this.playlist,
    required this.settings,
    required this.about,
    required this.dockEdges,
    this.equalizerCurve = EqualizerSettings.flat,
    this.activeSkinId = 'builtin',
    this.skinsDirectory,
    this.resumeLastSession = true,
    this.confirmBeforeQuit = false,
    this.scrollTitle = true,
    this.minimizeHidesSecondaries = true,
    this.dockSnapStrength = DockSnapStrength.normal,
    this.playlistCollectionWidth = defaultPlaylistCollectionWidth,
    this.playlistCollectionCollapsed = false,
  });

  /// Width the playlist collection panel opens at when nothing is persisted.
  ///
  /// Lives here rather than with the panel geometry in `TrampMetrics` because
  /// it is the default of a persisted preference, and settings must not depend
  /// on the theme layer.
  static const defaultPlaylistCollectionWidth = 240.0;

  static const defaults = TrampSettings(
    zoomPercent: 75,
    alwaysOnTop: false,
    forceMono: false,
    main: WindowFrameState.mainDefault,
    equalizer: WindowFrameState.equalizerDefault,
    playlist: WindowFrameState.playlistDefault,
    settings: WindowFrameState.settingsDefault,
    about: WindowFrameState.aboutDefault,
    dockEdges: [],
  );

  static const validZoomPercents = <int>[
    50,
    75,
    100,
    125,
    150,
    200,
    250,
    300,
  ];

  final int zoomPercent;
  final bool alwaysOnTop;
  final bool forceMono;
  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final WindowFrameState settings;
  final WindowFrameState about;
  final List<DockEdge> dockEdges;
  final EqualizerSettings equalizerCurve;
  final String activeSkinId;
  final String? skinsDirectory;
  final bool resumeLastSession;
  final bool confirmBeforeQuit;
  final bool scrollTitle;
  final bool minimizeHidesSecondaries;
  final DockSnapStrength dockSnapStrength;

  /// Playlist Manager collection panel width, in logical pixels so global zoom
  /// scales it like the rest of the canvas.
  final double playlistCollectionWidth;
  final bool playlistCollectionCollapsed;

  TrampSettings copyWith({
    int? zoomPercent,
    bool? alwaysOnTop,
    bool? forceMono,
    WindowFrameState? main,
    WindowFrameState? equalizer,
    WindowFrameState? playlist,
    WindowFrameState? settings,
    WindowFrameState? about,
    List<DockEdge>? dockEdges,
    EqualizerSettings? equalizerCurve,
    String? activeSkinId,
    String? skinsDirectory,
    bool clearSkinsDirectory = false,
    bool? resumeLastSession,
    bool? confirmBeforeQuit,
    bool? scrollTitle,
    bool? minimizeHidesSecondaries,
    DockSnapStrength? dockSnapStrength,
    double? playlistCollectionWidth,
    bool? playlistCollectionCollapsed,
  }) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      alwaysOnTop: alwaysOnTop ?? this.alwaysOnTop,
      forceMono: forceMono ?? this.forceMono,
      main: main ?? this.main,
      equalizer: equalizer ?? this.equalizer,
      playlist: playlist ?? this.playlist,
      settings: settings ?? this.settings,
      about: about ?? this.about,
      dockEdges: dockEdges ?? this.dockEdges,
      equalizerCurve: equalizerCurve ?? this.equalizerCurve,
      activeSkinId: activeSkinId ?? this.activeSkinId,
      skinsDirectory:
          clearSkinsDirectory ? null : (skinsDirectory ?? this.skinsDirectory),
      resumeLastSession: resumeLastSession ?? this.resumeLastSession,
      confirmBeforeQuit: confirmBeforeQuit ?? this.confirmBeforeQuit,
      scrollTitle: scrollTitle ?? this.scrollTitle,
      minimizeHidesSecondaries:
          minimizeHidesSecondaries ?? this.minimizeHidesSecondaries,
      dockSnapStrength: dockSnapStrength ?? this.dockSnapStrength,
      playlistCollectionWidth:
          playlistCollectionWidth ?? this.playlistCollectionWidth,
      playlistCollectionCollapsed:
          playlistCollectionCollapsed ?? this.playlistCollectionCollapsed,
    );
  }

  Map<String, dynamic> toJson() => {
        'zoomPercent': zoomPercent,
        'alwaysOnTop': alwaysOnTop,
        'forceMono': forceMono,
        'main': main.toJson(),
        'equalizer': equalizer.toJson(),
        'playlist': playlist.toJson(),
        'settings': settings.toJson(),
        'about': about.toJson(),
        'dockEdges': dockEdges.map((e) => e.toJson()).toList(),
        'equalizerCurve': equalizerCurve.toJson(),
        'activeSkinId': activeSkinId,
        if (skinsDirectory != null) 'skinsDirectory': skinsDirectory,
        'resumeLastSession': resumeLastSession,
        'confirmBeforeQuit': confirmBeforeQuit,
        'scrollTitle': scrollTitle,
        'minimizeHidesSecondaries': minimizeHidesSecondaries,
        'dockSnapStrength': dockSnapStrength.name,
        'playlistCollectionWidth': playlistCollectionWidth,
        'playlistCollectionCollapsed': playlistCollectionCollapsed,
      };

  factory TrampSettings.fromJson(Map<String, dynamic> json) {
    final zoom = json['zoomPercent'];
    final curve = _parseEqualizerCurve(json);

    var main = WindowFrameState.mainDefault;
    var equalizer = WindowFrameState.equalizerDefault;
    var playlist = WindowFrameState.playlistDefault;
    var settings = WindowFrameState.settingsDefault;
    var about = WindowFrameState.aboutDefault;

    final mainJson = json['main'];
    if (mainJson is Map<String, dynamic>) {
      main = WindowFrameState.fromJson(mainJson, fallback: main);
    }

    final eqFrameJson = json['equalizer'];
    if (eqFrameJson is Map<String, dynamic> && _looksLikeFrame(eqFrameJson)) {
      equalizer = WindowFrameState.fromJson(eqFrameJson, fallback: equalizer);
    }

    final playlistJson = json['playlist'];
    if (playlistJson is Map<String, dynamic>) {
      playlist = WindowFrameState.fromJson(playlistJson, fallback: playlist);
    }

    final settingsJson = json['settings'];
    if (settingsJson is Map<String, dynamic>) {
      settings = WindowFrameState.fromJson(settingsJson, fallback: settings);
    }

    final aboutJson = json['about'];
    if (aboutJson is Map<String, dynamic>) {
      about = WindowFrameState.fromJson(aboutJson, fallback: about);
    }

    // Legacy top-level playlist size → playlist frame.
    final legacyW = _positiveDouble(json['playlistWindowWidth']);
    final legacyH = _positiveDouble(json['playlistWindowHeight']);
    if (legacyW != null || legacyH != null) {
      playlist = playlist.copyWith(
        width: legacyW ?? playlist.width,
        height: legacyH ?? playlist.height,
      );
    }

    // Legacy mutual-exclusion lower region → visibility.
    final region = json['lowerRegion'];
    if (region == 'equalizer') {
      equalizer = equalizer.copyWith(visible: true);
      playlist = playlist.copyWith(visible: false);
    } else if (region == 'playlist') {
      playlist = playlist.copyWith(visible: true);
      equalizer = equalizer.copyWith(visible: false);
    }

    return TrampSettings(
      zoomPercent: zoom is int && validZoomPercents.contains(zoom)
          ? zoom
          : defaults.zoomPercent,
      alwaysOnTop: json['alwaysOnTop'] is bool
          ? json['alwaysOnTop'] as bool
          : defaults.alwaysOnTop,
      forceMono: json['forceMono'] is bool
          ? json['forceMono'] as bool
          : defaults.forceMono,
      main: main,
      equalizer: equalizer,
      playlist: playlist,
      settings: settings,
      about: about,
      dockEdges: _parseDockEdges(json['dockEdges']),
      equalizerCurve: curve,
      activeSkinId: _parseActiveSkinId(
        json['activeSkinId'] ?? json['activeLookId'],
      ),
      skinsDirectory: _parseSkinsDirectory(
        json['skinsDirectory'] ?? json['looksDirectory'],
      ),
      resumeLastSession: json['resumeLastSession'] is bool
          ? json['resumeLastSession'] as bool
          : defaults.resumeLastSession,
      confirmBeforeQuit: json['confirmBeforeQuit'] is bool
          ? json['confirmBeforeQuit'] as bool
          : defaults.confirmBeforeQuit,
      scrollTitle: json['scrollTitle'] is bool
          ? json['scrollTitle'] as bool
          : defaults.scrollTitle,
      minimizeHidesSecondaries: json['minimizeHidesSecondaries'] is bool
          ? json['minimizeHidesSecondaries'] as bool
          : defaults.minimizeHidesSecondaries,
      dockSnapStrength:
          DockSnapStrength.tryParse(json['dockSnapStrength']) ??
              defaults.dockSnapStrength,
      playlistCollectionWidth: _positiveDouble(json['playlistCollectionWidth']) ??
          defaults.playlistCollectionWidth,
      playlistCollectionCollapsed: json['playlistCollectionCollapsed'] is bool
          ? json['playlistCollectionCollapsed'] as bool
          : defaults.playlistCollectionCollapsed,
    );
  }

  static String _parseActiveSkinId(Object? raw) {
    if (raw is! String || raw.isEmpty) return defaults.activeSkinId;
    if (raw == 'builtin' || isValidLookId(raw)) return raw;
    return defaults.activeSkinId;
  }

  static String? _parseSkinsDirectory(Object? raw) {
    if (raw is! String || raw.isEmpty) return null;
    return raw;
  }

  static EqualizerSettings _parseEqualizerCurve(Map<String, dynamic> json) {
    final named = json['equalizerCurve'];
    if (named is Map<String, dynamic>) {
      return EqualizerSettings.fromJson(named);
    }
    // Legacy: curve lived under `equalizer` before that key became the frame.
    final legacy = json['equalizer'];
    if (legacy is Map<String, dynamic> && _looksLikeEqualizerCurve(legacy)) {
      return EqualizerSettings.fromJson(legacy);
    }
    return EqualizerSettings.flat;
  }

  static bool _looksLikeFrame(Map<String, dynamic> json) =>
      json.containsKey('visible') ||
      json.containsKey('shaded') ||
      json.containsKey('left') ||
      json.containsKey('top');

  static bool _looksLikeEqualizerCurve(Map<String, dynamic> json) =>
      json.containsKey('gains') ||
      json.containsKey('enabled') ||
      json.containsKey('preamp');

  static List<DockEdge> _parseDockEdges(Object? raw) {
    if (raw is! List) return const [];
    final edges = <DockEdge>[];
    for (final item in raw) {
      if (item is! Map<String, dynamic>) continue;
      try {
        edges.add(DockEdge.fromJson(item));
      } catch (_) {
        // Skip corrupt edges; never fail startup.
      }
    }
    return List.unmodifiable(edges);
  }

  @override
  bool operator ==(Object other) =>
      other is TrampSettings &&
      other.zoomPercent == zoomPercent &&
      other.alwaysOnTop == alwaysOnTop &&
      other.forceMono == forceMono &&
      other.main == main &&
      other.equalizer == equalizer &&
      other.playlist == playlist &&
      other.settings == settings &&
      other.about == about &&
      _listEquals(other.dockEdges, dockEdges) &&
      other.equalizerCurve == equalizerCurve &&
      other.activeSkinId == activeSkinId &&
      other.skinsDirectory == skinsDirectory &&
      other.resumeLastSession == resumeLastSession &&
      other.confirmBeforeQuit == confirmBeforeQuit &&
      other.scrollTitle == scrollTitle &&
      other.minimizeHidesSecondaries == minimizeHidesSecondaries &&
      other.dockSnapStrength == dockSnapStrength &&
      other.playlistCollectionWidth == playlistCollectionWidth &&
      other.playlistCollectionCollapsed == playlistCollectionCollapsed;

  @override
  int get hashCode => Object.hash(
        zoomPercent,
        alwaysOnTop,
        forceMono,
        main,
        equalizer,
        playlist,
        settings,
        about,
        Object.hashAll(dockEdges),
        equalizerCurve,
        activeSkinId,
        skinsDirectory,
        Object.hash(
          resumeLastSession,
          confirmBeforeQuit,
          scrollTitle,
          minimizeHidesSecondaries,
          dockSnapStrength,
          playlistCollectionWidth,
          playlistCollectionCollapsed,
        ),
      );

  @override
  String toString() =>
      'TrampSettings(zoomPercent: $zoomPercent, alwaysOnTop: $alwaysOnTop, '
      'forceMono: $forceMono, main: $main, equalizer: $equalizer, '
      'playlist: $playlist, settings: $settings, about: $about, '
      'dockEdges: $dockEdges, '
      'equalizerCurve: $equalizerCurve, activeSkinId: $activeSkinId, '
      'skinsDirectory: $skinsDirectory, resumeLastSession: $resumeLastSession, '
      'confirmBeforeQuit: $confirmBeforeQuit, scrollTitle: $scrollTitle, '
      'minimizeHidesSecondaries: $minimizeHidesSecondaries, '
      'dockSnapStrength: $dockSnapStrength, '
      'playlistCollectionWidth: $playlistCollectionWidth, '
      'playlistCollectionCollapsed: $playlistCollectionCollapsed)';
}

WindowId? _windowId(Object? value) {
  if (value is! String) return null;
  for (final id in WindowId.values) {
    if (id.name == value) return id;
  }
  return null;
}

DockSide? _dockSide(Object? value) {
  if (value is! String) return null;
  for (final side in DockSide.values) {
    if (side.name == value) return side;
  }
  return null;
}

double? _finiteDouble(Object? value) {
  final n = switch (value) {
    num v => v.toDouble(),
    _ => null,
  };
  if (n == null || !n.isFinite) return null;
  return n;
}

double? _positiveDouble(Object? value) {
  final n = _finiteDouble(value);
  if (n == null || n <= 0) return null;
  return n;
}

bool _listEquals<T>(List<T> a, List<T> b) {
  if (identical(a, b)) return true;
  if (a.length != b.length) return false;
  for (var i = 0; i < a.length; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}
