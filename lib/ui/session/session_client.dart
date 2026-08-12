import 'dart:async';
import 'dart:io';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/equalizer_settings.dart';
import '../../domain/tramp_settings.dart';
import '../../look/builtin_look.dart';
import '../../look/look_font_loader.dart';
import '../../look/resolved_look.dart';
import '../../platform/file_open.dart';
import '../../platform/tramp_window.dart';
import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_store.dart';
import '../../theme/look_scope.dart';
import '../../theme/tramp_metrics.dart';
import '../docking/dock_move_coalescer.dart';
import '../docking/linux_drag_poll.dart';
import '../docking/native_drag_tracker.dart';
import '../windows/equalizer_window.dart';
import '../windows/playlist_window.dart';
import '../windows/settings_window.dart';
import '../zoom/zoomed_canvas.dart';
import 'session_bus.dart';
import 'session_messages.dart';

/// Secondary-engine shell (EQ / playlist / settings). Mockup chrome for all.
class SessionClientApp extends StatefulWidget {
  const SessionClientApp({
    super.key,
    required this.role,
    required this.windowController,
  });

  final WindowRole role;
  final WindowController windowController;

  @override
  State<SessionClientApp> createState() => _SessionClientAppState();
}

class _MemoryPlaylistStore implements PlaylistStore {
  String? last;

  @override
  Future<String?> readLastPlaylistPath() async => last;

  @override
  Future<void> writeLastPlaylistPath(String? path) async => last = path;
}

