#include "collection.h"

#include "m3u.h"

#include <QFile>
#include <QFileInfo>

namespace aoide {

void PlaylistCollection::load(const SupportStore& store) {
  entries_ = store.readCollectionIndex();
  trackSets_ = store.readTrackSets();
  sortEntries();
  // Reading the state files says nothing about what is still on the disk, and
  // this is not the place to go and ask — `validateReferences` is, right after
  // this, where the session has already decided to wait.
  validationValid_ = false;
  missingTracks_.clear();
}

bool PlaylistCollection::onDisk(const QString& path) const {
  return exists_ ? exists_(path) : QFileInfo::exists(path);
}

void PlaylistCollection::saveIndex(const SupportStore& store) const {
  store.writeCollectionIndex(entries_);
}

void PlaylistCollection::saveTrackSets(const SupportStore& store) {
  QSet<QString> live;
  for (const SavedPlaylist& e : entries_) live.insert(e.path);
  trackSets_ = pruneTrackSets(trackSets_, live);
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
    if (!t.title.trimmed().isEmpty() || !t.artist.trimmed().isEmpty() ||
        !t.album.trimmed().isEmpty()) {
      CachedTrackMeta tags = trackSets_.meta.value(n);
      if (!t.title.trimmed().isEmpty()) tags.title = t.title.trimmed();
      if (!t.artist.trimmed().isEmpty()) tags.artist = t.artist.trimmed();
      if (!t.album.trimmed().isEmpty()) tags.album = t.album.trimmed();
      trackSets_.meta.insert(n, tags);
    }
  }
  e.totalDurationMs = total;
  e.modifiedMs = QFileInfo(e.path).lastModified().toMSecsSinceEpoch();
  trackSets_.byEntry.insert(e.path, paths);
  trackSetsDirty_ = true;
  // This list was just read off the disk — on add, on Refresh, on Save — so
  // asking about its own tracks is bounded by the playlist the caller already
  // paid to read. It is also the only way a mid-session deletion reaches the
  // figures, and it is what makes Refresh tell the truth.
  checkTrackFiles(paths);
  validationValid_ = false;
}

