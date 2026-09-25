#include "collection.h"

#include "m3u.h"

#include <QFile>
#include <QFileInfo>

namespace aoide {

void PlaylistCollection::load(const SupportStore& store) {
  entries_ = store.readCollectionIndex();
  groups_ = store.readPlaylistGroups();
  favorites_ = store.readFavorites();
  trackSets_ = store.readTrackSets();
  for (SavedPlaylist& entry : entries_) {
    if (isReservedPlaylistName(entry.displayName())) entry.name = availableSavedFavoritesName();
  }
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

bool PlaylistCollection::saveIndex(const SupportStore& store) const {
  const bool indexOk = store.writeCollectionIndex(entries_);
  const bool groupsOk = store.writePlaylistGroups(groups_);
  const bool favoritesOk = store.writeFavorites(favorites_);
  return indexOk && groupsOk && favoritesOk;
}

bool PlaylistCollection::saveTrackSets(const SupportStore& store) {
  QSet<QString> live;
  for (const SavedPlaylist& e : entries_) live.insert(e.path);
  trackSets_ = pruneTrackSets(trackSets_, live);
  return store.writeTrackSets(trackSets_);
}

bool PlaylistCollection::renameGroup(int id, const QString& name) {
  const QString trimmed = name.trimmed();
  if (id < 0 || id >= groups_.size() || trimmed.isEmpty()) return false;
  if (groups_[id].name == trimmed) return false;
  groups_[id].name = trimmed;
  return true;
}

bool PlaylistCollection::setGroups(const QString& path, const QSet<int>& groupIds) {
  const int index = indexOf(path);
  if (index < 0) return false;
  for (int id : groupIds) {
    if (id < 0 || id >= groups_.size()) return false;
  }
  if (entries_[index].groupIds == groupIds) return false;
  entries_[index].groupIds = groupIds;
  return true;
}

SavedPlaylist PlaylistCollection::favoritesEntry() const {
  SavedPlaylist entry;
  entry.path = favoritesPlaylistPath();
  entry.name = QStringLiteral("Favorites");
  entry.trackCount = favorites_.size();
  for (const Track& track : favorites_) {
    entry.totalDurationMs += qMax<qint64>(0, track.durationMs.value_or(0));
  }
  return entry;
}

QVector<Track> PlaylistCollection::favoriteTracks() const {
  QVector<Track> tracks = favorites_;
  for (Track& track : tracks) track.disabled = missingTracks_.contains(track.path);
  return tracks;
}

bool PlaylistCollection::isFavorite(const QString& trackPath) const {
  if (trackPath.isEmpty()) return false;
  const QString path = normalizePlaylistPath(trackPath);
  return std::any_of(favorites_.cbegin(), favorites_.cend(), [&](const Track& track) {
    return track.path == path;
  });
}

bool PlaylistCollection::setFavorite(const Track& track, bool favorite) {
  if (track.path.isEmpty() || track.path.contains(QChar::Null) ||
      isFavoritesPlaylist(track.path)) return false;
  const QString path = normalizePlaylistPath(track.path);
  for (int i = 0; i < favorites_.size(); ++i) {
    if (favorites_[i].path != path) continue;
    if (favorite) return false;
    favorites_.removeAt(i);
    return true;
  }
  if (!favorite) return false;
  QVector<Track> tracks{track};
  tracks[0].path = path;
  hydrateDurations(tracks);
  favorites_.push_back(tracks[0]);
  checkTrackFiles({path});
  return true;
}

int PlaylistCollection::indexOf(const QString& path) const {
  const QString n = normalizePlaylistPath(path);
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_[i].path == n) return i;
  }
  return -1;
}

QString PlaylistCollection::availableSavedFavoritesName() const {
  QSet<QString> names;
  for (const SavedPlaylist& entry : entries_) {
    names.insert(entry.displayName().normalized(QString::NormalizationForm_KC).toCaseFolded());
  }
  QString name = QStringLiteral("Favorites (saved)");
  int suffix = 2;
  while (names.contains(name.toCaseFolded())) {
    name = QStringLiteral("Favorites (saved %1)").arg(suffix++);
  }
  return name;
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
    if (trackMetadata(t).hasTags()) {
      Track cached;
      applyTrackMetadata(cached, trackSets_.meta.value(n), true);
      applyTrackMetadata(cached, trackMetadata(t), true);
      cached.durationMs.reset();
      trackSets_.meta.insert(n, trackMetadata(cached));
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
    applyTrackMetadata(t, trackSets_.meta.value(n), false);
  }
}

