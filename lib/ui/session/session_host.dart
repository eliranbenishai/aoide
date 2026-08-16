import 'dart:async';
import 'dart:io';
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:media_kit/media_kit.dart' hide Track;
import 'package:window_manager/window_manager.dart';

import '../../domain/about_stats.dart';
import '../../domain/collection_figures.dart';
import '../../domain/track.dart';
import '../../domain/tramp_settings.dart';
import '../../eq/equalizer_controller.dart';
import '../../eq/mpv_equalizer_sink.dart';
import '../../look/look_installer.dart';
import '../../platform/app_support_dir.dart';
import '../../platform/harness_flags.dart';
import '../../platform/file_open.dart';
import '../../platform/open_url.dart';
import '../../platform/os_window.dart';
import '../../platform/session_resume_store.dart';
import '../../platform/settings_store.dart';
import '../../platform/tramp_window.dart';
import '../../platform/usage_store.dart';
import '../../playback/media_kit_player_engine.dart';
import '../../playback/playback_controller.dart';
import '../../playback/player_engine.dart';
import '../../playlist/altered_playlist_store.dart';
import '../../playlist/playlist_collection_controller.dart';
import '../../playlist/playlist_collection_store.dart';
import '../../playlist/playlist_controller.dart';
import '../../playlist/playlist_sort.dart';
import '../../playlist/playlist_store.dart';
import '../../playlist/track_metadata_probe.dart';
import '../../theme/tramp_metrics.dart';
import '../docking/dock_layout.dart';
import '../docking/dock_move_coalescer.dart';
import '../docking/docking_coordinator.dart';
import '../docking/linux_drag_poll.dart';
import '../docking/native_drag_tracker.dart';
import '../windows/about_window.dart';
import '../windows/equalizer_window.dart';
import '../windows/main_player_window.dart';
import '../windows/playlist_window.dart';
import '../windows/settings_window.dart';
import '../zoom/zoomed_canvas.dart';
import 'always_on_top.dart';
import 'minimize_group.dart';
import 'session_messages.dart';
import 'session_quit.dart';
import 'session_visibility.dart';
import '../../look/look_controller.dart';
import '../../theme/look_scope.dart';

part 'session_host_chrome.dart';

/// Main-engine session owner: controllers/settings, docking frames, EQ/PL windows.
class SessionHostApp extends StatefulWidget {
  const SessionHostApp({
    super.key,
    this.launchArgs = const [],
    this.settingsStore,
    this.engine,
    this.playlistStore,
    this.playlistCollectionStore,
  });

  final List<String> launchArgs;
  final SettingsStore? settingsStore;
  final PlayerEngine? engine;
  final PlaylistStore? playlistStore;
  final PlaylistCollectionStore? playlistCollectionStore;

  @override
  State<SessionHostApp> createState() => _SessionHostAppState();
}

