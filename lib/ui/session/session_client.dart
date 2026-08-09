import 'dart:async';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:window_manager/window_manager.dart';

import '../../domain/equalizer_settings.dart';
import '../../domain/tramp_settings.dart';
import '../../platform/file_open.dart';
import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_store.dart';
import '../../theme/mockup_tokens.dart';
import '../../theme/tramp_metrics.dart';
import '../docking/dock_move_coalescer.dart';
import '../windows/equalizer_window.dart';
import '../windows/playlist_window.dart';
import 'session_bus.dart';
import 'session_messages.dart';

/// Secondary-engine shell (EQ / playlist). Mockup chrome for both roles.
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
  String? _lastEventType;

  EqualizerSettings _eqSettings = EqualizerSettings.flat;
  bool _eqShaded = false;
  final List<String> _presetNames = EqualizerPresets.builtIn.keys.toList();

  late final PlaylistController _playlist;
  bool _plShaded = false;
  int? _playingIndex;
  bool _playing = false;
  Size _playlistSize = TrampMetrics.playlistDefault;
  int _zoomPercent = 100;
  double _logicalLeft = 0;
  double _logicalTop = 0;
  bool _applyingFrame = false;
  bool _nativeDragging = false;
  final DockMoveCoalescer _nativeSyncCoalescer = DockMoveCoalescer();
  Timer? _resizeDebounce;
  Timer? _nativeDragEndFallback;

  @override
  void initState() {
    super.initState();
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
      WindowRole.main => 'Tramp',
    };
    await windowManager.setTitle(title);
    await windowManager.setAsFrameless();
    // Secondaries must not appear as separate Windows taskbar buttons.
    await windowManager.setSkipTaskbar(true);
    // Edge resize only on the playlist window.
    await windowManager.setResizable(widget.role == WindowRole.playlist);
    if (widget.role == WindowRole.playlist) {
      final zoom = _zoomPercent / 100.0;
      await windowManager.setMinimumSize(Size(400 * zoom, 200 * zoom));
    }
  }

  Future<dynamic> _onWindowMethod(MethodCall call) async {
    switch (call.method) {
      case SessionBus.applyFrameMethod:
        final args = Map<String, dynamic>.from(call.arguments as Map);
        await _applyFrame(args);
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
    setState(() {
      _lastEventType = event.type;
      switch (event) {
        case EqSnapshotEvent(:final settings):
          _eqSettings = settings;
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
          }
        default:
          break;
      }
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
      // During native drag the OS owns this HWND — never fight it with
      // host setPosition echoes (host should skip us; this is a safeguard).
      if (!_nativeDragging) {
        await windowManager.setPosition(Offset(left, top));
      }
      return;
    }

    _applyingFrame = true;
    try {
      if (widget.role == WindowRole.playlist) {
        final zoom = _zoomPercent / 100.0;
        await windowManager.setMinimumSize(Size(400 * zoom, 200 * zoom));
        await windowManager.setResizable(!_plShaded);
      } else {
        await windowManager.setMinimumSize(Size(width, height));
        await windowManager.setResizable(false);
      }
      await windowManager.setSize(Size(width, height));
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
    _playlistSize = logical;
    await _send(
      ResizePlaylistCommand(width: logical.width, height: logical.height),
    );
    if (mounted) setState(() {});
  }

  Future<void> _hideInsteadOfClose() async {
    final windowId = switch (widget.role) {
      WindowRole.equalizer => WindowId.equalizer,
      WindowRole.playlist => WindowId.playlist,
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
    _nativeDragEndFallback?.cancel();
  }

  void _armNativeDragEndFallback() {
    _nativeDragEndFallback?.cancel();
    _nativeDragEndFallback = Timer(const Duration(milliseconds: 180), () {
      if (!_nativeDragging) return;
      unawaited(
        _nativeSyncCoalescer.flush(() => _reportNativeDrag(ended: true)),
      );
    });
  }

  Future<void> _reportNativeDrag({required bool ended}) async {
    if (!_nativeDragging && !ended) return;
    if (ended) _nativeDragEndFallback?.cancel();
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
        ended: ended,
      ),
    );
    if (ended) _nativeDragging = false;
  }

  @override
  void onWindowMove() {
    if (!_nativeDragging) return;
    _nativeSyncCoalescer.schedule(() => _reportNativeDrag(ended: false));
    _armNativeDragEndFallback();
  }

  @override
  void onWindowMoved() {
    if (!_nativeDragging) return;
    _nativeDragEndFallback?.cancel();
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
    _nativeDragEndFallback?.cancel();
    windowManager.removeListener(this);
    unawaited(widget.windowController.setWindowMethodHandler(null));
    _playlist.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (widget.role == WindowRole.equalizer) {
      return MaterialApp(
        debugShowCheckedModeBanner: false,
        home: ColoredBox(
          color: MockupTokens.shellDeep,
          child: Align(
            alignment: Alignment.topLeft,
            child: EqualizerWindow(
              settings: _eqSettings,
              shaded: _eqShaded,
              presetNames: _presetNames,
              zoom: _zoomPercent / 100.0,
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
        home: ColoredBox(
          color: MockupTokens.shellDeep,
          child: Align(
            alignment: Alignment.topLeft,
            child: PlaylistWindow(
              playlist: _playlist,
              size: _playlistSize,
              shaded: _plShaded,
              playingIndex: _playingIndex,
              playing: _playing,
              zoom: _zoomPercent / 100.0,
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
          ),
        ),
      );
    }

    // Main role should not use SessionClientApp.
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: ColoredBox(
        color: MockupTokens.shellMid,
        child: Center(
          child: Text(
            _lastEventType == null
                ? 'unexpected main role on client'
                : 'last event: $_lastEventType',
            style: const TextStyle(color: MockupTokens.inkDim, fontSize: 12),
          ),
        ),
      ),
    );
  }
}
