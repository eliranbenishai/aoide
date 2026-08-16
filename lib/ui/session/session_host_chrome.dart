part of 'session_host.dart';

extension on _SessionHostAppState {
  OsWindow? _osWindow(WindowId id) => switch (id) {
        WindowId.equalizer => _equalizerWindow,
        WindowId.playlist => _playlistWindow,
        WindowId.settings => _settingsWindow,
        WindowId.about => _aboutWindow,
        WindowId.main => null,
      };

  void Function(
    Offset logicalTopLeft, {
    required bool shiftUndock,
    required bool ended,
  }) _dockMove(WindowId id) {
    return (topLeft, {required shiftUndock, required ended}) {
      unawaited(
        _handleDockMove(
          id,
          topLeft,
          shiftUndock: shiftUndock,
          ended: ended,
        ),
      );
    };
  }

  ValueGetter<Offset> _dockOrigin(WindowId id) {
    return () {
      final frame = _docking.layout.frameOf(id);
      return Offset(frame.left, frame.top);
    };
  }

  Future<void> _startOsDrag(WindowId id) {
    _osWindow(id)?.startDrag();
    return Future<void>.value();
  }

  Future<void> _startPlaylistResize(ResizeEdge edge) {
    _playlistWindow?.startResize(edge);
    return Future<void>.value();
  }

  AboutStats get _aboutStats {
    if (_aboutFiguresRevision < 0) return AboutStats.unmeasured;
    return AboutStats(
      playlists: _aboutFigures.playlists,
      tracks: _aboutFigures.tracks,
      totalDuration: _aboutFigures.totalDuration,
      spins: _playback.spins,
    );
  }

