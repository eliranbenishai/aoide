import 'dart:async';

import 'package:flutter/material.dart';
import 'package:media_kit/media_kit.dart';
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';
import 'package:window_manager/window_manager.dart';

import 'eq/equalizer_controller.dart';
import 'eq/mpv_equalizer_sink.dart';
import 'platform/file_open.dart';
import 'platform/launch_args.dart';
import 'platform/os_media_controls.dart';
import 'platform/settings_store.dart';
import 'platform/tramp_window.dart';
import 'playback/media_kit_player_engine.dart';
import 'playback/playback_controller.dart';
import 'playback/player_engine.dart';
import 'playlist/playlist_controller.dart';
import 'playlist/playlist_store.dart';
import 'theme/tramp_metrics.dart';
import 'theme/tramp_theme.dart';
import 'ui/chrome/about_dialog.dart';
import 'ui/equalizer/equalizer_panel.dart';
import 'ui/lower_region.dart';
import 'ui/main_player/main_player_panel.dart';
import 'ui/playlist_panel.dart';
import 'ui/tramp_shell.dart';
import 'ui/window_layout.dart';
import 'ui/zoom/zoom_controller.dart';

/// Matches `pubspec.yaml` `version` (semver before `+build`).
const String trampAppVersion = '0.1.0';

class TrampApp extends StatefulWidget {
  const TrampApp({
    super.key,
    this.launchArgs = const [],
    this.engine,
    this.osMediaControls,
    this.settingsStore,
  });

  final List<String> launchArgs;

  final PlayerEngine? engine;

  final OsMediaControls? osMediaControls;

  final SettingsStore? settingsStore;

  @override
  State<TrampApp> createState() => _TrampAppState();
}

class _TrampAppState extends State<TrampApp> with WindowListener {
  late final PlaylistController _playlist;
  late final PlaybackController _playback;
  late final OsMediaControls _osMediaControls;
  late final SettingsStore _settingsStore;
  late final ZoomController _zoom;
  late final EqualizerController _equalizer;
  LowerRegion _lowerRegion = LowerRegion.playlist;
  bool _equalizerCollapsed = false;
  double? _playlistWindowWidth;
  double? _playlistWindowHeight;
  Timer? _resizePersistDebounce;
  final FocusNode _playlistFocusNode = FocusNode();

  @override
  void initState() {
    super.initState();

    final store = FilePlaylistStore(
      supportDir: getApplicationSupportDirectory,
    );

    _playlist = PlaylistController(store: store);

    final Player? sharedPlayer =
        widget.engine == null ? Player() : null;
    _playback = PlaybackController(
      playlist: _playlist,
      engine: widget.engine ??
          MediaKitPlayerEngine(
            player: sharedPlayer,
            onMetadata: (path, update) {
              final tracks = List.of(_playlist.playlist.tracks);

              final index = tracks.indexWhere((track) => track.path == path);

              if (index < 0) return;

              tracks[index] = update(tracks[index]);

              _playlist.setTracks(
                tracks,
                sourcePath: _playlist.playlist.sourcePath,
              );
            },
          ),
    );

    _osMediaControls = widget.osMediaControls ?? createOsMediaControls();
    _settingsStore = widget.settingsStore ??
        FileSettingsStore(supportDir: getApplicationSupportDirectory);
    _zoom = ZoomController(
      workArea: const Size(1920, 1080),
      onPercentChanged: _onZoomChanged,
    );
    _equalizer = EqualizerController(
      store: _settingsStore,
      sink: sharedPlayer != null
          ? MpvEqualizerSink(sharedPlayer)
          : const NoopEqualizerSink(),
    );
    windowManager.addListener(this);
    unawaited(_osMediaControls.start(_playback));
    unawaited(_bootstrapPlaylist());
    unawaited(_restoreSettings());
  }

  Future<void> _bootstrapPlaylist() async {
    final action = parseLaunchArgs(widget.launchArgs);
    if (action.openPlaylist != null || action.openTracks.isNotEmpty) {
      await _applyLaunchAction(action);
      return;
    }

    await _playlist.restoreLastPlaylist();
  }

  Future<void> _applyLaunchAction(LaunchAction action) async {
    if (action.openPlaylist != null) {
      await _playlist.openPlaylistFile(action.openPlaylist!);
      return;
    }

    if (action.openTracks.isEmpty) return;

    _playlist.setTracks(tracksFromPaths(action.openTracks));
    await _playback.playIndex(0);
  }

