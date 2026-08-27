#pragma once

#include "track.h"

#include <QByteArray>
#include <QString>
#include <QVector>
#include <functional>

namespace aoide {

/// Decodes playlist bytes to text. Honours a UTF-8 or UTF-16 byte-order mark and
/// falls back to Latin-1 when the bytes are not valid UTF-8, because playlists
/// written on Windows are routinely CP1252. Decoding everything as UTF-8 turned
/// those titles into replacement characters and left their paths unresolvable.
QString decodeM3uBytes(const QByteArray& bytes);

class M3uCodec {
 public:
  using Exists = std::function<bool(const QString&)>;

  explicit M3uCodec(Exists exists = {});

  QVector<Track> parse(const QString& contents, const QString& playlistFilePath) const;
  QString encode(const QVector<Track>& tracks) const;

 private:
  QString resolve(const QString& line, const QString& dir) const;
  static QStringList segments(const QString& line);

  Exists exists_;
};

}  // namespace aoide
