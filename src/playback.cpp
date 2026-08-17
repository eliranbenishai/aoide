#include "playback.h"

#include <algorithm>

namespace tramp {

PlaybackController::PlaybackController(PlaylistController* playlist, PlayerEngine* engine)
    : playlist_(playlist), engine_(engine) {
  previousTrackCount_ = playlist_->tracks().size();
  bindEngine();
}

void PlaybackController::bindEngine() {
  engine_->onPlaying = [this](bool value) {
    if (playing_ == value) return;
    playing_ = value;
    notify();
  };
  engine_->onPosition = [this](qint64 ms) {
    positionMs_ = ms;
    if (onPosition_) onPosition_();
  };
  engine_->onDuration = [this](qint64 ms) {
    if (durationMs_ == ms) return;
    durationMs_ = ms;
    notify();
  };
  engine_->onCompleted = [this]() { onCompleted(); };
  engine_->onFormat = [this](AudioFormatInfo info) {
    bool changed = false;
    if (info.bitrateKbps && format_.bitrateKbps != info.bitrateKbps) {
      format_.bitrateKbps = info.bitrateKbps;
      changed = true;
    }
    if (info.sampleRateHz && format_.sampleRateHz != info.sampleRateHz) {
      format_.sampleRateHz = info.sampleRateHz;
      changed = true;
    }
    if (info.channels && format_.channels != info.channels) {
      format_.channels = info.channels;
      changed = true;
    }
    if (changed && !playing_) notify();
  };
  engine_->onError = [this](const QString& message) { onEngineError(message); };
  engine_->onMetadata = [this](const QString& path, const QString& title, const QString& artist,
                               const QString& album, qint64 durationMs) {
    Track next;
    for (const Track& t : playlist_->tracks()) {
      if (t.path == path) {
        next = t;
        break;
      }
    }
    if (next.path.isEmpty()) return;
    if (!title.isEmpty()) next.title = title;
    if (!artist.isEmpty()) next.artist = artist;
    if (!album.isEmpty()) next.album = album;
    if (durationMs > 0) next.durationMs = durationMs;
    playlist_->updateTrackByPath(path, next);
  };
}

void PlaybackController::notify() {
  if (onChanged_) onChanged_();
}

std::optional<Track> PlaybackController::currentTrack() const {
  if (!playingIndex_ || *playingIndex_ < 0 || *playingIndex_ >= playlist_->tracks().size()) {
    return std::nullopt;
  }
  return playlist_->tracks()[*playingIndex_];
}

void PlaybackController::playPause() {
  const auto selected = playlist_->selectedIndex();
  const bool nothingOpen = !playingIndex_;
  const bool selectionDiffers = selected && selected != playingIndex_;
  if (nothingOpen || selectionDiffers) {
    if (selected) {
      playIndex(*selected);
    } else if (playingIndex_) {
      pauseOrResumeCurrent();
    } else if (!playlist_->tracks().isEmpty()) {
      playIndex(0);
    }
    return;
  }
  pauseOrResumeCurrent();
}

void PlaybackController::stop() {
  engine_->stop();
  mediaOpen_ = false;
  playing_ = false;
  positionMs_ = 0;
  notify();
}

void PlaybackController::next() {
  const auto tracks = playlist_->tracks();
  if (tracks.isEmpty()) return;
  const int current = playingIndex_.value_or(playlist_->selectedIndex().value_or(0));
  const auto next = nextIndex(current, tracks.size(), shuffle_, repeatMode_, shuffledOrder_);
  if (!next) {
    stop();
    return;
  }
  playIndex(*next);
}

void PlaybackController::previous() {
  const auto tracks = playlist_->tracks();
  if (tracks.isEmpty()) return;
  const int current = playingIndex_.value_or(playlist_->selectedIndex().value_or(0));
  const auto prev = previousIndex(current, tracks.size(), shuffle_, repeatMode_, shuffledOrder_);
  if (!prev) {
    engine_->seekMs(0);
    return;
  }
  playIndex(*prev);
}

void PlaybackController::playIndex(int index) {
  const auto tracks = playlist_->tracks();
  if (index < 0 || index >= tracks.size()) return;
  playingIndex_ = index;
  playingPath_ = tracks[index].path;
  format_ = {};
  failureMessage_.clear();
  playlist_->select(index);
  engine_->open(tracks[index]);
  mediaOpen_ = true;
  engine_->play();
  playing_ = true;
  notify();
}

void PlaybackController::pauseOrResumeCurrent() {
  if (!playingIndex_) return;
  if (playing_) {
    engine_->pause();
    playing_ = false;
    notify();
    return;
  }
  if (!mediaOpen_) {
    playIndex(*playingIndex_);
    return;
  }
  engine_->play();
  playing_ = true;
  notify();
}

void PlaybackController::seekMs(qint64 positionMs) {
  positionMs_ = positionMs;
  engine_->seekMs(positionMs);
  if (onPosition_) onPosition_();
}

void PlaybackController::pollClock() {
  const qint64 ms = engine_->queryPositionMs();
  if (ms >= 0) positionMs_ = ms;
}

void PlaybackController::setVolume(double volume) {
  volume_ = std::clamp(volume, 0.0, 1.0);
  if (!muted_) engine_->setVolume(volume_);
  if (volume_ > 0) preMuteVolume_ = volume_;
  notify();
}

void PlaybackController::toggleMute() {
  if (muted_) {
    muted_ = false;
    volume_ = preMuteVolume_;
    engine_->setVolume(preMuteVolume_);
  } else {
    preMuteVolume_ = volume_;
    muted_ = true;
    engine_->setVolume(0);
  }
  notify();
}

void PlaybackController::toggleShuffle() { setShuffle(!shuffle_); }

void PlaybackController::setShuffle(bool on) {
  shuffle_ = on;
  if (shuffle_) rebuildShuffleOrder();
  notify();
}

void PlaybackController::cycleRepeatMode() {
  switch (repeatMode_) {
    case RepeatMode::off:
      repeatMode_ = RepeatMode::all;
      break;
    case RepeatMode::all:
      repeatMode_ = RepeatMode::one;
      break;
    case RepeatMode::one:
      repeatMode_ = RepeatMode::off;
      break;
  }
  notify();
}

void PlaybackController::setRepeatMode(RepeatMode mode) {
  repeatMode_ = mode;
  notify();
}

void PlaybackController::onEngineError(const QString& message) {
  if (playingPath_.isEmpty()) return;
  failureMessage_ = message;
  playing_ = false;
  mediaOpen_ = false;
  notify();
}

void PlaybackController::onCompleted() {
  countSpin();
  switch (repeatMode_) {
    case RepeatMode::one:
      engine_->seekMs(0);
      engine_->play();
      break;
    case RepeatMode::all:
      next();
      break;
    case RepeatMode::off: {
      const auto current = playingIndex_;
      if (!current || *current >= playlist_->tracks().size() - 1) {
        stop();
      } else {
        next();
      }
      break;
    }
  }
}

void PlaybackController::countSpin() {
  ++spins_;
  if (onSpin_) onSpin_(spins_);
  notify();
}

void PlaybackController::rebuildShuffleOrder() {
  shuffledOrder_ = shuffledOrder(playlist_->tracks().size(), int(playingIndex_.value_or(0) + 1));
}

void PlaybackController::onPlaylistChanged() {
  if (shuffle_) rebuildShuffleOrder();
  const auto tracks = playlist_->tracks();
  const int previousLength = previousTrackCount_;
  previousTrackCount_ = tracks.size();
  if (!playingPath_.isEmpty()) {
    int newIndex = -1;
    for (int i = 0; i < tracks.size(); ++i) {
      if (tracks[i].path == playingPath_) {
        newIndex = i;
        break;
      }
    }
    if (newIndex != -1) {
      playingIndex_ = newIndex;
    } else if (tracks.size() == previousLength - 1) {
      const auto advanceIndex = playingIndex_;
      playingPath_.clear();
      if (advanceIndex && *advanceIndex < tracks.size()) {
        playIndex(*advanceIndex);
        return;
      }
      playingIndex_.reset();
      mediaOpen_ = false;
      engine_->stop();
    } else {
      playingIndex_.reset();
      playingPath_.clear();
      mediaOpen_ = false;
      engine_->stop();
    }
  }
  notify();
}

}  // namespace tramp
