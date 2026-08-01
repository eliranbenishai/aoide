import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:smtc_windows/smtc_windows.dart';

import '../playback/playback_controller.dart';
import 'os_media_controls.dart';

class WindowsOsMediaControls implements OsMediaControls {
  SMTCWindows? _smtc;
  PlaybackController? _playback;
  StreamSubscription<PressedButton>? _buttonSubscription;
  VoidCallback? _playbackListener;

  @override
  Future<void> start(PlaybackController playback) async {
    if (_smtc != null) return;

    _playback = playback;
    await SMTCWindows.initialize();
    _smtc = SMTCWindows(
      config: const SMTCConfig(
        playEnabled: true,
        pauseEnabled: true,
        nextEnabled: true,
        prevEnabled: true,
        stopEnabled: true,
        fastForwardEnabled: false,
        rewindEnabled: false,
      ),
    );

    _buttonSubscription = _smtc!.buttonPressStream.listen(_onButtonPressed);
    _playbackListener = _syncFromPlayback;
    playback.addListener(_playbackListener!);
    _syncFromPlayback();
  }

  @override
  Future<void> stop() async {
    final listener = _playbackListener;
    if (listener != null) {
      _playback?.removeListener(listener);
    }
    _playbackListener = null;

    await _buttonSubscription?.cancel();
    _buttonSubscription = null;

    await _smtc?.dispose();
    _smtc = null;
    _playback = null;
  }

  void _onButtonPressed(PressedButton button) {
    final playback = _playback;
    if (playback == null) return;

    switch (button) {
      case PressedButton.play:
        if (!playback.playing) {
          unawaited(playback.playPause());
        }
      case PressedButton.pause:
        if (playback.playing) {
          unawaited(playback.playPause());
        }
      case PressedButton.next:
        unawaited(playback.next());
      case PressedButton.previous:
        unawaited(playback.previous());
      case PressedButton.stop:
        unawaited(playback.stop());
      default:
        break;
    }
  }

  void _syncFromPlayback() {
    final playback = _playback;
    final smtc = _smtc;
    if (playback == null || smtc == null) return;

    final track = playback.currentTrack;
    if (track != null) {
      unawaited(
        smtc.updateMetadata(
          MusicMetadata(
            title: track.displayTitle,
            artist: track.artist,
            album: track.album,
          ),
        ),
      );
    } else {
      unawaited(smtc.clearMetadata());
    }

    unawaited(
      smtc.setPlaybackStatus(
        playback.playing ? PlaybackStatus.playing : PlaybackStatus.paused,
      ),
    );

    final durationMs = playback.duration.inMilliseconds;
    unawaited(
      smtc.updateTimeline(
        PlaybackTimeline(
          startTimeMs: 0,
          endTimeMs: durationMs,
          positionMs: playback.position.inMilliseconds,
        ),
      ),
    );
  }
}
