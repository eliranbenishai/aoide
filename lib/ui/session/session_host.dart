import 'dart:async';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/tramp_settings.dart';
import '../../eq/equalizer_controller.dart';
import '../../platform/file_open.dart';
import '../../platform/settings_store.dart';
import '../../playback/media_kit_player_engine.dart';
import '../../playback/playback_controller.dart';
import '../../playback/player_engine.dart';
import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_store.dart';
import '../../theme/mockup_tokens.dart';
import '../chrome/about_dialog.dart';
import '../docking/dock_layout.dart';
import '../docking/docking_coordinator.dart';
import '../windows/main_player_window.dart';
import 'always_on_top.dart';
import 'minimize_group.dart';
import 'session_bus.dart';
import 'session_messages.dart';

/// Main-engine session owner: controllers/settings, docking frames, EQ/PL windows.
class SessionHostApp extends StatefulWidget {
  const SessionHostApp({
    super.key,
    this.launchArgs = const [],
    this.settingsStore,
    this.engine,
    this.playlistStore,
  });

  final List<String> launchArgs;
  final SettingsStore? settingsStore;
  final PlayerEngine? engine;
  final PlaylistStore? playlistStore;

  @override
  State<SessionHostApp> createState() => _SessionHostAppState();
}

class _SessionHostAppState extends State<SessionHostApp> with WindowListener {
  late final SettingsStore _settingsStore;
  late final SessionBus _bus;
  late final PlaylistController _playlist;
  late final PlaybackController _playback;
  late final EqualizerController _equalizer;
  late DockingCoordinator _docking;
  int _zoomPercent = TrampSettings.defaults.zoomPercent;
  bool _alwaysOnTop = TrampSettings.defaults.alwaysOnTop;
  bool _forceMono = TrampSettings.defaults.forceMono;

  WindowController? _equalizerWindow;
  WindowController? _playlistWindow;
  bool _eqReady = false;
  bool _playlistReady = false;
  bool _bootstrapped = false;
  final MinimizeGroupCycle _minimizeGroup = MinimizeGroupCycle();

  @override
  void initState() {
    super.initState();
    _settingsStore = widget.settingsStore ??
        FileSettingsStore(supportDir: getApplicationSupportDirectory);
    _bus = SessionBus();
    _docking = DockingCoordinator(DockLayout.defaults);
    _playlist = PlaylistController(
      store: widget.playlistStore ??
          FilePlaylistStore(supportDir: getApplicationSupportDirectory),
    );
    _playback = PlaybackController(
      playlist: _playlist,
      engine: widget.engine ?? MediaKitPlayerEngine(),
    );
    // Audible EQ is Task 11 — sink may still be noop; UI still drives apply.
    _equalizer = EqualizerController(
      store: _settingsStore,
      sink: const NoopEqualizerSink(),
    );
    _playlist.addListener(_onPlaylistChanged);
    windowManager.addListener(this);
    unawaited(_bootstrap());
  }

  void _onPlaylistChanged() {
    if (mounted) setState(() {});
  }

  Future<void> _bootstrap() async {
    await _bus.bindHost(_onCommand);
    // Main close quits the process after tearing down secondary engines.
    await windowManager.setPreventClose(true);
    await windowManager.setAsFrameless();
    await windowManager.setResizable(false);
    await windowManager.setTitle('Tramp — Main');

    final settings = await _settingsStore.read();
    _zoomPercent = settings.zoomPercent;
    _alwaysOnTop = settings.alwaysOnTop;
    _forceMono = settings.forceMono;
    _docking = DockingCoordinator(DockLayout.fromSettings(settings));

    await _equalizer.load();
    await _playlist.restoreLastPlaylist();
    await _ensureSecondaryWindows();
    await _applyAllFrames();
    await _applyAlwaysOnTop();

    if (mounted) {
      setState(() => _bootstrapped = true);
    }
  }

  Future<void> _ensureSecondaryWindows() async {
    _equalizerWindow ??= await WindowController.create(
      WindowConfiguration(
        hiddenAtLaunch: true,
        arguments: encodeWindowArguments(WindowRole.equalizer),
      ),
    );
    _playlistWindow ??= await WindowController.create(
      WindowConfiguration(
        hiddenAtLaunch: true,
        arguments: encodeWindowArguments(WindowRole.playlist),
      ),
    );
  }

