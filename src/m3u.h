#pragma once

#include "track.h"

#include <QString>
#include <QVector>
#include <functional>

namespace tramp {

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

}  // namespace tramp
