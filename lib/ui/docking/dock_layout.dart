import '../../domain/tramp_settings.dart';

/// In-memory multi-window layout (positions, shade, visibility, dock graph).
///
/// Aligns with [TrampSettings] frame fields so the coordinator can sync to
/// persistence without duplicating [WindowId] / [DockEdge] types.
class DockLayout {
  const DockLayout({
    required this.main,
    required this.equalizer,
    required this.playlist,
    required this.settings,
    required this.dockEdges,
  });

  static const defaults = DockLayout(
    main: WindowFrameState.mainDefault,
    equalizer: WindowFrameState.equalizerDefault,
    playlist: WindowFrameState.playlistDefault,
    settings: WindowFrameState.settingsDefault,
    dockEdges: [],
  );

  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final WindowFrameState settings;
  final List<DockEdge> dockEdges;

  WindowFrameState frameOf(WindowId id) {
    switch (id) {
      case WindowId.main:
        return main;
      case WindowId.equalizer:
        return equalizer;
      case WindowId.playlist:
        return playlist;
      case WindowId.settings:
        return settings;
    }
  }

  DockLayout copyWith({
    WindowFrameState? main,
    WindowFrameState? equalizer,
    WindowFrameState? playlist,
    WindowFrameState? settings,
    List<DockEdge>? dockEdges,
  }) {
    return DockLayout(
      main: main ?? this.main,
      equalizer: equalizer ?? this.equalizer,
      playlist: playlist ?? this.playlist,
      settings: settings ?? this.settings,
      dockEdges: dockEdges ?? this.dockEdges,
    );
  }

  DockLayout withFrame(WindowId id, WindowFrameState frame) {
    switch (id) {
      case WindowId.main:
        return copyWith(main: frame);
      case WindowId.equalizer:
        return copyWith(equalizer: frame);
      case WindowId.playlist:
        return copyWith(playlist: frame);
      case WindowId.settings:
        return copyWith(settings: frame);
    }
  }

  factory DockLayout.fromSettings(TrampSettings settings) {
    return DockLayout(
      main: settings.main,
      equalizer: settings.equalizer,
      playlist: settings.playlist,
      settings: settings.settings,
      dockEdges: List<DockEdge>.unmodifiable(settings.dockEdges),
    );
  }
}