  Future<void> _onCommand(SessionCommand command) async {
    switch (command) {
      case ClientReadyCommand(:final role):
        if (role == WindowRole.equalizer) {
          _eqReady = true;
          await _pushEqSnapshot(role);
        } else if (role == WindowRole.playlist) {
          _playlistReady = true;
        }
        await _applyRoleFrame(role);
        await _pushDockSnapshot(role);
      case ToggleWindowCommand(:final window, :final visible):
        if (window == WindowId.main) return;
        _docking.setVisible(window, visible);
        await _persistLayout();
        await _applyRoleFrame(_roleFor(window));
        await _broadcastDockSnapshot();
        if (mounted) setState(() {});
      case SetShadedCommand(:final window, :final shaded):
        _docking.setShaded(window, shaded);
        await _persistLayout();
        await _applyRoleFrame(_roleFor(window));
        await _broadcastDockSnapshot();
      case AlwaysOnTopCommand(:final enabled):
        _alwaysOnTop = enabled;
        await _applyAlwaysOnTop();
        await _persistLayout();
        if (mounted) setState(() {});
      case ZoomStepCommand(:final delta):
        await _stepZoom(delta);
      case MonoCommand(:final enabled):
        _forceMono = enabled;
        await _persistLayout();
        if (mounted) setState(() {});
      case EqGainCommand(:final band, :final gain):
        _equalizer.setGain(band, gain);
        await _broadcastEqSnapshot();
      case EqPreampCommand(:final preamp):
        _equalizer.setPreamp(preamp);
        await _broadcastEqSnapshot();
      case EqEnabledCommand(:final enabled):
        _equalizer.setEnabled(enabled);
        await _broadcastEqSnapshot();
      case EqAutoCommand(:final enabled):
        _equalizer.setAuto(enabled);
        await _broadcastEqSnapshot();
      case ApplyPresetCommand(:final name):
        _equalizer.applyPreset(name);
        await _broadcastEqSnapshot();
      case TransportCommand():
      case SeekCommand():
      case VolumeCommand():
      case PlaylistOpCommand():
        // Controllers wire in later tasks; accept commands so clients can send.
        break;
    }
  }

  Future<void> _pushEqSnapshot(WindowRole role) async {
    final controller = switch (role) {
      WindowRole.equalizer => _equalizerWindow,
      WindowRole.playlist => _playlistWindow,
      WindowRole.main => null,
    };
    if (controller == null) return;
    try {
      await SessionBus.pushEvent(
        controller,
        EqSnapshotEvent(_equalizer.settings),
      );
    } catch (error, stack) {
      debugPrint('SessionHost pushEq($role) failed: $error\n$stack');
    }
  }

  Future<void> _broadcastEqSnapshot() =>
      _broadcast(EqSnapshotEvent(_equalizer.settings));

  Future<void> _stepZoom(int delta) async {
    final steps = TrampSettings.validZoomPercents;
    final index = steps.indexOf(_zoomPercent);
    final nextIndex = (index < 0 ? 0 : index) + delta;
    if (nextIndex < 0 || nextIndex >= steps.length) return;
    _zoomPercent = steps[nextIndex];
    await _applyAllFrames();
    await _persistLayout();
    await _broadcast(ZoomChangedEvent(_zoomPercent));
    await _broadcastDockSnapshot();
    if (mounted) setState(() {});
  }

  WindowRole _roleFor(WindowId id) => switch (id) {
        WindowId.main => WindowRole.main,
        WindowId.equalizer => WindowRole.equalizer,
        WindowId.playlist => WindowRole.playlist,
      };

  Future<void> _applyAllFrames() async {
    await _applyMainFrame();
    if (_eqReady) await _applyRoleFrame(WindowRole.equalizer);
    if (_playlistReady) await _applyRoleFrame(WindowRole.playlist);
  }

  Future<void> _applyMainFrame() async {
    final zoom = _zoomPercent / 100.0;
    final rect = _docking.frameFor(WindowId.main, zoom);
    final visible = _docking.layout.main.visible;
    await windowManager.setMinimumSize(rect.size);
    await windowManager.setSize(rect.size);
    await windowManager.setPosition(rect.topLeft);
    await windowManager.setAlwaysOnTop(
      effectiveAlwaysOnTop(alwaysOnTop: _alwaysOnTop, visible: visible),
    );
    if (visible) {
      await windowManager.show();
      await windowManager.focus();
    } else {
      await windowManager.hide();
    }
  }

