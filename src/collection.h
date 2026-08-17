#pragma once

#include "persist.h"

#include <QSet>
#include <algorithm>

namespace tramp {

class PlaylistCollection {
 public:
  void load(const SupportStore& store);
  void saveIndex(const SupportStore& store) const;
  void saveTrackSets(const SupportStore& store) const;

  QVector<SavedPlaylist> entries() const { return entries_; }
  QString selectedPath() const { return selectedPath_; }
  QSet<QString> disabledPaths() const { return disabledPaths_; }
  int figuresRevision() const { return figuresRevision_; }

  void add(const QString& path);
  void addWritten(const QString& path, const QVector<Track>& tracks);
  void remove(const QString& path);
  void select(const QString& path);
  void rename(const QString& path, const QString& name);
  bool resolveForLoad(const QString& path, SavedPlaylist* out) const;
  void validateReferences();
  CollectionFigures readFigures() const;

 private:
  int indexOf(const QString& path) const;
  void sortEntries();
  void refreshFigures(SavedPlaylist& e, const QVector<Track>& tracks);
  void bumpFigures();

  QVector<SavedPlaylist> entries_;
  QString selectedPath_;
  QSet<QString> disabledPaths_;
  CollectionTrackSets trackSets_;
  int figuresRevision_ = 0;
  bool trackSetsDirty_ = false;
};

/// Left-panel highlight follows the loaded current playlist, not merely the
/// last row click (which is empty after restart until something is clicked).
inline QString collectionHighlightPath(const QString& currentSourcePath,
                                       const QString& selectedPath) {
  if (!currentSourcePath.isEmpty()) return normalizePlaylistPath(currentSourcePath);
  return selectedPath;
}

}  // namespace tramp
