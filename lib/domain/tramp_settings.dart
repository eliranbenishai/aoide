import 'equalizer_settings.dart';

/// Default Y for EQ/playlist stacked under main (classic×3 main height = 348).
const double _defaultStackTop = 348;

/// Identifies one of the three product windows in dock / settings graphs.
enum WindowId { main, equalizer, playlist }

/// Which edge of [DockEdge.a] touches [DockEdge.b].
enum DockSide { left, right, top, bottom }

/// Persisted frame for one of the three windows.
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
    visible: false,
    shaded: false,
    left: 0,
    top: _defaultStackTop,
  );

  static const playlistDefault = WindowFrameState(
    visible: true,
    shaded: false,
    left: 0,
    top: _defaultStackTop,
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
    required this.dockEdges,
    this.equalizerCurve = EqualizerSettings.flat,
  });

  static const defaults = TrampSettings(
    zoomPercent: 100,
    alwaysOnTop: false,
    forceMono: false,
    main: WindowFrameState.mainDefault,
    equalizer: WindowFrameState.equalizerDefault,
    playlist: WindowFrameState.playlistDefault,
    dockEdges: [],
  );

  static const validZoomPercents = <int>[100, 125, 150, 200, 250, 300];

  final int zoomPercent;
  final bool alwaysOnTop;
  final bool forceMono;
  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final List<DockEdge> dockEdges;
  final EqualizerSettings equalizerCurve;

  TrampSettings copyWith({
    int? zoomPercent,
    bool? alwaysOnTop,
    bool? forceMono,
    WindowFrameState? main,
    WindowFrameState? equalizer,
    WindowFrameState? playlist,
    List<DockEdge>? dockEdges,
    EqualizerSettings? equalizerCurve,
  }) {
    return TrampSettings(
      zoomPercent: zoomPercent ?? this.zoomPercent,
      alwaysOnTop: alwaysOnTop ?? this.alwaysOnTop,
      forceMono: forceMono ?? this.forceMono,
      main: main ?? this.main,
      equalizer: equalizer ?? this.equalizer,
      playlist: playlist ?? this.playlist,
      dockEdges: dockEdges ?? this.dockEdges,
      equalizerCurve: equalizerCurve ?? this.equalizerCurve,
    );
  }

  Map<String, dynamic> toJson() => {
        'zoomPercent': zoomPercent,
        'alwaysOnTop': alwaysOnTop,
        'forceMono': forceMono,
        'main': main.toJson(),
        'equalizer': equalizer.toJson(),
        'playlist': playlist.toJson(),
        'dockEdges': dockEdges.map((e) => e.toJson()).toList(),
        'equalizerCurve': equalizerCurve.toJson(),
      };

  factory TrampSettings.fromJson(Map<String, dynamic> json) {
    final zoom = json['zoomPercent'];
    final curve = _parseEqualizerCurve(json);

    var main = WindowFrameState.mainDefault;
    var equalizer = WindowFrameState.equalizerDefault;
    var playlist = WindowFrameState.playlistDefault;

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
      dockEdges: _parseDockEdges(json['dockEdges']),
      equalizerCurve: curve,
    );
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
      _listEquals(other.dockEdges, dockEdges) &&
      other.equalizerCurve == equalizerCurve;

  @override
  int get hashCode => Object.hash(
        zoomPercent,
        alwaysOnTop,
        forceMono,
        main,
        equalizer,
        playlist,
        Object.hashAll(dockEdges),
        equalizerCurve,
      );

  @override
  String toString() =>
      'TrampSettings(zoomPercent: $zoomPercent, alwaysOnTop: $alwaysOnTop, '
      'forceMono: $forceMono, main: $main, equalizer: $equalizer, '
      'playlist: $playlist, dockEdges: $dockEdges, '
      'equalizerCurve: $equalizerCurve)';
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
