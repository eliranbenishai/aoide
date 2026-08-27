#pragma once

#include "equalizer.h"
#include "look.h"
#include "track.h"
#include "window_spec.h"

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

inline bool operator==(const CollectionRowView& a, const CollectionRowView& b) {
  return a.name == b.name && a.count == b.count && a.selected == b.selected &&
         a.disabled == b.disabled;
}
inline bool operator!=(const CollectionRowView& a, const CollectionRowView& b) {
  return !(a == b);
}

struct TrackRowView {
  QString artist;
  QString title;
  QString time;
  bool selected = false;
  bool playing = false;
  bool disabled = false;
};

inline bool operator==(const TrackRowView& a, const TrackRowView& b) {
  return a.artist == b.artist && a.title == b.title && a.time == b.time &&
         a.selected == b.selected && a.playing == b.playing && a.disabled == b.disabled;
}
inline bool operator!=(const TrackRowView& a, const TrackRowView& b) { return !(a == b); }

struct SessionView {
  /// Marks a paint as the fidelity reference: animation is held still and the
  /// title marquee does not run, so two dumps of the same state match. It says
  /// nothing about the content — that comes from [goldenDemoView].
  bool goldenDemo = false;
  bool eqOn = true;
  bool plOn = true;
  bool skinsOn = false;
  bool trackInfoEnabled = false;
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
  /// Whether each zoom button has a step left to take. A step runs out at the
  /// end of the ladder, and zooming in also runs out when the next step would
  /// make the visible cluster taller or wider than the work area it sits on —
  /// so these are the session's answer, not arithmetic a painter can redo.
  ///
  /// Default enabled: a synthesised view has no cluster and no display to
  /// measure against, and the golden demo is one of those.
  bool zoomInEnabled = true;
  bool zoomOutEnabled = true;
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
  /// The playlist is still taking on track data: the rows are there, but some
  /// of their durations and titles are still being asked for. Lights the
  /// Refresh lamp, and now covers every ingest — open, add, Refresh, Save and
  /// drop — rather than only the one that used to freeze the window.
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
  /// The session spectrogram could not be measured. Read from
  /// `Spectrogram::synthetic`, never from the per-frame levels that go silent
  /// on pause. The 120 s decode timeout is the same mark.
  bool spectrumUnmeasured = false;
  /// The session installed `MissingAudioEngine`. Durable display-well mark;
  /// the panel subtitle still carries the reason when an open is refused.
  bool noAudioEngine = false;
  /// A settings or state-file write has not yet succeeded. Settings-row mark
  /// that stays until that file writes.
  bool persistWriteFailed = false;
  QString audioDeviceLabel = QStringLiteral("Auto");
  bool audioExclusive = false;
};

/// Whether the display well's marquee is moving rather than held at the start.
///
/// The one thing the title's scroll clock decides that outlives a frame: an
/// overflowing line is painted on the live pass once it is moving and on the
/// cached chassis while it is still held, so the cache turns over when the hold
/// ends and on none of the frames either side of it.
bool titleMarqueeRunning(const SessionView& view);

/// Locked empty-state copy (decision 9). Two centred lines in a well that has
/// no rows — never a third line that repeats the playlist footer's drop hint.
struct EmptyWellCopy {
  QString heading;
  QString body;
};

inline EmptyWellCopy playlistEmptyCopy() {
  return {QStringLiteral("THIS LIST IS EMPTY"),
          QStringLiteral("Drop files here, or open one from PLAYLISTS.")};
}

inline EmptyWellCopy collectionEmptyCopy() {
  return {
    QStringLiteral("NO SAVED PLAYLISTS"),
    QStringLiteral("Tramp only saves references to your playlist files. You can create new ones and name them whatever you want, without affecting the files.")
  };
}

inline QString resumePlaybackLabel() { return QStringLiteral("Resume playback"); }

/// What the main display well paints for the title.
///
/// `nowPlayingDisplay` stays about a track: it still answers `No track` when
/// nothing is open. The painter applies this swap from the view it is handed,
/// because the session snapshot is not rewritten for a first-run flag that
/// does not exist. A stopped current track still shows its title; `No track`
/// is only when nothing is loaded.
inline QString mainEmptyTitle(const SessionView& view) {
  if (view.tracks.isEmpty() && view.title == QStringLiteral("No track")) {
    return QStringLiteral("Drop files to play");
  }
  return view.title;
}

/// Whether [a] and [b] would put the same pixels on [id].
///
/// Deliberately not one comparison over the whole snapshot: the point of asking
/// is that scrolling the playlist must not re-rasterise Settings, and a
/// struct-wide equality says every panel changed whenever any field did. Each
/// panel answers for the fields its own painter reads, and for the shell and
/// title bar it shares with the other four.
///
/// Answering "no" when the truth is "yes" costs a redundant rebuild. Answering
/// "yes" when the truth is "no" leaves the listener looking at pixels that have
/// stopped being true, so the groups below are widened, never narrowed, when in
/// doubt.
bool paintsSame(WindowId id, const SessionView& a, const SessionView& b);

/// The demo state every golden dump and paint benchmark is measured in.
///
/// It is data rather than something the painters synthesise, so a caller can
/// photograph a state the demo does not open on — another settings tab, a
/// scrolled list, a disabled row — by saying so on the view. A painter that
/// overrode the view instead would make that state unreachable, and an
/// unreachable state is an unwatched one. This is the only way to get the demo:
/// there was once a second, bare-flag one that painted an empty player.
SessionView goldenDemoView();

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