class _SessionHostAppState extends State<SessionHostApp>
    with WindowListener, WidgetsBindingObserver {
  late final SettingsStore _settingsStore;
  late final LookController _lookController;
  late final PlaylistController _playlist;
  late final PlaylistCollectionController _collection;
  late final PlaybackController _playback;
  late final EqualizerController _equalizer;
  late final TrackMetadataProbe _trackProbe;
  late DockingCoordinator _docking;
  late final SessionResumeStore _resumeStore;
  int _zoomPercent = TrampSettings.defaults.zoomPercent;
  bool _alwaysOnTop = TrampSettings.defaults.alwaysOnTop;
  bool _forceMono = TrampSettings.defaults.forceMono;
  bool _resumeLastSession = TrampSettings.defaults.resumeLastSession;
  bool _confirmBeforeQuit = TrampSettings.defaults.confirmBeforeQuit;
  bool _scrollTitle = TrampSettings.defaults.scrollTitle;
  bool _minimizeHidesSecondaries =
      TrampSettings.defaults.minimizeHidesSecondaries;
  DockSnapStrength _dockSnapStrength = TrampSettings.defaults.dockSnapStrength;
  double _playlistCollectionWidth =
      TrampSettings.defaults.playlistCollectionWidth;
  bool _playlistCollectionCollapsed =
      TrampSettings.defaults.playlistCollectionCollapsed;

  OsWindow? _equalizerWindow;
  OsWindow? _playlistWindow;
  OsWindow? _settingsWindow;
  OsWindow? _aboutWindow;
  bool _eqReady = false;
  bool _playlistReady = false;
  bool _settingsReady = false;
  bool _aboutReady = false;
  bool _bootstrapped = false;
  /// OS mapping is delayed until chrome has painted (not merely bootstrapped).
  bool _revealWindows = false;
  final MinimizeGroupCycle _minimizeGroup = MinimizeGroupCycle();
  final DockMoveCoalescer _dockMoveCoalescer = DockMoveCoalescer();
  final DockMoveCoalescer _nativeSyncCoalescer = DockMoveCoalescer();
  WindowId? _dockDragWindow;
  /// Last seen current-playlist origin, so the collection highlight moves only
  /// when the current playlist is actually loaded from somewhere else.
  String? _lastPlaylistSource;
  bool _nativeDragging = false;
  /// Last reading sent to the About window, and what it was computed from.
  /// Both start below any real value so the first push always happens.
  CollectionFigures _aboutFigures = CollectionFigures.empty;
  int _aboutFiguresRevision = -1;
  int _aboutSpins = -1;
  Timer? _resumeSaveTimer;
  late final NativeDragTracker _mainNativeDrag;
  late final LinuxDragPoll _mainLinuxDragPoll;
  Timer? _playlistResizeDebounce;
  Timer? _collectionWidthDebounce;
  double? _pendingCollectionWidth;
  Size? _lastPlaylistPixelSize;
  /// Guards [onWindowFocus] → raise → main [focus] from re-entering.
  bool _raisingFocusGroup = false;

  @override
  void initState() {
    super.initState();
    _mainNativeDrag = NativeDragTracker(
      // Linux never emits onWindowMoved; quiet finalize is the real end.
      // Arm only after motion (see LinuxDragPoll) — not on press.
      quietFinalizeDelay: Platform.isLinux
          ? const Duration(milliseconds: 400)
          : const Duration(milliseconds: 750),
      onQuietFinalize: () {
        _mainLinuxDragPoll.stop();
        // Soft end: siblings only — do not fight the OS HWND if drag resumes.
        unawaited(
          _nativeSyncCoalescer.flush(
            () => _syncNativeDrag(
              _dockDragWindow ?? WindowId.main,
              ended: true,
              softEnd: true,
            ),
          ),
        );
      },
    );
    _mainLinuxDragPoll = LinuxDragPoll(
      getPosition: _pixelPositionOfDragged,
      onMotion: (_) {
        if (!_mainNativeDrag.onMoveEvent()) return;
        _nativeDragging = true;
        final id = _dockDragWindow ?? WindowId.main;
        _nativeSyncCoalescer.schedule(() => _syncNativeDrag(id, ended: false));
      },
    );
    _settingsStore = widget.settingsStore ??
        FileSettingsStore(supportDir: trampSupportDirectory);
    _resumeStore = FileSessionResumeStore(
      supportDir: trampSupportDirectory,
    );
    _lookController = LookController(
      settingsStore: _settingsStore,
      supportDir: trampSupportDirectory,
    );
    _lookController.addListener(_onLookChanged);
    _docking = _createDocking(DockLayout.defaults);
    // Share one media_kit Player so EQ `af` and transport hit the same libmpv.
    final Player? sharedPlayer =
        widget.engine == null ? Player() : null;
    _playlist = PlaylistController(
      store: widget.playlistStore ??
          FilePlaylistStore(supportDir: trampSupportDirectory),
      // An altered current playlist is kept continuously while the session
      // runs, so quitting can go on writing nothing at all.
      alteredStore: FileAlteredPlaylistStore(
        supportDir: trampSupportDirectory,
      ),
    );
    _collection = PlaylistCollectionController(
      store: widget.playlistCollectionStore ??
          FilePlaylistCollectionStore(
            supportDir: trampSupportDirectory,
          ),
    );
    _playback = PlaybackController(
      playlist: _playlist,
      engine: widget.engine ??
          MediaKitPlayerEngine(
            player: sharedPlayer,
            onMetadata: _onPlayingTrackMetadata,
          ),
      // Lifetime spins are history, so they keep their own small file rather
      // than riding on settings — resetting a preference must not erase them.
      usageStore: FileUsageStore(supportDir: trampSupportDirectory),
    );
    _equalizer = EqualizerController(
      store: _settingsStore,
      sink: sharedPlayer != null
          ? MpvEqualizerSink(sharedPlayer)
          : const NoopEqualizerSink(),
    );
    _trackProbe = MediaKitTrackMetadataProbe();
    _playlist.addListener(_onPlaylistChanged);
    _collection.addListener(_onCollectionChanged);
    _playback.addListener(_onPlaybackChanged);
    windowManager.addListener(this);
    WidgetsBinding.instance.addObserver(this);
    unawaited(_bootstrap());
  }

  void _onPlayingTrackMetadata(String path, Track Function(Track) update) {
    _playlist.updateTrackByPath(path, update);
  }

  void _onPlaylistChanged() {
    // The highlighted row follows where the current playlist came *from*, and
    // only when that actually changes: adding an entry highlights it without
    // loading it, and an unrelated edit (a track selection, say) must not then
    // drag the highlight back. An origin outside the collection clears it.
    final source = _playlist.playlist.sourcePath;
    if (source != _lastPlaylistSource) {
      _lastPlaylistSource = source;
      _collection.select(source);
    }
    if (mounted) setState(() {});
  }

  void _onCollectionChanged() {
    unawaited(_refreshAboutStats());
    if (mounted) setState(() {});
  }

  void _onPlaybackChanged() {
    unawaited(_refreshAboutStats());
    _scheduleResumeSave();
  }

  void _onLookChanged() {
    if (mounted) setState(() {});
  }

  void _scheduleResumeSave() {
    _resumeSaveTimer?.cancel();
    _resumeSaveTimer = Timer(const Duration(seconds: 2), () {
      unawaited(_persistResume());
    });
  }

  Future<void> _persistResume() async {
    if (!_resumeLastSession) return;
    await _resumeStore.write(
      SessionResume(
        playingIndex: _playback.playingIndex,
        positionMs: _playback.position.inMilliseconds,
        wasPlaying: _playback.playing,
      ),
    );
  }

  /// The one write quit already pays for, plus the spin count.
  ///
  /// Quitting stays immediate — this is two small files, not a teardown — but
  /// a listener who closes Tramp within the spin debounce would otherwise lose
  /// the track that just finished, and a lifetime total that drops the last
  /// one is not a lifetime total.
  Future<void> _persistOnQuit() async {
    await _persistResume();
    await _playback.flushUsage();
  }

  Future<void> _bootstrap() async {
    try {
      await windowManager.setPreventClose(true);
      // Same rule as the old secondary engines: ITaskbarList is created only in
      // waitUntilReadyToShow. setSkipTaskbar without it native-crashes on
      // Windows (null taskbar_). The runner never maps this HWND, so hide()
      // is also a deadlock risk there (ShowWindow nested in the UI isolate).
      await windowManager.waitUntilReadyToShow(
        WindowOptions(backgroundColor: trampWindowFill()),
      );
      try {
        if (!Platform.isWindows) {
          await windowManager.hide();
        }
        await windowManager.setSkipTaskbar(true);
      } catch (_) {}
      await windowManager.setAsFrameless();
      // Let MockupShell rounded corners punch through to the desktop.
      await windowManager.setBackgroundColor(trampWindowFill());
      await windowManager.setResizable(false);
      await windowManager.setTitle('Tramp — Main');
      try {
        await windowManager.setIcon('assets/branding/app_icon.png');
      } catch (_) {}

      final settings = await _settingsStore.read();
      _applySettingsFields(settings);
      _docking = _createDocking(DockLayout.fromSettings(settings));

      await _lookController.bootstrap(
        settings: settings,
        supportDir: trampSupportDirectory,
      );
      await _equalizer.load();
      // The collection index holds only what the left panel paints, so this stays
      // a small read even for a large collection. Validating the references it
      // points at is deliberately *not* here — that waits until after launch.
      await _collection.bootstrap();
      // One tiny file, the same size as the resume snapshot: this is what makes
      // the spin count read as a lifetime total rather than a per-session one.
      await _playback.loadUsage();
      if (_resumeLastSession) {
        await _playlist.restoreCurrentPlaylist();
        unawaited(_enrichMissingTrackMetadata());
        await _restorePlaybackResume();
      }
      await _playback.setForceMono(_forceMono);

      // Extra views share this isolate — create them before the first chrome
      // frame so ViewAnchor is present immediately.
      await _ensureSecondaryWindows();
      if (mounted) {
        setState(() => _bootstrapped = true);
      }
      await WidgetsBinding.instance.endOfFrame;
      await WidgetsBinding.instance.endOfFrame;
      _revealWindows = true;
      await _applyAllFrames();
      await _applyAlwaysOnTop();
      // The session is up and the windows are mapped: now, and only now, check the
      // listener's playlist files. Unawaited so a collection on a sleeping drive
      // cannot hold up anything that follows, and last so nothing waits on it.
      unawaited(_collection.validateReferences());
      if (HarnessFlags.positionBench) {
        await _runPositionBench();
        trampExitProcess(0);
      }
      if (trampAutoQuitRequested()) {
        unawaited(_autoQuitForHarness());
      }
    } catch (error, stack) {
      debugPrint('Tramp bootstrap failed: $error\n$stack');
      try {
        await windowManager.setSkipTaskbar(false);
        await windowManager.show();
      } catch (_) {}
      if (mounted) {
        setState(() {
          _bootstrapped = true;
          _revealWindows = true;
        });
      }
      try {
        await _applyAllFrames();
      } catch (_) {}
    }
  }

  Future<void> _autoQuitForHarness() async {
    final deadline = DateTime.now().add(const Duration(seconds: 20));
    while (DateTime.now().isBefore(deadline)) {
      if (_eqReady && _playlistReady && _settingsReady && _aboutReady) {
        break;
      }
      await Future<void>.delayed(const Duration(milliseconds: 50));
    }
    stderr.writeln(
      'TRAMP_QUIT_BEGIN ${DateTime.now().microsecondsSinceEpoch} '
      'eq=$_eqReady pl=$_playlistReady set=$_settingsReady about=$_aboutReady',
    );
    await stderr.flush();
    await _quit();
  }

  void _applySettingsFields(TrampSettings settings) {
    _zoomPercent = settings.zoomPercent;
    _alwaysOnTop = settings.alwaysOnTop;
    _forceMono = settings.forceMono;
    _resumeLastSession = settings.resumeLastSession;
    _confirmBeforeQuit = settings.confirmBeforeQuit;
    _scrollTitle = settings.scrollTitle;
    _minimizeHidesSecondaries = settings.minimizeHidesSecondaries;
    _dockSnapStrength = settings.dockSnapStrength;
    _playlistCollectionWidth = settings.playlistCollectionWidth;
    _playlistCollectionCollapsed = settings.playlistCollectionCollapsed;
  }

  Future<void> _restorePlaybackResume() async {
    final resume = await _resumeStore.read();
    final index = resume.playingIndex;
    if (index == null) return;
    if (index < 0 || index >= _playlist.playlist.tracks.length) return;
    await _playback.playIndex(index);
    if (resume.positionMs > 0) {
      await _playback.seek(Duration(milliseconds: resume.positionMs));
    }
    if (!resume.wasPlaying && _playback.playing) {
      await _playback.playPause();
    }
  }

  Future<void> _ensureSecondaryWindows() async {
    if (HarnessFlags.soloMain) {
      return;
    }
    final zoom = _zoomPercent / 100.0;
    Size seed(WindowId id, Size fallback) {
      try {
        return _docking.frameFor(id, zoom).size;
      } catch (_) {
        return TrampMetrics.zoomed(fallback, _zoomPercent);
      }
    }

    _equalizerWindow ??= OsWindow.create(
      size: seed(WindowId.equalizer, TrampMetrics.equalizer),
      title: 'Tramp — Equalizer',
    );
    _playlistWindow ??= OsWindow.create(
      size: seed(WindowId.playlist, TrampMetrics.playlistDefault),
      title: 'Tramp — Playlist',
      resizable: true,
    );
    _settingsWindow ??= OsWindow.create(
      size: seed(WindowId.settings, TrampMetrics.settings),
      title: 'Tramp — Settings',
    );
    _aboutWindow ??= OsWindow.create(
      size: seed(WindowId.about, TrampMetrics.about),
      title: 'Tramp — About',
    );
    _eqReady = true;
    _playlistReady = true;
    _settingsReady = true;
    _aboutReady = true;
  }

  Future<void> _onCommand(SessionCommand command) async {
    switch (command) {
      case ClientReadyCommand(:final role):
        await _applyRoleFrame(role);
        if (role == WindowRole.about) {
          unawaited(_refreshAboutStats());
        }
      case ToggleWindowCommand(:final window, :final visible):
        if (window == WindowId.main) return;
        _docking.setVisible(window, visible);
        await _persistLayout();
        await _applyRoleFrame(_roleFor(window));
        if (visible && window == WindowId.settings) {
          await _orderSettingsOnTop();
        }
        if (visible && window == WindowId.about) {
          await _raiseAbout();
          await _refreshAboutStats();
        }
        if (mounted) setState(() {});
      case SetShadedCommand(:final window, :final shaded):
        _docking.setShaded(window, shaded);
        await _persistLayout();
        await _applyRoleFrame(_roleFor(window));
        if (mounted) setState(() {});
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
      case UpdateGeneralSettingsCommand():
        await _handleUpdateGeneralSettings(command);
      case ActivateSkinCommand(:final id):
        await _lookController.activate(id);
        if (mounted) setState(() {});
      case InstallSkinPathCommand(:final path, :final isDirectory):
        await _handleInstallSkin(path, isDirectory: isDirectory);
      case SetSkinsDirectoryCommand(:final path):
        await _lookController.setSkinsDirectory(path);
        if (mounted) setState(() {});
      case ResetSettingsCommand():
        await _handleResetSettings();
      case EqGainCommand(:final band, :final gain):
        _equalizer.setGain(band, gain);
      case EqPreampCommand(:final preamp):
        _equalizer.setPreamp(preamp);
      case EqEnabledCommand(:final enabled):
        _equalizer.setEnabled(enabled);
      case EqAutoCommand(:final enabled):
        _equalizer.setAuto(enabled);
      case ApplyPresetCommand(:final name):
        _equalizer.applyPreset(name);
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
      case ResizePlaylistCollectionCommand(:final width, :final collapsed):
        await _handlePlaylistCollectionResize(width, collapsed: collapsed);
      case AddSavedPlaylistCommand(:final path):
        await _collection.add(path);
      case RemoveSavedPlaylistCommand(:final path):
        await _collection.remove(path);
      case SelectSavedPlaylistCommand(:final path):
        _collection.select(path);
      case RenameSavedPlaylistCommand(:final path, :final name):
        await _collection.rename(path, name);
      case CreatePlaylistFromCurrentCommand(:final path):
        await _handleCreatePlaylistFromCurrent(path);
      case CreatePlaylistFromSelectionCommand(:final path):
        await _handleCreatePlaylistFromSelection(path);
      case LoadSavedPlaylistCommand(:final path):
        await _handleLoadSavedPlaylist(path);
      case MoveWindowCommand(
          :final window,
          :final left,
          :final top,
          :final shiftUndock,
          :final ended,
          :final softEnd,
        ):
        // Non-ended moves return immediately so IPC is not blocked on OS moves.
        final future = _handleDockMove(
          window,
          Offset(left, top),
          shiftUndock: shiftUndock,
          ended: ended,
          softEnd: softEnd,
        );
        if (ended) await future;
    }
  }

  Future<void> _handleUpdateGeneralSettings(
    UpdateGeneralSettingsCommand command,
  ) async {
    if (command.resumeLastSession != null) {
      _resumeLastSession = command.resumeLastSession!;
    }
    if (command.confirmBeforeQuit != null) {
      _confirmBeforeQuit = command.confirmBeforeQuit!;
    }
    if (command.scrollTitle != null) {
      _scrollTitle = command.scrollTitle!;
    }
    if (command.minimizeHidesSecondaries != null) {
      _minimizeHidesSecondaries = command.minimizeHidesSecondaries!;
    }
    if (command.dockSnapStrength != null) {
      _dockSnapStrength = command.dockSnapStrength!;
      _docking.snapThreshold = _dockSnapStrength.snapPixels;
    }
    await _persistLayout();
    if (mounted) setState(() {});
  }

  Future<void> _handleInstallSkin(
    String path, {
    required bool isDirectory,
  }) async {
    Future<LookConflictChoice> replace(_) async => LookConflictChoice.replace;
    if (isDirectory) {
      await _lookController.installDirectory(
        Directory(path),
        onConflict: replace,
      );
    } else {
      await _lookController.installZip(File(path), onConflict: replace);
    }
    if (mounted) setState(() {});
  }

  /// Preferences reset; content survives.
  ///
  /// Only `settings.json` is rewritten. Installed skins, the playlist
  /// collection index and its companion track sets, a kept altered current
  /// playlist, the lifetime spin count in `usage.json`, and the resume snapshot
  /// all live in their own files and are deliberately left alone — a listener
  /// fixing a preference must not lose what they keep, and a spin count is
  /// history rather than a preference.
  Future<void> _handleResetSettings() async {
    const next = TrampSettings.defaults;
    await _settingsStore.write(next);
    _applySettingsFields(next);
    _docking = _createDocking(DockLayout.fromSettings(next));
    await _lookController.bootstrap(
      settings: next,
      supportDir: trampSupportDirectory,
    );
    await _playback.setForceMono(_forceMono);
    await _applyAllFrames();
    await _applyAlwaysOnTop();
    if (mounted) setState(() {});
  }

  DockingCoordinator _createDocking(DockLayout layout) {
    return DockingCoordinator(
      layout,
      snapThreshold: _dockSnapStrength.snapPixels,
    );
  }

  /// Title-bar drag → [DockingCoordinator.move] → coalesce OS position applies.
  ///
  /// During a native OS drag the dragged HWND is owned by the system; we only
  /// push **docked siblings** (latest-wins, position-only) and snap on pan-end.
  /// Main carries its dock-edge cohort only — free windows stay put.
  Future<void> _handleDockMove(
    WindowId id,
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
    bool softEnd = false,
  }) async {
    _docking.move(
      id,
      logicalTopLeft,
      shiftUndock: shiftUndock,
      // Snap on every gesture end — including quiet softEnd. Linux
      // window_manager never emits onWindowMoved, so softEnd is the only
      // finalize path that can create dock edges for main-drag cohorts.
      snap: ended,
    );
    _dockDragWindow = id;

    if (ended) {
      await _dockMoveCoalescer.flush(() async {
        // Soft end on main: never fight the OS-owned HWND (drag may resume).
        // Soft end on EQ/PL: must apply snap setPosition — Linux has no
        // onWindowMoved, so this is how dock edges and HWNDs stay aligned.
        await _applyDockGroupFrames(
          id,
          positionOnly: softEnd,
          skip: softEnd && id == WindowId.main ? id : null,
        );
      });
      _dockDragWindow = null;
      _nativeDragging = false;
      await _persistLayout();
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
    _mainNativeDrag.started();
    _mainLinuxDragPoll.start(force: id != WindowId.main);
  }

  Future<Offset> _pixelPositionOfDragged() async {
    final id = _dockDragWindow ?? WindowId.main;
    if (id == WindowId.main) return windowManager.getPosition();
    return _osWindow(id)?.getPosition() ?? Offset.zero;
  }

  Future<void> _syncNativeDrag(
    WindowId id, {
    required bool ended,
    bool softEnd = false,
  }) async {
    if (!_nativeDragging && !ended) return;
    if (ended && !softEnd) {
      _mainNativeDrag.endedConfirmed();
    }
    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    final Offset pos;
    if (id == WindowId.main) {
      pos = await windowManager.getPosition();
    } else {
      pos = _osWindow(id)?.getPosition() ?? Offset.zero;
    }
    final logical = Offset(pos.dx / zoom, pos.dy / zoom);
    await _handleDockMove(
      id,
      logical,
      shiftUndock: HardwareKeyboard.instance.isShiftPressed,
      ended: ended,
      softEnd: softEnd,
    );
    if (ended) {
      _mainLinuxDragPoll.stop();
    }
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
      case 'selectRange':
        final index = command.index;
        if (index != null) _playlist.selectRange(index);
      case 'toggleSelect':
        final index = command.index;
        if (index != null) _playlist.toggleSelection(index);
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
            unawaited(_enrichMissingTrackMetadata());
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
          unawaited(_enrichMissingTrackMetadata());
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
      case 'move':
        final from = command.index;
        final to = command.toIndex;
        if (from != null && to != null) _playlist.move(from, to);
    }
  }

  /// Clicking a saved playlist loads it into the current playlist.
  ///
  /// Goes through [PlaylistController.openPlaylistFile] like every other load,
  /// so the current playlist gets its origin and the last-playlist path is
  /// persisted. Replacing the current playlist is unguarded here, matching
  /// today's behaviour for opening any playlist file.
  ///
  /// A **disabled playlist** resolves to null and the click does nothing at all
  /// — the collection module owns that judgement, and it re-checks the file so a
  /// row that has just gone missing is marked as such on the way past.
  Future<void> _handleLoadSavedPlaylist(String path) async {
    final entry = await _collection.resolveForLoad(path);
    if (entry == null) return;
    await _playlist.openPlaylistFile(entry.path);
    unawaited(_enrichMissingTrackMetadata());
  }

  /// Turns the current playlist into a **saved playlist** at [path].
  ///
  /// The write goes through [PlaylistController.savePlaylistFile] because it is
  /// the whole track list going to a file that becomes its origin: that is the
  /// one thing that lowers the altered state, and the one thing that forgets
  /// the kept altered list, so a playlist the listener just saved cannot come
  /// back altered after a restart.
  ///
  /// The reference is then kept the way an add keeps one, so the new row lands
  /// with a real count, duration, modification time, and track set — and a save
  /// over a file already in the collection updates that entry instead of
  /// twinning it.
  Future<void> _handleCreatePlaylistFromCurrent(String path) async {
    // An empty current playlist has nothing to keep; the window's create
    // control is already disabled, and this is the same answer from the host.
    if (_playlist.playlist.tracks.isEmpty) return;
    await _playlist.savePlaylistFile(path);
    await _collection.addWritten(path);
  }

  /// Pulls the selected tracks out into a **saved playlist** at [path].
  ///
  /// Deliberately the whole handler: the collection module writes the file and
  /// keeps the reference, and [_playlist] is only *read*. Nothing here can
  /// change the current playlist's tracks, its origin, or its altered state,
  /// which is the difference between this and create-from-current — the rest of
  /// the current playlist is still unsaved, so it must stay protected.
  ///
  /// Nothing is loaded either. The new entry is highlighted by `addWritten` and
  /// the listener stays exactly where they were.
  Future<void> _handleCreatePlaylistFromSelection(String path) async {
    // No selection, nothing to pull out. The window's control is already
    // disabled; this is the same answer from the host.
    if (_playlist.selectedIndices.isEmpty) return;
    await _collection.createFromSelection(
      path,
      _playlist.playlist.tracks,
      _playlist.selectedIndices,
    );
  }

  Future<void> _handlePlaylistResize(double width, double height) async {
    final w = width.clamp(TrampMetrics.playlistMin.width, 4000.0);
    final h = height.clamp(TrampMetrics.playlistMin.height, 4000.0);
    final current = _docking.layout.playlist;
    final logicalH = current.shaded
        ? (current.height ?? TrampMetrics.playlistDefault.height)
        : h;
    _docking.resizePlaylist(Size(w, logicalH));
    await _persistLayout();
    // Do not re-push the playlist frame — the OS window already has this size.
  }

  /// Divider drag / collapse toggle for the Playlist Manager collection panel.
  ///
  /// The panel keeps [TrampMetrics.playlistCollectionMinWidth] and the track
  /// list keeps [TrampMetrics.playlistMin] width, so the footer controls can
  /// never be squeezed into overflow.
  Future<void> _handlePlaylistCollectionResize(
    double width, {
    required bool collapsed,
  }) async {
    final frame = _docking.layout.playlist;
    final windowWidth = frame.width ?? TrampMetrics.playlistDefault.width;
    final widest = math.max(
      TrampMetrics.playlistCollectionMinWidth,
      windowWidth -
          TrampMetrics.playlistDividerWidth -
          TrampMetrics.playlistMin.width,
    );
    _playlistCollectionWidth = width.clamp(
      TrampMetrics.playlistCollectionMinWidth,
      widest,
    );
    _playlistCollectionCollapsed = collapsed;
    await _persistLayout();
    if (mounted) setState(() {});
  }

  /// Sends the About window a fresh reading, but only when there is one to
  /// send and someone to read it.
  ///
  /// Three guards, and each earns its keep:
  ///
  /// * The window must be **open**. The deduplicated figures cost a read of the
  ///   companion track-set file, and the About window is hidden at launch, so
  ///   this is what keeps that file off the startup path — it is read when the
  ///   figures are wanted and not before.
  /// * The collection's [PlaylistCollectionController.figuresRevision] must
  ///   have moved. It moves on the four events that can change a figure — a
  ///   playlist added, saved, removed, or found changed on disk — and not on a
  ///   row being highlighted, which is far more frequent.
  /// * Spins must have moved. Playback notifies on every position tick, and
  ///   only end-of-stream changes the count.
  Future<void> _refreshAboutStats() async {
    if (!_aboutReady || !_docking.layout.about.visible) return;
    final revision = _collection.figuresRevision;
    final spins = _playback.spins;
    if (revision == _aboutFiguresRevision && spins == _aboutSpins) return;
    _aboutSpins = spins;
    if (revision != _aboutFiguresRevision) {
      _aboutFiguresRevision = revision;
      _aboutFigures = await _collection.readFigures();
    }
    if (mounted) setState(() {});
  }

  Future<void> _stepZoom(int delta) async {
    final steps = TrampSettings.validZoomPercents;
    final index = steps.indexOf(_zoomPercent);
    final nextIndex = (index < 0 ? 0 : index) + delta;
    if (nextIndex < 0 || nextIndex >= steps.length) return;
    final fromZoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    _zoomPercent = steps[nextIndex];
    final toZoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    // Logical frames are zoom-scaled for window_manager; rebase TLs so free
    // windows keep their screen corner and docked ones stay flush to partners.
    _docking.reanchorForZoom(fromZoom: fromZoom, toZoom: toZoom);
    await _applyAllFrames();
    await _persistLayout();
    if (mounted) setState(() {});
  }

  WindowRole _roleFor(WindowId id) => switch (id) {
        WindowId.main => WindowRole.main,
        WindowId.equalizer => WindowRole.equalizer,
        WindowId.playlist => WindowRole.playlist,
        WindowId.settings => WindowRole.settings,
        WindowId.about => WindowRole.about,
      };

  Future<void> _applyAllFrames() async {
    await _applyMainFrame();
    if (_eqReady) await _applyRoleFrame(WindowRole.equalizer);
    if (_playlistReady) await _applyRoleFrame(WindowRole.playlist);
    if (_settingsReady) await _applyRoleFrame(WindowRole.settings);
    if (_aboutReady) await _applyRoleFrame(WindowRole.about);
  }

  Future<void> _runPositionBench() async {
    const n = 80;
    Future<List<int>> timeOps(Future<void> Function() op) async {
      final times = <int>[];
      for (var i = 0; i < n; i++) {
        final sw = Stopwatch()..start();
        await op();
        times.add(sw.elapsedMicroseconds);
      }
      times.sort();
      return times;
    }

    final gets = await timeOps(() async {
      await windowManager.getPosition();
    });
    final sets = await timeOps(() async {
      await windowManager.setPosition(const Offset(200, 200));
    });
    int pct(List<int> xs, int p) => xs[(n * p) ~/ 100];
    stderr.writeln(
      'TRAMP_POS_BENCH get_us p50=${pct(gets, 50)} p95=${pct(gets, 95)} '
      'max=${gets.last}',
    );
    stderr.writeln(
      'TRAMP_POS_BENCH set_us p50=${pct(sets, 50)} p95=${pct(sets, 95)} '
      'max=${sets.last}',
    );
    await stderr.flush();
  }

  Future<void> _applyMainFrame({bool positionOnly = false}) async {
    final zoom = _zoomPercent / 100.0;
    final rect = _docking.frameFor(WindowId.main, zoom);
    if (positionOnly) {
      await windowManager.setPosition(rect.topLeft);
      return;
    }
    final visible = sessionWindowShouldShow(
      sessionReady: _revealWindows,
      layoutVisible: _docking.layout.main.visible,
    );
    await resizeTrampWindow(
      size: rect.size,
      minimumSize: rect.size,
      pinSize: true,
    );
    await windowManager.setPosition(rect.topLeft);
    await windowManager.setAlwaysOnTop(
      effectiveAlwaysOnTop(
        alwaysOnTop: _alwaysOnTop,
        visible: visible,
      ),
    );
    if (visible) {
      await windowManager.setSkipTaskbar(false);
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
      settingsVisible: layout.settings.visible,
      aboutVisible: layout.about.visible,
    );
    await windowManager.setAlwaysOnTop(targets.contains(WindowId.main));
    // Secondaries apply AOT via apply_frame (visible ∩ global flag).
    if (_eqReady) await _applyRoleFrame(WindowRole.equalizer);
    if (_playlistReady) await _applyRoleFrame(WindowRole.playlist);
    if (_settingsReady) await _applyRoleFrame(WindowRole.settings);
    if (_aboutReady) await _applyRoleFrame(WindowRole.about);
  }

  Future<void> _applyRoleFrame(
    WindowRole role, {
    bool positionOnly = false,
  }) async {
    if (role == WindowRole.main) {
      await _applyMainFrame(positionOnly: positionOnly);
      return;
    }
    final window = switch (role) {
      WindowRole.equalizer => _equalizerWindow,
      WindowRole.playlist => _playlistWindow,
      WindowRole.settings => _settingsWindow,
      WindowRole.about => _aboutWindow,
      WindowRole.main => null,
    };
    if (window == null) return;
    if (role == WindowRole.equalizer && !_eqReady) return;
    if (role == WindowRole.playlist && !_playlistReady) return;
    if (role == WindowRole.settings && !_settingsReady) return;
    if (role == WindowRole.about && !_aboutReady) return;

    final id = switch (role) {
      WindowRole.equalizer => WindowId.equalizer,
      WindowRole.playlist => WindowId.playlist,
      WindowRole.settings => WindowId.settings,
      WindowRole.about => WindowId.about,
      WindowRole.main => WindowId.main,
    };
    final zoom = _zoomPercent / 100.0;
    final rect = _docking.frameFor(id, zoom);
    final visible = _docking.layout.frameOf(id).visible;
    final show = sessionWindowShouldShow(
      sessionReady: _revealWindows,
      layoutVisible: visible,
      minimizeSuppressed: _minimizeGroup.shouldSuppressShow(id),
    );
    try {
      window.applyFrame(
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
      if (id == WindowId.playlist && !positionOnly) {
        _lastPlaylistPixelSize = rect.size;
      }
    } catch (error, stack) {
      debugPrint('SessionHost applyFrame($role) failed: $error\n$stack');
    }
  }

  /// Main title-bar minimize → hide visible secondaries, then OS-minimize main.
  Future<void> _minimizeVisibleGroup() async {
    await _hideVisibleSecondariesForMinimize();
    await windowManager.minimize();
  }

  /// Snapshot + OS-hide currently visible EQ/PL/settings (layout unchanged).
  Future<void> _hideVisibleSecondariesForMinimize() async {
    if (!_minimizeHidesSecondaries) return;
    final layout = _docking.layout;
    final toHide = _minimizeGroup.begin(
      equalizerVisible: layout.equalizer.visible,
      playlistVisible: layout.playlist.visible,
      settingsVisible: layout.settings.visible,
      aboutVisible: layout.about.visible,
    );
    await _setSecondariesHidden(toHide, hidden: true);
  }

  /// Main restore → best-effort show secondaries that were visible at minimize.
  Future<void> _restoreVisibleGroup() async {
    final layout = _docking.layout;
    final toShow = _minimizeGroup.end(
      equalizerVisible: layout.equalizer.visible,
      playlistVisible: layout.playlist.visible,
      settingsVisible: layout.settings.visible,
      aboutVisible: layout.about.visible,
    );
    await _setSecondariesHidden(toShow, hidden: false);
  }

  Future<void> _setSecondariesHidden(
    Set<WindowId> ids, {
    required bool hidden,
  }) async {
    for (final id in ids) {
      final window = _osWindow(id);
      if (window == null) continue;
      try {
        if (hidden) {
          window.native.hide();
        } else {
          window.native.show();
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
        settings: layout.settings,
        about: layout.about,
        dockEdges: layout.dockEdges,
        resumeLastSession: _resumeLastSession,
        confirmBeforeQuit: _confirmBeforeQuit,
        scrollTitle: _scrollTitle,
        minimizeHidesSecondaries: _minimizeHidesSecondaries,
        dockSnapStrength: _dockSnapStrength,
        playlistCollectionWidth: _playlistCollectionWidth,
        playlistCollectionCollapsed: _playlistCollectionCollapsed,
      ),
    );
  }

  SettingsSnapshotEvent _settingsSnapshot() {
    return SettingsSnapshotEvent(
      resumeLastSession: _resumeLastSession,
      confirmBeforeQuit: _confirmBeforeQuit,
      scrollTitle: _scrollTitle,
      minimizeHidesSecondaries: _minimizeHidesSecondaries,
      dockSnapStrength: _dockSnapStrength,
      skins: [
        for (final m in _lookController.installed)
          SkinCatalogEntry(id: m.id, name: m.name, author: m.author),
      ],
      activeSkinId: _lookController.activeSkinId,
      lastSkinError: _lookController.lastError,
      playlistCollectionWidth: _playlistCollectionWidth,
      playlistCollectionCollapsed: _playlistCollectionCollapsed,
    );
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
    unawaited(_enrichMissingTrackMetadata());
    if (_playback.currentTrack == null) {
      await _playback.playIndex(_playlist.playlist.tracks.length - tracks.length);
    }
  }

  /// Background-fill durations (and tags) so PL TOTAL / row times are real.
  Future<void> _enrichMissingTrackMetadata() async {
    final pending = _playlist.playlist.tracks
        .where((t) => t.duration == null || t.duration! <= Duration.zero)
        .toList();
    for (final track in pending) {
      // Track may have been removed while earlier probes ran.
      if (!_playlist.playlist.tracks.any((t) => t.path == track.path)) {
        continue;
      }
      final enriched = await _trackProbe.enrich(track);
      if (enriched == track) continue;
      _playlist.updateTrackByPath(track.path, (_) => enriched);
    }
  }

  Future<void> _handleOptionsAction(BuildContext context, String action) async {
    switch (action) {
      case 'settings':
        final visible = !_docking.layout.settings.visible;
        await _onCommand(
          ToggleWindowCommand(window: WindowId.settings, visible: visible),
        );
      case 'info':
        await _showTrackInfo(context);
      case 'about':
        if (_docking.layout.about.visible) {
          await _raiseAbout();
        } else {
          await _onCommand(
            ToggleWindowCommand(window: WindowId.about, visible: true),
          );
        }
      case 'quit':
        await _quit(context: context);
    }
  }

  Future<void> _showTrackInfo(BuildContext context) async {
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
      builder: (context) {
        final palette = LookScope.of(context).palette;
        return AlertDialog(
          backgroundColor: palette.shellMid,
          title: Text(
            'Track info',
            style: TextStyle(color: palette.inkDefault),
          ),
          content: Text(
            message,
            style: TextStyle(color: palette.inkDim),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('OK'),
            ),
          ],
        );
      },
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

  /// Show+focus visible EQ/PL, refocus main, then order settings on top.
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
          _equalizerWindow!.raise();
        } catch (error, stack) {
          debugPrint('SessionHost raise(eq) failed: $error\n$stack');
        }
      }
      if (layout.playlist.visible &&
          _playlistReady &&
          _playlistWindow != null &&
          !_minimizeGroup.shouldSuppressShow(WindowId.playlist)) {
        try {
          _playlistWindow!.raise();
        } catch (error, stack) {
          debugPrint('SessionHost raise(pl) failed: $error\n$stack');
        }
      }
      await windowManager.focus();
      // Settings stays above other Tramp windows without stealing focus.
      await _orderSettingsOnTop();
    } finally {
      // Defer clear so the focus() echo does not re-enter immediately.
      Future<void>.delayed(const Duration(milliseconds: 100), () {
        _raisingFocusGroup = false;
      });
    }
  }

  Future<void> _orderSettingsOnTop() async {
    final layout = _docking.layout;
    if (!layout.settings.visible ||
        !_settingsReady ||
        _settingsWindow == null ||
        _minimizeGroup.shouldSuppressShow(WindowId.settings)) {
      return;
    }
    try {
      _settingsWindow!.raise(focus: false);
    } catch (error, stack) {
      debugPrint('SessionHost orderTop(settings) failed: $error\n$stack');
    }
  }

  Future<void> _raiseAbout() async {
    final layout = _docking.layout;
    if (!layout.about.visible ||
        !_aboutReady ||
        _aboutWindow == null ||
        _minimizeGroup.shouldSuppressShow(WindowId.about)) {
      return;
    }
    try {
      _aboutWindow!.raise();
    } catch (error, stack) {
      debugPrint('SessionHost raise(about) failed: $error\n$stack');
    }
  }

  @override
  void didChangeMetrics() {
    if (_nativeDragging) return;
    _playlistResizeDebounce?.cancel();
    _playlistResizeDebounce = Timer(const Duration(milliseconds: 120), () {
      unawaited(_persistPlaylistSizeFromOs());
    });
  }

  Future<void> _persistPlaylistSizeFromOs() async {
    final os = _playlistWindow;
    if (os == null || !_playlistReady) return;
    if (!_docking.layout.playlist.visible) return;
    final zoom = (_zoomPercent / 100.0).clamp(0.5, 4.0);
    final px = os.getSize();
    final last = _lastPlaylistPixelSize;
    if (last != null &&
        (px.width - last.width).abs() < 2 &&
        (px.height - last.height).abs() < 2) {
      return;
    }
    _lastPlaylistPixelSize = px;
    await _handlePlaylistResize(px.width / zoom, px.height / zoom);
  }

  void _scheduleCollectionWidth(double width) {
    _pendingCollectionWidth = width;
    _collectionWidthDebounce?.cancel();
    _collectionWidthDebounce = Timer(const Duration(milliseconds: 120), () {
      unawaited(
        _handlePlaylistCollectionResize(
          _pendingCollectionWidth ?? _playlistCollectionWidth,
          collapsed: _playlistCollectionCollapsed,
        ),
      );
    });
  }

  @override
  void onWindowMove() {
    if (_dockDragWindow != null && _dockDragWindow != WindowId.main) return;
    // Tracker gates + arms quiet end; also resumes after a soft quiet finalize.
    if (!_mainNativeDrag.onMoveEvent()) return;
    _nativeDragging = true;
    _dockDragWindow = WindowId.main;
    if (LinuxDragPoll.isNeeded && !_mainLinuxDragPoll.isRunning) {
      _mainLinuxDragPoll.start();
    }
    _nativeSyncCoalescer.schedule(
      () => _syncNativeDrag(WindowId.main, ended: false),
    );
  }

  @override
  void onWindowMoved() {
    if (_dockDragWindow != null && _dockDragWindow != WindowId.main) return;
    if (!_nativeDragging && !_mainNativeDrag.isActive && !_mainNativeDrag.softEnded) {
      return;
    }
    _mainLinuxDragPoll.stop();
    _mainNativeDrag.endedConfirmed();
    _nativeDragging = true;
    _dockDragWindow = WindowId.main;
    unawaited(
      _nativeSyncCoalescer.flush(
        () => _syncNativeDrag(WindowId.main, ended: true),
      ),
    );
  }

  @override
  void onWindowClose() {
    unawaited(_quit());
  }

  Future<void> _quit({BuildContext? context}) async {
    if (_confirmBeforeQuit) {
      final ctx = context ?? (mounted ? this.context : null);
      if (ctx != null && ctx.mounted) {
        final look = LookScope.of(ctx);
        final palette = look.palette;
        final ok = await showDialog<bool>(
          context: ctx,
          builder: (dialogContext) => AlertDialog(
            backgroundColor: palette.shellMid,
            title: Text(
              'Quit Tramp?',
              style: TextStyle(color: palette.inkDefault),
            ),
            content: Text(
              'Are you sure you want to quit?',
              style: TextStyle(color: palette.inkDim),
            ),
            actions: [
              TextButton(
                onPressed: () => Navigator.of(dialogContext).pop(false),
                child: const Text('Cancel'),
              ),
              TextButton(
                onPressed: () => Navigator.of(dialogContext).pop(true),
                child: const Text('Quit'),
              ),
            ],
          ),
        );
        if (ok != true) return;
      }
    }
    // Conceal chrome immediately. Persist, then `_exit` the whole process.
    for (final window in [
      _equalizerWindow,
      _playlistWindow,
      _settingsWindow,
      _aboutWindow,
    ]) {
      if (window == null) continue;
      try {
        window.native.hide();
      } catch (_) {}
    }
    unawaited(() async {
      try {
        await windowManager.hide();
      } catch (_) {}
    }());
    await finishSessionQuit(persist: _persistOnQuit);
  }

  @override
  void dispose() {
    _resumeSaveTimer?.cancel();
    _playlistResizeDebounce?.cancel();
    _collectionWidthDebounce?.cancel();
    _mainLinuxDragPoll.dispose();
    _mainNativeDrag.dispose();
    WidgetsBinding.instance.removeObserver(this);
    windowManager.removeListener(this);
    _lookController.removeListener(_onLookChanged);
    _lookController.dispose();
    _equalizer.dispose();
    _playlist.removeListener(_onPlaylistChanged);
    _collection.removeListener(_onCollectionChanged);
    _collection.dispose();
    _playback.removeListener(_onPlaybackChanged);
    final probe = _trackProbe;
    if (probe is MediaKitTrackMetadataProbe) {
      unawaited(probe.dispose());
    }
    _equalizerWindow?.destroy();
    _playlistWindow?.destroy();
    _settingsWindow?.destroy();
    _aboutWindow?.destroy();
    unawaited(_playback.dispose());
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final layout = _docking.layout;
    final zoom = _zoomPercent / 100.0;
    final main = MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      color: trampWindowFill(),
      builder: (context, child) => ListenableBuilder(
        listenable: _lookController,
        builder: (context, _) => LookScope(
          look: _lookController.resolved,
          child: child ?? const SizedBox.shrink(),
        ),
      ),
      home: ColoredBox(
        color: trampWindowFill(),
        child: !_bootstrapped
            ? const SizedBox.shrink()
            : ZoomedCanvas(
                factor: zoom,
                logicalSize: TrampMetrics.mainPlayer,
                child: MainPlayerWindow(
                  playback: _playback,
                  trackCount: _playlist.playlist.tracks.length,
                  forceMono: _forceMono,
                  alwaysOnTop: _alwaysOnTop,
                  equalizerVisible: layout.equalizer.visible,
                  playlistVisible: layout.playlist.visible,
                  scrollTitle: _scrollTitle,
                  zoom: zoom,
                  dockLogicalTopLeft: () => Offset(
                    _docking.layout.main.left,
                    _docking.layout.main.top,
                  ),
                  onDockMove: (topLeft, {required shiftUndock, required ended}) {
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
                  onOptionsAction: (context, action) =>
                      unawaited(_handleOptionsAction(context, action)),
                  onMinimize: () => unawaited(_minimizeVisibleGroup()),
                  onZoomOut: () => unawaited(_stepZoom(-1)),
                  onZoomIn: () => unawaited(_stepZoom(1)),
                  onClose: () => unawaited(_quit()),
                ),
              ),
      ),
    );
    final extras = <Widget>[
      for (final view in [
        _equalizerView(),
        _playlistView(),
        _settingsView(),
        _aboutView(),
      ])
        if (view != null) view,
    ];
    if (extras.isEmpty) return main;
    return ViewAnchor(
      view: ViewCollection(views: extras),
      child: main,
    );
  }
}