  Future<void> _restoreSettings() async {
    final settings = await _settingsStore.read();
    await _equalizer.load();
    if (!mounted) return;
    setState(() {
      // Interim shell: map multi-window visibility → single lower region.
      _lowerRegion = settings.equalizer.visible && !settings.playlist.visible
          ? LowerRegion.equalizer
          : LowerRegion.playlist;
      _equalizerCollapsed = settings.equalizer.shaded;
      // Assigned before setPercent: a restored zoom step fires
      // _onZoomChanged, which applies the window mode using these values.
      _playlistWindowWidth = settings.playlist.width;
      _playlistWindowHeight = settings.playlist.height;
      _zoom.setPercent(settings.zoomPercent);
    });
    // setPercent no-ops when the restored step is already current, so the
    // startup snap to the restored region/size has to happen explicitly.
    unawaited(_applyWindowMode());
  }

  Future<void> _persistSettings() async {
    final current = await _settingsStore.read();
    final showEq = _lowerRegion == LowerRegion.equalizer;
    await _settingsStore.write(
      current.copyWith(
        zoomPercent: _zoom.percent,
        equalizer: current.equalizer.copyWith(
          visible: showEq,
          shaded: _equalizerCollapsed,
        ),
        playlist: current.playlist.copyWith(visible: !showEq),
      ),
    );
  }

  void _onZoomChanged(int percent) {
    unawaited(_applyWindowMode());
    unawaited(_persistSettings());
  }

  /// Snaps the live window to the current mode's target (ADR 0003): the fixed
  /// EQ stack in equalizer mode, the restored/default size with free resize in
  /// playlist mode.
  Future<void> _applyWindowMode() async {
    final target = windowModeTarget(
      lowerRegion: _lowerRegion,
      factor: _zoom.factor,
      storedPlaylistWidth: _playlistWindowWidth,
      storedPlaylistHeight: _playlistWindowHeight,
      equalizerCollapsed: _equalizerCollapsed,
    );
    await setTrampWindowResizable(target.resizable);
    await resizeTrampWindow(size: target.size, minimumSize: target.minimumSize);
  }

  void _selectRegion(LowerRegion region) {
    final previousRegion = _lowerRegion;
    final previousCollapsed = _equalizerCollapsed;
    setState(() {
      // Tapping the visible region's own button toggles the equalizer's
      // windowshade rather than doing nothing.
      if (region == _lowerRegion && region == LowerRegion.equalizer) {
        _equalizerCollapsed = !_equalizerCollapsed;
      } else {
        _lowerRegion = region;
        _equalizerCollapsed = false;
      }
    });
    unawaited(_persistSettings());
    // Region or windowshade changes both alter the target window height.
    if (region != previousRegion || _equalizerCollapsed != previousCollapsed) {
      unawaited(_applyWindowMode());
    }
  }

  /// Toggles the equalizer windowshade and snaps the window to the new height.
  void _toggleEqualizerCollapsed() {
    setState(() => _equalizerCollapsed = !_equalizerCollapsed);
    unawaited(_persistSettings());
    if (_lowerRegion == LowerRegion.equalizer) {
      unawaited(_applyWindowMode());
    }
  }

  @override
  void onWindowResized() {
    // Windows/macOS: fired once when the resize drag ends.
    _resizePersistDebounce?.cancel();
    unawaited(_persistPlaylistWindowSize());
  }

  @override
  void onWindowResize() {
    // Linux has no end-of-resize event; a quiet spell after live resize
    // events stands in for it.
    _resizePersistDebounce?.cancel();
    _resizePersistDebounce = Timer(
      const Duration(milliseconds: 400),
      () => unawaited(_persistPlaylistWindowSize()),
    );
  }

  Future<void> _persistPlaylistWindowSize() async {
    if (_lowerRegion != LowerRegion.playlist) return;
    final size = await windowManager.getSize();
    // Stored logical (zoom-independent) so the size restores proportionally
    // at any zoom step.
    final logical = logicalPlaylistWindowSize(size, _zoom.factor);
    _playlistWindowWidth = logical.width;
    _playlistWindowHeight = logical.height;
    final current = await _settingsStore.read();
    await _settingsStore.write(
      current.copyWith(
        playlist: current.playlist.copyWith(
          width: logical.width,
          height: logical.height,
        ),
      ),
    );
  }

  @override
  void dispose() {
    _resizePersistDebounce?.cancel();
    windowManager.removeListener(this);
    _playlistFocusNode.dispose();
    unawaited(_osMediaControls.stop());
    unawaited(_playback.dispose());

    super.dispose();
  }

