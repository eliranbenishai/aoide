#include "session.h"

#include "chrome_command.h"
#include "chrome_layout.h"
#include "files.h"
#include "host_shell.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "look.h"
#include "m3u.h"
#include "native_file_dialog.h"
#ifdef TRAMP_HAVE_MPV
#include "mpv_engine.h"
#include "pcm_decoder.h"
#endif
#include "duration_probe.h"
#include "player_engine.h"
#include "popup_anchor.h"
#include "support_dir.h"
#include "wait_cursor.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QSet>
#include <QUrl>
#include <QWidget>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <memory>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace tramp {
namespace {

/// Probe answers come back in batches. Small enough that the first rows fill in
/// while the rest of the list is still being asked about, big enough that a
/// thousand-track open is tens of repaints rather than a thousand.
constexpr int kProbeBatchSize = 24;
constexpr int kProbeBatchMs = 120;

}  // namespace

TrampSession::TrampSession(QObject* parent)
    : QObject(parent), store_(trampSupportDirectory()) {
  settings_ = store_.readSettings();
  collection_.load(store_);
#ifdef TRAMP_HAVE_MPV
  auto* mpv = new MpvEngine();
  if (mpv->available()) {
    engine_.reset(mpv);
  } else {
    delete mpv;
    engine_ = std::make_unique<MissingAudioEngine>(
        QStringLiteral("libmpv could not start, so playback is unavailable"));
    noAudioEngine_ = true;
  }
#else
  engine_ = std::make_unique<MissingAudioEngine>(
      QStringLiteral("this build has no audio engine"));
  noAudioEngine_ = true;
#endif
  playback_ = std::make_unique<PlaybackController>(&playlist_, engine_.get());
  playback_->setSpins(store_.readUsage().spins);
#ifdef TRAMP_HAVE_MPV
  analyzer_ = SpectrumAnalyzer(SpectrumAnalyzer::CancellablePcmLoader(
      [](const QString& path, const SpectrumAnalyzer::CancelFn& stillWanted) {
        return MpvPcmDecoder().decode(path, stillWanted);
      }));
#endif
  spectrumTimer_.setInterval(33);
  spectrumTimer_.setTimerType(Qt::CoarseTimer);
  QObject::connect(&spectrumTimer_, &QTimer::timeout, this, [this]() { tickSpectrum(); });
  marqueeTimer_.setInterval(50);
  marqueeTimer_.setTimerType(Qt::CoarseTimer);
  QObject::connect(&marqueeTimer_, &QTimer::timeout, this, [this]() {
    if (!spectrumTimer_.isActive()) emit mainChromeChanged();
  });
  marqueeClock_.start();
  DockLayout layout;
  copyPanelFrames(layout, settings_);
  layout.dockEdges = settings_.dockEdges;
  layout_ = LayoutSync(layout, settings_.zoomPercent);
  layout_.setSurfaces(this);
  layout_.docking().ensureMainVisible();
  layout_.docking().setSnapThreshold(snapPixels(settings_.dockSnapStrength));

  persistTimer_.setSingleShot(true);
  persistTimer_.setInterval(400);
  QObject::connect(&persistTimer_, &QTimer::timeout, this, [this]() { persistNow(); });
  alteredTimer_.setSingleShot(true);
  alteredTimer_.setInterval(2000);
  QObject::connect(&alteredTimer_, &QTimer::timeout, this, [this]() {
    if (playlist_.altered()) {
      store_.writeAltered({playlist_.tracks(), playlist_.sourcePath()});
    } else {
      store_.clearAltered();
    }
  });
  usageTimer_.setSingleShot(true);
  usageTimer_.setInterval(2000);
  QObject::connect(&usageTimer_, &QTimer::timeout, this, [this]() {
    store_.writeUsage({playback_->spins()});
    HostWindow* about = windowFor(WindowId::about);
    if (about && about->isVisible()) refreshAboutFigures();
  });
  aboutTimer_.setSingleShot(true);
  aboutTimer_.setInterval(50);
  QObject::connect(&aboutTimer_, &QTimer::timeout, this, [this]() {
    figures_ = collection_.readFigures();
    figuresLoaded_ = true;
    refreshChrome();
  });
  eqApplyTimer_.setSingleShot(true);
  eqApplyTimer_.setInterval(50);
  QObject::connect(&eqApplyTimer_, &QTimer::timeout, this, [this]() { applyEq(); });
  collectionPersistTimer_.setSingleShot(true);
  collectionPersistTimer_.setInterval(300);
  QObject::connect(&collectionPersistTimer_, &QTimer::timeout, this, [this]() {
    persistCollectionCache();
  });

  playlist_.setOnChanged([this]() {
    if (playback_) playback_->onPlaylistChanged();
    scheduleAltered();
    refreshChrome();
  });
  bindPlayback();
  engine_->setForceMono(settings_.forceMono);
  applyEq();
  {
    WaitCursorScope wait;
    skins_.bootstrap(trampSupportDirectory(), bundledSkinsDir(), settings_);
  }
  syncTitleMarquee();
}

TrampSession::~TrampSession() {
  // Cancel, then wait. Bumping the generations first means a worker already past
  // its alive check still bails at its next iteration; the join is what makes the
  // raw `this` in a worker body safe, because the destructor cannot get to the
  // members until every worker has returned. Nothing here can deadlock: workers
  // only ever post queued calls, they never block on the GUI thread.
  ++spectrumGen_;
  ++durationGen_;
  ++verifyGen_;
  workers_.stopAndJoin();
  spectrumTimer_.stop();
  marqueeTimer_.stop();
  persistNow();
}

void TrampSession::detachWindows() {
  persistNow();
  windows_.clear();
  shell_ = nullptr;
}

void TrampSession::bindPlayback() {
  playback_->setOnChanged([this]() {
    syncSpectrum();
    syncTitleMarquee();
    refreshChrome();
  });
  playback_->setOnPosition([this]() {
    if (!spectrumTimer_.isActive()) emit mainChromeChanged();
  });
  playback_->setOnSpin([this](int) { scheduleUsage(); refreshChrome(); });
  playback_->setOnTrackDuration([this](const QString& path, qint64 ms) {
    collection_.mergeTrackDuration(path, ms);
    if (!collectionPersistTimer_.isActive()) collectionPersistTimer_.start();
    figures_ = collection_.readFigures();
    figuresLoaded_ = true;
  });
}

void TrampSession::syncSpectrum() {
  QString path;
  if (const auto track = playback_->currentTrack()) {
    path = track->path;
  }
  if (path != spectrumPath_) {
    ++spectrumGen_;
    spectrumPath_ = path;
    spectrumReady_ = false;
    spectrogram_ = {};
    spectrumHold_.reset();
    if (!path.isEmpty()) startSpectrumDecode(path, spectrumGen_);
  }
  if (playback_->playing()) {
    if (!spectrumTimer_.isActive()) {
      tickSpectrum();
      spectrumTimer_.start();
    }
    return;
  }
  if (playback_->paused()) {
    // Pause holds the last frame: the track is still cued up under the needle.
    spectrumTimer_.stop();
    return;
  }
  // Stopped, or the track ran out. Keep ticking so the bars fall to rest instead
  // of freezing mid-song, which reads as if something were still sounding.
  if (!spectrumHold_.atRest() && !spectrumTimer_.isActive()) spectrumTimer_.start();
}

