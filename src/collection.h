#pragma once

#include "persist.h"

#include <QElapsedTimer>
#include <QSet>
#include <algorithm>

namespace tramp {

class PlaylistCollection {
 public:
  /// How long a look at the collection's files stays good for. Reads re-check
  /// on their own so a playlist file deleted or restored mid-session is noticed
  /// without a restart — but the Playlist Manager asks once per row per
  /// repaint, and a stat per ask would put a dropped network share in the paint
  /// path.
  static constexpr int kPresenceIntervalMs = 2000;

  void load(const SupportStore& store);
  void saveIndex(const SupportStore& store) const;
  /// Writes the cache, collecting its garbage on the way out. A pass over what
  /// the collection still holds cannot leak by forgetting a call site, the way
  /// per-removal bookkeeping can.
  void saveTrackSets(const SupportStore& store);

  QVector<SavedPlaylist> entries() const { return entries_; }
  QString selectedPath() const { return selectedPath_; }
  QSet<QString> disabledPaths() const;
  int figuresRevision() const { return figuresRevision_; }
  void setPresenceIntervalMs(int ms) { presenceIntervalMs_ = ms; }

  QVector<Track> add(const QString& path);
  void addWritten(const QString& path, const QVector<Track>& tracks);
  void remove(const QString& path);
  void select(const QString& path);
  void rename(const QString& path, const QString& name);
  bool contains(const QString& path) const;
  bool resolveForLoad(const QString& path, SavedPlaylist* out) const;
  void validateReferences();
  QVector<Track> tracksFor(const QString& path) const;
  void hydrateDurations(QVector<Track>& tracks) const;
  void mergeTrackDuration(const QString& trackPath, qint64 durationMs);
  void mergeTrackTags(const QString& trackPath, const QString& title, const QString& artist,
                      const QString& album);
  CollectionFigures readFigures() const;

 private:
  int indexOf(const QString& path) const;
  void sortEntries();
  void refreshFigures(SavedPlaylist& e, const QVector<Track>& tracks);
  void bumpFigures();
  /// Re-stat the playlist files and the tracks they name, unless the last look
  /// is still fresh. Everything that reports what is on this machine — the
  /// disabled set, the About figures — reads what this leaves behind.
  void refreshPresence() const;
  void forgetPresence() { presenceValid_ = false; }

  QVector<SavedPlaylist> entries_;
  QString selectedPath_;
  CollectionTrackSets trackSets_;
  int figuresRevision_ = 0;
  bool trackSetsDirty_ = false;
  int presenceIntervalMs_ = kPresenceIntervalMs;
  mutable QSet<QString> disabledPaths_;
  mutable QSet<QString> presentTracks_;
  mutable QElapsedTimer presenceAge_;
  mutable bool presenceValid_ = false;
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

}  // namespace tramp
