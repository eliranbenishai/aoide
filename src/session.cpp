#include "session.h"

#include "audio_output.h"
#include "chrome_command.h"
#include "chrome_layout.h"
#include "document_portal.h"
#include "files.h"
#include "host_shell.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "look.h"
#include "m3u.h"
#include "skin_preview.h"
#include "native_file_dialog.h"
#ifdef AOIDE_HAVE_MPV
#include "mpv_engine.h"
#include "pcm_decoder.h"
#endif
#include "duration_probe.h"
#include "player_engine.h"
#include "popup_anchor.h"
#include "support_dir.h"
#include "wait_cursor.h"
#include "aoide_fonts.h"
#include "aoide_metrics.h"

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

namespace aoide {
namespace {

/// Probe answers come back in batches. Small enough that the first rows fill in
/// while the rest of the list is still being asked about, big enough that a
/// thousand-track open is tens of repaints rather than a thousand.
constexpr int kProbeBatchSize = 24;
constexpr int kProbeBatchMs = 120;

}  // namespace

AoideSession::AoideSession(QObject* parent)
    : QObject(parent), store_(aoideSupportDirectory()) {
  settings_ = store_.readSettings();
  collection_.load(store_);
#ifdef AOIDE_HAVE_MPV
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
#ifdef AOIDE_HAVE_MPV
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
    const bool wasFailed = persistHealth_.anyFailed();
    if (playlist_.altered()) {
      persistHealth_.alteredOk =
          store_.writeAltered({playlist_.tracks(), playlist_.sourcePath()});
    } else {
      store_.clearAltered();
    }
    if (persistHealth_.anyFailed() != wasFailed) refreshChrome();
  });
  usageTimer_.setSingleShot(true);
  usageTimer_.setInterval(2000);
  QObject::connect(&usageTimer_, &QTimer::timeout, this, [this]() {
    const bool wasFailed = persistHealth_.anyFailed();
    persistHealth_.usageOk = store_.writeUsage({playback_->spins()});
    if (persistHealth_.anyFailed() != wasFailed) refreshChrome();
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
  engine_->setAudioDevice(normalizeAudioDeviceName(settings_.audioDevice));
  engine_->setAudioExclusive(settings_.audioExclusive);
  refreshAudioOutputs();
  applyEq();
  {
    WaitCursorScope wait;
    skins_.bootstrap(aoideSupportDirectory(), bundledSkinsDir(), settings_);
    refreshSkinPreviews();
  }
  syncTitleMarquee();
}

AoideSession::~AoideSession() {
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

void AoideSession::detachWindows() {
  persistNow();
  windows_.clear();
  shell_ = nullptr;
}

void AoideSession::bindPlayback() {
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

void AoideSession::syncSpectrum() {
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

void AoideSession::tickSpectrum() {
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

void AoideSession::syncTitleMarquee() {
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

qint64 AoideSession::titleScrollMs() const {
  if (!settings_.scrollTitle || !marqueeClock_.isValid()) return 0;
  return marqueeClock_.elapsed();
}

void AoideSession::startSpectrumDecode(const QString& path, int gen) {
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

void AoideSession::setWindows(const PanelWindows& windows) { windows_ = windows; }

void AoideSession::setShell(HostShell* shell) {
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

void AoideSession::bootstrap(const QStringList& argvFiles) {
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

void AoideSession::applyEq() { engine_->setEqualizerAf(buildEqualizerAf(settings_.equalizerCurve)); }

void AoideSession::scheduleApplyEq() {
  if (!eqApplyTimer_.isActive()) eqApplyTimer_.start();
}

void AoideSession::refreshEqChrome() {
  if (HostWindow* eq = windowFor(WindowId::equalizer)) eq->applyEqualizer(settings_.equalizerCurve);
}

bool AoideSession::confirmQuit() const { return settings_.confirmBeforeQuit; }

bool AoideSession::windowShouldShow(WindowId id) const {
  return layout_.layout().frameOf(id).visible;
}

void AoideSession::applyAlwaysOnTop() {
  if (shell_) shell_->setAlwaysOnTop(settings_.alwaysOnTop);
}

void AoideSession::refreshAboutFigures() {
  figures_ = collection_.readFigures();
  figuresLoaded_ = true;
  refreshChrome();
}

void AoideSession::persistCollectionCache() {
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
QVector<Track> AoideSession::ingestPlaylistFile(const QString& path) {
  const QVector<Track> tracks = collection_.add(path);
  collection_.addWritten(path, tracks);
  persistCollectionCache();
  return tracks;
}

void AoideSession::schedulePathVerify() {
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

void AoideSession::refreshCurrentPlaylist() {
  const QString path = playlist_.sourcePath();
  if (path.isEmpty() || !QFileInfo::exists(path)) return;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
  const QString contents = decodeM3uBytes(file.readAll());
  // An empty playlist file legitimately empties the list. A file that is not
  // playlist text at all is no answer, so the list it would have replaced
  // stands — and there is nothing to ask the listener about.
  if (!isPlaylistText(contents)) return;
  if (!confirmReplaceAltered(QStringLiteral(
          "Refreshing this playlist replaces it with the file on disk. "
          "Missing tracks are removed."))) {
    return;
  }
  const QVector<Track> parsed = M3uCodec().parse(contents, path);
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

void AoideSession::startDurationProbe(const QVector<Track>& tracks, bool overwrite) {
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

void AoideSession::applyProbedBatch(const QVector<ProbedTrack>& batch, int gen, bool overwrite) {
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

void AoideSession::probeFinished(int gen) {
  if (gen != durationGen_.load()) return;
  probeOutstanding_.clear();
  setIngesting(false);
}

void AoideSession::setIngesting(bool ingesting) {
  if (ingesting_ == ingesting) return;
  ingesting_ = ingesting;
  refreshChrome();
}

void AoideSession::persistNow() {
  if (qEnvironmentVariable("AOIDE_AUTO_QUIT") == QLatin1String("1")) return;
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
  SessionResume resume;
  resume.playingIndex = playback_->playingIndex();
  resume.positionMs = playback_->positionMs();
  resume.wasPlaying = playback_->playing();
  AlteredPlaylist altered;
  if (playlist_.altered()) {
    altered = {playlist_.tracks(), playlist_.sourcePath()};
  }
  const bool wasFailed = persistHealth_.anyFailed();
  writeSessionPersist(store_, persistHealth_, settings_, resume, {playback_->spins()},
                      playlist_.sourcePath(), playlist_.altered() ? &altered : nullptr);
  if (collectionPersistTimer_.isActive()) {
    collectionPersistTimer_.stop();
    persistCollectionCache();
  }
  if (persistHealth_.anyFailed() != wasFailed) refreshChrome();
}

void AoideSession::schedulePersist() { persistTimer_.start(); }
void AoideSession::scheduleAltered() { alteredTimer_.start(); }
void AoideSession::scheduleUsage() { usageTimer_.start(); }

HostWindow* AoideSession::windowFor(WindowId id) const { return windows_[id]; }

/// A dialog belongs to the panel it was raised from, so it lands over that
/// panel rather than wherever the window manager felt like. Main stands in when
/// the panel has no window — the dump and bench paths run with fewer than five.
QWidget* AoideSession::dialogParent(WindowId id) const {
  if (HostWindow* window = windowFor(id)) return window;
  return windowFor(WindowId::main);
}

QRect AoideSession::hostRect() const { return shell_ ? shell_->virtualDesktop() : QRect(); }

QRect AoideSession::workAreaFor(QRect clusterNative) const {
  // An L-shaped monitor arrangement leaves dead zones inside the virtual
  // desktop that belong to no screen, so a cluster's centre can land on
  // nothing. The primary screen is the honest answer then: it is the display a
  // listener who has mislaid their layout will look at.
  const QScreen* screen =
      clusterNative.isNull() ? nullptr : QGuiApplication::screenAt(clusterNative.center());
  if (!screen) screen = QGuiApplication::primaryScreen();
  return screen ? screen->availableGeometry() : QRect();
}

SessionView AoideSession::view() const {
  SessionView v;
  v.eqOn = layout_.layout().equalizer.visible;
  v.plOn = layout_.layout().playlist.visible;
  v.skinsOn = layout_.layout().skins.visible;
  v.trackInfoEnabled = playback_->currentTrack().has_value();
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
  v.persistWriteFailed = persistHealth_.anyFailed();
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
  v.audioDeviceLabel = audioDeviceDisplayLabel(settings_.audioDevice, audioOutputs_);
  v.audioExclusive = settings_.audioExclusive;
  v.aboutPlaylists = figures_.playlists;
  v.aboutTracks = figures_.tracks;
  v.aboutTimeMs = figures_.totalDurationMs;
  v.aboutSpins = playback_->spins();
  v.aboutMeasured = figuresLoaded_;
  v.look = skins_.tokens();
  v.skins = skins_.catalog();
  v.activeSkinId = settings_.activeSkinId;
  v.skinsError = skins_.lastError();
  const QRectF skinsViewport = skinsListViewport(skinsPane(kSkins));
  v.skinsScroll = std::max(0, std::min(skinsScroll_, skinsListMaxScroll(skins_.catalog().size(),
                                                                        skinsViewport)));

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

MainLiveReadouts AoideSession::mainLive() const {
  MainLiveReadouts live;
  live.positionMs = playback_->positionMs();
  live.durationMs = playback_->durationMs();
  live.showElapsed = settings_.showElapsed;
  live.titleScrollMs = titleScrollMs();
  live.spectrum = spectrumHold_.bars;
  live.spectrumPeaks = spectrumHold_.peaks;
  return live;
}

void AoideSession::refreshChrome() {
  if (holdChrome_) {
    chromeHeld_ = true;
    return;
  }
  emit chromeChanged();
}

void AoideSession::setZoomPercent(qreal percent) {
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

void AoideSession::setWindowVisible(WindowId id, bool visible) {
  layout_.docking().setVisible(id, visible);
  if (visible) {
    layout_.docking().nudgeOffMainIfStacked(id);
    emit requestShow(id);
    layout_.clampToHost(id);
    layout_.place();
    if (id == WindowId::settings || id == WindowId::skins) emit requestRaise(id);
    if (id == WindowId::settings) refreshAudioOutputs();
    if (id == WindowId::skins) {
      WaitCursorScope wait;
      skins_.rescan();
      refreshSkinPreviews();
    }
    if (id == WindowId::about) refreshAboutFigures();
  } else {
    emit requestHide(id);
    layout_.place();
  }
  applyAlwaysOnTop();
  schedulePersist();
  refreshChrome();
}

void AoideSession::setShaded(WindowId id, bool shaded) {
  layout_.docking().setShaded(id, shaded);
  schedulePersist();
  layout_.place();
}

void AoideSession::extraClosed(WindowId id) {
  layout_.docking().setVisible(id, false);
  applyAlwaysOnTop();
  layout_.place();
  schedulePersist();
  refreshChrome();
}

void AoideSession::mainMinimized(bool minimized) {
  if (!settings_.minimizeHidesSecondaries) return;
  layout_.setMainMinimized(minimized);
  if (!minimized) applyAlwaysOnTop();
  layout_.place();
  if (!minimized) raiseSettingsIfShowing();
}

void AoideSession::mainActivated() { raiseSettingsIfShowing(); }

/// Settings sits above the cluster: anything that brings main forward has to
/// bring it along, or it disappears behind the player it configures.
void AoideSession::raiseSettingsIfShowing() {
  HostWindow* settings = windowFor(WindowId::settings);
  if (settings && settings->isVisible()) settings->raise();
  HostWindow* skins = windowFor(WindowId::skins);
  if (skins && skins->isVisible()) skins->raise();
}

void AoideSession::playTrackAt(int index) {
  const auto tracks = playlist_.tracks();
  if (index < 0 || index >= tracks.size() || tracks[index].disabled) return;
  playback_->playIndex(index);
}
/// Space and the play media key are a single toggle, unlike the separate Play and
/// Pause faces on the chrome. Routing them at K::play only ever started playback.
void AoideSession::togglePlayPause() { playback_->playPause(); }

void AoideSession::selectAllTracks() { playlist_.selectAll(); }
void AoideSession::removeSelectedTracks() { playlist_.removeSelected(); }

void AoideSession::windowMoved(WindowId id, QPoint nativeTopLeft, bool finalize) {
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

void AoideSession::titleDragBegan(WindowId id) {
  Q_UNUSED(id);
  titleDragging_ = true;
}

void AoideSession::titleDragEnded(WindowId id) {
  if (!titleDragging_) return;
  titleDragging_ = false;
  HostWindow* w = windowFor(id);
  if (w) windowMoved(id, w->nativeTopLeft(), id != WindowId::main);
  else schedulePersist();
}

void AoideSession::extraWasMapped(WindowId id) {
  if (id == WindowId::main) return;
  layout_.place();
}

void AoideSession::reapplyWindowFrames() {
  layout_.place();
}

void AoideSession::placePanels(const QVector<PanelPlacement>& panels) {
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

void AoideSession::playlistResized(QSize native) {
  if (layout_.placing()) return;
  const qreal z = layout_.zoomPercent() / 100.0;
  layout_.docking().resizePlaylist(QSizeF(native.width() / z, native.height() / z));
  layout_.clampToHost(WindowId::playlist);
  layout_.place();
  schedulePersist();
}

void AoideSession::applyDroppedPaths(const QStringList& paths, bool replace) {
  openPaths(paths, !replace && !playlist_.tracks().isEmpty());
}

void AoideSession::openPaths(const QStringList& paths, bool enqueue) {
  // Every path from outside the app arrives here -- argv, a drop, a pick -- and
  // some of them are document-portal exports that expire at logout. Trade them
  // for the paths they stand in for before anything writes them down. A
  // directory is swapped before it is walked, so its contents come out durable
  // too, and an M3U before it is parsed, so its relative entries resolve
  // against the real folder.
  const QStringList incoming = durablePaths(paths);
  QStringList playlists;
  QStringList others;
  for (const QString& p : incoming) {
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

QString AoideSession::pickAudio(bool multiple) {
  FilePick pick;
  pick.parent = windowFor(WindowId::main);
  pick.title = multiple ? QStringLiteral("Add audio files") : QStringLiteral("Open audio");
  pick.filter = qtFileFilter(QStringLiteral("Audio"), audioExtensions());
  pick.kind = multiple ? FilePickKind::openFiles : FilePickKind::openFile;
  if (multiple) {
    QStringList kept;
    for (const QString& path : pickFiles(pick)) {
      if (isAudioPath(path)) kept.push_back(path);
    }
    return kept.join(QLatin1Char('\n'));
  }
  const QString path = pickFile(pick);
  return isAudioPath(path) ? path : QString();
}

QString AoideSession::pickPlaylist(bool save) {
  FilePick pick;
  pick.parent = windowFor(WindowId::main);
  pick.filter = qtFileFilter(QStringLiteral("Playlists"), playlistExtensions());
  if (save) {
    pick.title = QStringLiteral("Save playlist");
    pick.suggestedName = QStringLiteral("playlist.m3u");
    pick.kind = FilePickKind::saveFile;
    return pickFile(pick);
  }
  pick.title = QStringLiteral("Open playlist");
  pick.kind = FilePickKind::openFile;
  const QString path = pickFile(pick);
  return isPlaylistPath(path) ? path : QString();
}

void AoideSession::loadCollectionRow(int index) {
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

void AoideSession::handleRelease(WindowId id) {
  sliderKind_ = ChromeHit::Kind::none;
  if (eqApplyTimer_.isActive()) {
    eqApplyTimer_.stop();
    applyEq();
  }
  if (id == WindowId::equalizer || id == WindowId::playlist) {
    if (HostWindow* w = windowFor(id)) windowMoved(id, w->nativeTopLeft(), true);
  }
}

void AoideSession::handleWheel(WindowId id, int delta) {
  // A zero delta used to scroll down one row; Mac trackpads send that with
  // a non-zero pixelDelta that the window now consumes.
  if (delta == 0) return;
  if (id == WindowId::skins) {
    const QRectF viewport = skinsListViewport(skinsPane(kSkins));
    const int maxScroll = skinsListMaxScroll(skins_.catalog().size(), viewport);
    const int step = delta > 0 ? -int(qRound(skinsGridRowStride(viewport)))
                               : int(qRound(skinsGridRowStride(viewport)));
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

void AoideSession::handleDrag(WindowId id, ChromeHit hit, QPoint logical) {
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
  } else if (sliderKind_ == ChromeHit::Kind::settingsSkinScroll ||
             hit.kind == ChromeHit::Kind::settingsSkinScroll) {
    const QRectF viewport = skinsListViewport(skinsPane(kSkins));
    const int maxScroll = skinsListMaxScroll(skins_.catalog().size(), viewport);
    const QRectF track = skinsListScrollTrack(viewport);
    const QRectF thumb = skinsListThumb(track, viewport, skins_.catalog().size(), skinsScroll_);
    const qreal travel = track.height() - thumb.height();
    const qreal t =
        travel <= 0 ? 0 : std::clamp((logical.y() - track.top() - thumb.height() / 2) / travel,
                                     qreal(0), qreal(1));
    skinsScroll_ = int(qRound(t * maxScroll));
    sliderKind_ = ChromeHit::Kind::settingsSkinScroll;
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

void AoideSession::presentChromeOutcome(const ChromeCommandOutcome& out, WindowId id,
                                        const ChromeHit& hit, QPoint logical) {
  if (out.beginSlider) {
    sliderKind_ = out.sliderKind;
    sliderIndex_ = out.sliderIndex;
    handleDrag(id, hit, sliderPressPoint(hit.rect, logical));
  }
  if (out.toggleVisible) {
    setWindowVisible(*out.toggleVisible, !windowShouldShow(*out.toggleVisible));
  }
  if (out.settingsTab) {
    settingsTab_ = *out.settingsTab;
    if (settingsTab_ == 1) refreshAudioOutputs();
  }
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
    case ChromeIntent::saveCurrentPlaylist:
      saveCurrentPlaylist();
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
    case ChromeIntent::showTrackInfo:
      showTrackInfo();
      break;
    case ChromeIntent::showEqPresets:
      presentEqPresets(hit);
      break;
    case ChromeIntent::showAudioDevices:
      presentAudioDevices(hit);
      break;
    case ChromeIntent::openWebsite:
      QDesktopServices::openUrl(QUrl(QStringLiteral("https://aoide.music")));
      break;
    case ChromeIntent::resetSettings:
      presentResetSettings();
      break;
    case ChromeIntent::showSkinInstallMenu:
      presentSkinInstallMenu(hit);
      break;
    case ChromeIntent::openSkinsDirectory:
      QDesktopServices::openUrl(QUrl::fromLocalFile(skins_.skinsDirectory()));
      break;
    case ChromeIntent::rescanSkins: {
      WaitCursorScope wait;
      skins_.rescan();
      refreshSkinPreviews();
      refreshChrome();
      break;
    }
    case ChromeIntent::activateSkin: {
      const auto catalog = skins_.catalog();
      if (out.collectionRow < 0 || out.collectionRow >= catalog.size()) break;
      const QString id = catalog[out.collectionRow].id;
      if (id == settings_.activeSkinId) break;
      withWaitCursor(this, [this, id]() {
        if (skins_.activate(id, settings_)) {
          schedulePersist();
        }
        refreshChrome();
      });
      break;
    }
    case ChromeIntent::removeSkin:
      presentSkinRemove(out.collectionRow);
      break;
    case ChromeIntent::pickSkinZip:
      presentSkinZipInstall();
      break;
    case ChromeIntent::pickSkinFolder:
      presentSkinFolderInstall();
      break;
    case ChromeIntent::none:
      break;
  }
}

void AoideSession::presentPlCreateMenu(const ChromeHit& hit) {
  enum Row { kFromCurrent, kFromSelection };
  const QVector<ChromeMenuItem> items{
      ChromeMenuItem::action(QStringLiteral("From current playlist"),
                             !playlist_.tracks().isEmpty()),
      ChromeMenuItem::action(QStringLiteral("From selection"),
                             !playlist_.selectedIndices().isEmpty()),
  };
  const int chosen = execAnchoredMenu(items, windowFor(WindowId::playlist), hit.rect, PopupAnchor::aboveLeft);
  if (chosen == kFromCurrent) {
    createPlaylistFromCurrent();
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

void AoideSession::createPlaylistFromCurrent() {
  const QString path = pickPlaylist(true);
  if (path.isEmpty()) return;
  if (reportPlaylistWriteFailure(playlist_.savePlaylistFile(path), path)) {
    collection_.addWritten(path, playlist_.tracks());
    persistCollectionCache();
    startDurationProbe(playlist_.tracks());
    refreshChrome();
  }
}

void AoideSession::saveCurrentPlaylist() {
  if (!playlist_.altered()) return;
  if (playlist_.sourcePath().isEmpty()) {
    createPlaylistFromCurrent();
    return;
  }
  const QString path = playlist_.sourcePath();
  if (!reportPlaylistWriteFailure(playlist_.savePlaylistFile(path), path)) return;
  if (collection_.contains(path)) {
    collection_.addWritten(path, playlist_.tracks());
    persistCollectionCache();
  }
  startDurationProbe(playlist_.tracks());
  refreshChrome();
}

void AoideSession::renameCollectionRow(int index) {
  const auto entries = collection_.entries();
  if (index < 0 || index >= entries.size()) return;
  collection_.select(entries[index].path);
  // Publish the selection before the modal: the row being renamed is the one
  // the listener double-clicked, and the highlight is what says so.
  refreshChrome();
  presentPlRename();
}

void AoideSession::presentPlRename() {
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
      dialogParent(WindowId::playlist), QStringLiteral("Rename playlist"),
      QStringLiteral("New name for this playlist\n"
                     "Aoide only renames its own entry — the file on disk keeps its name."),
      QLineEdit::Normal, current, &ok);
  if (!ok) return;
  collection_.rename(collection_.selectedPath(), name);
  persistCollectionCache();
  refreshChrome();
}

void AoideSession::presentPlSortMenu(const ChromeHit& hit) {
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

void AoideSession::presentPlOptionsMenu(const ChromeHit& hit) {
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

void AoideSession::refreshAudioOutputs() {
  audioOutputs_ = withAutoAudioDevice(engine_->listAudioOutputs());
}

void AoideSession::presentAudioDevices(const ChromeHit& hit) {
  refreshAudioOutputs();
  const QString current = normalizeAudioDeviceName(settings_.audioDevice);
  QVector<ChromeMenuItem> items;
  items.reserve(audioOutputs_.size());
  for (const AudioOutputDevice& device : audioOutputs_) {
    items.push_back(ChromeMenuItem::check(
        audioDeviceDisplayLabel(device.name, audioOutputs_),
        normalizeAudioDeviceName(device.name) == current));
  }
  const int chosen =
      execAnchoredMenu(items, windowFor(WindowId::settings), hit.rect, PopupAnchor::belowLeft);
  if (chosen == kChromeMenuNone || chosen < 0 || chosen >= audioOutputs_.size()) return;
  settings_.audioDevice = normalizeAudioDeviceName(audioOutputs_[chosen].name);
  engine_->setAudioDevice(settings_.audioDevice);
  schedulePersist();
  refreshChrome();
}

void AoideSession::presentEqPresets(const ChromeHit& hit) {
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

void AoideSession::presentResetSettings() {
  resetSettingsExceptSkins(settings_);
  {
    WaitCursorScope wait;
    skins_.setSkinsDirectory({}, settings_);
    refreshSkinPreviews();
  }
  applyEq();
  engine_->setForceMono(false);
  engine_->setAudioDevice(kDefaultAudioDeviceName());
  engine_->setAudioExclusive(false);
  layout_.docking().setSnapThreshold(snapPixels(settings_.dockSnapStrength));
  applyAlwaysOnTop();
  syncTitleMarquee();
  schedulePersist();
  refreshChrome();
}

void AoideSession::presentSkinInstallMenu(const ChromeHit& hit) {
  enum Row { kZip, kFolder };
  const QVector<ChromeMenuItem> items{
      ChromeMenuItem::action(QStringLiteral("Install ZIP")),
      ChromeMenuItem::action(QStringLiteral("Install Folder")),
  };
  const int chosen =
      execAnchoredMenu(items, windowFor(WindowId::skins), hit.rect, PopupAnchor::aboveLeft);
  if (chosen == kZip) {
    presentSkinZipInstall();
    return;
  }
  if (chosen == kFolder) presentSkinFolderInstall();
}

void AoideSession::presentSkinZipInstall() {
  FilePick pick;
  pick.parent = windowFor(WindowId::skins);
  pick.title = QStringLiteral("Install skin");
  pick.filter = QStringLiteral("Skin zip (*.zip)");
  pick.kind = FilePickKind::openFile;
  const QString path = pickFile(pick);
  if (!path.isEmpty()) {
    WaitCursorScope wait;
    if (skins_.installZip(path, skinConflictPrompt())) {
      refreshSkinPreviews();
      schedulePersist();
    }
  }
  refreshChrome();
}

void AoideSession::presentSkinFolderInstall() {
  FilePick pick;
  pick.parent = windowFor(WindowId::skins);
  pick.title = QStringLiteral("Install skin folder");
  pick.kind = FilePickKind::openDirectory;
  const QString path = pickFile(pick);
  if (!path.isEmpty()) {
    WaitCursorScope wait;
    if (skins_.installDirectory(path, skinConflictPrompt())) {
      refreshSkinPreviews();
      schedulePersist();
    }
  }
  refreshChrome();
}

void AoideSession::handleHit(WindowId id, ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical) {
  dragOrigin_ = logical;
  ChromeCommandRouter router(*playback_, playlist_, settings_, collection_, *engine_,
                             layout_.docking());
  presentChromeOutcome(router.handle(id, hit, mods, logical), id, hit, logical);
}

void AoideSession::showOptionsMenu(QRect logicalHit) {
  // The rules keep the window toggle and the destructive row away from the
  // openers. Row indices count them, hence the members.
  enum Row { kAlwaysOnTop, kRuleTop, kOpenFiles, kSettings, kRuleAbout, kAbout, kQuit };
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

int AoideSession::execAnchoredMenu(const QVector<ChromeMenuItem>& items, HostWindow* host,
                                   QRect logicalHit, PopupAnchor anchor) {
  if (!host) return kChromeMenuNone;
  if (logicalHit.isEmpty()) logicalHit = QRect(0, 0, 1, 1);
  const QRect widget = host->widgetRectFromLogical(logicalHit);
  const QRect global(host->mapToGlobal(widget.topLeft()), widget.size());
  return execChromeMenu(host, items, global, anchor, layout_.zoomPercent(), skins_.tokens());
}

void AoideSession::showTrackInfo() {
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

bool AoideSession::reportPlaylistWriteFailure(bool wrote, const QString& path) {
  if (wrote) return true;
  QMessageBox::warning(
      windowFor(WindowId::main), QStringLiteral("Playlist not saved"),
      QStringLiteral("Aoide could not write %1.\n\nThe playlist still has its "
                     "unsaved changes, and the file on disk is unchanged.")
          .arg(QDir::toNativeSeparators(path)));
  return false;
}

bool AoideSession::confirmReplaceAltered(const QString& consequence) {
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

void AoideSession::quitFromMenu() {
  if (HostWindow* main = windowFor(WindowId::main)) main->close();
}

QString AoideSession::bundledSkinsDir() const { return aoide::bundledSkinsDir(); }

void AoideSession::refreshSkinPreviews() {
  skins_.ensurePreviews([](const QString& id, const QString& path,
                           const QVector<LookManifest>& installed, QString* error) {
    return writeSkinPreviewPng(id, installed, path, error);
  });
}

void AoideSession::presentSkinRemove(int index) {
  const auto catalog = skins_.catalog();
  if (index < 0 || index >= catalog.size()) return;
  const SkinCatalogEntry& entry = catalog[index];
  if (!entry.canRemove) return;
  QWidget* parent = dialogParent(WindowId::skins);
  QString text = QStringLiteral("Remove “%1” from this folder? The skin’s files will be deleted.")
                     .arg(entry.name);
  if (isBundledHomageId(entry.id)) {
    text += QStringLiteral("\n\nReset Settings will install it again.");
  }
  QMessageBox box(parent);
  box.setWindowTitle(QStringLiteral("Remove skin?"));
  box.setText(text);
  box.setIcon(QMessageBox::Question);
  QPushButton* cancel = box.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
  QPushButton* remove = box.addButton(QStringLiteral("Remove"), QMessageBox::DestructiveRole);
  box.setDefaultButton(cancel);
  WaitCursorPause pause;
  box.exec();
  if (box.clickedButton() != remove) return;
  WaitCursorScope wait;
  skins_.remove(entry.id, settings_);
  refreshChrome();
}

SkinController::ConflictFn AoideSession::skinConflictPrompt() {
  return [this](const SkinConflict& conflict) {
    QWidget* parent = dialogParent(WindowId::skins);
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

}  // namespace aoide
