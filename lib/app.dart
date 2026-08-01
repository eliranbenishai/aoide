import 'dart:async';

import 'package:flutter/material.dart';

import 'package:path/path.dart' as p;

import 'package:path_provider/path_provider.dart';

import 'platform/file_open.dart';
import 'platform/os_media_controls.dart';

import 'playback/media_kit_player_engine.dart';

import 'playback/playback_controller.dart';

import 'playback/player_engine.dart';

import 'playlist/playlist_controller.dart';

import 'playlist/playlist_store.dart';

import 'theme/tramp_theme.dart';

import 'ui/playlist_panel.dart';

import 'ui/transport_panel.dart';

import 'ui/tramp_shell.dart';

class TrampApp extends StatefulWidget {
  const TrampApp({
    super.key,
    this.launchArgs = const [],
    this.engine,
    this.osMediaControls,
  });

  final List<String> launchArgs;

  final PlayerEngine? engine;

  final OsMediaControls? osMediaControls;

  @override
  State<TrampApp> createState() => _TrampAppState();
}

class _TrampAppState extends State<TrampApp> {
  late final PlaylistController _playlist;

  late final PlaybackController _playback;

  late final OsMediaControls _osMediaControls;

  @override
  void initState() {
    super.initState();

    final store = FilePlaylistStore(
      supportDir: getApplicationSupportDirectory,
    );

    _playlist = PlaylistController(store: store);

    _playback = PlaybackController(
      playlist: _playlist,
      engine: widget.engine ??
          MediaKitPlayerEngine(
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
    unawaited(_osMediaControls.start(_playback));
    unawaited(_playlist.restoreLastPlaylist());
  }

  @override
  void dispose() {
    unawaited(_osMediaControls.stop());
    unawaited(_playback.dispose());

    super.dispose();
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
      final path = await pickFolder();

      if (path == null) return;

      _playlist.addTracks(tracksFromPaths([path]));

      return;
    }

    if (action != 'files') return;

    final paths = await pickAudioFiles();

    if (paths == null || paths.isEmpty) return;

    _playlist.addTracks(tracksFromPaths(paths));
  }

  Future<void> _handleDroppedPaths(List<String> paths) async {
    if (paths.length == 1 && isPlaylistPath(paths.single)) {
      await _playlist.openPlaylistFile(paths.single);

      return;
    }

    _playlist.addTracks(tracksFromPaths(paths));
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tramp',
      debugShowCheckedModeBanner: false,
      theme: buildTrampTheme(),
      home: ListenableBuilder(
        listenable: _playlist,
        builder: (context, _) {
          return TrampShell(
            playback: _playback,
            playlistController: _playlist,
            hasTracks: _playlist.playlist.tracks.isNotEmpty,
            onDropPaths: _handleDroppedPaths,
            onOpenFiles: () async {
              final paths = await pickAudioFiles();
              if (paths == null || paths.isEmpty) return;
              _playlist.addTracks(tracksFromPaths(paths));
            },
            onSavePlaylist: _savePlaylist,
            transport: TransportPanel(
              playback: _playback,
              hasTracks: _playlist.playlist.tracks.isNotEmpty,
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
