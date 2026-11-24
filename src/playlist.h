#pragma once

#include "m3u.h"
#include "track.h"

#include <QHash>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>
#include <functional>
#include <optional>

namespace aoide {

enum class PlaylistSortKey { title, artist, duration, path };

/// Writes `tracks` to `path` as M3U through a temporary that is renamed on
/// commit, so a failed or interrupted write leaves the listener's existing file
/// untouched. Returns false when nothing was written.
bool writeM3uFile(const QString& path, const QVector<Track>& tracks,
                  const M3uCodec& codec = M3uCodec());

/// Exact on-disk representation, including the platform line ending.
QByteArray m3uFileBytes(const QVector<Track>& tracks, const M3uCodec& codec = M3uCodec());
bool writeM3uBytes(const QString& path, const QByteArray& contents,
                   const QByteArray* expectedContents = nullptr);

class PlaylistController {
 public:
  // A single-row removal identifies its former position. A replacement
  // can have identical remaining paths and still leave playback off-list.
  using Changed = std::function<void(std::optional<int> removedIndex)>;

  QVector<Track> tracks() const { return tracks_; }
  QString sourcePath() const { return sourcePath_; }
  std::optional<int> selectedIndex() const { return selectedIndex_; }
  QSet<int> selectedIndices() const { return selectedIndices_; }
  bool altered() const { return altered_; }

  void setOnChanged(Changed cb) { onChanged_ = std::move(cb); }

  void setTracks(const QVector<Track>& tracks, const QString& sourcePath = {});
  /// A different playlist, not a re-read of the same file. Clamping the old
  /// highlight would light a row the listener never chose in this list.
  void loadTracks(const QVector<Track>& tracks, const QString& sourcePath = {});
  void restoreAlteredTracks(const QVector<Track>& tracks, const QString& sourcePath = {});
  void addTracks(const QVector<Track>& tracks);
  void removeAt(int index);
  void removeSelected();
  void move(int oldIndex, int newIndex);
  void select(int index);
  void selectRange(int index);
  void toggleSelection(int index);
  void selectIndices(const QSet<int>& indices);
  void selectAll();
  void invertSelection();
  void sortBy(PlaylistSortKey key);
  void reverseTracks();
  void clear();
  bool updateTrackByPath(const QString& path, const Track& next);
  bool applyDurations(const QMap<QString, qint64>& durations);
  bool applyMetadata(const QString& path, const TrackMetadata& metadata);
  bool applyMetadataBatch(const QHash<QString, TrackMetadata>& byNormalizedPath, bool overwrite);
  void markMissingPaths(const QSet<QString>& missingNormalized);

  bool openPlaylistFile(const QString& path, const M3uCodec& codec = M3uCodec());
  bool savePlaylistFile(const QString& path, const M3uCodec& codec = M3uCodec());

 private:
  void notify(std::optional<int> removedIndex = std::nullopt);
  void replaceTracks(const QVector<Track>& tracks, const QString& sourcePath);
  static int compareTracks(const Track& a, const Track& b, PlaylistSortKey key);

  QVector<Track> tracks_;
  QString sourcePath_;
  std::optional<int> selectedIndex_;
  QSet<int> selectedIndices_;
  bool altered_ = false;
  Changed onChanged_;
};

}  // namespace aoide