  /// Pin every currently visible tramp window when the global flag is on.
  Future<void> _applyAlwaysOnTop() async {
    final layout = _docking.layout;
    final targets = alwaysOnTopTargets(
      alwaysOnTop: _alwaysOnTop,
      mainVisible: layout.main.visible,
      equalizerVisible: layout.equalizer.visible,
      playlistVisible: layout.playlist.visible,
    );
    await windowManager.setAlwaysOnTop(targets.contains(WindowId.main));
    // Secondaries apply AOT via apply_frame (visible ∩ global flag).
    if (_eqReady) await _applyRoleFrame(WindowRole.equalizer);
    if (_playlistReady) await _applyRoleFrame(WindowRole.playlist);
  }

  Future<void> _applyRoleFrame(WindowRole role) async {
    if (role == WindowRole.main) {
      await _applyMainFrame();
      return;
    }
    final controller = role == WindowRole.equalizer
        ? _equalizerWindow
        : _playlistWindow;
    if (controller == null) return;
    if (role == WindowRole.equalizer && !_eqReady) return;
    if (role == WindowRole.playlist && !_playlistReady) return;

    final id =
        role == WindowRole.equalizer ? WindowId.equalizer : WindowId.playlist;
    final zoom = _zoomPercent / 100.0;
    final rect = _docking.frameFor(id, zoom);
    final visible = _docking.layout.frameOf(id).visible;
    final show = visible && !_minimizeGroup.shouldSuppressShow(id);
    try {
      await SessionBus.pushFrame(
        controller,
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        visible: show,
        alwaysOnTop: effectiveAlwaysOnTop(
          alwaysOnTop: _alwaysOnTop,
          visible: show,
        ),
      );
    } catch (error, stack) {
      // Client may be restarting; ready handshake will retry.
      debugPrint('SessionHost pushFrame($role) failed: $error\n$stack');
    }
  }

  /// Main title-bar minimize → hide visible secondaries, then OS-minimize main.
  Future<void> _minimizeVisibleGroup() async {
    await _hideVisibleSecondariesForMinimize();
    await windowManager.minimize();
  }

  /// Snapshot + OS-hide currently visible EQ/PL (layout visibility unchanged).
  Future<void> _hideVisibleSecondariesForMinimize() async {
    final layout = _docking.layout;
    final toHide = _minimizeGroup.begin(
      equalizerVisible: layout.equalizer.visible,
      playlistVisible: layout.playlist.visible,
    );
    await _setSecondariesHidden(toHide, hidden: true);
  }

  /// Main restore → best-effort show secondaries that were visible at minimize.
  Future<void> _restoreVisibleGroup() async {
    final layout = _docking.layout;
    final toShow = _minimizeGroup.end(
      equalizerVisible: layout.equalizer.visible,
      playlistVisible: layout.playlist.visible,
    );
    await _setSecondariesHidden(toShow, hidden: false);
  }

  Future<void> _setSecondariesHidden(
    Set<WindowId> ids, {
    required bool hidden,
  }) async {
    for (final id in ids) {
      final controller = switch (id) {
        WindowId.equalizer => _equalizerWindow,
        WindowId.playlist => _playlistWindow,
        WindowId.main => null,
      };
      if (controller == null) continue;
      try {
        if (hidden) {
          await controller.hide();
        } else {
          await controller.show();
        }
      } catch (error, stack) {
        debugPrint(
          'SessionHost secondary ${hidden ? 'hide' : 'show'}($id) '
          'failed: $error\n$stack',
        );
      }
    }
  }

  Future<void> _persistLayout() async {
    final current = await _settingsStore.read();
    final layout = _docking.layout;
    await _settingsStore.write(
      current.copyWith(
        zoomPercent: _zoomPercent,
        alwaysOnTop: _alwaysOnTop,
        forceMono: _forceMono,
        main: layout.main,
        equalizer: layout.equalizer,
        playlist: layout.playlist,
        dockEdges: layout.dockEdges,
      ),
    );
  }

  DockSnapshotEvent _dockSnapshot() {
    final layout = _docking.layout;
    return DockSnapshotEvent(
      main: layout.main,
      equalizer: layout.equalizer,
      playlist: layout.playlist,
      dockEdges: layout.dockEdges,
      zoomPercent: _zoomPercent,
    );
  }

  Future<void> _pushDockSnapshot(WindowRole role) async {
    final controller = switch (role) {
      WindowRole.equalizer => _equalizerWindow,
      WindowRole.playlist => _playlistWindow,
      WindowRole.main => null,
    };
    if (controller == null) return;
    try {
      await SessionBus.pushEvent(controller, _dockSnapshot());
    } catch (error, stack) {
      debugPrint('SessionHost pushEvent($role) failed: $error\n$stack');
    }
  }

  Future<void> _broadcastDockSnapshot() => _broadcast(_dockSnapshot());

