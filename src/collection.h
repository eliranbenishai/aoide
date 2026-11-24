#pragma once

#include "persist.h"

#include <QSet>
#include <algorithm>
#include <functional>
#include <optional>

namespace aoide {

class PlaylistCollection {
 public:
  /// How the collection asks whether a file is on disk. `M3uCodec` takes the
  /// same kind of probe; tests swap it in to count the asking.
  using Exists = std::function<bool(const QString&)>;

  void setExists(Exists exists) { exists_ = std::move(exists); }

  void load(const SupportStore& store);
  bool saveIndex(const SupportStore& store) const;
  /// Writes the cache, collecting its garbage on the way out. A pass over what
  /// the collection still holds cannot leak by forgetting a call site, the way
  /// per-removal bookkeeping can.
  bool saveTrackSets(const SupportStore& store);

  QVector<SavedPlaylist> entries() const { return entries_; }
  QVector<PlaylistGroup> groups() const { return groups_; }
  bool renameGroup(int id, const QString& name);
  bool setGroups(const QString& path, const QSet<int>& groupIds);
  /// The automatic playlist is separate from saved file references. Its track
  /// records survive removal of any saved playlist that also names them.
  SavedPlaylist favoritesEntry() const;
  QVector<Track> favoriteTracks() const;
  bool isFavorite(const QString& trackPath) const;
  bool setFavorite(const Track& track, bool favorite);
  QString selectedPath() const { return selectedPath_; }
  const QSet<QString>& disabledPaths() const { return disabledPaths_; }

  QVector<Track> add(const QString& path);
  void addWritten(const QString& path, const QVector<Track>& tracks);
  void remove(const QString& path);
  void select(const QString& path);
  void rename(const QString& path, const QString& name);
  bool contains(const QString& path) const;
  bool resolveForLoad(const QString& path, SavedPlaylist* out) const;
  struct Presence {
    quint64 revision;
    QSet<QString> missingPlaylists;
    std::optional<QSet<QString>> missingTracks;
  };
  using PresenceCheck = std::function<std::optional<Presence>(const std::function<bool()>&)>;
  // Capture paths on the owner thread. The returned check only uses that
  // snapshot and can do its blocking filesystem calls on a worker.
  PresenceCheck preparePresenceCheck(bool includeTracks, const QString& currentSource = {}) const;
  bool applyPresence(Presence presence);
  QVector<Track> tracksFor(const QString& path) const;
  void hydrateDurations(QVector<Track>& tracks) const;
  void mergeTrackDuration(const QString& trackPath, qint64 durationMs);
  void mergeTrackTags(const QString& trackPath, const TrackMetadata& metadata,
                      bool overwrite = false);
  /// A pure read of what the last track pass found. Called once per probed
  /// duration during an ingest, so it must not touch the filesystem.
  CollectionFigures readFigures() const;

 private:
  int indexOf(const QString& path) const;
  void updateCachedDuration(const QString& path, qint64 durationMs);
  void indexPlaylistTracks(SavedPlaylist& entry);
  void forgetPlaylistTracks(const QString& playlist);
  QString availableSavedFavoritesName() const;
  void sortEntries();
  void refreshFigures(SavedPlaylist& e, const QVector<Track>& tracks);
  bool onDisk(const QString& path) const;
  /// The track pass for one list that has just been built — bounded by that
  /// playlist, which is the list the caller has already read off the disk.
  void checkTrackFiles(const QStringList& paths);

  QVector<SavedPlaylist> entries_;
  QVector<PlaylistGroup> groups_ = defaultPlaylistGroups();
  QVector<Track> favorites_;
  QString selectedPath_;
  CollectionTrackSets trackSets_;
  QMultiHash<QString, QString> playlistsByTrack_;
  Exists exists_;
  bool trackSetsDirty_ = false;
  quint64 referenceRevision_ = 0;
  /// Tracks the last pass found nothing for. A path nobody has asked about yet
  /// counts towards the figures: the honest failure is to over-count the
  /// listener's music, not to hide it.
  QSet<QString> missingTracks_;
  QSet<QString> disabledPaths_;
};

/// Left-panel highlight follows the loaded current playlist, not merely the
/// last row click (which is empty after restart until something is clicked).
inline QString collectionHighlightPath(const QString& currentSourcePath,
                                       const QString& selectedPath) {
  if (!currentSourcePath.isEmpty()) return normalizePlaylistPath(currentSourcePath);
  return selectedPath;
}

QVector<Track> dropMissingTrackFiles(const QVector<Track>& tracks);

/// Garbage-collect the track-set cache: keep the list of every playlist still
/// in the collection, and the durations and tags those lists mention. Drop the
/// rest.
///
/// A **disabled playlist** keeps its list — the entry survives, the file is
/// what is gone, and the cache is the only thing left to paint the list from.
/// Paths are compared normalized on both sides, because a key that never got
/// normalized is a leak, while dropping one is a duration the app cannot get
/// back without re-probing the file.
CollectionTrackSets pruneTrackSets(const CollectionTrackSets& sets,
                                   const QSet<QString>& livePlaylists);

}  // namespace aoide
