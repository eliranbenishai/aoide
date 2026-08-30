#pragma once

#include "player_engine.h"
#include "playlist.h"
#include "transport.h"

#include <QVector>
#include <functional>
#include <optional>

namespace aoide {

class PlaybackController {
 public:
  PlaybackController(PlaylistController* playlist, PlayerEngine* engine);

  bool playing() const { return playing_; }
  bool paused() const { return mediaOpen_ && !playing_; }
  bool muted() const { return muted_; }
  double volume() const { return volume_; }
  qint64 positionMs() const { return positionMs_; }
  qint64 durationMs() const { return durationMs_; }
  bool shuffle() const { return shuffle_; }
  RepeatMode repeatMode() const { return repeatMode_; }
  std::optional<int> playingIndex() const { return playingIndex_; }
  /// A current track exists, but it is not a row of the **current playlist** —
  /// the list was replaced while this file stayed on the transport.
  bool offList() const { return !playingPath_.isEmpty() && !playingIndex_; }
  AudioFormatInfo format() const { return format_; }
  int spins() const { return spins_; }
  QString failureMessage() const { return failureMessage_; }
  std::optional<Track> currentTrack() const;

  void setSpins(int spins) { spins_ = spins; }
  void setOnChanged(std::function<void()> cb) { onChanged_ = std::move(cb); }
  void setOnPosition(std::function<void()> cb) { onPosition_ = std::move(cb); }
  void setOnSpin(std::function<void(int)> cb) { onSpin_ = std::move(cb); }
  void setOnTrackDuration(std::function<void(QString, qint64)> cb) {
    onTrackDuration_ = std::move(cb);
  }

  void playPause();
  void stop();
  void next();
  void previous();
  void playIndex(int index);
  /// Start at [index], or at the next playable row when that one is a
  /// **disabled track**, saying which row was skipped. Auto-start and resume
  /// hand over an index nothing has checked; `playIndex` refuses a disabled
  /// row, and silence with no explanation is worse than a mild surprise.
  void playFrom(int index);
  /// Open and play a file that is not a row of the current playlist. Resume
  /// after quit has no playingTrack_ in a fresh process, and playIndex needs
  /// a row the list does not have.
  void playTrack(const Track& track);
  void seekMs(qint64 positionMs);
  void setVolume(double volume);
  void toggleMute();
  void toggleShuffle();
  void cycleRepeatMode();
  void setShuffle(bool on);
  void setRepeatMode(RepeatMode mode);

  void pollClock();
  void onPlaylistChanged();

 private:
  void bindEngine();
  void pauseOrResumeCurrent();
  /// Deal a fresh pass over the list. [openOnCurrent] puts the playing track at
  /// the head so the pass still covers every other row; a wrap wants the
  /// opposite, because the track that just finished must not open the new pass.
  void rebuildShuffleOrder(bool openOnCurrent);
  /// Drop what the last track left behind. Runs before the playlist is told
  /// about the new one: selecting notifies, and a repaint on that notify would
  /// otherwise pair the new title with the old format chip and stale failure.
  void clearLastTrack();
  /// playIndex and playTrack agree on the load; they differ on whether a row
  /// is current.
  void openAndPlay(const Track& track);
  void notify();
  void countSpin();
  /// Credit the stretch of audio between two clock readings, so a jump the
  /// listener made with the seek bar is not mistaken for listening.
  void accrueListened(qint64 positionMs);
  void resetListenTally();
  bool heardEnoughForSpin() const;
  void onCompleted();
  void onEngineError(const QString& message);

  PlaylistController* playlist_;
  PlayerEngine* engine_;
  bool playing_ = false;
  bool muted_ = false;
  double volume_ = 1.0;
  double preMuteVolume_ = 1.0;
  qint64 positionMs_ = 0;
  qint64 durationMs_ = 0;
  bool shuffle_ = false;
  RepeatMode repeatMode_ = RepeatMode::off;
  QVector<int> shuffledOrder_;
  std::optional<int> playingIndex_;
  QString playingPath_;
  std::optional<Track> playingTrack_;
  bool mediaOpen_ = false;
  QVector<QString> previousPaths_;
  AudioFormatInfo format_;
  QString failureMessage_;
  qint64 heardMs_ = 0;
  qint64 listenCursorMs_ = 0;
  int spins_ = 0;
  std::function<void()> onChanged_;
  std::function<void()> onPosition_;
  std::function<void(int)> onSpin_;
  std::function<void(QString, qint64)> onTrackDuration_;
};

}  // namespace aoide
