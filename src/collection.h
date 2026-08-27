#pragma once

#include "persist.h"

#include <QElapsedTimer>
#include <QSet>
#include <algorithm>
#include <functional>

namespace aoide {

/// What the collection knows about its own files comes from two passes of very
/// different size, and they are kept apart on purpose.
///
/// The **validation pass** asks about the *playlist files* — one question per
/// collection entry, dozens at most. It is cheap enough that a read can bring
/// it up to date, which is how a playlist file deleted or restored mid-session
/// is noticed without a restart.
///
/// The **track pass** asks about every track the collection references, which
/// is thousands on a real machine and can be a network share that has dropped.
/// It runs only on the events that change the answer — load, add, Refresh,
/// Save, and the explicit `validateReferences` at bootstrap — and never from a
/// read. `readFigures` is therefore pure: no repaint and no probe callback can
/// set a filesystem sweep going.
class PlaylistCollection {
 public:
  /// How the collection asks whether a file is on disk. `M3uCodec` takes the
  /// same kind of probe; tests swap it in to count the asking.
  using Exists = std::function<bool(const QString&)>;

  /// How long a validation pass over the playlist files stays good for.
  static constexpr int kValidationIntervalMs = 2000;

  void setExists(Exists exists) { exists_ = std::move(exists); }
  void setValidationIntervalMs(int ms) { validationIntervalMs_ = ms; }

  void load(const SupportStore& store);
  void saveIndex(const SupportStore& store) const;
  /// Writes the cache, collecting its garbage on the way out. A pass over what
  /// the collection still holds cannot leak by forgetting a call site, the way
  /// per-removal bookkeeping can.
  void saveTrackSets(const SupportStore& store);

  QVector<SavedPlaylist> entries() const { return entries_; }
  QString selectedPath() const { return selectedPath_; }
  /// Brings the validation pass up to date if it has gone stale — one question
  /// per entry, never per track.
  QSet<QString> disabledPaths() const;

  QVector<Track> add(const QString& path);
  void addWritten(const QString& path, const QVector<Track>& tracks);
  void remove(const QString& path);
  void select(const QString& path);
  void rename(const QString& path, const QString& name);
  bool contains(const QString& path) const;
  bool resolveForLoad(const QString& path, SavedPlaylist* out) const;
  /// Ask the disk about everything the collection references: the playlist
  /// files, and every track they name. The whole-collection pass lives here, at
  /// a moment the caller has chosen — bootstrap — rather than behind a getter.
  void validateReferences();
  QVector<Track> tracksFor(const QString& path) const;
  void hydrateDurations(QVector<Track>& tracks) const;
  void mergeTrackDuration(const QString& trackPath, qint64 durationMs);
  void mergeTrackTags(const QString& trackPath, const QString& title, const QString& artist,
                      const QString& album);
  /// A pure read of what the last track pass found. Called once per probed
  /// duration during an ingest, so it must not touch the filesystem.
  CollectionFigures readFigures() const;

 private:
  int indexOf(const QString& path) const;
  void sortEntries();
  void refreshFigures(SavedPlaylist& e, const QVector<Track>& tracks);
  bool onDisk(const QString& path) const;
  /// The validation pass: playlist files only, skipped while the last one is
  /// still fresh.
  void validatePlaylistFiles() const;
  /// The track pass, over everything the collection references. Only ever
  /// called from an event that changed the answer.
  void checkAllTrackFiles();
  /// The track pass for one list that has just been built — bounded by that
  /// playlist, which is the list the caller has already read off the disk.
  void checkTrackFiles(const QStringList& paths);

  QVector<SavedPlaylist> entries_;
  QString selectedPath_;
  CollectionTrackSets trackSets_;
  Exists exists_;
  bool trackSetsDirty_ = false;
  int validationIntervalMs_ = kValidationIntervalMs;
  /// Tracks the last pass found nothing for. A path nobody has asked about yet
  /// counts towards the figures: the honest failure is to over-count the
  /// listener's music, not to hide it.
  QSet<QString> missingTracks_;
  mutable QSet<QString> disabledPaths_;
  mutable QElapsedTimer validationAge_;
  mutable bool validationValid_ = false;
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