void TrampSession::tickSpectrum() {
  playback_->pollClock();
  if (playback_->playing() || playback_->paused()) {
    const AudioLevels frame =
        spectrumFrame(spectrogram_, playback_->playing() && spectrumReady_, playback_->positionMs());
    spectrumHold_.apply(frame);
  } else {
    spectrumHold_.release();
    if (spectrumHold_.atRest()) {
      spectrumHold_.reset();
      spectrumTimer_.stop();
    }
  }
  emit mainChromeChanged();
}

void TrampSession::syncTitleMarquee() {
  QString id;
  if (const auto track = playback_->currentTrack()) {
    id = track->path + QLatin1Char('\n') + track->displayTitle() + QLatin1Char('\n') +
         track->album;
  }
  if (id != marqueeIdentity_) {
    marqueeIdentity_ = id;
    marqueeClock_.restart();
  }
  if (settings_.scrollTitle && !id.isEmpty()) {
    if (!marqueeTimer_.isActive()) marqueeTimer_.start();
  } else if (marqueeTimer_.isActive()) {
    marqueeTimer_.stop();
  }
}

qint64 TrampSession::titleScrollMs() const {
  if (!settings_.scrollTitle || !marqueeClock_.isValid()) return 0;
  return marqueeClock_.elapsed();
}

void TrampSession::startSpectrumDecode(const QString& path, int gen) {
  const SpectrumAnalyzer analyzer = analyzer_;
  const auto alive = workers_.alive();
  workers_.start([this, path, gen, analyzer, alive]() {
#ifdef Q_OS_UNIX
    nice(19);
#endif
    // Both reads are atomic, which is what makes them legal from here — a
    // QPointer is not, and `this` outliving the read is the crew's job.
    const auto stillWanted = [this, gen, alive]() {
      return alive->load() && gen == spectrumGen_.load();
    };
    const Spectrogram spec = analyzer.load(path, stillWanted);
    if (!stillWanted()) return;
    QMetaObject::invokeMethod(
        this,
        [this, spec, gen]() {
          if (gen != spectrumGen_.load()) return;
          const bool wasSynthetic = spectrogram_.synthetic;
          spectrogram_ = spec;
          spectrumReady_ = true;
          // The well mark is chassis, so a spectrogram that arrives unmeasured
          // has to rebuild it. Live readouts only carry the bars.
          if (spec.synthetic != wasSynthetic) refreshChrome();
          tickSpectrum();
        },
        Qt::QueuedConnection);
  });
}

void TrampSession::setWindows(const PanelWindows& windows) { windows_ = windows; }

void TrampSession::setShell(HostShell* shell) {
  if (shell_ == shell) return;
  if (shell_) disconnect(shell_, nullptr, this, nullptr);
  shell_ = shell;
  if (shell_) {
    connect(shell_, &HostShell::desktopGeometryChanged, this, [this]() {
      layout_.fitClusterToHost();
      layout_.place();
    });
  }
}

void TrampSession::bootstrap(const QStringList& argvFiles) {
  {
    WaitCursorScope wait;
    collection_.validateReferences();
    const auto kept = store_.readAltered();
    if (!kept.isEmpty()) {
      playlist_.restoreAlteredTracks(kept.tracks, kept.sourcePath);
    } else {
      const QString last = store_.readLastPlaylistPath();
      if (!last.isEmpty() && collection_.contains(last)) {
        playlist_.setTracks(collection_.tracksFor(last), last);
      }
    }
    if (!playlist_.sourcePath().isEmpty()) {
      collection_.select(playlist_.sourcePath());
    }
  }
  if (!argvFiles.isEmpty()) {
    openPaths(argvFiles, !playlist_.tracks().isEmpty());
  }
  if (settings_.resumeLastSession) {
    const auto resume = store_.readResume();
    if (resume.playingIndex && *resume.playingIndex >= 0 &&
        *resume.playingIndex < playlist_.tracks().size()) {
      playback_->playFrom(*resume.playingIndex);
      if (resume.positionMs > 0) playback_->seekMs(resume.positionMs);
      if (!resume.wasPlaying) playback_->playPause();
    }
  }
  schedulePathVerify();
  layout_.docking().nudgeOffMainIfStacked(WindowId::equalizer);
  layout_.docking().nudgeOffMainIfStacked(WindowId::playlist);
  layout_.fitClusterToHost();
  layout_.place();
  if (settings_.about.visible) refreshAboutFigures();
  applyAlwaysOnTop();
  refreshChrome();
}

void TrampSession::applyEq() { engine_->setEqualizerAf(buildEqualizerAf(settings_.equalizerCurve)); }

void TrampSession::scheduleApplyEq() {
  if (!eqApplyTimer_.isActive()) eqApplyTimer_.start();
}

void TrampSession::refreshEqChrome() {
  if (HostWindow* eq = windowFor(WindowId::equalizer)) eq->applyEqualizer(settings_.equalizerCurve);
}

bool TrampSession::confirmQuit() const { return settings_.confirmBeforeQuit; }

bool TrampSession::windowShouldShow(WindowId id) const {
  return layout_.layout().frameOf(id).visible;
}

void TrampSession::applyAlwaysOnTop() {
  if (shell_) shell_->setAlwaysOnTop(settings_.alwaysOnTop);
}

void TrampSession::refreshAboutFigures() {
  figures_ = collection_.readFigures();
  figuresLoaded_ = true;
  refreshChrome();
}

void TrampSession::persistCollectionCache() {
  collection_.saveIndex(store_);
  collection_.saveTrackSets(store_);
  figures_ = collection_.readFigures();
  figuresLoaded_ = true;
  HostWindow* about = windowFor(WindowId::about);
  if (about && about->isVisible()) refreshChrome();
}

/// Rows first, durations later. `collection_.add` reads the file and fills in
/// whatever the cache already knows; the rest is a question for a worker, and
/// the caller asks it once it has decided what the list is.
QVector<Track> TrampSession::ingestPlaylistFile(const QString& path) {
  const QVector<Track> tracks = collection_.add(path);
  collection_.addWritten(path, tracks);
  persistCollectionCache();
  return tracks;
}

void TrampSession::schedulePathVerify() {
  const QVector<Track> tracks = playlist_.tracks();
  ++verifyGen_;
  if (tracks.isEmpty()) return;
  const int gen = verifyGen_.load();
  const auto alive = workers_.alive();
  workers_.start([this, tracks, gen, alive]() {
    QSet<QString> missing;
    for (const Track& t : tracks) {
      // Per track, not per playlist: a long list is most of what teardown would
      // otherwise have to sit through.
      if (!alive->load() || gen != verifyGen_.load()) return;
      if (!QFileInfo::exists(t.path)) missing.insert(normalizePlaylistPath(t.path));
    }
    if (!alive->load() || gen != verifyGen_.load()) return;
    QMetaObject::invokeMethod(
        this,
        [this, missing, gen]() {
          if (gen != verifyGen_.load()) return;
          playlist_.markMissingPaths(missing);
          refreshChrome();
        },
        Qt::QueuedConnection);
  });
}