  Future<void> _openFiles() async {
    final paths = await pickAudioFiles();
    if (paths == null || paths.isEmpty) return;
    _playlist.addTracks(tracksFromPaths(paths));
  }

  Future<void> _openFolder() async {
    final path = await pickFolder();
    if (path == null) return;
    _playlist.addTracks(tracksFromPaths([path]));
  }

  Future<void> _openPlaylist() async {
    final path = await pickPlaylistFile();

    if (path == null) return;

    await _playlist.openPlaylistFile(path);
  }

  Future<void> _savePlaylist() async {
    var path = await pickSavePlaylistPath();

    if (path == null) return;

    if (!isPlaylistPath(path)) {
      path = p.setExtension(path, '.m3u');
    }

    await _playlist.savePlaylistFile(path);
  }

  Future<void> _addFiles(BuildContext context) async {
    final action = await showMenu<String>(
      context: context,
      position: const RelativeRect.fromLTRB(200, 200, 0, 0),
      items: const [
        PopupMenuItem(value: 'files', child: Text('Add files…')),
        PopupMenuItem(value: 'folder', child: Text('Add folder…')),
      ],
    );

    if (action == 'folder') {
      await _openFolder();
      return;
    }

    if (action != 'files') return;

    await _openFiles();
  }

  Future<void> _handleDroppedPaths(List<String> paths) async {
    if (paths.length == 1 && isPlaylistPath(paths.single)) {
      await _playlist.openPlaylistFile(paths.single);

      return;
    }

    _playlist.addTracks(tracksFromPaths(paths));
  }

  void _showMainMenu(BuildContext context) {
    final overlay =
        Overlay.of(context).context.findRenderObject()! as RenderBox;
    unawaited(showMenu<String>(
      context: context,
      position: RelativeRect.fromLTRB(
        TrampMetrics.frame,
        TrampMetrics.frame + TrampMetrics.titleBar,
        overlay.size.width,
        0,
      ),
      items: const [
        PopupMenuItem(value: 'files', child: Text('Open files…')),
        PopupMenuItem(value: 'folder', child: Text('Open folder…')),
        PopupMenuItem(value: 'playlist', child: Text('Open playlist…')),
        PopupMenuItem(value: 'save', child: Text('Save playlist…')),
        PopupMenuDivider(),
        PopupMenuItem(value: 'about', child: Text('About Tramp…')),
        PopupMenuItem(value: 'quit', child: Text('Exit')),
      ],
    ).then((choice) async {
      switch (choice) {
        case 'files':
          await _openFiles();
        case 'folder':
          await _openFolder();
        case 'playlist':
          await _openPlaylist();
        case 'save':
          await _savePlaylist();
        case 'about':
          if (context.mounted) {
            await showTrampAboutDialog(context, version: trampAppVersion);
          }
        case 'quit':
          await windowManager.close();
      }
    }));
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      theme: buildTrampTheme(),
      home: ListenableBuilder(
        listenable: Listenable.merge([_playlist, _zoom]),
        builder: (context, _) {
          final hasTracks = _playlist.playlist.tracks.isNotEmpty;
          return TrampShell(
            playback: _playback,
            playlistController: _playlist,
            hasTracks: hasTracks,
            playlistFocusNode: _playlistFocusNode,
            onDropPaths: _handleDroppedPaths,
            onOpenFiles: _openFiles,
            onSavePlaylist: _savePlaylist,
            zoom: _zoom,
            lowerRegion: _lowerRegion,
            equalizerCollapsed: _equalizerCollapsed,
            mainPlayer: MainPlayerPanel(
              playback: _playback,
              zoom: _zoom,
              lowerRegion: _lowerRegion,
              hasTracks: hasTracks,
              onSelectRegion: _selectRegion,
              onOpenFiles: () => unawaited(_openFiles()),
              onOpenMenu: () => _showMainMenu(context),
            ),
            equalizer: EqualizerPanel(
              controller: _equalizer,
              collapsed: _equalizerCollapsed,
              onCollapse: _toggleEqualizerCollapsed,
              onClose: () => _selectRegion(LowerRegion.playlist),
            ),
            playlist: PlaylistPanel(
              playlist: _playlist,
              playback: _playback,
              onOpen: _openPlaylist,
              onSave: _savePlaylist,
              onAddFiles: () => _addFiles(context),
            ),
          );
        },
      ),
    );
  }
}
