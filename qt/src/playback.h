#pragma once

#include "player_engine.h"
#include "playlist.h"
#include "transport.h"

#include <QVector>
#include <functional>
#include <optional>

namespace tramp {

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
  AudioFormatInfo format() const { return format_; }
  int spins() const { return spins_; }
  QString failureMessage() const { return failureMessage_; }
  std::optional<Track> currentTrack() const;

  void setSpins(int spins) { spins_ = spins; }
  void setOnChanged(std::function<void()> cb) { onChanged_ = std::move(cb); }
  void setOnSpin(std::function<void(int)> cb) { onSpin_ = std::move(cb); }

  void playPause();
  void stop();
  void next();
  void previous();
  void playIndex(int index);
  void seekMs(qint64 positionMs);
  void setVolume(double volume);
  void toggleMute();
  void toggleShuffle();
  void cycleRepeatMode();
  void setShuffle(bool on);
  void setRepeatMode(RepeatMode mode);

  void onPlaylistChanged();

 private:
  void bindEngine();
  void pauseOrResumeCurrent();
  void rebuildShuffleOrder();
  void notify();
  void countSpin();
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
  bool mediaOpen_ = false;
  int previousTrackCount_ = 0;
  AudioFormatInfo format_;
  QString failureMessage_;
  int spins_ = 0;
  std::function<void()> onChanged_;
  std::function<void(int)> onSpin_;
};

}  // namespace tramp
