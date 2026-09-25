#pragma once

#include "track.h"

#include <QByteArray>
#include <QString>
#include <QVector>
#include <functional>

namespace aoide {

/// Compatibility helper for callers without a playlist path or review UI.
/// Preserves Unicode and falls back to Windows-1252 for legacy bytes. Use
/// M3uCodec::decode to detect Hebrew text and review uncertain encodings.
QString decodeM3uBytes(const QByteArray& bytes);

struct M3uTextAlternative {
  QString text;
  QString encoding;
  /// Existing paths among at most 256 distinct hints, when comparing choices.
  int matchedPaths = 0;
};

struct M3uDecodeResult {
  QString text;
  QString encoding;
  bool recovered = false;
  bool needsReview = false;
  int matchedPaths = 0;
  /// Includes the chosen text first, then distinct alternatives. Encoding
  /// labels are unique within a result and suitable for an import selector.
  QVector<M3uTextAlternative> alternatives;
};

/// Decoded playlist text with a NUL in it is not a playlist. A filename
/// cannot contain NUL, and UTF-8 / legacy playlist text has none; UTF-16
/// with a BOM is already handled by decodeM3uBytes.
inline bool isPlaylistText(const QString& contents) {
  return !contents.contains(QChar::Null);
}

class M3uCodec {
 public:
  using Exists = std::function<bool(const QString&)>;

  explicit M3uCodec(Exists exists = {});

  /// Existing paths are evidence for legacy encodings; script/language alone
  /// is not. An ambiguous result requires review before accepting its text.
  M3uDecodeResult decode(const QByteArray& bytes, const QString& playlistFilePath = {}) const;
  QVector<Track> parse(const QString& contents, const QString& playlistFilePath) const;
  QString encode(const QVector<Track>& tracks) const;

 private:
  QString resolve(const QString& line, const QString& dir) const;
  static QStringList segments(const QString& line);

  Exists exists_;
};

}  // namespace aoide