  Widget _wrapSecondary({
    required OsWindow window,
    required Size? logicalSize,
    required Widget child,
  }) {
    final zoom = _zoomPercent / 100.0;
    return window.attach(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        color: trampWindowFill(),
        builder: (context, appChild) => ListenableBuilder(
          listenable: _lookController,
          builder: (context, _) => LookScope(
            look: _lookController.resolved,
            child: appChild ?? const SizedBox.shrink(),
          ),
        ),
        home: ColoredBox(
          color: trampWindowFill(),
          child: logicalSize == null
              ? child
              : ZoomedCanvas(
                  factor: zoom,
                  logicalSize: logicalSize,
                  child: child,
                ),
        ),
      ),
    );
  }

  Widget? _equalizerView() {
    final window = _equalizerWindow;
    if (window == null) return null;
    final layout = _docking.layout;
    final zoom = _zoomPercent / 100.0;
    return _wrapSecondary(
      window: window,
      logicalSize: TrampMetrics.equalizer,
      child: ListenableBuilder(
        listenable: _equalizer,
        builder: (context, _) => EqualizerWindow(
          settings: _equalizer.settings,
          shaded: layout.equalizer.shaded,
          presetNames: _equalizer.presetNames,
          zoom: zoom,
          dockLogicalTopLeft: _dockOrigin(WindowId.equalizer),
          onDockMove: _dockMove(WindowId.equalizer),
          onNativeDragStarted: () => _onNativeDragStarted(WindowId.equalizer),
          startDragging: () => _startOsDrag(WindowId.equalizer),
          onSessionCommand: (cmd) => unawaited(_onCommand(cmd)),
          onCollapse: () => unawaited(
            _onCommand(
              SetShadedCommand(
                window: WindowId.equalizer,
                shaded: !layout.equalizer.shaded,
              ),
            ),
          ),
          onClose: () => unawaited(
            _onCommand(
              ToggleWindowCommand(window: WindowId.equalizer, visible: false),
            ),
          ),
        ),
      ),
    );
  }

  Widget? _playlistView() {
    final window = _playlistWindow;
    if (window == null) return null;
    final layout = _docking.layout;
    final zoom = _zoomPercent / 100.0;
    return _wrapSecondary(
      window: window,
      logicalSize: null,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final logical = Size(
            constraints.maxWidth / zoom,
            constraints.maxHeight / zoom,
          );
          return ZoomedCanvas(
            factor: zoom,
            child: ListenableBuilder(
              listenable: Listenable.merge([
                _playlist,
                _collection,
                _playback,
              ]),
              builder: (context, _) => PlaylistWindow(
                playlist: _playlist,
                size: logical,
                shaded: layout.playlist.shaded,
                playingIndex: _playback.playingIndex,
                playing: _playback.playing,
                collectionWidth: _playlistCollectionWidth,
                collectionCollapsed: _playlistCollectionCollapsed,
                onCollectionWidthChanged: _scheduleCollectionWidth,
                onCollectionCollapsedChanged: (collapsed) => unawaited(
                  _handlePlaylistCollectionResize(
                    _playlistCollectionWidth,
                    collapsed: collapsed,
                  ),
                ),
                collection: _collection.entries,
                selectedCollectionPath: _collection.selectedPath,
                disabledCollectionPaths: _collection.disabledPaths,
                onAddSavedPlaylist: () => unawaited(_pickAddSavedPlaylist()),
                altered: _playlist.altered,
                pickSavePlaylistPath: pickSavePlaylistPath,
                zoom: zoom,
                dockLogicalTopLeft: _dockOrigin(WindowId.playlist),
                onDockMove: _dockMove(WindowId.playlist),
                onNativeDragStarted: () =>
                    _onNativeDragStarted(WindowId.playlist),
                startDragging: () => _startOsDrag(WindowId.playlist),
                startResizing: _startPlaylistResize,
                onSessionCommand: (cmd) => unawaited(_onCommand(cmd)),
                onAddFiles: () => unawaited(_pickAddAudioFiles()),
                onLoadPlaylist: () => unawaited(_pickLoadPlaylist()),
                onSavePlaylist: () => unawaited(_pickSavePlaylist()),
                onDropPaths: _dropPlaylistPaths,
                onCollapse: () => unawaited(
                  _onCommand(
                    SetShadedCommand(
                      window: WindowId.playlist,
                      shaded: !layout.playlist.shaded,
                    ),
                  ),
                ),
                onClose: () => unawaited(
                  _onCommand(
                    ToggleWindowCommand(
                      window: WindowId.playlist,
                      visible: false,
                    ),
                  ),
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  Widget? _settingsView() {
    final window = _settingsWindow;
    if (window == null) return null;
    final layout = _docking.layout;
    final zoom = _zoomPercent / 100.0;
    return _wrapSecondary(
      window: window,
      logicalSize: TrampMetrics.settings,
      child: SettingsWindow(
        snapshot: _settingsSnapshot(),
        shaded: layout.settings.shaded,
        zoom: zoom,
        dockLogicalTopLeft: _dockOrigin(WindowId.settings),
        onDockMove: _dockMove(WindowId.settings),
        onNativeDragStarted: () => _onNativeDragStarted(WindowId.settings),
        startDragging: () => _startOsDrag(WindowId.settings),
        onSessionCommand: (cmd) => unawaited(_onCommand(cmd)),
        onCollapse: () => unawaited(
          _onCommand(
            SetShadedCommand(
              window: WindowId.settings,
              shaded: !layout.settings.shaded,
            ),
          ),
        ),
        onClose: () => unawaited(
          _onCommand(
            ToggleWindowCommand(window: WindowId.settings, visible: false),
          ),
        ),
      ),
    );
  }

  Widget? _aboutView() {
    final window = _aboutWindow;
    if (window == null) return null;
    final layout = _docking.layout;
    final zoom = _zoomPercent / 100.0;
    return _wrapSecondary(
      window: window,
      logicalSize: TrampMetrics.about,
      child: AboutWindow(
        stats: _aboutStats,
        shaded: layout.about.shaded,
        zoom: zoom,
        onOpenUrl: (uri) => unawaited(openExternalUrl(uri)),
        dockLogicalTopLeft: _dockOrigin(WindowId.about),
        onDockMove: _dockMove(WindowId.about),
        onNativeDragStarted: () => _onNativeDragStarted(WindowId.about),
        startDragging: () => _startOsDrag(WindowId.about),
        onCollapse: () => unawaited(
          _onCommand(
            SetShadedCommand(
              window: WindowId.about,
              shaded: !layout.about.shaded,
            ),
          ),
        ),
        onClose: () => unawaited(
          _onCommand(
            ToggleWindowCommand(window: WindowId.about, visible: false),
          ),
        ),
      ),
    );
  }

  Future<void> _pickAddAudioFiles() async {
    final paths = await pickAudioFiles();
    if (paths == null || paths.isEmpty) return;
    await _onCommand(PlaylistOpCommand('addPaths', paths: paths));
  }

  Future<void> _pickLoadPlaylist() async {
    final path = await pickPlaylistFile();
    if (path == null || path.isEmpty) return;
    await _onCommand(PlaylistOpCommand('openPlaylist', path: path));
  }

  Future<void> _pickSavePlaylist() async {
    final path = await pickSavePlaylistPath();
    if (path == null || path.isEmpty) return;
    await _onCommand(PlaylistOpCommand('savePlaylist', path: path));
  }

  Future<void> _pickAddSavedPlaylist() async {
    final path = await pickPlaylistFile();
    if (path == null || path.isEmpty) return;
    await _onCommand(AddSavedPlaylistCommand(path));
  }

  void _dropPlaylistPaths(List<String> paths) {
    final playlistPaths = paths.where(isPlaylistPath).toList();
    final audioPaths = paths.where(isAudioPath).toList();
    if (playlistPaths.isNotEmpty) {
      unawaited(
        _onCommand(
          PlaylistOpCommand('openPlaylist', path: playlistPaths.first),
        ),
      );
    }
    if (audioPaths.isNotEmpty) {
      unawaited(_onCommand(PlaylistOpCommand('addPaths', paths: audioPaths)));
    }
  }
}