  Future<void> _broadcast(SessionEvent event) async {
    for (final controller in [_equalizerWindow, _playlistWindow]) {
      if (controller == null) continue;
      try {
        await SessionBus.pushEvent(controller, event);
      } catch (error, stack) {
        debugPrint(
          'SessionHost broadcast ${event.type} failed: $error\n$stack',
        );
      }
    }
  }

  Future<void> _handleLocalCommand(SessionCommand command) async {
    await _onCommand(command);
  }

  Future<void> _openFiles() async {
    final paths = await pickAudioFiles();
    if (paths == null || paths.isEmpty) return;
    final tracks = tracksFromPaths(paths);
    if (tracks.isEmpty) return;
    _playlist.addTracks(tracks);
    if (_playback.currentTrack == null) {
      await _playback.playIndex(_playlist.playlist.tracks.length - tracks.length);
    }
  }

  Future<void> _showOptions() async {
    final choice = await showMenu<String>(
      context: context,
      position: const RelativeRect.fromLTRB(40, 80, 0, 0),
      items: [
        PopupMenuItem(
          value: 'aot',
          child: Text(_alwaysOnTop ? 'Always on top ✓' : 'Always on top'),
        ),
        const PopupMenuItem(value: 'about', child: Text('About Tramp')),
        const PopupMenuItem(value: 'quit', child: Text('Quit')),
      ],
    );
    if (!mounted || choice == null) return;
    switch (choice) {
      case 'aot':
        await _handleLocalCommand(AlwaysOnTopCommand(!_alwaysOnTop));
      case 'about':
        await showTrampAboutDialog(context, version: '0.1.0');
      case 'quit':
        await _quit();
    }
  }

  Future<void> _showTrackInfo() async {
    final track = _playback.currentTrack;
    final message = track == null
        ? 'No track loaded.'
        : [
            track.displayTitle,
            if (track.artist != null) 'Artist: ${track.artist}',
            if (track.album != null) 'Album: ${track.album}',
            'Path: ${track.path}',
          ].join('\n');
    await showDialog<void>(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: MockupTokens.shellMid,
        title: const Text('Track info', style: TextStyle(color: MockupTokens.ink)),
        content: Text(message, style: const TextStyle(color: MockupTokens.inkDim)),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('OK'),
          ),
        ],
      ),
    );
  }

  @override
  void onWindowMinimize() {
    // Taskbar / OS minimize: hide visible secondaries (main already minimized).
    if (_minimizeGroup.isActive) return;
    unawaited(_hideVisibleSecondariesForMinimize());
  }

  @override
  void onWindowRestore() {
    unawaited(_restoreVisibleGroup());
  }

  @override
  void onWindowClose() {
    unawaited(_quit());
  }

  Future<void> _quit() async {
    for (final controller in [_equalizerWindow, _playlistWindow]) {
      if (controller == null) continue;
      try {
        await controller.invokeMethod('session_shutdown');
      } catch (_) {
        try {
          await controller.hide();
        } catch (_) {}
      }
    }
    await windowManager.setPreventClose(false);
    await windowManager.destroy();
  }

  @override
  void dispose() {
    windowManager.removeListener(this);
    _playlist.removeListener(_onPlaylistChanged);
    unawaited(_bus.unbind());
    unawaited(_playback.dispose());
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final layout = _docking.layout;
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      home: ColoredBox(
        color: MockupTokens.shellDeep,
        child: !_bootstrapped
            ? const Center(
                child: Text(
                  'starting session…',
                  style: TextStyle(color: MockupTokens.inkDim, fontSize: 12),
                ),
              )
            : Center(
                child: MainPlayerWindow(
                  playback: _playback,
                  trackCount: _playlist.playlist.tracks.length,
                  forceMono: _forceMono,
                  alwaysOnTop: _alwaysOnTop,
                  equalizerVisible: layout.equalizer.visible,
                  playlistVisible: layout.playlist.visible,
                  onSessionCommand: (cmd) => unawaited(_handleLocalCommand(cmd)),
                  onOpenFiles: () => unawaited(_openFiles()),
                  onOpenOptions: () => unawaited(_showOptions()),
                  onShowTrackInfo: () => unawaited(_showTrackInfo()),
                  onMinimize: () => unawaited(_minimizeVisibleGroup()),
                  onZoomOut: () => unawaited(_stepZoom(-1)),
                  onZoomIn: () => unawaited(_stepZoom(1)),
                  onClose: () => unawaited(_quit()),
                ),
              ),
      ),
    );
  }
}
