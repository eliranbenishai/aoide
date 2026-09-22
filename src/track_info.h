#pragma once

#include "chrome_layout.h"
#include "session_view.h"

namespace aoide {

/// Shared by the painter, full-value tooltips and Copy details. No disk reads:
/// opening this panel only reads the transport's current snapshot.
struct TrackInfoField {
  QString label;
  QString value;
  QRectF rect;
};

struct TrackInfoLayout {
  QRectF hero;
  QRectF tags;
  QRectF audio;
  QRectF file;
  QRectF copy;

  explicit TrackInfoLayout(QSize logical) {
    const QRectF inner = panelBody(logical).adjusted(22, 14, -22, -14);
    hero = QRectF(inner.left(), inner.top(), inner.width(), 86);
    tags = QRectF(inner.left(), hero.bottom() + 12, inner.width(), 174);
    audio = QRectF(inner.left(), tags.bottom() + 12, inner.width(), 64);
    file = QRectF(inner.left(), audio.bottom() + 12, inner.width(), 64);
    copy = QRectF(inner.right() - 128, inner.bottom() - 30, 128, 30);
  }
};

inline QVector<TrackInfoField> trackInfoFields(const SessionView& view, QSize logical = kTrackInfo) {
  if (!view.currentTrack) return {};
  const Track& t = *view.currentTrack;
  const TrackInfoLayout layout(logical);
  const QRectF tags = layout.tags.adjusted(16, 24, -16, -6);
  const qreal half = (tags.width() - 24) / 2;
  const auto tag = [&](int row, bool right = false) {
    return QRectF(tags.left() + (right ? half + 24 : 0), tags.top() + row * 36,
                  half, 32);
  };
  const QRectF audio = layout.audio.adjusted(16, 23, -16, -7);
  const auto format = [&](int column) {
    return QRectF(audio.left() + column * audio.width() / 4, audio.top(),
                  audio.width() / 4 - 8, audio.height());
  };
  const qint64 duration = view.durationMs > 0 ? view.durationMs : t.durationMs.value_or(0);
  return {
      {QStringLiteral("Title"), t.displayTitle(), layout.hero.adjusted(16, 25, -16, -29)},
      {QStringLiteral("Artist"), t.artist, layout.hero.adjusted(16, 59, -16, -7)},
      {QStringLiteral("Album"), t.album, QRectF(tags.left(), tags.top(), tags.width(), 32)},
      {QStringLiteral("Album artist"), t.albumArtist, tag(1)},
      {QStringLiteral("Composer"), t.composer, tag(1, true)},
      {QStringLiteral("Year"), t.year ? QString::number(*t.year) : QString(), tag(2)},
      {QStringLiteral("Genre"), t.genre, tag(2, true)},
      {QStringLiteral("Track"), t.trackNumber, tag(3)},
      {QStringLiteral("Disc"), t.discNumber, tag(3, true)},
      {QStringLiteral("Duration"), duration > 0 ? formatClock(duration) : QString(), format(0)},
      {QStringLiteral("Bitrate"), view.bitrate, format(1)},
      {QStringLiteral("Sample rate"), view.sampleRate, format(2)},
      {QStringLiteral("Channels"), view.channels, format(3)},
      {QStringLiteral("File"), QFileInfo(t.path).fileName(), layout.file.adjusted(16, 8, -16, -24)},
      {QStringLiteral("Location"), t.path, layout.file.adjusted(16, 40, -16, -6)},
  };
}

inline QString trackInfoText(const SessionView& view) {
  QStringList lines;
  for (const auto& field : trackInfoFields(view)) {
    lines << field.label + QStringLiteral(": ") +
                 (field.value.trimmed().isEmpty() ? QStringLiteral("—") : field.value);
  }
  if (view.currentTrack) lines << QStringLiteral("Format: ") + view.formatChip;
  return lines.join(QLatin1Char('\n'));
}

}  // namespace aoide
