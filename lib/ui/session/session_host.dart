import 'dart:async';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/tramp_settings.dart';
import '../../platform/settings_store.dart';
import '../../theme/mockup_tokens.dart';
import '../docking/dock_layout.dart';
import '../docking/docking_coordinator.dart';
import 'always_on_top.dart';
import 'session_bus.dart';
import 'session_messages.dart';

/// Main-engine session owner: controllers/settings, docking frames, EQ/PL windows.
class SessionHostApp extends StatefulWidget {
  const SessionHostApp({
    super.key,
    this.launchArgs = const [],
    this.settingsStore,
  });

  final List<String> launchArgs;
  final SettingsStore? settingsStore;

  @override
  State<SessionHostApp> createState() => _SessionHostAppState();
}

class _SessionHostAppState extends State<SessionHostApp> with WindowListener {
  late final SettingsStore _settingsStore;
  late final SessionBus _bus;
  late DockingCoordinator _docking;
  int _zoomPercent = TrampSettings.defaults.zoomPercent;
  bool _alwaysOnTop = TrampSettings.defaults.alwaysOnTop;

  WindowController? _equalizerWindow;
  WindowController? _playlistWindow;
  bool _eqReady = false;
  bool _playlistReady = false;
  bool _bootstrapped = false;

  @override
  void initState() {
    super.initState();
    _settingsStore = widget.settingsStore ??
        FileSettingsStore(supportDir: getApplicationSupportDirectory);
    _bus = SessionBus();
    _docking = DockingCoordinator(DockLayout.defaults);
    windowManager.addListener(this);
    unawaited(_bootstrap());
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
    _docking = DockingCoordinator(DockLayout.fromSettings(settings));

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
      case AlwaysOnTopCommand(:final enabled):
        _alwaysOnTop = enabled;
        await _applyAlwaysOnTop();
        await _persistLayout();
      case ZoomStepCommand(:final delta):
        final steps = TrampSettings.validZoomPercents;
        final index = steps.indexOf(_zoomPercent);
        final nextIndex = (index < 0 ? 0 : index) + delta;
        if (nextIndex < 0 || nextIndex >= steps.length) return;
        _zoomPercent = steps[nextIndex];
        await _applyAllFrames();
        await _persistLayout();
        await _broadcast(
          ZoomChangedEvent(_zoomPercent),
        );
        await _broadcastDockSnapshot();
      case TransportCommand():
      case SeekCommand():
      case VolumeCommand():
      case MonoCommand():
      case EqGainCommand():
      case PlaylistOpCommand():
        // Controllers wire in later tasks; accept commands so clients can send.
        break;
    }
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
    try {
      await SessionBus.pushFrame(
        controller,
        left: rect.left,
        top: rect.top,
        width: rect.width,
        height: rect.height,
        visible: visible,
        alwaysOnTop: effectiveAlwaysOnTop(
          alwaysOnTop: _alwaysOnTop,
          visible: visible,
        ),
      );
    } catch (error, stack) {
      // Client may be restarting; ready handshake will retry.
      debugPrint('SessionHost pushFrame($role) failed: $error\n$stack');
    }
  }

  Future<void> _persistLayout() async {
    final current = await _settingsStore.read();
    final layout = _docking.layout;
    await _settingsStore.write(
      current.copyWith(
        zoomPercent: _zoomPercent,
        alwaysOnTop: _alwaysOnTop,
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
    unawaited(_bus.unbind());
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      home: ColoredBox(
        color: MockupTokens.shell,
        child: Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Text(
                'Tramp — Main',
                style: TextStyle(
                  color: MockupTokens.phos,
                  fontSize: 24,
                  fontWeight: FontWeight.w700,
                ),
              ),
              const SizedBox(height: 8),
              Text(
                _bootstrapped
                    ? 'session host · zoom $_zoomPercent%'
                    : 'starting session…',
                style: const TextStyle(
                  color: MockupTokens.inkDim,
                  fontSize: 12,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
