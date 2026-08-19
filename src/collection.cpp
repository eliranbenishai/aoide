#include "collection.h"

#include "m3u.h"

#include <QFile>
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
    if (t.durationMs && *t.durationMs > 0) {
      total += *t.durationMs;
      trackSets_.durationsMs.insert(n, *t.durationMs);
    } else {
      total += qMax<qint64>(0, trackSets_.durationsMs.value(n, 0));
    }
  }
  e.totalDurationMs = total;
  e.modifiedMs = QFileInfo(e.path).lastModified().toMSecsSinceEpoch();
  trackSets_.byEntry.insert(e.path, paths);
  trackSetsDirty_ = true;
  bumpFigures();
}

void PlaylistCollection::hydrateDurations(QVector<Track>& tracks) const {
  for (Track& t : tracks) {
    if (t.durationMs && *t.durationMs > 0) continue;
    const QString n = normalizePlaylistPath(t.path);
    const qint64 cached = trackSets_.durationsMs.value(n, 0);
    if (cached > 0) t.durationMs = cached;
    else t.durationMs.reset();
  }
}

void PlaylistCollection::mergeTrackDuration(const QString& trackPath, qint64 durationMs) {
  if (durationMs <= 0) return;
  const QString n = normalizePlaylistPath(trackPath);
  if (trackSets_.durationsMs.value(n, -1) == durationMs) return;
  trackSets_.durationsMs.insert(n, durationMs);
  trackSetsDirty_ = true;
  for (SavedPlaylist& e : entries_) {
    const QStringList paths = trackSets_.byEntry.value(e.path);
    qint64 total = 0;
    bool hit = false;
    for (const QString& p : paths) {
      if (p == n) hit = true;
      total += trackSets_.durationsMs.value(p, 0);
    }
    if (hit) e.totalDurationMs = total;
  }
  bumpFigures();
}

QVector<Track> PlaylistCollection::add(const QString& path) {
  const QString n = normalizePlaylistPath(path);
  QVector<Track> tracks;
  QFile f(n);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    tracks = M3uCodec().parse(QString::fromUtf8(f.readAll()), n);
    hydrateDurations(tracks);
  }
  const int existing = indexOf(n);
  if (existing >= 0) {
    if (QFileInfo::exists(n)) refreshFigures(entries_[existing], tracks);
    selectedPath_ = entries_[existing].path;
    return tracks;
  }
  SavedPlaylist e;
  e.path = n;
  refreshFigures(e, tracks);
  entries_.push_back(e);
  selectedPath_ = n;
  sortEntries();
  return tracks;
}

void PlaylistCollection::addWritten(const QString& path, const QVector<Track>& tracks) {
  const QString n = normalizePlaylistPath(path);
  QVector<Track> hydrated = tracks;
  hydrateDurations(hydrated);
  int i = indexOf(n);
  if (i < 0) {
    SavedPlaylist e;
    e.path = n;
    entries_.push_back(e);
    i = entries_.size() - 1;
  }
  refreshFigures(entries_[i], hydrated);
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

bool PlaylistCollection::contains(const QString& path) const { return indexOf(path) >= 0; }

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
    auto tracks = M3uCodec().parse(QString::fromUtf8(f.readAll()), e.path);
    hydrateDurations(tracks);
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
