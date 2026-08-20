#pragma once

#include "equalizer.h"
#include "look.h"
#include "track.h"

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <array>
#include <optional>

namespace tramp {

struct CollectionRowView {
  QString name;
  int count = 0;
  bool selected = false;
  bool disabled = false;
};

struct TrackRowView {
  QString artist;
  QString title;
  QString time;
  bool selected = false;
  bool playing = false;
  bool disabled = false;
};

struct SessionView {
  bool goldenDemo = false;
  bool eqOn = true;
  bool plOn = true;
  bool showElapsed = true;
  qint64 positionMs = 0;
  qint64 durationMs = 0;
  QString title = QStringLiteral("No track");
  QString subtitle;
  QString bitrate = QStringLiteral("— kbps");
  QString sampleRate = QStringLiteral("— kHz");
  QString channels = QStringLiteral("—");
  QString formatChip = QStringLiteral("—");
  double volume = 1.0;
  bool muted = false;
  bool forceMono = false;
  bool playing = false;
  bool paused = false;
  bool shuffle = false;
  RepeatMode repeat = RepeatMode::off;
  int zoomPercent = 75;
  std::array<qreal, 20> spectrum{};
  std::array<qreal, 20> spectrumPeaks{};
  EqualizerSettings eq;
  QVector<TrackRowView> tracks;
  QSet<int> selectedIndices;
  std::optional<int> playingIndex;
  int trackScroll = 0;
  QVector<CollectionRowView> collection;
  QString collectionSelected;
  qreal collectionWidth = 240;
  bool collectionCollapsed = false;
  QString playlistName;
  bool playlistAltered = false;
  qint64 playlistTotalMs = 0;
  int playlistTrackCount = 0;
  bool playlistRefreshEnabled = false;
  bool playlistRefreshing = false;
  int settingsTab = 0;
  bool resumeLastSession = true;
  bool confirmBeforeQuit = false;
  bool scrollTitle = true;
  qint64 titleScrollMs = 0;
  bool minimizeHidesSecondaries = true;
  int dockSnap = 1;
  int aboutPlaylists = 0;
  int aboutTracks = 0;
  qint64 aboutTimeMs = 0;
  int aboutSpins = 0;
  bool aboutMeasured = false;
  ChromeTokens look = ChromeTokens::builtin();
  QVector<SkinCatalogEntry> skins;
  QString activeSkinId = QStringLiteral("builtin");
  QString skinsError;
  int skinsScroll = 0;

  static SessionView golden();
};

struct NowPlayingDisplay {
  QString title = QStringLiteral("No track");
  QString subtitle;
  QString formatChip = QStringLiteral("—");
};

inline NowPlayingDisplay nowPlayingDisplay(const std::optional<Track>& track,
                                           std::optional<int> playingIndex, int playlistSize) {
  NowPlayingDisplay out;
  if (!track) return out;
  const bool inList = playingIndex && *playingIndex >= 0 && *playingIndex < playlistSize;
  QStringList parts;
  if (inList) parts << QString::number(*playingIndex + 1) + QLatin1Char('.');
  if (!track->artist.trimmed().isEmpty()) {
    parts << track->artist.trimmed() + QStringLiteral(" —");
  }
  parts << track->displayTitle();
  out.title = parts.join(QLatin1Char(' '));
  QStringList sub;
  if (!track->album.trimmed().isEmpty()) sub << track->album.trimmed();
  if (inList) {
    sub << QStringLiteral("track %1 of %2").arg(*playingIndex + 1).arg(playlistSize);
  }
  out.subtitle = sub.join(QStringLiteral(" · ")).toUpper();
  const QString ext = QFileInfo(track->path).suffix().toUpper();
  out.formatChip = ext.isEmpty() ? QStringLiteral("—") : ext;
  return out;
}

struct MainLiveReadouts {
  qint64 positionMs = 0;
  qint64 durationMs = 0;
  bool showElapsed = true;
  qint64 titleScrollMs = 0;
  std::array<qreal, 20> spectrum{};
  std::array<qreal, 20> spectrumPeaks{};
};

}  // namespace tramp
