#include "playback.h"

#include <algorithm>

namespace aoide {

namespace {

/// A **Spin** is one track played through to the end, so most of the running
/// time has to have actually gone past. The rest is the fade, the applause, or
/// the second the listener dragged the seek bar over.
constexpr int kSpinHeardPercent = 90;

/// Anything longer than this between two clock readings is a jump, not
/// listening. Generous, because the tick can be starved by a slow repaint —
/// a seek the listener asked for is discounted where it happens, not here.
constexpr qint64 kListenStepCapMs = 5000;

QVector<QString> trackPaths(const QVector<Track>& tracks) {
  QVector<QString> paths;
  paths.reserve(tracks.size());
  for (const Track& t : tracks) paths.push_back(t.path);
  return paths;
}

/// Where a pass over the list starts: the head of the shuffle order, or the top
/// of the list when nothing is shuffled.
std::optional<int> firstPlayableInPass(int length, const QVector<int>& order,
                                       const std::function<bool(int)>& playable) {
  if (!order.isEmpty()) {
    for (int i : order) {
      if (i >= 0 && i < length && playable(i)) return i;
    }
    return std::nullopt;
  }
  for (int i = 0; i < length; ++i) {
    if (playable(i)) return i;
  }
  return std::nullopt;
}

bool isSameListMinusPath(const QVector<QString>& previous, const QVector<Track>& next,
                         const QString& removed) {
  if (next.size() != previous.size() - 1) return false;
  int j = 0;
  bool skipped = false;
  for (const QString& path : previous) {
    if (!skipped && path == removed) {
      skipped = true;
      continue;
    }
    if (j >= next.size() || next[j].path != path) return false;
    ++j;
  }
  return skipped && j == next.size();
}

}  // namespace

PlaybackController::PlaybackController(PlaylistController* playlist, PlayerEngine* engine)
    : playlist_(playlist), engine_(engine) {
  previousPaths_ = trackPaths(playlist_->tracks());
  bindEngine();
}

void PlaybackController::bindEngine() {
  engine_->onPlaying = [](bool) {};  // chrome is optimistic; mpv pause events lag
  engine_->onPosition = [this](qint64 ms) {
    accrueListened(ms);
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
    if (next.path.isEmpty()) {
      if (playingTrack_ && playingTrack_->path == path) next = *playingTrack_;
      else return;
    }
    if (!title.isEmpty()) next.title = title;
    if (!artist.isEmpty()) next.artist = artist;
    if (!album.isEmpty()) next.album = album;
    if (durationMs > 0) next.durationMs = durationMs;
    const bool inList = playlist_->updateTrackByPath(path, next);
    if (playingPath_ == path) playingTrack_ = next;
    if (durationMs > 0 && onTrackDuration_) onTrackDuration_(path, durationMs);
    if (!inList && playingPath_ == path) notify();
  };
}

void PlaybackController::notify() {
  if (onChanged_) onChanged_();
}

std::optional<Track> PlaybackController::currentTrack() const {
  if (playingPath_.isEmpty()) return std::nullopt;
  const auto tracks = playlist_->tracks();
  if (playingIndex_ && *playingIndex_ >= 0 && *playingIndex_ < tracks.size() &&
      tracks[*playingIndex_].path == playingPath_) {
    return tracks[*playingIndex_];
  }
  for (const Track& t : tracks) {
    if (t.path == playingPath_) return t;
  }
  if (playingTrack_ && playingTrack_->path == playingPath_) return playingTrack_;
  return std::nullopt;
}

void PlaybackController::playPause() {
  const auto tracks = playlist_->tracks();
  const auto selected = playlist_->selectedIndex();
  const bool haveNowPlaying = !playingPath_.isEmpty();
  const bool selectedPlayable =
      selected && *selected >= 0 && *selected < tracks.size() && !tracks[*selected].disabled;
  const bool selectionDiffers = selected && playingIndex_ && selected != playingIndex_;
  if (selectedPlayable && (!haveNowPlaying || selectionDiffers)) {
    playIndex(*selected);
    return;
  }
  if (!haveNowPlaying) {
    if (!selected) {
      for (int i = 0; i < tracks.size(); ++i) {
        if (!tracks[i].disabled) {
          playIndex(i);
          return;
        }
      }
    }
    return;
  }
  if (selectionDiffers) return;
  pauseOrResumeCurrent();
}

void PlaybackController::stop() {
  engine_->stop();
  mediaOpen_ = false;
  playing_ = false;
  positionMs_ = 0;
  // Stop is rewind-and-silence, not eject. The current track stays on the
  // transport so the display and Play still know what they were on. The
  // failure that ended a load is spent; title, length and format are not.
  failureMessage_.clear();
  resetListenTally();
  notify();
}

void PlaybackController::next() {
  const auto tracks = playlist_->tracks();
  if (tracks.isEmpty()) return;
  const int current = playingIndex_.value_or(playlist_->selectedIndex().value_or(0));
  auto playable = [&](int i) { return i >= 0 && i < tracks.size() && !tracks[i].disabled; };
  auto next = nextPlayableIndex(current, tracks.size(), shuffle_, RepeatMode::off, shuffledOrder_,
                                playable);
  if (!next && repeatMode_ == RepeatMode::all) {
    // This pass is over. Deal the next one rather than replaying the order the
    // listener already heard, then start it from its head.
    if (shuffle_) rebuildShuffleOrder(false);
    next = firstPlayableInPass(tracks.size(), shuffle_ ? shuffledOrder_ : QVector<int>{}, playable);
  }
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
  auto playable = [&](int i) { return i >= 0 && i < tracks.size() && !tracks[i].disabled; };
  const auto prev = previousPlayableIndex(current, tracks.size(), shuffle_, repeatMode_,
                                          shuffledOrder_, playable);
  if (!prev) {
    engine_->seekMs(0);
    return;
  }
  playIndex(*prev);
}

void PlaybackController::playIndex(int index) {
  const auto tracks = playlist_->tracks();
  if (index < 0 || index >= tracks.size()) return;
  if (tracks[index].disabled) return;
  playingIndex_ = index;
  playingPath_ = tracks[index].path;
  playingTrack_ = tracks[index];
  format_ = {};
  failureMessage_.clear();
  resetListenTally();
  playlist_->select(index);
  engine_->open(tracks[index]);
  // mpv reports a loadfile failure inline from open(), and onEngineError has
  // already cleared the flags by the time we get here. Do not put them back.
  if (!failureMessage_.isEmpty()) {
    mediaOpen_ = false;
    playing_ = false;
    notify();
    return;
  }
  mediaOpen_ = true;
  engine_->play();
  playing_ = true;
  notify();
}

void PlaybackController::playFrom(int index) {
  const auto tracks = playlist_->tracks();
  if (index < 0 || index >= tracks.size()) return;
  if (!tracks[index].disabled) {
    playIndex(index);
    return;
  }
  auto playable = [&](int i) { return i >= 0 && i < tracks.size() && !tracks[i].disabled; };
  const auto fallback =
      nextPlayableIndex(index, tracks.size(), false, RepeatMode::all, {}, playable);
  if (!fallback) {
    failureMessage_ = QStringLiteral("No playable track in this playlist");
    notify();
    return;
  }
  const QString skipped = tracks[index].displayTitle();
  playIndex(*fallback);
  if (failureMessage_.isEmpty()) {
    failureMessage_ = QStringLiteral("Skipped %1 — the file is missing").arg(skipped);
    notify();
  }
}

void PlaybackController::pauseOrResumeCurrent() {
  if (playingPath_.isEmpty() && !playingIndex_) return;
  if (playing_) {
    engine_->pause();
    playing_ = false;
    notify();
    return;
  }
  if (!mediaOpen_) {
    if (playingIndex_) playIndex(*playingIndex_);
    return;
  }
  engine_->play();
  playing_ = true;
  notify();
}

void PlaybackController::seekMs(qint64 positionMs) {
  positionMs_ = positionMs;
  listenCursorMs_ = positionMs;
  engine_->seekMs(positionMs);
  if (onPosition_) onPosition_();
}

void PlaybackController::pollClock() {
  const qint64 ms = engine_->queryPositionMs();
  if (ms < 0) return;
  accrueListened(ms);
  positionMs_ = ms;
}

void PlaybackController::accrueListened(qint64 positionMs) {
  const qint64 step = positionMs - listenCursorMs_;
  listenCursorMs_ = positionMs;
  if (step > 0 && step <= kListenStepCapMs) heardMs_ += step;
}

void PlaybackController::resetListenTally() {
  heardMs_ = 0;
  listenCursorMs_ = 0;
}

bool PlaybackController::heardEnoughForSpin() const {
  // No length to measure against: take the end of the file at its word rather
  // than never counting a spin for a track whose duration nothing reported.
  if (durationMs_ <= 0) return true;
  return heardMs_ * 100 >= durationMs_ * kSpinHeardPercent;
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
  if (shuffle_) rebuildShuffleOrder(true);
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
  // Stop is what unloads media. Leaving the refused file attached keeps mpv
  // holding an errored load that nothing will ever play.
  engine_->stop();
  notify();
}

void PlaybackController::onCompleted() {
  if (heardEnoughForSpin()) countSpin();
  resetListenTally();
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

void PlaybackController::rebuildShuffleOrder(bool openOnCurrent) {
  shuffledOrder_ = shuffledOrder(playlist_->tracks().size());
  if (!playingIndex_ || shuffledOrder_.size() < 2) return;
  const int at = shuffledOrder_.indexOf(*playingIndex_);
  if (at < 0) return;
  std::swap(shuffledOrder_[at], shuffledOrder_[openOnCurrent ? 0 : shuffledOrder_.size() - 1]);
}

void PlaybackController::onPlaylistChanged() {
  if (shuffle_) rebuildShuffleOrder(true);
  const auto tracks = playlist_->tracks();
  const QVector<QString> previous = previousPaths_;
  previousPaths_ = trackPaths(tracks);
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
      playingTrack_ = tracks[newIndex];
    } else if (isSameListMinusPath(previous, tracks, playingPath_)) {
      const auto advanceIndex = playingIndex_;
      playingPath_.clear();
      playingTrack_.reset();
      if (advanceIndex && *advanceIndex < tracks.size()) {
        playIndex(*advanceIndex);
        return;
      }
      playingIndex_.reset();
      mediaOpen_ = false;
      engine_->stop();
    } else {
      // Playlist replaced (loaded another saved list). Keep the open file
      // playing; only a true one-track removal should advance/stop.
      playingIndex_.reset();
    }
  }
  notify();
}

}  // namespace aoide
