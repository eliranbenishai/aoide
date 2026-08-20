#include "playlist.h"

#include "persist.h"

#include <QFile>
#include <algorithm>

namespace tramp {
namespace {

int clampIndex(std::optional<int> index, int length) {
  if (!index.has_value() || length == 0) {
    return -1;
  }
  if (*index < 0) {
    return 0;
  }
  if (*index >= length) {
    return length - 1;
  }
  return *index;
}

}  // namespace

void PlaylistController::notify() {
  if (onChanged_) {
    onChanged_();
  }
}

void PlaylistController::replaceTracks(const QVector<Track>& tracks, const QString& sourcePath) {
  tracks_ = tracks;
  sourcePath_ = sourcePath;
  const int clamped = clampIndex(selectedIndex_, tracks_.size());
  if (clamped < 0) {
    selectedIndex_.reset();
    selectedIndices_.clear();
  } else {
    selectedIndex_ = clamped;
    if (selectedIndices_.isEmpty()) {
      selectedIndices_.insert(clamped);
    }
    QSet<int> next;
    for (int i : selectedIndices_) {
      if (i >= 0 && i < tracks_.size()) {
        next.insert(i);
      }
    }
    if (selectedIndex_) {
      next.insert(*selectedIndex_);
    }
    selectedIndices_ = next;
  }
}

void PlaylistController::setTracks(const QVector<Track>& tracks, const QString& sourcePath) {
  replaceTracks(tracks, sourcePath);
  altered_ = false;
  notify();
}

void PlaylistController::restoreAlteredTracks(const QVector<Track>& tracks,
                                              const QString& sourcePath) {
  replaceTracks(tracks, sourcePath);
  altered_ = true;
  notify();
}

void PlaylistController::addTracks(const QVector<Track>& tracks) {
  if (tracks.isEmpty()) {
    return;
  }
  tracks_ += tracks;
  altered_ = true;
  notify();
}

void PlaylistController::removeAt(int index) {
  if (index < 0 || index >= tracks_.size()) {
    return;
  }
  tracks_.removeAt(index);
  altered_ = true;
  QSet<int> next;
  for (int i : selectedIndices_) {
    if (i < index) {
      next.insert(i);
    } else if (i > index) {
      next.insert(i - 1);
    }
  }
  if (selectedIndex_) {
    if (*selectedIndex_ == index) {
      selectedIndex_.reset();
    } else if (*selectedIndex_ > index) {
      selectedIndex_ = *selectedIndex_ - 1;
    }
  }
  if (selectedIndex_) {
    next.insert(*selectedIndex_);
  }
  selectedIndices_ = next;
  notify();
}

void PlaylistController::removeSelected() {
  QSet<int> remove = selectedIndices_;
  if (remove.isEmpty() && selectedIndex_) {
    remove.insert(*selectedIndex_);
  }
  if (remove.isEmpty()) {
    return;
  }
  QVector<Track> kept;
  kept.reserve(tracks_.size());
  for (int i = 0; i < tracks_.size(); ++i) {
    if (!remove.contains(i)) {
      kept.push_back(tracks_[i]);
    }
  }
  const int removed = tracks_.size() - kept.size();
  tracks_ = kept;
  if (removed > 0) {
    altered_ = true;
  }
  selectedIndex_.reset();
  selectedIndices_.clear();
  notify();
}

void PlaylistController::move(int oldIndex, int newIndex) {
  if (oldIndex < 0 || oldIndex >= tracks_.size() || newIndex < 0 || newIndex > tracks_.size()) {
    return;
  }
  int insertIndex = newIndex;
  if (oldIndex < newIndex) {
    insertIndex -= 1;
  }
  const Track item = tracks_.takeAt(oldIndex);
  tracks_.insert(insertIndex, item);
  if (insertIndex != oldIndex) {
    altered_ = true;
  }
  auto remap = [&](int i) {
    if (i == oldIndex) {
      return insertIndex;
    }
    if (oldIndex < insertIndex && i > oldIndex && i <= insertIndex) {
      return i - 1;
    }
    if (insertIndex < oldIndex && i >= insertIndex && i < oldIndex) {
      return i + 1;
    }
    return i;
  };
  QSet<int> next;
  for (int i : selectedIndices_) {
    next.insert(remap(i));
  }
  if (selectedIndex_) {
    selectedIndex_ = remap(*selectedIndex_);
    next.insert(*selectedIndex_);
  }
  selectedIndices_ = next;
  notify();
}

void PlaylistController::select(int index) {
  if (index < 0 || index >= tracks_.size()) {
    return;
  }
  selectedIndex_ = index;
  selectedIndices_.clear();
  selectedIndices_.insert(index);
  notify();
}

void PlaylistController::selectRange(int index) {
  if (index < 0 || index >= tracks_.size()) {
    return;
  }
  if (!selectedIndex_ || *selectedIndex_ >= tracks_.size()) {
    select(index);
    return;
  }
  const int from = std::min(*selectedIndex_, index);
  const int to = std::max(*selectedIndex_, index);
  selectedIndices_.clear();
  for (int i = from; i <= to; ++i) {
    selectedIndices_.insert(i);
  }
  notify();
}

void PlaylistController::toggleSelection(int index) {
  if (index < 0 || index >= tracks_.size()) {
    return;
  }
  if (selectedIndices_.contains(index)) {
    selectedIndices_.remove(index);
    if (selectedIndex_ == index) {
      if (selectedIndices_.isEmpty()) {
        selectedIndex_.reset();
      } else {
        QList<int> sorted = selectedIndices_.values();
        std::sort(sorted.begin(), sorted.end());
        selectedIndex_ = sorted.first();
      }
    }
  } else {
    selectedIndices_.insert(index);
    selectedIndex_ = index;
  }
  notify();
}

void PlaylistController::selectAll() {
  selectedIndices_.clear();
  for (int i = 0; i < tracks_.size(); ++i) {
    selectedIndices_.insert(i);
  }
  selectedIndex_ = tracks_.isEmpty() ? std::optional<int>{} : 0;
  notify();
}

void PlaylistController::invertSelection() {
  QSet<int> next;
  for (int i = 0; i < tracks_.size(); ++i) {
    if (!selectedIndices_.contains(i)) {
      next.insert(i);
    }
  }
  selectedIndices_ = next;
  if (next.isEmpty()) {
    selectedIndex_.reset();
  } else {
    QList<int> sorted = next.values();
    std::sort(sorted.begin(), sorted.end());
    selectedIndex_ = sorted.first();
  }
  notify();
}

int PlaylistController::compareTracks(const Track& a, const Track& b, PlaylistSortKey key) {
  auto cmp = [](const QString& x, const QString& y) {
    return QString::localeAwareCompare(x.toLower(), y.toLower());
  };
  switch (key) {
    case PlaylistSortKey::title:
      return cmp(a.displayTitle(), b.displayTitle());
    case PlaylistSortKey::artist:
      return cmp(a.artist, b.artist);
    case PlaylistSortKey::duration: {
      const qint64 da = a.durationMs.value_or(-1);
      const qint64 db = b.durationMs.value_or(-1);
      if (da < db) return -1;
      if (da > db) return 1;
      return 0;
    }
    case PlaylistSortKey::path:
      return cmp(a.path, b.path);
  }
  return 0;
}

void PlaylistController::sortBy(PlaylistSortKey key) {
  if (tracks_.size() < 2) {
    return;
  }
  QSet<QString> selectedPaths;
  for (int i : selectedIndices_) {
    if (i >= 0 && i < tracks_.size()) {
      selectedPaths.insert(tracks_[i].path);
    }
  }
  const QString anchorPath =
      selectedIndex_ && *selectedIndex_ < tracks_.size() ? tracks_[*selectedIndex_].path
                                                         : QString();
  const QVector<Track> previous = tracks_;
  std::stable_sort(tracks_.begin(), tracks_.end(),
                   [key](const Track& a, const Track& b) { return compareTracks(a, b, key) < 0; });
  if (previous != tracks_) {
    altered_ = true;
  }
  selectedIndices_.clear();
  for (int i = 0; i < tracks_.size(); ++i) {
    if (selectedPaths.contains(tracks_[i].path)) {
      selectedIndices_.insert(i);
    }
  }
  if (anchorPath.isEmpty()) {
    if (selectedIndices_.isEmpty()) {
      selectedIndex_.reset();
    } else {
      QList<int> sorted = selectedIndices_.values();
      std::sort(sorted.begin(), sorted.end());
      selectedIndex_ = sorted.first();
    }
  } else {
    const int anchor = [&] {
      for (int i = 0; i < tracks_.size(); ++i) {
        if (tracks_[i].path == anchorPath) {
          return i;
        }
      }
      return -1;
    }();
    selectedIndex_ = anchor >= 0 ? std::optional<int>(anchor) : std::optional<int>{};
    if (selectedIndex_) {
      selectedIndices_.insert(*selectedIndex_);
    }
  }
  notify();
}

void PlaylistController::reverseTracks() {
  if (tracks_.size() < 2) {
    return;
  }
  std::reverse(tracks_.begin(), tracks_.end());
  altered_ = true;
  QSet<int> next;
  for (int i : selectedIndices_) {
    next.insert(tracks_.size() - 1 - i);
  }
  selectedIndices_ = next;
  if (selectedIndex_) {
    selectedIndex_ = tracks_.size() - 1 - *selectedIndex_;
  }
  notify();
}

void PlaylistController::clear() {
  if (!tracks_.isEmpty()) {
    altered_ = true;
  }
  tracks_.clear();
  sourcePath_.clear();
  selectedIndex_.reset();
  selectedIndices_.clear();
  notify();
}

bool PlaylistController::applyDurations(const QMap<QString, qint64>& durations) {
  if (durations.isEmpty()) return false;
  bool changed = false;
  for (Track& t : tracks_) {
    const QString n = normalizePlaylistPath(t.path);
    qint64 ms = 0;
    if (durations.contains(n)) ms = durations.value(n);
    else if (durations.contains(t.path)) ms = durations.value(t.path);
    if (ms <= 0) continue;
    if (t.durationMs && *t.durationMs == ms) continue;
    t.durationMs = ms;
    changed = true;
  }
  if (changed) notify();
  return changed;
}

bool PlaylistController::applyMetadata(const QString& path, const QString& title,
                                       const QString& artist, const QString& album,
                                       qint64 durationMs) {
  const QString n = normalizePlaylistPath(path);
  bool changed = false;
  for (Track& t : tracks_) {
    if (normalizePlaylistPath(t.path) != n && t.path != path) continue;
    auto take = [&](const QString& src, QString& dest) {
      const QString trimmed = src.trimmed();
      if (trimmed.isEmpty() || dest.trimmed() == trimmed) return;
      if (!dest.trimmed().isEmpty()) return;
      dest = trimmed;
      changed = true;
    };
    take(title, t.title);
    take(artist, t.artist);
    take(album, t.album);
    if (durationMs > 0 && (!t.durationMs || *t.durationMs != durationMs)) {
      t.durationMs = durationMs;
      changed = true;
    }
  }
  if (changed) notify();
  return changed;
}

void PlaylistController::markMissingPaths(const QSet<QString>& missingNormalized) {
  bool changed = false;
  for (Track& t : tracks_) {
    const bool missing = missingNormalized.contains(normalizePlaylistPath(t.path)) ||
                         missingNormalized.contains(t.path);
    if (t.disabled == missing) continue;
    t.disabled = missing;
    changed = true;
  }
  if (changed) notify();
}

bool PlaylistController::updateTrackByPath(const QString& path, const Track& next) {
  for (Track& t : tracks_) {
    if (t.path == path) {
      if (t.path == next.path && t.title == next.title && t.artist == next.artist &&
          t.album == next.album && t.durationMs == next.durationMs && t.disabled == next.disabled) {
        return false;
      }
      const bool disabled = t.disabled;
      t = next;
      t.disabled = disabled;
      notify();
      return true;
    }
  }
  return false;
}

bool PlaylistController::openPlaylistFile(const QString& path, const M3uCodec& codec) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }
  const QString contents = QString::fromUtf8(file.readAll());
  setTracks(codec.parse(contents, path), path);
  return true;
}

bool PlaylistController::savePlaylistFile(const QString& path, const M3uCodec& codec) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    return false;
  }
  file.write(codec.encode(tracks_).toUtf8());
  sourcePath_ = path;
  altered_ = false;
  notify();
  return true;
}

}  // namespace tramp
