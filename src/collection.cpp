#include "collection.h"

#include "m3u.h"

#include <QFileInfo>

namespace tramp {

void PlaylistCollection::load(const SupportStore& store) {
  entries_ = store.readCollectionIndex();
  trackSets_ = store.readTrackSets();
  sortEntries();
}

void PlaylistCollection::saveIndex(const SupportStore& store) const {
  store.writeCollectionIndex(entries_);
}

void PlaylistCollection::saveTrackSets(const SupportStore& store) const {
  store.writeTrackSets(trackSets_);
}

int PlaylistCollection::indexOf(const QString& path) const {
  const QString n = normalizePlaylistPath(path);
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_[i].path == n) return i;
  }
  return -1;
}

void PlaylistCollection::sortEntries() {
  std::sort(entries_.begin(), entries_.end(), [](const SavedPlaylist& a, const SavedPlaylist& b) {
    const int byName = a.displayName().toLower().compare(b.displayName().toLower());
    if (byName != 0) return byName < 0;
    return a.path < b.path;
  });
}

void PlaylistCollection::bumpFigures() { ++figuresRevision_; }

void PlaylistCollection::refreshFigures(SavedPlaylist& e, const QVector<Track>& tracks) {
  e.trackCount = tracks.size();
  qint64 total = 0;
  QStringList paths;
  for (const Track& t : tracks) {
    const QString n = normalizePlaylistPath(t.path);
    paths.push_back(n);
    if (t.durationMs) {
      total += *t.durationMs;
      trackSets_.durationsMs.insert(n, *t.durationMs);
    }
  }
  e.totalDurationMs = total;
  e.modifiedMs = QFileInfo(e.path).lastModified().toMSecsSinceEpoch();
  trackSets_.byEntry.insert(e.path, paths);
  trackSetsDirty_ = true;
  bumpFigures();
}

void PlaylistCollection::add(const QString& path) {
  const QString n = normalizePlaylistPath(path);
  const int existing = indexOf(n);
  if (existing >= 0) {
    selectedPath_ = entries_[existing].path;
    return;
  }
  SavedPlaylist e;
  e.path = n;
  QFile f(n);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    const auto tracks = M3uCodec().parse(QString::fromUtf8(f.readAll()), n);
    refreshFigures(e, tracks);
  }
  entries_.push_back(e);
  selectedPath_ = n;
  sortEntries();
}

void PlaylistCollection::addWritten(const QString& path, const QVector<Track>& tracks) {
  const QString n = normalizePlaylistPath(path);
  int i = indexOf(n);
  if (i < 0) {
    SavedPlaylist e;
    e.path = n;
    entries_.push_back(e);
    i = entries_.size() - 1;
  }
  refreshFigures(entries_[i], tracks);
  selectedPath_ = n;
  sortEntries();
}

void PlaylistCollection::remove(const QString& path) {
  const int i = indexOf(path);
  if (i < 0) return;
  const QString n = entries_[i].path;
  entries_.removeAt(i);
  trackSets_.byEntry.remove(n);
  if (selectedPath_ == n) selectedPath_.clear();
  disabledPaths_.remove(n);
  bumpFigures();
}

void PlaylistCollection::select(const QString& path) {
  const int i = indexOf(path);
  selectedPath_ = i >= 0 ? entries_[i].path : QString();
}

void PlaylistCollection::rename(const QString& path, const QString& name) {
  const int i = indexOf(path);
  if (i < 0) return;
  entries_[i].name = name.trimmed();
  sortEntries();
}

bool PlaylistCollection::resolveForLoad(const QString& path, SavedPlaylist* out) const {
  const int i = indexOf(path);
  if (i < 0) return false;
  const QString n = entries_[i].path;
  if (disabledPaths_.contains(n) || !QFileInfo::exists(n)) return false;
  if (out) *out = entries_[i];
  return true;
}

void PlaylistCollection::validateReferences() {
  disabledPaths_.clear();
  for (SavedPlaylist& e : entries_) {
    const QFileInfo info(e.path);
    if (!info.exists()) {
      disabledPaths_.insert(e.path);
      continue;
    }
    const qint64 mtime = info.lastModified().toMSecsSinceEpoch();
    if (e.modifiedMs != 0 && mtime == e.modifiedMs) continue;
    QFile f(e.path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      disabledPaths_.insert(e.path);
      continue;
    }
    const auto tracks = M3uCodec().parse(QString::fromUtf8(f.readAll()), e.path);
    refreshFigures(e, tracks);
  }
}

CollectionFigures PlaylistCollection::readFigures() const {
  CollectionFigures fig;
  fig.playlists = entries_.size();
  QSet<QString> unique;
  qint64 total = 0;
  for (const SavedPlaylist& e : entries_) {
    const QStringList paths = trackSets_.byEntry.value(e.path);
    for (const QString& p : paths) {
      if (unique.contains(p)) continue;
      unique.insert(p);
      total += trackSets_.durationsMs.value(p, 0);
    }
  }
  fig.tracks = unique.size();
  fig.totalDurationMs = total;
  return fig;
}

}  // namespace tramp
