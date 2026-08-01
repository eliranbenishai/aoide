import 'dart:async';

import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';

import 'playback/fake_player_engine.dart';
import 'playback/playback_controller.dart';
import 'playlist/playlist_controller.dart';
import 'playlist/playlist_store.dart';
import 'theme/tramp_theme.dart';
import 'ui/playlist_panel.dart';
import 'ui/transport_panel.dart';
import 'ui/tramp_shell.dart';

class TrampApp extends StatefulWidget {
  const TrampApp({super.key, this.launchArgs = const []});

  final List<String> launchArgs;

  @override
  State<TrampApp> createState() => _TrampAppState();
}

class _TrampAppState extends State<TrampApp> {
  late final PlaylistController _playlist;
  late final PlaybackController _playback;

  @override
  void initState() {
    super.initState();
    final store = FilePlaylistStore(
      supportDir: getApplicationSupportDirectory,
    );
    _playlist = PlaylistController(store: store);
    _playback = PlaybackController(
      playlist: _playlist,
      engine: FakePlayerEngine(),
    );
    unawaited(_playlist.restoreLastPlaylist());
  }

  @override
  void dispose() {
    unawaited(_playback.dispose());
    super.dispose();
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
            transport: TransportPanel(
              playback: _playback,
              hasTracks: _playlist.playlist.tracks.isNotEmpty,
            ),
            playlist: PlaylistPanel(
              playlist: _playlist,
              playback: _playback,
            ),
          );
        },
      ),
    );
  }
}