void PlaylistCollection::hydrateDurations(QVector<Track>& tracks) const {
  for (Track& t : tracks) {
    const QString n = normalizePlaylistPath(t.path);
    if (!t.durationMs || *t.durationMs <= 0) {
      const qint64 cached = trackSets_.durationsMs.value(n, 0);
      if (cached > 0) t.durationMs = cached;
      else t.durationMs.reset();
    }
    const CachedTrackMeta tags = trackSets_.meta.value(n);
    auto take = [](const QString& src, QString& dest) {
      if (dest.trimmed().isEmpty() && !src.trimmed().isEmpty()) dest = src;
    };
    take(tags.title, t.title);
    take(tags.artist, t.artist);
    take(tags.album, t.album);
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
}

void PlaylistCollection::mergeTrackTags(const QString& trackPath, const QString& title,
                                        const QString& artist, const QString& album) {
  const QString n = normalizePlaylistPath(trackPath);
  CachedTrackMeta tags = trackSets_.meta.value(n);
  auto take = [](const QString& src, QString& dest) -> bool {
    const QString trimmed = src.trimmed();
    if (trimmed.isEmpty() || dest.trimmed() == trimmed) return false;
    if (!dest.trimmed().isEmpty()) return false;
    dest = trimmed;
    return true;
  };
  bool changed = false;
  changed = take(title, tags.title) || changed;
  changed = take(artist, tags.artist) || changed;
  changed = take(album, tags.album) || changed;
  if (!changed) return;
  trackSets_.meta.insert(n, tags);
  trackSetsDirty_ = true;
}

QVector<Track> PlaylistCollection::add(const QString& path) {
  const QString n = normalizePlaylistPath(path);
  QVector<Track> tracks;
  QFile f(n);
  if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    tracks = M3uCodec().parse(decodeM3uBytes(f.readAll()), n);
    hydrateDurations(tracks);
  }
  const int existing = indexOf(n);
  if (existing >= 0) {
    // A re-add of a playlist whose file has gone keeps the figures it had: an
    // unreadable list would otherwise zero them. Asked through the same probe
    // as every other existence check, so a test double is believed here too.
    if (onDisk(n)) refreshFigures(entries_[existing], tracks);
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
  // Nothing to ask the disk: the figures walk the lists that are left, so the
  // paths of a playlist that has gone are simply never visited again. They stay
  // in `missingTracks_` until something asks about them, which costs nothing
  // and is corrected if the playlist comes back.
  validationValid_ = false;
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
  if (out) *out = entries_[i];
  return true;
}

void PlaylistCollection::validateReferences() {
  validationValid_ = false;
  validatePlaylistFiles();
  checkAllTrackFiles();
}

void PlaylistCollection::validatePlaylistFiles() const {
  if (validationValid_ && validationAge_.isValid() &&
      validationAge_.elapsed() < validationIntervalMs_) {
    return;
  }
  disabledPaths_.clear();
  for (const SavedPlaylist& e : entries_) {
    if (!onDisk(e.path)) disabledPaths_.insert(e.path);
  }
  validationValid_ = true;
  validationAge_.restart();
}

void PlaylistCollection::checkAllTrackFiles() {
  QSet<QString> reachable;
  for (const SavedPlaylist& e : entries_) {
    for (const QString& track : trackSets_.byEntry.value(e.path)) reachable.insert(track);
  }
  missingTracks_.clear();
  for (const QString& track : reachable) {
    if (!onDisk(track)) missingTracks_.insert(track);
  }
}

void PlaylistCollection::checkTrackFiles(const QStringList& paths) {
  for (const QString& track : paths) {
    if (onDisk(track)) missingTracks_.remove(track);
    else missingTracks_.insert(track);
  }
}

QSet<QString> PlaylistCollection::disabledPaths() const {
  validatePlaylistFiles();
  return disabledPaths_;
}

QVector<Track> PlaylistCollection::tracksFor(const QString& path) const {
  const QString n = normalizePlaylistPath(path);
  QVector<Track> tracks;
  for (const QString& p : trackSets_.byEntry.value(n)) {
    Track t;
    t.path = p;
    const qint64 cached = trackSets_.durationsMs.value(p, 0);
    if (cached > 0) t.durationMs = cached;
    const CachedTrackMeta tags = trackSets_.meta.value(p);
    t.title = tags.title;
    t.artist = tags.artist;
    t.album = tags.album;
    tracks.push_back(t);
  }
  return tracks;
}

CollectionFigures PlaylistCollection::readFigures() const {
  // The stats well is headed ON THIS MACHINE, so it counts the files that are
  // on it. A track the collection remembers but the disk no longer has is not
  // dropped from the cache — it comes back with its file — it just does not
  // count while it is missing. What the last track pass found is read here and
  // nothing more: this runs once per probed duration during an ingest.
  CollectionFigures fig;
  fig.playlists = entries_.size();
  QSet<QString> unique;
  qint64 total = 0;
  for (const SavedPlaylist& e : entries_) {
    const QStringList paths = trackSets_.byEntry.value(e.path);
    for (const QString& p : paths) {
      if (unique.contains(p) || missingTracks_.contains(p)) continue;
      unique.insert(p);
      total += trackSets_.durationsMs.value(p, 0);
    }
  }
  fig.tracks = unique.size();
  fig.totalDurationMs = total;
  return fig;
}

CollectionTrackSets pruneTrackSets(const CollectionTrackSets& sets,
                                   const QSet<QString>& livePlaylists) {
  QSet<QString> live;
  for (const QString& p : livePlaylists) live.insert(normalizePlaylistPath(p));

  CollectionTrackSets kept;
  QSet<QString> referenced;
  for (auto it = sets.byEntry.begin(); it != sets.byEntry.end(); ++it) {
    if (!live.contains(normalizePlaylistPath(it.key()))) continue;
    kept.byEntry.insert(it.key(), it.value());
    for (const QString& track : it.value()) referenced.insert(normalizePlaylistPath(track));
  }
  for (auto it = sets.durationsMs.begin(); it != sets.durationsMs.end(); ++it) {
    if (referenced.contains(normalizePlaylistPath(it.key()))) {
      kept.durationsMs.insert(it.key(), it.value());
    }
  }
  for (auto it = sets.meta.begin(); it != sets.meta.end(); ++it) {
    if (referenced.contains(normalizePlaylistPath(it.key()))) {
      kept.meta.insert(it.key(), it.value());
    }
  }
  return kept;
}

QVector<Track> dropMissingTrackFiles(const QVector<Track>& tracks) {
  QVector<Track> kept;
  kept.reserve(tracks.size());
  for (const Track& t : tracks) {
    if (QFileInfo::exists(t.path)) kept.push_back(t);
  }
  return kept;
}

}  // namespace aoide
