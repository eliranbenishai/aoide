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
    required this.about,
    required this.dockEdges,
  });

  static const defaults = DockLayout(
    main: WindowFrameState.mainDefault,
    equalizer: WindowFrameState.equalizerDefault,
    playlist: WindowFrameState.playlistDefault,
    settings: WindowFrameState.settingsDefault,
    about: WindowFrameState.aboutDefault,
    dockEdges: [],
  );

  final WindowFrameState main;
  final WindowFrameState equalizer;
  final WindowFrameState playlist;
  final WindowFrameState settings;
  final WindowFrameState about;
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
      case WindowId.about:
        return about;
    }
  }

  DockLayout copyWith({
    WindowFrameState? main,
    WindowFrameState? equalizer,
    WindowFrameState? playlist,
    WindowFrameState? settings,
    WindowFrameState? about,
    List<DockEdge>? dockEdges,
  }) {
    return DockLayout(
      main: main ?? this.main,
      equalizer: equalizer ?? this.equalizer,
      playlist: playlist ?? this.playlist,
      settings: settings ?? this.settings,
      about: about ?? this.about,
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
      case WindowId.about:
        return copyWith(about: frame);
    }
  }

  factory DockLayout.fromSettings(TrampSettings settings) {
    return DockLayout(
      main: settings.main,
      equalizer: settings.equalizer,
      playlist: settings.playlist,
      settings: settings.settings,
      about: settings.about,
      dockEdges: List<DockEdge>.unmodifiable(settings.dockEdges),
    );
  }
}
