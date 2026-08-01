import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import '../playback/playback_controller.dart';
import 'os_media_controls.dart';

class MacOsMediaControls implements OsMediaControls {
  static const _channel = MethodChannel('com.tramp/os_media_controls');

  PlaybackController? _playback;
  VoidCallback? _playbackListener;

  @override
  Future<void> start(PlaybackController playback) async {
    if (_playback != null) return;

    _playback = playback;
    _channel.setMethodCallHandler(_onMethodCall);
    _playbackListener = _syncFromPlayback;
    playback.addListener(_playbackListener!);
    _syncFromPlayback();
  }

  @override
  Future<void> stop() async {
    _channel.setMethodCallHandler(null);

    final listener = _playbackListener;
    if (listener != null) {
      _playback?.removeListener(listener);
    }
    _playbackListener = null;
    _playback = null;

    await _channel.invokeMethod<void>('clearState');
  }

  Future<void> _onMethodCall(MethodCall call) async {
    final playback = _playback;
    if (playback == null || call.method != 'onMediaKey') return;

    final action = call.arguments;
    if (action is! Map) return;

    switch (action['action']) {
      case 'play':
        if (!playback.playing) {
          await playback.playPause();
        }
      case 'pause':
        if (playback.playing) {
          await playback.playPause();
        }
      case 'toggle':
        await playback.playPause();
      case 'next':
        await playback.next();
      case 'previous':
        await playback.previous();
      case 'stop':
        await playback.stop();
    }
  }

  void _syncFromPlayback() {
    final playback = _playback;
    if (playback == null) return;

    final track = playback.currentTrack;
    unawaited(
      _channel.invokeMethod<void>('updateState', {
        'title': track?.displayTitle ?? '',
        'artist': track?.artist ?? '',
        'album': track?.album ?? '',
        'playing': playback.playing,
        'position': playback.position.inMilliseconds,
        'duration': playback.duration.inMilliseconds,
      }),
    );
  }
}
