import 'dart:async';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:media_kit/media_kit.dart';
import 'package:path_provider/path_provider.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/tramp_settings.dart';
import '../../eq/equalizer_controller.dart';
import '../../eq/mpv_equalizer_sink.dart';
import '../../platform/file_open.dart';
import '../../platform/settings_store.dart';
import '../../playback/media_kit_player_engine.dart';
import '../../playback/playback_controller.dart';
import '../../playback/player_engine.dart';
import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_sort.dart';
import '../../playlist/playlist_store.dart';
import '../../theme/mockup_tokens.dart';
import '../../theme/tramp_metrics.dart';
import '../chrome/about_dialog.dart';
import '../docking/dock_layout.dart';
import '../docking/dock_move_coalescer.dart';
import '../docking/docking_coordinator.dart';
import '../windows/main_player_window.dart';
import '../zoom/zoomed_canvas.dart';
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
  final DockMoveCoalescer _dockMoveCoalescer = DockMoveCoalescer();
  final DockMoveCoalescer _nativeSyncCoalescer = DockMoveCoalescer();
  WindowId? _dockDragWindow;
  bool _nativeDragging = false;
  Timer? _nativeDragEndFallback;
  /// Guards [onWindowFocus] → raise → main [focus] from re-entering.
  bool _raisingFocusGroup = false;

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
    // Share one media_kit Player so EQ `af` and transport hit the same libmpv.
    final Player? sharedPlayer =
        widget.engine == null ? Player() : null;
    _playback = PlaybackController(
      playlist: _playlist,
      engine: widget.engine ??
          MediaKitPlayerEngine(player: sharedPlayer),
    );
    _equalizer = EqualizerController(
      store: _settingsStore,
      sink: sharedPlayer != null
          ? MpvEqualizerSink(sharedPlayer)
          : const NoopEqualizerSink(),
    );
    _playlist.addListener(_onPlaylistChanged);
    _playback.addListener(_onPlaybackChanged);
    windowManager.addListener(this);
    unawaited(_bootstrap());
  }

  void _onPlaylistChanged() {
    unawaited(_broadcastPlaylistSnapshot());
    if (mounted) setState(() {});
  }

  void _onPlaybackChanged() {
    unawaited(_broadcastPlaylistSnapshot());
    unawaited(_broadcastPlaybackSnapshot());
  }

  Future<void> _bootstrap() async {
    await _bus.bindHost(_onCommand);
    // Main close quits the process after tearing down secondary engines.
    await windowManager.setPreventClose(true);
    await windowManager.setAsFrameless();
    // Let MockupShell rounded corners punch through to the desktop.
    await windowManager.setBackgroundColor(const Color(0x00000000));
    await windowManager.setResizable(false);
    await windowManager.setTitle('Tramp — Main');

    final settings = await _settingsStore.read();
    _zoomPercent = settings.zoomPercent;
    _alwaysOnTop = settings.alwaysOnTop;
    _forceMono = settings.forceMono;
    _docking = DockingCoordinator(DockLayout.fromSettings(settings));

    await _equalizer.load();
    await _playlist.restoreLastPlaylist();
    await _playback.setForceMono(_forceMono);
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
          await _pushPlaylistSnapshot(role);
          await _pushPlaybackSnapshot(role);
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
        await _playback.setForceMono(enabled);
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
      case TransportCommand(:final action):
        await _handleTransport(action);
      case SeekCommand(:final positionMs):
        await _playback.seek(Duration(milliseconds: positionMs));
      case VolumeCommand(:final volume):
        _playback.setVolume(volume);
      case PlaylistOpCommand():
        await _handlePlaylistOp(command);
      case ResizePlaylistCommand(:final width, :final height):
        await _handlePlaylistResize(width, height);
      case MoveWindowCommand(
          :final window,
          :final left,
          :final top,
          :final shiftUndock,
          :final ended,
        ):
        // Non-ended moves return immediately so IPC is not blocked on OS moves.
        final future = _handleDockMove(
          window,
          Offset(left, top),
          shiftUndock: shiftUndock,
          ended: ended,
        );
        if (ended) await future;
    }
  }

  /// Title-bar drag → [DockingCoordinator.move] → coalesce OS position applies.
  ///
  /// During a native OS drag the dragged HWND is owned by the system; we only
  /// push **siblings** (latest-wins, position-only) and snap on pan-end.
  Future<void> _handleDockMove(
    WindowId id,
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  }) async {
    _docking.move(
      id,
      logicalTopLeft,
      shiftUndock: shiftUndock,
      snap: ended,
    );
    _dockDragWindow = id;

    if (ended) {
      await _dockMoveCoalescer.flush(() async {
        await _applyDockGroupFrames(id, positionOnly: false);
      });
      _dockDragWindow = null;
      _nativeDragging = false;
      await _persistLayout();
      await _broadcastDockSnapshot();
      if (mounted) setState(() {});
      return;
    }

    _dockMoveCoalescer.schedule(() async {
      final dragged = _dockDragWindow ?? id;
      // OS already moved [dragged]; only reposition docked partners.
      await _applyDockGroupFrames(
        dragged,
        positionOnly: true,
        skip: dragged,
      );
    });
  }

  Future<void> _applyDockGroupFrames(
    WindowId id, {
    required bool positionOnly,
    WindowId? skip,
  }) {
    final group = _docking.moveCohortOf(id);
    return Future.wait([
      for (final member in group)
        if (member != skip)
          _applyRoleFrame(_roleFor(member), positionOnly: positionOnly),
    ]);
  }

  void _onNativeDragStarted(WindowId id) {
    _nativeDragging = true;
    _dockDragWindow = id;
    _nativeDragEndFallback?.cancel();
  }

  void _armNativeDragEndFallback() {
    // WM_EXITSIZEMOVE / "moved" is not always delivered after startDragging.
    // If move events go quiet, finalize snap as if the gesture ended.
    _nativeDragEndFallback?.cancel();
    _nativeDragEndFallback = Timer(const Duration(milliseconds: 180), () {
      if (!_nativeDragging || _dockDragWindow != WindowId.main) return;
      unawaited(
        _nativeSyncCoalescer.flush(() => _syncNativeMainDrag(ended: true)),
      );
    });
  }

  Future<void> _syncNativeMainDrag({required bool ended}) async {
    if (!_nativeDragging && !ended) return;
    if (ended) _nativeDragEndFallback?.cancel();
    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    final pos = await windowManager.getPosition();
    final logical = Offset(pos.dx / zoom, pos.dy / zoom);
    await _handleDockMove(
      WindowId.main,
      logical,
      shiftUndock: HardwareKeyboard.instance.isShiftPressed,
      ended: ended,
    );
  }

  Future<void> _handleTransport(String action) async {
    switch (action) {
      case 'playPause':
        await _playback.playPause();
      case 'stop':
        await _playback.stop();
      case 'next':
        await _playback.next();
      case 'previous':
        await _playback.previous();
    }
  }

  Future<void> _handlePlaylistOp(PlaylistOpCommand command) async {
    switch (command.op) {
      case 'playIndex':
        final index = command.index;
        if (index != null) await _playback.playIndex(index);
      case 'select':
        final index = command.index;
        if (index != null) _playlist.select(index);
      case 'removeSelected':
        _playlist.removeSelected();
      case 'clear':
        _playlist.clear();
      case 'selectAll':
        _playlist.selectAll();
      case 'invertSelection':
        _playlist.invertSelection();
      case 'addPaths':
        final paths = command.paths;
        if (paths != null && paths.isNotEmpty) {
          final tracks = tracksFromPaths(paths);
          if (tracks.isNotEmpty) {
            _playlist.addTracks(tracks);
            if (_playback.currentTrack == null) {
              await _playback.playIndex(
                _playlist.playlist.tracks.length - tracks.length,
              );
            }
          }
        }
      case 'openPlaylist':
        final path = command.path;
        if (path != null && path.isNotEmpty) {
          await _playlist.openPlaylistFile(path);
        }
      case 'savePlaylist':
        final path = command.path;
        if (path != null && path.isNotEmpty) {
          await _playlist.savePlaylistFile(path);
        }
      case 'sort':
        final key = PlaylistSortKey.values.asNameMap()[command.sortKey];
        if (key != null) _playlist.sortBy(key);
      case 'reverse':
        _playlist.reverseTracks();
    }
  }

  Future<void> _handlePlaylistResize(double width, double height) async {
    final w = width.clamp(400.0, 4000.0);
    final h = height.clamp(200.0, 4000.0);
    final current = _docking.layout.playlist;
    final logicalH = current.shaded
        ? (current.height ?? TrampMetrics.playlistDefault.height)
        : h;
    _docking.resizePlaylist(Size(w, logicalH));
    await _persistLayout();
    // Do not re-push the playlist frame — the OS window already has this size.
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

  PlaylistSnapshotEvent _playlistSnapshot() {
    return PlaylistSnapshotEvent(
      tracks: List.of(_playlist.playlist.tracks),
      selectedIndices: _playlist.selectedIndices.toList()..sort(),
      selectedIndex: _playlist.selectedIndex,
      sourcePath: _playlist.playlist.sourcePath,
      playingIndex: _playback.playingIndex,
      playing: _playback.playing,
    );
  }

  Future<void> _pushPlaylistSnapshot(WindowRole role) async {
    final controller = switch (role) {
      WindowRole.playlist => _playlistWindow,
      WindowRole.equalizer => _equalizerWindow,
      WindowRole.main => null,
    };
    if (controller == null) return;
    try {
      await SessionBus.pushEvent(controller, _playlistSnapshot());
    } catch (error, stack) {
      debugPrint('SessionHost pushPlaylist($role) failed: $error\n$stack');
    }
  }

  Future<void> _broadcastPlaylistSnapshot() =>
      _broadcast(_playlistSnapshot());

  PlaybackSnapshotEvent _playbackSnapshot() {
    return PlaybackSnapshotEvent(
      playing: _playback.playing,
      positionMs: _playback.position.inMilliseconds,
      durationMs: _playback.duration.inMilliseconds,
      volume: _playback.volume,
      muted: _playback.muted,
      shuffle: _playback.shuffle,
      repeatMode: _playback.repeatMode.name,
      playingPath: _playback.currentTrack?.path,
    );
  }

  Future<void> _pushPlaybackSnapshot(WindowRole role) async {
    final controller = switch (role) {
      WindowRole.playlist => _playlistWindow,
      WindowRole.equalizer => _equalizerWindow,
      WindowRole.main => null,
    };
    if (controller == null) return;
    try {
      await SessionBus.pushEvent(controller, _playbackSnapshot());
    } catch (error, stack) {
      debugPrint('SessionHost pushPlayback($role) failed: $error\n$stack');
    }
  }

  Future<void> _broadcastPlaybackSnapshot() =>
      _broadcast(_playbackSnapshot());

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

  Future<void> _applyMainFrame({bool positionOnly = false}) async {
    final zoom = _zoomPercent / 100.0;
    final rect = _docking.frameFor(WindowId.main, zoom);
    if (positionOnly) {
      await windowManager.setPosition(rect.topLeft);
      return;
    }
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

  Future<void> _applyRoleFrame(
    WindowRole role, {
    bool positionOnly = false,
  }) async {
    if (role == WindowRole.main) {
      await _applyMainFrame(positionOnly: positionOnly);
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
        positionOnly: positionOnly,
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
  void onWindowFocus() {
    // Taskbar / Alt-Tab / click: raise visible EQ/PL with the main player.
    unawaited(_raiseVisibleGroupWithMain());
  }

  /// Show+focus visible secondaries, then keep main as the focused HWND.
  Future<void> _raiseVisibleGroupWithMain() async {
    if (_raisingFocusGroup || !_bootstrapped || _nativeDragging) return;
    if (_minimizeGroup.isActive) return;
    _raisingFocusGroup = true;
    try {
      final layout = _docking.layout;
      if (layout.equalizer.visible &&
          _eqReady &&
          _equalizerWindow != null &&
          !_minimizeGroup.shouldSuppressShow(WindowId.equalizer)) {
        try {
          await SessionBus.pushRaise(_equalizerWindow!);
        } catch (error, stack) {
          debugPrint('SessionHost raise(eq) failed: $error\n$stack');
        }
      }
      if (layout.playlist.visible &&
          _playlistReady &&
          _playlistWindow != null &&
          !_minimizeGroup.shouldSuppressShow(WindowId.playlist)) {
        try {
          await SessionBus.pushRaise(_playlistWindow!);
        } catch (error, stack) {
          debugPrint('SessionHost raise(pl) failed: $error\n$stack');
        }
      }
      await windowManager.focus();
    } finally {
      // Defer clear so the focus() echo does not re-enter immediately.
      Future<void>.delayed(const Duration(milliseconds: 100), () {
        _raisingFocusGroup = false;
      });
    }
  }

  @override
  void onWindowMove() {
    if (!_nativeDragging || _dockDragWindow != WindowId.main) return;
    _nativeSyncCoalescer.schedule(() => _syncNativeMainDrag(ended: false));
    _armNativeDragEndFallback();
  }

  @override
  void onWindowMoved() {
    if (!_nativeDragging || _dockDragWindow != WindowId.main) return;
    _nativeDragEndFallback?.cancel();
    unawaited(
      _nativeSyncCoalescer.flush(() => _syncNativeMainDrag(ended: true)),
    );
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
    _nativeDragEndFallback?.cancel();
    windowManager.removeListener(this);
    _playlist.removeListener(_onPlaylistChanged);
    _playback.removeListener(_onPlaybackChanged);
    unawaited(_bus.unbind());
    unawaited(_playback.dispose());
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final layout = _docking.layout;
    final zoom = _zoomPercent / 100.0;
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      color: const Color(0x00000000),
      home: ColoredBox(
        color: const Color(0x00000000),
        child: !_bootstrapped
            ? const Center(
                child: Text(
                  'starting session…',
                  style: TextStyle(color: MockupTokens.inkDim, fontSize: 12),
                ),
              )
            : ZoomedCanvas(
                factor: zoom,
                child: MainPlayerWindow(
                  playback: _playback,
                  trackCount: _playlist.playlist.tracks.length,
                  forceMono: _forceMono,
                  alwaysOnTop: _alwaysOnTop,
                  equalizerVisible: layout.equalizer.visible,
                  playlistVisible: layout.playlist.visible,
                  zoom: zoom,
                  dockLogicalTopLeft: () => Offset(
                    _docking.layout.main.left,
                    _docking.layout.main.top,
                  ),
                  onDockMove: (topLeft, {required shiftUndock, required ended}) {
                    // Fallback path when nativeDragging is disabled (tests).
                    unawaited(
                      _handleDockMove(
                        WindowId.main,
                        topLeft,
                        shiftUndock: shiftUndock,
                        ended: ended,
                      ),
                    );
                  },
                  onNativeDragStarted: () =>
                      _onNativeDragStarted(WindowId.main),
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