void TrampSession::refreshCurrentPlaylist() {
  const QString path = playlist_.sourcePath();
  if (path.isEmpty() || !QFileInfo::exists(path)) return;
  if (!confirmReplaceAltered(QStringLiteral(
          "Refreshing this playlist replaces it with the file on disk. "
          "Missing tracks are removed."))) {
    return;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
  const QVector<Track> parsed = M3uCodec().parse(decodeM3uBytes(file.readAll()), path);
  QVector<Track> tracks = dropMissingTrackFiles(parsed);
  const bool dropped = tracks.size() != parsed.size();
  collection_.hydrateDurations(tracks);
  if (dropped) playlist_.restoreAlteredTracks(tracks, path);
  else playlist_.setTracks(tracks, path);
  if (collection_.contains(path)) {
    collection_.addWritten(path, tracks);
    persistCollectionCache();
  }
  collection_.select(path);
  // The rows are already there, reading whatever the cache knew. Refresh means
  // believe the files rather than the file that listed them, so the probe that
  // corrects them overwrites.
  startDurationProbe(tracks, true);
  refreshChrome();
}

void TrampSession::startDurationProbe(const QVector<Track>& tracks, bool overwrite) {
  QStringList paths;
  QSet<QString> seen;
  auto ask = [&](const QString& path) {
    if (path.isEmpty() || seen.contains(path)) return;
    seen.insert(path);
    paths.push_back(path);
  };
  if (overwrite) {
    for (const Track& t : tracks) ask(t.path);
  } else {
    for (const QString& path : pathsNeedingAudioProbe(tracks)) ask(path);
  }
  // Starting here supersedes whatever worker was running, so anything that run
  // still owed an answer for rides along. Dropping it would leave those rows
  // reading --:-- until the listener opened the list again.
  for (const QString& path : probeOutstanding_) ask(path);

  ++durationGen_;
  probeOutstanding_.clear();
  if (paths.isEmpty()) {
    setIngesting(false);
    return;
  }
  for (const QString& path : paths) probeOutstanding_.insert(path);
  setIngesting(true);

  const int gen = durationGen_.load();
  const auto alive = workers_.alive();
  workers_.start([this, paths, gen, alive, overwrite]() {
    // Both reads are atomic, which is what makes them legal from here; `this`
    // is safe because the crew joins this thread before the session's members
    // go. Nothing below waits on the GUI thread — every call out is queued.
    const auto stillWanted = [this, gen, alive]() {
      return alive->load() && gen == durationGen_.load();
    };
    QVector<ProbedTrack> batch;
    QElapsedTimer sinceFlush;
    sinceFlush.start();
    auto flush = [&]() {
      if (batch.isEmpty()) return;
      QMetaObject::invokeMethod(
          this, [this, batch, gen, overwrite]() { applyProbedBatch(batch, gen, overwrite); },
          Qt::QueuedConnection);
      batch.clear();
      sinceFlush.restart();
    };
    probeAudioDurations(paths, stillWanted,
                        [&](const QString& path, const ProbedAudio& probed) {
                          if (!stillWanted()) return;
                          batch.push_back({path, probed.title, probed.artist, probed.album,
                                           probed.durationMs.value_or(0)});
                          if (batch.size() >= kProbeBatchSize ||
                              sinceFlush.elapsed() >= kProbeBatchMs) {
                            flush();
                          }
                        });
    flush();
    // Report the ending either way. The generation check on the GUI thread is
    // what decides whether this run still owns the lamp.
    QMetaObject::invokeMethod(
        this, [this, gen]() { probeFinished(gen); }, Qt::QueuedConnection);
  });
}

void TrampSession::applyProbedBatch(const QVector<ProbedTrack>& batch, int gen, bool overwrite) {
  // A batch from a superseded run is not wrong; it is about a list nobody is
  // looking at any more.
  if (gen != durationGen_.load()) return;
  holdChrome_ = true;
  bool touchedCache = false;
  bool gotDuration = false;
  // Read the list once for the whole batch. No answer in a batch names the same
  // row twice, and nothing turns the event loop in here, so a row taken now is
  // still the row when its answer comes up.
  QMap<QString, Track> rows;
  if (overwrite) {
    for (const Track& t : playlist_.tracks()) rows.insert(normalizePlaylistPath(t.path), t);
  }
  for (const ProbedTrack& answer : batch) {
    probeOutstanding_.remove(answer.path);
    if (overwrite) {
      // `applyMetadata` deliberately never replaces a title that is already
      // there, which is the wrong answer for Refresh: the point of Refresh is
      // that the file wins.
      const auto row = rows.constFind(normalizePlaylistPath(answer.path));
      if (row != rows.constEnd()) {
        ProbedAudio probed;
        probed.title = answer.title;
        probed.artist = answer.artist;
        probed.album = answer.album;
        if (answer.durationMs > 0) probed.durationMs = answer.durationMs;
        Track next = *row;
        applyProbedAudio(next, probed, true);
        // Keeps whatever the path verify has said about the row since.
        playlist_.updateTrackByPath(next.path, next);
      }
    } else {
      playlist_.applyMetadata(answer.path, answer.title, answer.artist, answer.album,
                              answer.durationMs);
    }
    collection_.mergeTrackTags(answer.path, answer.title, answer.artist, answer.album);
    if (answer.durationMs > 0) {
      collection_.mergeTrackDuration(answer.path, answer.durationMs);
      gotDuration = true;
      touchedCache = true;
    } else if (!answer.title.trimmed().isEmpty() || !answer.artist.trimmed().isEmpty() ||
               !answer.album.trimmed().isEmpty()) {
      touchedCache = true;
    }
  }
  if (touchedCache && !collectionPersistTimer_.isActive()) collectionPersistTimer_.start();
  if (gotDuration) {
    // A pure read of what the last track pass found. No batch of probe answers
    // may set a filesystem sweep going.
    figures_ = collection_.readFigures();
    figuresLoaded_ = true;
  }
  holdChrome_ = false;
  if (chromeHeld_) {
    chromeHeld_ = false;
    refreshChrome();
  }
}

void TrampSession::probeFinished(int gen) {
  if (gen != durationGen_.load()) return;
  probeOutstanding_.clear();
  setIngesting(false);
}

void TrampSession::setIngesting(bool ingesting) {
  if (ingesting_ == ingesting) return;
  ingesting_ = ingesting;
  refreshChrome();
}

void TrampSession::persistNow() {
  if (qEnvironmentVariable("TRAMP_AUTO_QUIT") == QLatin1String("1")) return;
  if (!windowFor(WindowId::main)) return;
  // The settings are a view of the layout taken at the moment of writing, not a
  // second copy kept level with it. Nothing reads a frame back out of a widget:
  // where a panel is, how big it is and whether it is shaded are all decided in
  // the layout and pushed from there.
  const DockLayout& live = layout_.layout();
  copyPanelFrames(settings_, live);
  settings_.dockEdges = live.dockEdges;
  settings_.zoomPercent = layout_.zoomPercent();
  // Main cannot be hidden, and a file saying otherwise launches with no player.
  settings_.main.visible = true;
  store_.writeSettings(settings_);
  if (!playlist_.sourcePath().isEmpty()) {
    store_.writeLastPlaylistPath(playlist_.sourcePath());
  }
  SessionResume resume;
  resume.playingIndex = playback_->playingIndex();
  resume.positionMs = playback_->positionMs();
  resume.wasPlaying = playback_->playing();
  store_.writeResume(resume);
  store_.writeUsage({playback_->spins()});
  if (collectionPersistTimer_.isActive()) {
    collectionPersistTimer_.stop();
    persistCollectionCache();
  }
  if (playlist_.altered()) {
    store_.writeAltered({playlist_.tracks(), playlist_.sourcePath()});
  }
}

void TrampSession::schedulePersist() { persistTimer_.start(); }
void TrampSession::scheduleAltered() { alteredTimer_.start(); }
void TrampSession::scheduleUsage() { usageTimer_.start(); }

HostWindow* TrampSession::windowFor(WindowId id) const { return windows_[id]; }

/// A dialog belongs to the panel it was raised from, so it lands over that
/// panel rather than wherever the window manager felt like. Main stands in when
/// the panel has no window — the dump and bench paths run with fewer than five.
QWidget* TrampSession::dialogParent(WindowId id) const {
  if (HostWindow* window = windowFor(id)) return window;
  return windowFor(WindowId::main);
}

QRect TrampSession::hostRect() const { return shell_ ? shell_->virtualDesktop() : QRect(); }

QRect TrampSession::workAreaFor(QRect clusterNative) const {
  // An L-shaped monitor arrangement leaves dead zones inside the virtual
  // desktop that belong to no screen, so a cluster's centre can land on
  // nothing. The primary screen is the honest answer then: it is the display a
  // listener who has mislaid their layout will look at.
  const QScreen* screen =
      clusterNative.isNull() ? nullptr : QGuiApplication::screenAt(clusterNative.center());
  if (!screen) screen = QGuiApplication::primaryScreen();
  return screen ? screen->availableGeometry() : QRect();
}

SessionView TrampSession::view() const {
  SessionView v;
  v.eqOn = layout_.layout().equalizer.visible;
  v.plOn = layout_.layout().playlist.visible;
  v.showElapsed = settings_.showElapsed;
  v.titleScrollMs = titleScrollMs();
  v.positionMs = playback_->positionMs();
  v.durationMs = playback_->durationMs();
  v.volume = playback_->volume();
  v.muted = playback_->muted();
  v.forceMono = settings_.forceMono;
  v.playing = playback_->playing();
  v.paused = playback_->paused();
  v.shuffle = playback_->shuffle();
  v.repeat = playback_->repeatMode();
  v.zoomPercent = layout_.zoomPercent();
  v.zoomInEnabled = layout_.zoomStepUp().has_value();
  v.zoomOutEnabled = layout_.zoomStepDown().has_value();
  v.spectrum = spectrumHold_.bars;
  v.spectrumPeaks = spectrumHold_.peaks;
  v.spectrumUnmeasured = spectrogram_.synthetic;
  v.noAudioEngine = noAudioEngine_;
  v.eq = settings_.equalizerCurve;
  v.playingIndex = playback_->playingIndex();
  v.selectedIndices = playlist_.selectedIndices();
  const qreal plH = layout_.layout().playlist.height.value_or(kPlaylistDefault.height());
  v.trackScroll = std::max(
      0, std::min(trackScroll_, playlistListMaxScroll(int(playlist_.tracks().size()),
                                                     playlistListWellHeight(plH))));
  v.collectionWidth = settings_.playlistCollectionWidth;
  v.collectionCollapsed = settings_.playlistCollectionCollapsed;
  v.playlistAltered = playlist_.altered();
  v.settingsTab = settingsTab_;
  v.resumeLastSession = settings_.resumeLastSession;
  v.confirmBeforeQuit = settings_.confirmBeforeQuit;
  v.scrollTitle = settings_.scrollTitle;
  v.minimizeHidesSecondaries = settings_.minimizeHidesSecondaries;
  v.dockSnap = int(settings_.dockSnapStrength);
  v.aboutPlaylists = figures_.playlists;
  v.aboutTracks = figures_.tracks;
  v.aboutTimeMs = figures_.totalDurationMs;
  v.aboutSpins = playback_->spins();
  v.aboutMeasured = figuresLoaded_;
  v.look = skins_.tokens();
  v.skins = skins_.catalog();
  v.activeSkinId = settings_.activeSkinId;
  v.skinsError = skins_.lastError();
  const QRectF skinsViewport = skinsListViewport(settingsPane(kSettings));
  v.skinsScroll = std::max(0, std::min(skinsScroll_, skinsListMaxScroll(skins_.catalog().size(),
                                                                        skinsViewport.height())));

  const auto tracks = playlist_.tracks();
  qint64 total = 0;
  int listed = 0;
  for (int i = 0; i < tracks.size(); ++i) {
    TrackRowView row;
    row.artist = tracks[i].artist;
    row.title = tracks[i].displayTitle();
    row.time = tracks[i].durationMs ? formatClock(*tracks[i].durationMs) : QStringLiteral("--:--");
    row.selected = playlist_.selectedIndices().contains(i);
    row.playing = playback_->playingIndex() == i;
    row.disabled = tracks[i].disabled;
    v.tracks.push_back(row);
    if (tracks[i].disabled) continue;
    ++listed;
    if (tracks[i].durationMs) total += *tracks[i].durationMs;
  }
  v.playlistTotalMs = total;
  v.playlistTrackCount = listed;
  v.playlistRefreshEnabled =
      !playlist_.sourcePath().isEmpty() && QFileInfo::exists(playlist_.sourcePath());
  v.playlistRefreshing = ingesting_;
  if (!playlist_.sourcePath().isEmpty()) {
    v.playlistName = QFileInfo(playlist_.sourcePath()).fileName();
  }

  const auto nowPlaying =
      nowPlayingDisplay(playback_->currentTrack(), playback_->playingIndex(), tracks.size());
  v.title = nowPlaying.title;
  v.subtitle = nowPlaying.subtitle;
  v.formatChip = nowPlaying.formatChip;

  const AudioFormatInfo fmt = playback_->format();
  v.bitrate = fmt.bitrateKbps ? QStringLiteral("%1 kbps").arg(*fmt.bitrateKbps)
                              : QStringLiteral("— kbps");
  if (fmt.sampleRateHz) {
    const int hz = *fmt.sampleRateHz;
    v.sampleRate = (hz % 1000 == 0) ? QStringLiteral("%1 kHz").arg(hz / 1000)
                                    : QStringLiteral("%1 kHz").arg(hz / 1000.0, 0, 'f', 1);
  }
  if (fmt.channels) {
    if (*fmt.channels == 1) v.channels = QStringLiteral("MONO");
    else if (*fmt.channels == 2) v.channels = QStringLiteral("STEREO");
    else v.channels = QStringLiteral("%1 CH").arg(*fmt.channels);
  }
  if (!playback_->failureMessage().isEmpty()) {
    v.subtitle = playback_->failureMessage().toUpper();
  }

  for (const SavedPlaylist& e : collection_.entries()) {
    CollectionRowView row;
    row.name = e.displayName();
    row.count = e.trackCount;
    const QString marked =
        collectionHighlightPath(playlist_.sourcePath(), collection_.selectedPath());
    row.selected = e.path == marked;
    row.disabled = collection_.disabledPaths().contains(e.path);
    v.collection.push_back(row);
  }
  return v;
}

MainLiveReadouts TrampSession::mainLive() const {
  MainLiveReadouts live;
  live.positionMs = playback_->positionMs();
  live.durationMs = playback_->durationMs();
  live.showElapsed = settings_.showElapsed;
  live.titleScrollMs = titleScrollMs();
  live.spectrum = spectrumHold_.bars;
  live.spectrumPeaks = spectrumHold_.peaks;
  return live;
}

void TrampSession::refreshChrome() {
  if (holdChrome_) {
    chromeHeld_ = true;
    return;
  }
  emit chromeChanged();
}

void TrampSession::setZoomPercent(int percent) {
  // The zoom buttons walk the ladder, so an off-ladder value, or a step the
  // display cannot hold, only arrives from a caller that did not ask first.
  // Refusing it here is what makes a disabled step disabled, rather than
  // applied and then clamped into a stack of overlapping panels.
  if (!layout_.setZoomPercent(percent)) return;
  schedulePersist();
  emit zoomChanged(percent);
  layout_.fitClusterToHost();
  layout_.place();
}

void TrampSession::setWindowVisible(WindowId id, bool visible) {
  layout_.docking().setVisible(id, visible);
  if (visible) {
    layout_.docking().nudgeOffMainIfStacked(id);
    emit requestShow(id);
    layout_.clampToHost(id);
    layout_.place();
    if (id == WindowId::settings) emit requestRaise(WindowId::settings);
    if (id == WindowId::about) refreshAboutFigures();
  } else {
    emit requestHide(id);
    layout_.place();
  }
  applyAlwaysOnTop();
  schedulePersist();
  refreshChrome();
}

void TrampSession::setShaded(WindowId id, bool shaded) {
  layout_.docking().setShaded(id, shaded);
  schedulePersist();
  layout_.place();
}

void TrampSession::extraClosed(WindowId id) {
  layout_.docking().setVisible(id, false);
  applyAlwaysOnTop();
  layout_.place();
  schedulePersist();
  refreshChrome();
}

void TrampSession::mainMinimized(bool minimized) {
  if (!settings_.minimizeHidesSecondaries) return;
  layout_.setMainMinimized(minimized);
  if (!minimized) applyAlwaysOnTop();
  layout_.place();
  if (!minimized) raiseSettingsIfShowing();
}

void TrampSession::mainActivated() { raiseSettingsIfShowing(); }

/// Settings sits above the cluster: anything that brings main forward has to
/// bring it along, or it disappears behind the player it configures.
void TrampSession::raiseSettingsIfShowing() {
  HostWindow* settings = windowFor(WindowId::settings);
  if (settings && settings->isVisible()) settings->raise();
}

void TrampSession::playTrackAt(int index) {
  const auto tracks = playlist_.tracks();
  if (index < 0 || index >= tracks.size() || tracks[index].disabled) return;
  playback_->playIndex(index);
}
/// Space and the play media key are a single toggle, unlike the separate Play and
/// Pause faces on the chrome. Routing them at K::play only ever started playback.
void TrampSession::togglePlayPause() { playback_->playPause(); }

void TrampSession::selectAllTracks() { playlist_.selectAll(); }
void TrampSession::removeSelectedTracks() { playlist_.removeSelected(); }

void TrampSession::windowMoved(WindowId id, QPoint nativeTopLeft, bool finalize) {
  if (layout_.placing()) return;
  // Shift undocks. The modifier is read here rather than carried on the move
  // signal because the drag is app-owned: what matters is whether Shift is down
  // at the moment the panel moves, and again when it is dropped, since a
  // Shift-drop must not snap back onto the edge it was pulled off.
  const bool shiftUndock = QGuiApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
  layout_.docking().move(id, layout_.nativeToLogical(nativeTopLeft), shiftUndock,
                         finalize && id != WindowId::main);
  if (id == WindowId::main) layout_.fitClusterToHost();
  else layout_.clampToHost(id);
  layout_.place();
  schedulePersist();
}

void TrampSession::titleDragBegan(WindowId id) {
  Q_UNUSED(id);
  titleDragging_ = true;
}

void TrampSession::titleDragEnded(WindowId id) {
  if (!titleDragging_) return;
  titleDragging_ = false;
  HostWindow* w = windowFor(id);
  if (w) windowMoved(id, w->nativeTopLeft(), id != WindowId::main);
  else schedulePersist();
}

void TrampSession::extraWasMapped(WindowId id) {
  if (id == WindowId::main) return;
  layout_.place();
}

void TrampSession::reapplyWindowFrames() {
  layout_.place();
}

void TrampSession::placePanels(const QVector<PanelPlacement>& panels) {
  QVector<HostPanelPlacement> shown;
  shown.reserve(panels.size());
  for (const PanelPlacement& panel : panels) {
    HostWindow* w = windowFor(panel.id);
    if (!w) continue;
    if (panel.id != WindowId::main) w->setShaded(panel.shaded);
    if (panel.id == WindowId::playlist) w->setPlaylistLogicalSize(panel.logicalSize);
    if (!panel.visible) {
      w->hide();
      continue;
    }
    shown.push_back({w, panel.screen});
  }

  if (shell_) {
    shell_->placePanels(shown);
    return;
  }
  // No shell means the panels are toplevels of their own — the dump-chrome and
  // test paths. There is nothing to punch, so a bare geometry push will do.
  for (const HostPanelPlacement& place : shown) {
    place.widget->setGeometry(place.screen);
    place.widget->show();
  }
}

void TrampSession::playlistResized(QSize native) {
  if (layout_.placing()) return;
  const qreal z = layout_.zoomPercent() / 100.0;
  layout_.docking().resizePlaylist(QSizeF(native.width() / z, native.height() / z));
  layout_.clampToHost(WindowId::playlist);
  layout_.place();
  schedulePersist();
}

void TrampSession::applyDroppedPaths(const QStringList& paths, bool replace) {
  openPaths(paths, !replace && !playlist_.tracks().isEmpty());
}

void TrampSession::openPaths(const QStringList& paths, bool enqueue) {
  QStringList playlists;
  QStringList others;
  for (const QString& p : paths) {
    if (isPlaylistPath(p)) playlists.push_back(p);
    else others.push_back(p);
  }
  // Opening a playlist, or opening audio without enqueueing, replaces the
  // current list and therefore discards unsaved edits. Ask first — and ask
  // before the wait cursor and the duration probe, not after paying for them.
  const bool replaces = !playlists.isEmpty() || (!enqueue && !others.isEmpty());
  if (replaces &&
      !confirmReplaceAltered(QStringLiteral("Opening these files replaces it."))) {
    return;
  }

  bool playFirst = false;
  if (!playlists.isEmpty()) {
    const QVector<Track> tracks = ingestPlaylistFile(playlists.first());
    playlist_.setTracks(tracks, playlists.first());
    playFirst = !playlist_.tracks().isEmpty();
    schedulePathVerify();
  }
  const auto audio = tracksFromPaths(others);
  if (!audio.isEmpty()) {
    if (!enqueue && playlists.isEmpty()) playlist_.setTracks(audio);
    else playlist_.addTracks(audio);
    if (!playFirst && !playback_->playingIndex()) playFirst = true;
  }
  // One route for a dropped file and an opened playlist alike: the rows are
  // already showing, and the durations arrive behind them.
  if (!playlists.isEmpty() || !audio.isEmpty()) startDurationProbe(playlist_.tracks());
  refreshChrome();
  if (playFirst) playback_->playFrom(0);
}

QString TrampSession::pickAudio(bool multiple) {
  FilePick pick;
  pick.parent = windowFor(WindowId::main);
  pick.title = multiple ? QStringLiteral("Add audio files") : QStringLiteral("Open audio");
  pick.filter = QStringLiteral("Audio (*.mp3 *.m4a *.aac *.flac *.wav *.ogg *.opus)");
  pick.kind = multiple ? FilePickKind::openFiles : FilePickKind::openFile;
  if (multiple) return pickFiles(pick).join(QLatin1Char('\n'));
  return pickFile(pick);
}

QString TrampSession::pickPlaylist(bool save) {
  FilePick pick;
  pick.parent = windowFor(WindowId::main);
  pick.filter = QStringLiteral("Playlists (*.m3u *.m3u8)");
  if (save) {
    pick.title = QStringLiteral("Save playlist");
    pick.suggestedName = QStringLiteral("playlist.m3u");
    pick.kind = FilePickKind::saveFile;
    return pickFile(pick);
  }
  pick.title = QStringLiteral("Open playlist");
  pick.kind = FilePickKind::openFile;
  return pickFile(pick);
}

void TrampSession::loadCollectionRow(int index) {
  const auto entries = collection_.entries();
  if (index < 0 || index >= entries.size()) return;
  SavedPlaylist e;
  if (!collection_.resolveForLoad(entries[index].path, &e)) {
    collection_.select(entries[index].path);
    refreshChrome();
    return;
  }
  if (samePlaylistFile(playlist_.sourcePath(), e.path)) {
    collection_.select(e.path);
    schedulePathVerify();
    refreshChrome();
    return;
  }
  if (!confirmReplaceAltered()) return;
  playlist_.setTracks(collection_.tracksFor(e.path), e.path);
  collection_.select(e.path);
  schedulePathVerify();
  refreshChrome();
}

void TrampSession::handleRelease(WindowId id) {
  sliderKind_ = ChromeHit::Kind::none;
  if (eqApplyTimer_.isActive()) {
    eqApplyTimer_.stop();
    applyEq();
  }
  if (id == WindowId::equalizer || id == WindowId::playlist) {
    if (HostWindow* w = windowFor(id)) windowMoved(id, w->nativeTopLeft(), true);
  }
}

void TrampSession::handleWheel(WindowId id, int delta) {
  if (id == WindowId::settings && settingsTab_ == 1) {
    const QRectF viewport = skinsListViewport(settingsPane(kSettings));
    const int maxScroll = skinsListMaxScroll(skins_.catalog().size(), viewport.height());
    const int step = delta > 0 ? -kSkinRowStride : kSkinRowStride;
    skinsScroll_ = std::max(0, std::min(skinsScroll_ + step, maxScroll));
    refreshChrome();
    return;
  }
  if (id != WindowId::playlist) return;
  const int step = delta > 0 ? -1 : 1;
  const qreal plH = layout_.layout().playlist.height.value_or(kPlaylistDefault.height());
  const int maxScroll =
      playlistListMaxScroll(int(playlist_.tracks().size()), playlistListWellHeight(plH));
  trackScroll_ = std::max(0, std::min(trackScroll_ + step, maxScroll));
  refreshChrome();
}

void TrampSession::handleDrag(WindowId id, ChromeHit hit, QPoint logical) {
  auto band = [](const QRect& r, int y) {
    const double t = sliderFractionY(r, y);
    return EqualizerSettings::kGainLimit - t * (EqualizerSettings::kGainLimit * 2);
  };
  if (sliderKind_ == ChromeHit::Kind::volume || hit.kind == ChromeHit::Kind::volume) {
    playback_->setVolume(sliderFractionX(hit.rect, logical.x()));
    sliderKind_ = ChromeHit::Kind::volume;
  } else if (sliderKind_ == ChromeHit::Kind::seek || hit.kind == ChromeHit::Kind::seek) {
    if (playback_->durationMs() > 0) {
      playback_->seekMs(qint64(sliderFractionX(hit.rect, logical.x()) * playback_->durationMs()));
    }
    sliderKind_ = ChromeHit::Kind::seek;
  } else if (sliderKind_ == ChromeHit::Kind::eqPreamp || hit.kind == ChromeHit::Kind::eqPreamp) {
    settings_.equalizerCurve = settings_.equalizerCurve.withPreamp(band(hit.rect, logical.y()));
    scheduleApplyEq();
    sliderKind_ = ChromeHit::Kind::eqPreamp;
    schedulePersist();
    refreshEqChrome();
  } else if (sliderKind_ == ChromeHit::Kind::eqBand || hit.kind == ChromeHit::Kind::eqBand) {
    const int idx = sliderIndex_ >= 0 ? sliderIndex_ : hit.index;
    settings_.equalizerCurve = settings_.equalizerCurve.withGain(idx, band(hit.rect, logical.y()));
    scheduleApplyEq();
    sliderKind_ = ChromeHit::Kind::eqBand;
    sliderIndex_ = idx;
    schedulePersist();
    refreshEqChrome();
  } else if (hit.kind == ChromeHit::Kind::plDivider) {
    const qreal x = logical.x();
    settings_.playlistCollectionWidth =
        std::clamp(double(x), double(kPlaylistCollectionMinWidth), 480.0);
    schedulePersist();
    refreshChrome();
  } else if (sliderKind_ == ChromeHit::Kind::plTrackRow) {
    const int from = sliderIndex_;
    const int dy = logical.y() - dragOrigin_.y();
    if (std::abs(dy) < 12) return;
    const int delta = dy / 37;
    const int to = from + delta;
    if (to == from || to < 0 || to >= playlist_.tracks().size()) return;
    playlist_.move(from, to < from ? to : to + 1);
    sliderIndex_ = to;
    dragOrigin_ = logical;
  }
}

void TrampSession::presentChromeOutcome(const ChromeCommandOutcome& out, WindowId id,
                                        const ChromeHit& hit, QPoint logical) {
  if (out.beginSlider) {
    sliderKind_ = out.sliderKind;
    sliderIndex_ = out.sliderIndex;
    handleDrag(id, hit, sliderPressPoint(hit.rect, logical));
  }
  if (out.toggleVisible) {
    setWindowVisible(*out.toggleVisible, !windowShouldShow(*out.toggleVisible));
  }
  if (out.settingsTab) settingsTab_ = *out.settingsTab;
  if (out.applyEq) applyEq();
  if (out.applyAlwaysOnTop) applyAlwaysOnTop();
  if (out.syncTitleMarquee) syncTitleMarquee();
  if (out.persist) schedulePersist();
  if (out.persistCollection) persistCollectionCache();
  if (out.refreshEq) refreshEqChrome();
  if (out.refreshChrome) refreshChrome();
  switch (out.intent) {
    case ChromeIntent::pickAudio: {
      const QString picked = pickAudio(true);
      if (!picked.isEmpty()) openPaths(picked.split(QLatin1Char('\n')), true);
      break;
    }
    case ChromeIntent::pickPlaylistFile: {
      const QString path = pickPlaylist(false);
      if (!path.isEmpty()) {
        startDurationProbe(ingestPlaylistFile(path));
        refreshChrome();
      }
      break;
    }
    case ChromeIntent::showPlCreateMenu:
      presentPlCreateMenu(hit);
      break;
    case ChromeIntent::renameCollectionEntry:
      presentPlRename();
      break;
    case ChromeIntent::showPlSortMenu:
      presentPlSortMenu(hit);
      break;
    case ChromeIntent::showPlOptionsMenu:
      presentPlOptionsMenu(hit);
      break;
    case ChromeIntent::refreshCurrentPlaylist:
      refreshCurrentPlaylist();
      break;
    case ChromeIntent::loadCollectionRow:
      loadCollectionRow(out.collectionRow);
      break;
    case ChromeIntent::showOptionsMenu:
      showOptionsMenu(hit.rect);
      break;
    case ChromeIntent::showEqPresets:
      presentEqPresets(hit);
      break;
    case ChromeIntent::openWebsite:
      QDesktopServices::openUrl(QUrl(QStringLiteral("https://tramp.music")));
      break;
    case ChromeIntent::resetSettings:
      presentResetSettings();
      break;
    case ChromeIntent::rescanSkins: {
      WaitCursorScope wait;
      skins_.rescan();
      refreshChrome();
      break;
    }
    case ChromeIntent::activateSkin: {
      const auto catalog = skins_.catalog();
      if (out.collectionRow < 0 || out.collectionRow >= catalog.size()) break;
      const QString id = catalog[out.collectionRow].id;
      withWaitCursor(this, [this, id]() {
        if (skins_.activate(id, settings_)) {
          schedulePersist();
        }
        refreshChrome();
      });
      break;
    }
    case ChromeIntent::pickSkinZip:
      presentSkinZipInstall();
      break;
    case ChromeIntent::pickSkinFolder:
      presentSkinFolderInstall();
      break;
    case ChromeIntent::pickSkinsDirectory:
      presentSkinsDirectoryPick();
      break;
    case ChromeIntent::resetSkinsDirectory: {
      WaitCursorScope wait;
      skins_.setSkinsDirectory({}, settings_);
      schedulePersist();
      refreshChrome();
      break;
    }
    case ChromeIntent::none:
      break;
  }
}

void TrampSession::presentPlCreateMenu(const ChromeHit& hit) {
  enum Row { kFromCurrent, kFromSelection };
  const QVector<ChromeMenuItem> items{
      ChromeMenuItem::action(QStringLiteral("From current playlist"),
                             !playlist_.tracks().isEmpty()),
      ChromeMenuItem::action(QStringLiteral("From selection"),
                             !playlist_.selectedIndices().isEmpty()),
  };
  const int chosen = execAnchoredMenu(items, windowFor(WindowId::playlist), hit.rect, PopupAnchor::aboveLeft);
  if (chosen == kFromCurrent) {
    const QString path = pickPlaylist(true);
    if (!path.isEmpty()) {
      if (reportPlaylistWriteFailure(playlist_.savePlaylistFile(path), path)) {
        collection_.addWritten(path, playlist_.tracks());
        persistCollectionCache();
        startDurationProbe(playlist_.tracks());
      }
    }
  } else if (chosen == kFromSelection) {
    const QString path = pickPlaylist(true);
    if (!path.isEmpty()) {
      QVector<Track> selected;
      QList<int> idx = playlist_.selectedIndices().values();
      std::sort(idx.begin(), idx.end());
      for (int i : idx) selected.push_back(playlist_.tracks()[i]);
      // Whatever the cache already knows goes into the file; the rest is
      // asked for behind the save, the same as every other ingest. Saving
      // from the current list has always written it this way.
      collection_.hydrateDurations(selected);
      if (reportPlaylistWriteFailure(writeM3uFile(path, selected), path)) {
        collection_.addWritten(path, selected);
        persistCollectionCache();
        startDurationProbe(selected);
      }
    }
  }
  refreshChrome();
}

void TrampSession::presentPlRename() {
  if (collection_.selectedPath().isEmpty()) return;
  QString current;
  for (const SavedPlaylist& e : collection_.entries()) {
    if (e.path == collection_.selectedPath()) {
      current = e.displayName();
      break;
    }
  }
  bool ok = false;
  const QString name = QInputDialog::getText(
      dialogParent(WindowId::playlist), QStringLiteral("Rename playlist"), QStringLiteral("Name"),
      QLineEdit::Normal, current, &ok);
  if (!ok) return;
  collection_.rename(collection_.selectedPath(), name);
  persistCollectionCache();
  refreshChrome();
}

void TrampSession::presentPlSortMenu(const ChromeHit& hit) {
  enum Row { kTitle, kArtist, kDuration, kPath, kReverse };
  const QVector<ChromeMenuItem> items{
      ChromeMenuItem::action(QStringLiteral("Title")),
      ChromeMenuItem::action(QStringLiteral("Artist")),
      ChromeMenuItem::action(QStringLiteral("Duration")),
      ChromeMenuItem::action(QStringLiteral("Path")),
      ChromeMenuItem::action(QStringLiteral("Reverse")),
  };
  switch (execAnchoredMenu(items, windowFor(WindowId::playlist), hit.rect, PopupAnchor::aboveLeft)) {
    case kTitle:
      playlist_.sortBy(PlaylistSortKey::title);
      break;
    case kArtist:
      playlist_.sortBy(PlaylistSortKey::artist);
      break;
    case kDuration:
      playlist_.sortBy(PlaylistSortKey::duration);
      break;
    case kPath:
      playlist_.sortBy(PlaylistSortKey::path);
      break;
    case kReverse:
      playlist_.reverseTracks();
      break;
    default:
      break;
  }
}

void TrampSession::presentPlOptionsMenu(const ChromeHit& hit) {
  enum Row { kSelectAll, kInvertSelection, kSave, kClear };
  const QVector<ChromeMenuItem> items{
      ChromeMenuItem::action(QStringLiteral("Select all")),
      ChromeMenuItem::action(QStringLiteral("Invert selection")),
      ChromeMenuItem::action(QStringLiteral("Save playlist…")),
      ChromeMenuItem::action(QStringLiteral("Clear")),
  };
  switch (execAnchoredMenu(items, windowFor(WindowId::playlist), hit.rect, PopupAnchor::aboveLeft)) {
    case kSelectAll:
      playlist_.selectAll();
      break;
    case kInvertSelection:
      playlist_.invertSelection();
      break;
    case kSave: {
      const QString path = pickPlaylist(true);
      if (!path.isEmpty() &&
          reportPlaylistWriteFailure(playlist_.savePlaylistFile(path), path) &&
          collection_.contains(path)) {
        collection_.addWritten(path, playlist_.tracks());
        persistCollectionCache();
        startDurationProbe(playlist_.tracks());
      }
      break;
    }
    case kClear:
      playlist_.clear();
      break;
    default:
      break;
  }
}

void TrampSession::presentEqPresets(const ChromeHit& hit) {
  const auto& presets = EqualizerPresets::builtIn();
  QVector<ChromeMenuItem> items;
  items.reserve(presets.size());
  for (const auto& preset : presets) items.push_back(ChromeMenuItem::action(preset.first));
  const int chosen =
      execAnchoredMenu(items, windowFor(WindowId::equalizer), hit.rect, PopupAnchor::belowLeft);
  if (chosen == kChromeMenuNone) return;
  settings_.equalizerCurve =
      settings_.equalizerCurve.withPreset(presets[chosen].first, presets[chosen].second);
  settings_.equalizerCurve.enabled = true;
  applyEq();
  schedulePersist();
  refreshEqChrome();
}

void TrampSession::presentResetSettings() {
  settings_ = TrampSettings{};
  applyEq();
  engine_->setForceMono(false);
  layout_.docking().setSnapThreshold(snapPixels(settings_.dockSnapStrength));
  applyAlwaysOnTop();
  {
    WaitCursorScope wait;
    skins_.setSkinsDirectory({}, settings_);
  }
  syncTitleMarquee();
  schedulePersist();
  refreshChrome();
}

void TrampSession::presentSkinZipInstall() {
  FilePick pick;
  pick.parent = windowFor(WindowId::settings);
  pick.title = QStringLiteral("Install skin");
  pick.filter = QStringLiteral("Skin zip (*.zip)");
  pick.kind = FilePickKind::openFile;
  const QString path = pickFile(pick);
  if (!path.isEmpty()) {
    WaitCursorScope wait;
    if (skins_.installZip(path, skinConflictPrompt())) {
      schedulePersist();
    }
  }
  refreshChrome();
}

void TrampSession::presentSkinFolderInstall() {
  FilePick pick;
  pick.parent = windowFor(WindowId::settings);
  pick.title = QStringLiteral("Install skin folder");
  pick.kind = FilePickKind::openDirectory;
  const QString path = pickFile(pick);
  if (!path.isEmpty()) {
    WaitCursorScope wait;
    if (skins_.installDirectory(path, skinConflictPrompt())) {
      schedulePersist();
    }
  }
  refreshChrome();
}

void TrampSession::presentSkinsDirectoryPick() {
  FilePick pick;
  pick.parent = windowFor(WindowId::settings);
  pick.title = QStringLiteral("Skins folder");
  pick.directory = skins_.skinsDirectory();
  pick.kind = FilePickKind::openDirectory;
  const QString path = pickFile(pick);
  if (!path.isEmpty()) {
    WaitCursorScope wait;
    skins_.setSkinsDirectory(path, settings_);
    schedulePersist();
  }
  refreshChrome();
}

void TrampSession::handleHit(WindowId id, ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical) {
  dragOrigin_ = logical;
  ChromeCommandRouter router(*playback_, playlist_, settings_, collection_, *engine_,
                             layout_.docking());
  presentChromeOutcome(router.handle(id, hit, mods, logical), id, hit, logical);
}

void TrampSession::showOptionsMenu(QRect logicalHit) {
  // The rules keep the window toggle and the destructive row away from the
  // four that just open something. Row indices count them, hence the members.
  enum Row { kAlwaysOnTop, kRuleTop, kSettings, kTrackInfo, kAbout, kOpenFiles, kRuleQuit, kQuit };
  const QVector<ChromeMenuItem> items = optionsMenuItems(settings_);
  if (logicalHit.isEmpty()) logicalHit = mainOptionsHit(kMainPlayer);
  switch (execAnchoredMenu(items, windowFor(WindowId::main), logicalHit, PopupAnchor::belowLeft)) {
    case kAlwaysOnTop:
      settings_.alwaysOnTop = !settings_.alwaysOnTop;
      applyAlwaysOnTop();
      schedulePersist();
      break;
    case kSettings:
      setWindowVisible(WindowId::settings, !windowShouldShow(WindowId::settings));
      break;
    case kTrackInfo:
      showTrackInfo();
      break;
    case kAbout:
      if (windowShouldShow(WindowId::about)) emit requestRaise(WindowId::about);
      else setWindowVisible(WindowId::about, true);
      break;
    case kOpenFiles: {
      const QString picked = pickAudio(true);
      if (!picked.isEmpty()) openPaths(picked.split(QLatin1Char('\n')), true);
      break;
    }
    case kQuit:
      quitFromMenu();
      break;
    default:
      break;
  }
}

int TrampSession::execAnchoredMenu(const QVector<ChromeMenuItem>& items, HostWindow* host,
                                   QRect logicalHit, PopupAnchor anchor) {
  if (!host) return kChromeMenuNone;
  if (logicalHit.isEmpty()) logicalHit = QRect(0, 0, 1, 1);
  const QRect widget = host->widgetRectFromLogical(logicalHit);
  const QRect global(host->mapToGlobal(widget.topLeft()), widget.size());
  return execChromeMenu(host, items, global, anchor, layout_.zoomPercent(), skins_.tokens());
}

void TrampSession::showTrackInfo() {
  const auto track = playback_->currentTrack();
  QString message = QStringLiteral("No track loaded.");
  if (track) {
    QStringList lines;
    lines << track->displayTitle();
    if (!track->artist.trimmed().isEmpty()) {
      lines << QStringLiteral("Artist: %1").arg(track->artist);
    }
    if (!track->album.trimmed().isEmpty()) {
      lines << QStringLiteral("Album: %1").arg(track->album);
    }
    lines << QStringLiteral("Path: %1").arg(track->path);
    message = lines.join(QLatin1Char('\n'));
  }
  QMessageBox::information(windowFor(WindowId::main), QStringLiteral("Track info"), message);
}

bool TrampSession::reportPlaylistWriteFailure(bool wrote, const QString& path) {
  if (wrote) return true;
  QMessageBox::warning(
      windowFor(WindowId::main), QStringLiteral("Playlist not saved"),
      QStringLiteral("Tramp could not write %1.\n\nThe playlist still has its "
                     "unsaved changes, and the file on disk is unchanged.")
          .arg(QDir::toNativeSeparators(path)));
  return false;
}

bool TrampSession::confirmReplaceAltered(const QString& consequence) {
  if (!playlist_.altered()) return true;
  QMessageBox box(windowFor(WindowId::main));
  box.setWindowTitle(QStringLiteral("Save the current playlist?"));
  const QString follow = consequence.isEmpty()
                             ? QStringLiteral("Loading another playlist replaces it.")
                             : consequence;
  box.setText(QStringLiteral("The current playlist has changes that are not in any file. ") +
              follow);
  QPushButton* cancel = box.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
  QPushButton* discard = box.addButton(QStringLiteral("Discard and load"), QMessageBox::DestructiveRole);
  QPushButton* save = box.addButton(QStringLiteral("Save and load"), QMessageBox::AcceptRole);
  box.setDefaultButton(cancel);
  box.setEscapeButton(cancel);
  box.exec();
  if (box.clickedButton() == cancel || box.clickedButton() == nullptr) return false;
  if (box.clickedButton() == save) {
    QString path = playlist_.sourcePath();
    if (path.isEmpty()) path = pickPlaylist(true);
    if (path.isEmpty()) return false;
    // If the save did not land, keep the list rather than discarding it.
    if (!reportPlaylistWriteFailure(playlist_.savePlaylistFile(path), path)) return false;
    if (collection_.contains(path)) {
      collection_.addWritten(path, playlist_.tracks());
      persistCollectionCache();
    }
  }
  Q_UNUSED(discard);
  return true;
}

void TrampSession::quitFromMenu() {
  if (HostWindow* main = windowFor(WindowId::main)) main->close();
}

QString TrampSession::bundledSkinsDir() const { return tramp::bundledSkinsDir(); }

SkinController::ConflictFn TrampSession::skinConflictPrompt() {
  return [this](const SkinConflict& conflict) {
    QWidget* parent = dialogParent(WindowId::settings);
    const QString text =
        QStringLiteral("A skin named \"%1\" is already installed%2.\nReplace it with \"%3\"%4?")
            .arg(conflict.installedName,
                 conflict.installedAuthor.isEmpty()
                     ? QString()
                     : QStringLiteral(" (%1)").arg(conflict.installedAuthor),
                 conflict.incomingName,
                 conflict.incomingAuthor.isEmpty()
                     ? QString()
                     : QStringLiteral(" (%1)").arg(conflict.incomingAuthor));
    WaitCursorPause pause;
    const auto reply = QMessageBox::question(parent, QStringLiteral("Replace skin?"), text,
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return reply == QMessageBox::Yes ? SkinConflictChoice::replace : SkinConflictChoice::cancel;
  };
}

}  // namespace tramp
