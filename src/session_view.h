#pragma once

#include "equalizer.h"
#include "look.h"
#include "track.h"

#include <QSet>
#include <QString>
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
  int settingsTab = 0;
  bool resumeLastSession = true;
  bool confirmBeforeQuit = false;
  bool scrollTitle = true;
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

  static SessionView golden();
};

struct MainLiveReadouts {
  qint64 positionMs = 0;
  qint64 durationMs = 0;
  bool showElapsed = true;
  std::array<qreal, 20> spectrum{};
  std::array<qreal, 20> spectrumPeaks{};
};

}  // namespace tramp