class _SessionClientAppState extends State<SessionClientApp>
    with WindowListener {
  final _bus = SessionBus();
  final _fontLoader = LookFontLoader();
  String? _lastEventType;

  EqualizerSettings _eqSettings = EqualizerSettings.flat;
  bool _eqShaded = false;
  final List<String> _presetNames = EqualizerPresets.builtIn.keys.toList();

  late final PlaylistController _playlist;
  ResolvedLook _look = BuiltinLook.resolved;
  int _lookApplyGeneration = 0;
  bool _plShaded = false;
  bool _settingsShaded = false;
  SettingsSnapshotEvent _settingsSnapshot = const SettingsSnapshotEvent(
    resumeLastSession: true,
    confirmBeforeQuit: false,
    scrollTitle: true,
    minimizeHidesSecondaries: true,
    dockSnapStrength: DockSnapStrength.normal,
    skins: [],
    activeSkinId: 'builtin',
  );
  int? _playingIndex;
  bool _playing = false;
  Size _playlistSize = TrampMetrics.playlistDefault;
  int _zoomPercent = TrampSettings.defaults.zoomPercent;
  double _logicalLeft = 0;
  double _logicalTop = 0;
  bool _applyingFrame = false;
  bool _nativeDragging = false;
  /// Ignore configure-event move echoes after host/OS setPosition (Linux).
  DateTime? _suppressNativeMoveUntil;
  final DockMoveCoalescer _nativeSyncCoalescer = DockMoveCoalescer();
  Timer? _resizeDebounce;
  late final NativeDragTracker _nativeDrag;
  late final LinuxDragPoll _linuxDragPoll;

  @override
  void initState() {
    super.initState();
    _nativeDrag = NativeDragTracker(
      // Linux never emits onWindowMoved; quiet finalize is the real end.
      quietFinalizeDelay: Platform.isLinux
          ? const Duration(milliseconds: 180)
          : const Duration(milliseconds: 750),
      onQuietFinalize: () {
        _linuxDragPoll.stop();
        unawaited(
          _nativeSyncCoalescer.flush(
            () => _reportNativeDrag(ended: true, softEnd: true),
          ),
        );
      },
    );
    _linuxDragPoll = LinuxDragPoll(
      onTick: () {
        if (!_nativeDrag.onMoveEvent()) return;
        _nativeDragging = true;
        _nativeSyncCoalescer.schedule(() => _reportNativeDrag(ended: false));
      },
    );
    _playlist = PlaylistController(store: _MemoryPlaylistStore());
    windowManager.addListener(this);
    unawaited(_bootstrap());
  }

  Future<void> _bootstrap() async {
    await windowManager.setPreventClose(true);
    await widget.windowController.setWindowMethodHandler(_onWindowMethod);
    await _configureChrome();
    await _bus.sendCommand(ClientReadyCommand(widget.role));
  }

  Future<void> _configureChrome() async {
    final title = switch (widget.role) {
      WindowRole.equalizer => 'Tramp — Equalizer',
      WindowRole.playlist => 'Tramp — Playlist',
      WindowRole.settings => 'Tramp — Settings',
      WindowRole.main => 'Tramp',
    };
    // waitUntilReadyToShow CoCreateInstances ITaskbarList; setSkipTaskbar
    // null-derefs without it (native crash → "Lost connection to device").
    await windowManager.waitUntilReadyToShow(
      const WindowOptions(backgroundColor: Color(0x00000000)),
    );
    await windowManager.setTitle(title);
    await windowManager.setAsFrameless();
    await windowManager.setBackgroundColor(const Color(0x00000000));
    // Secondaries must not appear as separate Windows taskbar buttons.
    await windowManager.setSkipTaskbar(true);
    // Edge resize only on the playlist window.
    await windowManager.setResizable(widget.role == WindowRole.playlist);
    // Shrink off the plugin's GTK default before the first host frame
    // (avoids a flash of 1280×720 black with chrome in the corner).
    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    final seedLogical = switch (widget.role) {
      WindowRole.equalizer => TrampMetrics.equalizer,
      WindowRole.playlist => TrampMetrics.playlistDefault,
      WindowRole.settings => TrampMetrics.settings,
      WindowRole.main => TrampMetrics.mainPlayer,
    };
    final seed = Size(seedLogical.width * zoom, seedLogical.height * zoom);
    if (widget.role == WindowRole.playlist) {
      await resizeTrampWindow(
        size: seed,
        minimumSize: Size(
          TrampMetrics.playlistMin.width * zoom,
          TrampMetrics.playlistMin.height * zoom,
        ),
        pinSize: false,
      );
    } else {
      await resizeTrampWindow(
        size: seed,
        minimumSize: seed,
        pinSize: true,
      );
    }
  }

  Future<dynamic> _onWindowMethod(MethodCall call) async {
    switch (call.method) {
      case SessionBus.applyFrameMethod:
        final args = Map<String, dynamic>.from(call.arguments as Map);
        await _applyFrame(args);
        return null;
      case SessionBus.raiseMethod:
        await windowManager.setSkipTaskbar(true);
        await windowManager.show();
        await widget.windowController.show();
        await windowManager.focus();
        return null;
      case SessionBus.orderTopMethod:
        await windowManager.setSkipTaskbar(true);
        await windowManager.show();
        await widget.windowController.show();
        // Intentionally no focus — keep the previously focused tramp window.
        return null;
      case SessionBus.eventMethod:
        final envelope = SessionEvent.decodeEnvelope(call.arguments);
        final event = SessionEvent.fromJson(envelope);
        _onSessionEvent(event);
        return null;
      case 'window_close':
        await _hideInsteadOfClose();
        return null;
      case 'session_shutdown':
        await windowManager.setPreventClose(false);
        await windowManager.destroy();
        return null;
      default:
        throw MissingPluginException('Not implemented: ${call.method}');
    }
  }

  void _onSessionEvent(SessionEvent event) {
    if (!mounted) return;
    if (event is LookSnapshotEvent) {
      unawaited(_applyLookSnapshot(event));
      return;
    }
    setState(() {
      _lastEventType = event.type;
      switch (event) {
        case EqSnapshotEvent(:final settings):
          _eqSettings = settings;
        case SettingsSnapshotEvent():
          _settingsSnapshot = event;
        case PlaylistSnapshotEvent(
            :final tracks,
            :final selectedIndices,
            :final selectedIndex,
            :final sourcePath,
            :final playingIndex,
            :final playing,
          ):
          _playlist.setTracks(tracks, sourcePath: sourcePath);
          _playlist.setSelectedIndices(
            selectedIndices,
            primary: selectedIndex,
          );
          _playingIndex = playingIndex;
          _playing = playing;
        case PlaybackSnapshotEvent(:final playing, :final playingPath):
          _playing = playing;
          if (playingPath != null) {
            final idx = _playlist.playlist.tracks
                .indexWhere((t) => t.path == playingPath);
            _playingIndex = idx >= 0 ? idx : _playingIndex;
          }
        case DockSnapshotEvent(
            :final equalizer,
            :final playlist,
            :final settings,
            :final zoomPercent,
          ):
          _zoomPercent = zoomPercent;
          if (widget.role == WindowRole.equalizer) {
            _eqShaded = equalizer.shaded;
            _logicalLeft = equalizer.left;
            _logicalTop = equalizer.top;
          } else if (widget.role == WindowRole.playlist) {
            _plShaded = playlist.shaded;
            _logicalLeft = playlist.left;
            _logicalTop = playlist.top;
            _playlistSize = Size(
              playlist.width ?? TrampMetrics.playlistDefault.width,
              playlist.height ?? TrampMetrics.playlistDefault.height,
            );
          } else if (widget.role == WindowRole.settings) {
            _settingsShaded = settings.shaded;
            _logicalLeft = settings.left;
            _logicalTop = settings.top;
          }
        default:
          break;
      }
    });
  }

  Future<void> _applyLookSnapshot(LookSnapshotEvent event) async {
    final generation = ++_lookApplyGeneration;
    final files = event.fontFiles;
    if (files != null && files.isNotEmpty && event.id != 'builtin') {
      for (final entry in files.entries) {
        final file = File(entry.value);
        if (!await file.exists()) continue;
        try {
          await _fontLoader.ensureFamily(
            packId: event.id,
            role: entry.key,
            file: file,
            weight: 400,
          );
        } catch (error, stack) {
          debugPrint(
            'SessionClient look font ${entry.key} failed: $error\n$stack',
          );
        }
      }
    }
    if (!mounted || generation != _lookApplyGeneration) return;
    setState(() {
      _lastEventType = event.type;
      _look = event.toResolved();
    });
  }

  Future<void> _applyFrame(Map<String, dynamic> args) async {
    final left = (args['left'] as num).toDouble();
    final top = (args['top'] as num).toDouble();
    final width = (args['width'] as num).toDouble();
    final height = (args['height'] as num).toDouble();
    final visible = args['visible'] == true;
    final alwaysOnTop = args['alwaysOnTop'] == true;
    final positionOnly = args['positionOnly'] == true;

    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    _logicalLeft = left / zoom;
    _logicalTop = top / zoom;

    if (positionOnly) {
      // Only skip while THIS window's OS drag is active — not after soft-end
      // or setPosition echoes that flip [_nativeDragging] without a real drag.
      if (!_nativeDrag.isActive) {
        _suppressNativeMoves();
        await windowManager.setPosition(Offset(left, top));
      }
      return;
    }

    _applyingFrame = true;
    try {
      final size = Size(width, height);
      if (widget.role == WindowRole.playlist) {
        final zoom = _zoomPercent / 100.0;
        await windowManager.setResizable(!_plShaded);
        await resizeTrampWindow(
          size: size,
          minimumSize: Size(
            TrampMetrics.playlistMin.width * zoom,
            TrampMetrics.playlistMin.height * zoom,
          ),
          pinSize: false,
        );
      } else {
        await windowManager.setResizable(false);
        await resizeTrampWindow(
          size: size,
          minimumSize: size,
          pinSize: true,
        );
      }
      _suppressNativeMoves();
      await windowManager.setPosition(Offset(left, top));
      await windowManager.setAlwaysOnTop(alwaysOnTop);
      if (visible) {
        // Re-assert after show — some hosts re-register taskbar buttons.
        await windowManager.setSkipTaskbar(true);
        await windowManager.show();
        await widget.windowController.show();
      } else {
        await windowManager.hide();
        await widget.windowController.hide();
      }
    } finally {
      // Ignore resize echoes from host-driven setSize.
      Future<void>.delayed(const Duration(milliseconds: 80), () {
        _applyingFrame = false;
      });
    }
  }

  @override
  void onWindowClose() {
    unawaited(_hideInsteadOfClose());
  }

  @override
  void onWindowResize() {
    if (widget.role != WindowRole.playlist) return;
    if (_applyingFrame || _plShaded) return;
    _resizeDebounce?.cancel();
    _resizeDebounce = Timer(const Duration(milliseconds: 120), () {
      unawaited(_reportPlaylistResize());
    });
  }

  Future<void> _reportPlaylistResize() async {
    if (_applyingFrame || _plShaded) return;
    final pixel = await windowManager.getSize();
    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    final logical = Size(pixel.width / zoom, pixel.height / zoom);
    if ((logical.width - _playlistSize.width).abs() < 0.5 &&
        (logical.height - _playlistSize.height).abs() < 0.5) {
      return;
    }
    // Paint path reads constraints live; this only persists / syncs host.
    _playlistSize = logical;
    await _send(
      ResizePlaylistCommand(width: logical.width, height: logical.height),
    );
  }

  Future<void> _hideInsteadOfClose() async {
    final windowId = switch (widget.role) {
      WindowRole.equalizer => WindowId.equalizer,
      WindowRole.playlist => WindowId.playlist,
      WindowRole.settings => WindowId.settings,
      WindowRole.main => WindowId.main,
    };
    try {
      await _bus.sendCommand(
        ToggleWindowCommand(window: windowId, visible: false),
      );
    } catch (_) {
      // Host may already be tearing down.
    }
    await windowManager.hide();
    await widget.windowController.hide();
  }

  Future<void> _send(SessionCommand command) async {
    try {
      await _bus.sendCommand(command);
    } catch (_) {
      // Host may be unavailable during teardown.
    }
  }

  WindowId get _windowId => switch (widget.role) {
        WindowRole.equalizer => WindowId.equalizer,
        WindowRole.playlist => WindowId.playlist,
        WindowRole.settings => WindowId.settings,
        WindowRole.main => WindowId.main,
      };

  void _onDockMove(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  }) {
    // Fallback when nativeDragging is disabled (tests).
    _logicalLeft = logicalTopLeft.dx;
    _logicalTop = logicalTopLeft.dy;
    unawaited(
      _send(
        MoveWindowCommand(
          window: _windowId,
          left: logicalTopLeft.dx,
          top: logicalTopLeft.dy,
          shiftUndock: shiftUndock,
          ended: ended,
        ),
      ),
    );
  }

  void _onNativeDragStarted() {
    _nativeDragging = true;
    _nativeDrag.started();
    _linuxDragPoll.start();
  }

  void _suppressNativeMoves() {
    _suppressNativeMoveUntil =
        DateTime.now().add(const Duration(milliseconds: 200));
  }

  Future<void> _reportNativeDrag({
    required bool ended,
    bool softEnd = false,
  }) async {
    if (!_nativeDragging && !ended) return;
    if (ended && !softEnd) {
      _nativeDrag.endedConfirmed();
    }
    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    final pos = await windowManager.getPosition();
    final logical = Offset(pos.dx / zoom, pos.dy / zoom);
    _logicalLeft = logical.dx;
    _logicalTop = logical.dy;
    await _send(
      MoveWindowCommand(
        window: _windowId,
        left: logical.dx,
        top: logical.dy,
        shiftUndock: HardwareKeyboard.instance.isShiftPressed,
        // Linux never emits onWindowMoved — quiet softEnd is the real end and
        // must still report ended so the host can snap / record dock edges.
        ended: ended,
        softEnd: softEnd,
      ),
    );
    if (ended) {
      _nativeDragging = false;
      _linuxDragPoll.stop();
      // Linux soft-end is the real gesture end; clear softEnded so snap
      // setPosition echoes cannot resume a phantom drag and peel edges.
      if (softEnd && Platform.isLinux) {
        _nativeDrag.endedConfirmed();
        _suppressNativeMoves();
      }
    }
  }

  @override
  void onWindowMove() {
    final until = _suppressNativeMoveUntil;
    if (until != null && DateTime.now().isBefore(until)) {
      return;
    }
    if (!_nativeDrag.onMoveEvent()) return;
    _nativeDragging = true;
    _nativeSyncCoalescer.schedule(() => _reportNativeDrag(ended: false));
  }

  @override
  void onWindowMoved() {
    if (!_nativeDragging && !_nativeDrag.isActive && !_nativeDrag.softEnded) {
      return;
    }
    _linuxDragPoll.stop();
    _nativeDrag.endedConfirmed();
    _nativeDragging = true;
    unawaited(
      _nativeSyncCoalescer.flush(() => _reportNativeDrag(ended: true)),
    );
  }

  void _toggleEqShade() {
    unawaited(
      _send(
        SetShadedCommand(
          window: WindowId.equalizer,
          shaded: !_eqShaded,
        ),
      ),
    );
  }

  void _togglePlShade() {
    unawaited(
      _send(
        SetShadedCommand(
          window: WindowId.playlist,
          shaded: !_plShaded,
        ),
      ),
    );
  }

  void _toggleSettingsShade() {
    unawaited(
      _send(
        SetShadedCommand(
          window: WindowId.settings,
          shaded: !_settingsShaded,
        ),
      ),
    );
  }

  Future<void> _addFiles() async {
    final paths = await pickAudioFiles();
    if (paths == null || paths.isEmpty) return;
    await _send(PlaylistOpCommand('addPaths', paths: paths));
  }

  Future<void> _loadPlaylist() async {
    final path = await pickPlaylistFile();
    if (path == null || path.isEmpty) return;
    await _send(PlaylistOpCommand('openPlaylist', path: path));
  }

  Future<void> _savePlaylist() async {
    final path = await pickSavePlaylistPath();
    if (path == null || path.isEmpty) return;
    await _send(PlaylistOpCommand('savePlaylist', path: path));
  }

  void _dropPaths(List<String> paths) {
    final playlistPaths = paths.where(isPlaylistPath).toList();
    final audioPaths = paths.where(isAudioPath).toList();
    if (playlistPaths.isNotEmpty) {
      unawaited(
        _send(PlaylistOpCommand('openPlaylist', path: playlistPaths.first)),
      );
    }
    if (audioPaths.isNotEmpty) {
      unawaited(_send(PlaylistOpCommand('addPaths', paths: audioPaths)));
    }
  }

  @override
  void dispose() {
    _resizeDebounce?.cancel();
    _linuxDragPoll.dispose();
    _nativeDrag.dispose();
    windowManager.removeListener(this);
    unawaited(widget.windowController.setWindowMethodHandler(null));
    _playlist.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final zoom = _zoomPercent / 100.0;
    if (widget.role == WindowRole.equalizer) {
      return MaterialApp(
        debugShowCheckedModeBanner: false,
        color: const Color(0x00000000),
        builder: (context, child) => LookScope(
          look: _look,
          child: child ?? const SizedBox.shrink(),
        ),
        home: ColoredBox(
          color: const Color(0x00000000),
          child: ZoomedCanvas(
            factor: zoom,
            logicalSize: TrampMetrics.equalizer,
            child: EqualizerWindow(
              settings: _eqSettings,
              shaded: _eqShaded,
              presetNames: _presetNames,
              zoom: zoom,
              dockLogicalTopLeft: () => Offset(_logicalLeft, _logicalTop),
              onDockMove: _onDockMove,
              onNativeDragStarted: _onNativeDragStarted,
              onSessionCommand: (cmd) => unawaited(_send(cmd)),
              onCollapse: _toggleEqShade,
              onClose: () => unawaited(_hideInsteadOfClose()),
            ),
          ),
        ),
      );
    }

    if (widget.role == WindowRole.playlist) {
      return MaterialApp(
        debugShowCheckedModeBanner: false,
        color: const Color(0x00000000),
        builder: (context, child) => LookScope(
          look: _look,
          child: child ?? const SizedBox.shrink(),
        ),
        home: ColoredBox(
          color: const Color(0x00000000),
          // Derive logical size from live window constraints every frame so
          // resize never anisotropic-stretches a stale canvas (zoom owns
          // proportions; only spacing grows). Host sync stays debounced.
          child: LayoutBuilder(
            builder: (context, constraints) {
              final logical = Size(
                constraints.maxWidth / zoom,
                constraints.maxHeight / zoom,
              );
              return ZoomedCanvas(
                factor: zoom,
                child: PlaylistWindow(
                  playlist: _playlist,
                  size: logical,
                  shaded: _plShaded,
                  playingIndex: _playingIndex,
                  playing: _playing,
                  zoom: zoom,
                  dockLogicalTopLeft: () => Offset(_logicalLeft, _logicalTop),
                  onDockMove: _onDockMove,
                  onNativeDragStarted: _onNativeDragStarted,
                  onSessionCommand: (cmd) => unawaited(_send(cmd)),
                  onAddFiles: () => unawaited(_addFiles()),
                  onLoadPlaylist: () => unawaited(_loadPlaylist()),
                  onSavePlaylist: () => unawaited(_savePlaylist()),
                  onDropPaths: _dropPaths,
                  onCollapse: _togglePlShade,
                  onClose: () => unawaited(_hideInsteadOfClose()),
                ),
              );
            },
          ),
        ),
      );
    }

    if (widget.role == WindowRole.settings) {
      return MaterialApp(
        debugShowCheckedModeBanner: false,
        color: const Color(0x00000000),
        builder: (context, child) => LookScope(
          look: _look,
          child: child ?? const SizedBox.shrink(),
        ),
        home: ColoredBox(
          color: const Color(0x00000000),
          child: ZoomedCanvas(
            factor: zoom,
            logicalSize: TrampMetrics.settings,
            child: SettingsWindow(
              snapshot: _settingsSnapshot,
              shaded: _settingsShaded,
              zoom: zoom,
              dockLogicalTopLeft: () => Offset(_logicalLeft, _logicalTop),
              onDockMove: _onDockMove,
              onNativeDragStarted: _onNativeDragStarted,
              onSessionCommand: (cmd) => unawaited(_send(cmd)),
              onCollapse: _toggleSettingsShade,
              onClose: () => unawaited(_hideInsteadOfClose()),
            ),
          ),
        ),
      );
    }

    // Main role should not use SessionClientApp.
    final palette = _look.palette;
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      color: const Color(0x00000000),
      builder: (context, child) => LookScope(
        look: _look,
        child: child ?? const SizedBox.shrink(),
      ),
      home: ColoredBox(
        color: palette.shellMid,
        child: Center(
          child: Text(
            _lastEventType == null
                ? 'unexpected main role on client'
                : 'last event: $_lastEventType',
            style: TextStyle(color: palette.inkDim, fontSize: 12),
          ),
        ),
      ),
    );
  }
}