void PlaylistCollection::mergeTrackDuration(const QString& trackPath, qint64 durationMs) {
  if (durationMs <= 0) return;
  const QString n = normalizePlaylistPath(trackPath);
  for (Track& favorite : favorites_) {
    if (favorite.path == n) favorite.durationMs = durationMs;
  }
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

void PlaylistCollection::mergeTrackTags(const QString& trackPath, const TrackMetadata& metadata,
                                         bool overwrite) {
  const QString n = normalizePlaylistPath(trackPath);
  for (Track& favorite : favorites_) {
    if (favorite.path == n) applyTrackMetadata(favorite, metadata, overwrite);
  }
  Track cached;
  applyTrackMetadata(cached, trackSets_.meta.value(n), true);
  const Track previous = cached;
  applyTrackMetadata(cached, metadata, overwrite);
  // Durations have their own cache and figures; this entry holds tags only.
  cached.durationMs.reset();
  if (cached == previous) return;
  trackSets_.meta.insert(n, trackMetadata(cached));
  trackSetsDirty_ = true;
}

QVector<Track> PlaylistCollection::add(const QString& path) {
  if (isFavoritesPlaylist(path)) {
    selectedPath_ = favoritesPlaylistPath();
    return favoriteTracks();
  }
  const QString n = normalizePlaylistPath(path);
  QVector<Track> tracks;
  QFile f(n);
  if (f.open(QIODevice::ReadOnly)) {
    const M3uCodec codec;
    const QString contents = codec.decode(f.readAll(), n).text;
    // A NUL means this is not a playlist (audio, or any binary). Parsing it
    // as M3U is unbounded work and not a useful result.
    if (!isPlaylistText(contents)) return {};
    tracks = codec.parse(contents, n);
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
  if (isReservedPlaylistName(e.displayName())) e.name = availableSavedFavoritesName();
  refreshFigures(e, tracks);
  entries_.push_back(e);
  selectedPath_ = n;
  sortEntries();
  return tracks;
}

void PlaylistCollection::addWritten(const QString& path, const QVector<Track>& tracks) {
  if (isFavoritesPlaylist(path)) return;
  const QString n = normalizePlaylistPath(path);
  QVector<Track> hydrated = tracks;
  hydrateDurations(hydrated);
  int i = indexOf(n);
  if (i < 0) {
    SavedPlaylist e;
    e.path = n;
    if (isReservedPlaylistName(e.displayName())) e.name = availableSavedFavoritesName();
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
  if (isFavoritesPlaylist(path)) {
    selectedPath_ = favoritesPlaylistPath();
    return;
  }
  const int i = indexOf(path);
  selectedPath_ = i >= 0 ? entries_[i].path : QString();
}

void PlaylistCollection::rename(const QString& path, const QString& name) {
  const int i = indexOf(path);
  if (i < 0) return;
  const QString displayName = name.trimmed().isEmpty() ? QFileInfo(entries_[i].path).completeBaseName()
                                                      : name.trimmed();
  if (isReservedPlaylistName(displayName)) return;
  entries_[i].name = name.trimmed();
  sortEntries();
}

bool PlaylistCollection::contains(const QString& path) const {
  return isFavoritesPlaylist(path) || indexOf(path) >= 0;
}

bool PlaylistCollection::resolveForLoad(const QString& path, SavedPlaylist* out) const {
  if (isFavoritesPlaylist(path)) {
    if (out) *out = favoritesEntry();
    return true;
  }
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
  for (const Track& track : favorites_) reachable.insert(track.path);
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
  if (isFavoritesPlaylist(path)) return favoriteTracks();
  const QString n = normalizePlaylistPath(path);
  QVector<Track> tracks;
  for (const QString& p : trackSets_.byEntry.value(n)) {
    Track t;
    t.path = p;
    const qint64 cached = trackSets_.durationsMs.value(p, 0);
    if (cached > 0) t.durationMs = cached;
    applyTrackMetadata(t, trackSets_.meta.value(p), false);
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
  for (const Track& track : favorites_) {
    if (unique.contains(track.path) || missingTracks_.contains(track.path)) continue;
    unique.insert(track.path);
    total += qMax<qint64>(0, track.durationMs.value_or(0));
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
