#include "session.h"

#include "chrome_layout.h"
#include "files.h"
#include "host_shell.h"
#include "host_shell_window.h"
#include "host_window.h"
#include "look.h"
#include "m3u.h"
#ifdef TRAMP_HAVE_MPV
#include "mpv_engine.h"
#include "pcm_decoder.h"
#endif
#include "player_engine.h"
#include "popup_anchor.h"
#include "support_dir.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QUrl>
#include <QWidget>
#include <QVector>
#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <memory>
#include <thread>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace tramp {

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
    engine_ = std::make_unique<NullEngine>();
  }
#else
  engine_ = std::make_unique<NullEngine>();
#endif
  playback_ = std::make_unique<PlaybackController>(&playlist_, engine_.get());
  playback_->setSpins(store_.readUsage().spins);
#ifdef TRAMP_HAVE_MPV
  analyzer_ = SpectrumAnalyzer([](const QString& path) { return MpvPcmDecoder().decode(path); });
#endif
  spectrumTimer_.setInterval(33);
  spectrumTimer_.setTimerType(Qt::CoarseTimer);
  QObject::connect(&spectrumTimer_, &QTimer::timeout, this, [this]() { tickSpectrum(); });
  DockLayout layout;
  layout.main = settings_.main;
  layout.equalizer = settings_.equalizer;
  layout.playlist = settings_.playlist;
  layout.settings = settings_.settings;
  layout.about = settings_.about;
  layout.dockEdges = settings_.dockEdges;
  docking_ = DockingCoordinator(layout);
  docking_.setSnapThreshold(snapPixels(settings_.dockSnapStrength));

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
    if (about_ && about_->isVisible()) refreshAboutFigures();
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

  playlist_.setOnChanged([this]() {
    if (playback_) playback_->onPlaylistChanged();
    scheduleAltered();
    refreshChrome();
  });
  bindPlayback();
  engine_->setForceMono(settings_.forceMono);
  applyEq();
  skins_.bootstrap(trampSupportDirectory(), bundledSkinsDir(), settings_);
}

TrampSession::~TrampSession() {
  ++spectrumGen_;
  spectrumTimer_.stop();
  persistNow();
}

void TrampSession::detachWindows() {
  persistNow();
  main_ = nullptr;
  eq_ = nullptr;
  pl_ = nullptr;
  settingsWin_ = nullptr;
  about_ = nullptr;
  shell_ = nullptr;
}

void TrampSession::bindPlayback() {
  playback_->setOnChanged([this]() {
    syncSpectrum();
    refreshChrome();
  });
  playback_->setOnPosition([this]() {
    if (!spectrumTimer_.isActive()) emit mainChromeChanged();
  });
  playback_->setOnSpin([this](int) { scheduleUsage(); refreshChrome(); });
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
  if (spectrumTimer_.isActive()) {
    spectrumTimer_.stop();
    spectrumHold_.apply(AudioLevels::silent());
  }
}

void TrampSession::tickSpectrum() {
  playback_->pollClock();
  const AudioLevels frame =
      spectrumFrame(spectrogram_, playback_->playing() && spectrumReady_, playback_->positionMs());
  spectrumHold_.apply(frame);
  emit mainChromeChanged();
}

void TrampSession::startSpectrumDecode(const QString& path, int gen) {
  const SpectrumAnalyzer analyzer = analyzer_;
  const QPointer<TrampSession> session(this);
  std::thread([path, gen, analyzer, session]() {
#ifdef Q_OS_UNIX
    nice(19);
#endif
    const Spectrogram spec = analyzer.load(path);
    TrampSession* host = session.data();
    if (!host) return;
    QMetaObject::invokeMethod(
        host,
        [session, spec, gen]() {
          if (!session || gen != session->spectrumGen_) return;
          session->spectrogram_ = spec;
          session->spectrumReady_ = true;
          session->tickSpectrum();
        },
        Qt::QueuedConnection);
  }).detach();
}

void TrampSession::setWindows(HostWindow* main, HostWindow* eq, HostWindow* pl,
                             HostWindow* settings, HostWindow* about) {
  main_ = main;
  eq_ = eq;
  pl_ = pl;
  settingsWin_ = settings;
  about_ = about;
}

void TrampSession::setShell(HostShell* shell) { shell_ = shell; }

void TrampSession::bootstrap(const QStringList& argvFiles) {
  const auto kept = store_.readAltered();
  if (!kept.isEmpty()) {
    playlist_.restoreAlteredTracks(kept.tracks, kept.sourcePath);
  } else {
    const QString last = store_.readLastPlaylistPath();
    if (!last.isEmpty() && QFileInfo::exists(last)) {
      playlist_.openPlaylistFile(last);
    }
  }
  if (!argvFiles.isEmpty()) {
    openPaths(argvFiles, !playlist_.tracks().isEmpty());
  }
  if (settings_.resumeLastSession) {
    const auto resume = store_.readResume();
    if (resume.playingIndex && *resume.playingIndex >= 0 &&
        *resume.playingIndex < playlist_.tracks().size()) {
      playback_->playIndex(*resume.playingIndex);
      if (resume.positionMs > 0) playback_->seekMs(resume.positionMs);
      if (!resume.wasPlaying) playback_->playPause();
    }
  }
  collection_.validateReferences();
  collection_.saveIndex(store_);
  collection_.saveTrackSets(store_);
  if (!playlist_.sourcePath().isEmpty()) {
    collection_.select(playlist_.sourcePath());
  }
  if (pl_ && settings_.playlist.width && settings_.playlist.height) {
    pl_->setPlaylistLogicalSize(
        QSize(int(*settings_.playlist.width), int(*settings_.playlist.height)));
  }
  docking_.nudgeOffMainIfStacked(WindowId::equalizer);
  docking_.nudgeOffMainIfStacked(WindowId::playlist);
  applyFramesToWindows();
  if (settings_.about.visible) refreshAboutFigures();
  applyAlwaysOnTop();
  refreshChrome();
}

void TrampSession::applyEq() { engine_->setEqualizerAf(buildEqualizerAf(settings_.equalizerCurve)); }

void TrampSession::scheduleApplyEq() {
  if (!eqApplyTimer_.isActive()) eqApplyTimer_.start();
}

void TrampSession::refreshEqChrome() {
  if (eq_) eq_->applyEqualizer(settings_.equalizerCurve);
}

bool TrampSession::confirmQuit() const { return settings_.confirmBeforeQuit; }

bool TrampSession::windowShouldShow(WindowId id) const {
  return docking_.layout().frameOf(id).visible;
}

void TrampSession::applyAlwaysOnTop() {
  if (shell_) shell_->setAlwaysOnTop(settings_.alwaysOnTop);
}

void TrampSession::applyFramesToWindows() {
  applyDockToWindows();
}

void TrampSession::refreshAboutFigures() {
  figures_ = collection_.readFigures();
  figuresLoaded_ = true;
  refreshChrome();
}

void TrampSession::persistNow() {
  if (qEnvironmentVariable("TRAMP_AUTO_QUIT") == QLatin1String("1")) return;
  if (!main_) return;
  if (!applyingDock_) syncLayoutFromWindows();
  auto capture = [&](HostWindow* w, WindowFrame& frame) {
    if (!w) return;
    frame.visible = docking_.layout().frameOf(w->id()).visible;
    frame.shaded = w->shaded();
    if (w->id() == WindowId::playlist) {
      const qreal z = settings_.zoomPercent / 100.0;
      frame.width = w->width() / z;
      frame.height = w->height() / z;
    }
  };
  settings_.main = docking_.layout().main;
  settings_.equalizer = docking_.layout().equalizer;
  settings_.playlist = docking_.layout().playlist;
  settings_.settings = docking_.layout().settings;
  settings_.about = docking_.layout().about;
  capture(main_, settings_.main);
  capture(eq_, settings_.equalizer);
  capture(pl_, settings_.playlist);
  capture(settingsWin_, settings_.settings);
  capture(about_, settings_.about);
  docking_.layout().main = settings_.main;
  docking_.layout().equalizer = settings_.equalizer;
  docking_.layout().playlist = settings_.playlist;
  docking_.layout().settings = settings_.settings;
  docking_.layout().about = settings_.about;
  settings_.dockEdges = docking_.layout().dockEdges;
  settings_.equalizerCurve = settings_.equalizerCurve;
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
  if (playlist_.altered()) {
    store_.writeAltered({playlist_.tracks(), playlist_.sourcePath()});
  }
}

void TrampSession::schedulePersist() { persistTimer_.start(); }
void TrampSession::scheduleAltered() { alteredTimer_.start(); }
void TrampSession::scheduleUsage() { usageTimer_.start(); }

HostWindow* TrampSession::windowFor(WindowId id) const {
  switch (id) {
    case WindowId::main:
      return main_;
    case WindowId::equalizer:
      return eq_;
    case WindowId::playlist:
      return pl_;
    case WindowId::settings:
      return settingsWin_;
    case WindowId::about:
      return about_;
  }
  return main_;
}

QPointF TrampSession::nativeToLogical(QPoint native) const {
  const qreal z = settings_.zoomPercent / 100.0;
  return QPointF(native.x() / z, native.y() / z);
}

QPoint TrampSession::logicalToNative(QPointF logical) const {
  const qreal z = settings_.zoomPercent / 100.0;
  return QPoint(int(std::lround(logical.x() * z)), int(std::lround(logical.y() * z)));
}

SessionView TrampSession::view() const {
  SessionView v;
  v.eqOn = docking_.layout().equalizer.visible;
  v.plOn = docking_.layout().playlist.visible;
  v.showElapsed = showElapsed_;
  v.positionMs = playback_->positionMs();
  v.durationMs = playback_->durationMs();
  v.volume = playback_->volume();
  v.muted = playback_->muted();
  v.forceMono = settings_.forceMono;
  v.playing = playback_->playing();
  v.paused = playback_->paused();
  v.shuffle = playback_->shuffle();
  v.repeat = playback_->repeatMode();
  v.zoomPercent = settings_.zoomPercent;
  v.spectrum = spectrumHold_.bars;
  v.spectrumPeaks = spectrumHold_.peaks;
  v.eq = settings_.equalizerCurve;
  v.playingIndex = playback_->playingIndex();
  v.selectedIndices = playlist_.selectedIndices();
  v.trackScroll = trackScroll_;
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
  for (int i = 0; i < tracks.size(); ++i) {
    TrackRowView row;
    row.artist = tracks[i].artist;
    row.title = tracks[i].displayTitle();
    row.time = tracks[i].durationMs ? formatClock(*tracks[i].durationMs) : QStringLiteral("--:--");
    row.selected = playlist_.selectedIndices().contains(i);
    row.playing = playback_->playingIndex() == i;
    v.tracks.push_back(row);
    if (tracks[i].durationMs) total += *tracks[i].durationMs;
  }
  v.playlistTotalMs = total;
  if (!playlist_.sourcePath().isEmpty()) {
    v.playlistName = QFileInfo(playlist_.sourcePath()).fileName();
  }

  const auto current = playback_->playingIndex();
  if (current && *current >= 0 && *current < tracks.size()) {
    const Track& t = tracks[*current];
    QStringList parts;
    parts << QString::number(*current + 1) + QLatin1Char('.');
    if (!t.artist.trimmed().isEmpty()) parts << t.artist.trimmed() + QStringLiteral(" —");
    parts << t.displayTitle();
    v.title = parts.join(QLatin1Char(' '));
    QStringList sub;
    if (!t.album.trimmed().isEmpty()) sub << t.album.trimmed();
    sub << QStringLiteral("track %1 of %2").arg(*current + 1).arg(tracks.size());
    v.subtitle = sub.join(QStringLiteral(" · ")).toUpper();
    const QString ext = QFileInfo(t.path).suffix().toUpper();
    v.formatChip = ext.isEmpty() ? QStringLiteral("—") : ext;
  } else {
    v.title = QStringLiteral("No track");
  }

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
  live.showElapsed = showElapsed_;
  live.spectrum = spectrumHold_.bars;
  live.spectrumPeaks = spectrumHold_.peaks;
  return live;
}

void TrampSession::refreshChrome() { emit chromeChanged(); }

void TrampSession::setZoomPercent(int percent) {
  settings_.zoomPercent = percent;
  schedulePersist();
  emit zoomChanged(percent);
  applyFramesToWindows();
}

void TrampSession::setWindowVisible(WindowId id, bool visible) {
  docking_.setVisible(id, visible);
  if (visible) {
    docking_.nudgeOffMainIfStacked(id);
    emit requestShow(id);
    applyFramesToWindows();
    if (id == WindowId::settings) emit requestRaise(WindowId::settings);
    if (id == WindowId::about) refreshAboutFigures();
  } else {
    emit requestHide(id);
    applyFramesToWindows();
  }
  applyAlwaysOnTop();
  schedulePersist();
  refreshChrome();
}

void TrampSession::setShaded(WindowId id, bool shaded) {
  docking_.setShaded(id, shaded);
  schedulePersist();
  applyFramesToWindows();
}

void TrampSession::extraClosed(WindowId id) {
  docking_.setVisible(id, false);
  applyAlwaysOnTop();
  applyFramesToWindows();
  schedulePersist();
  refreshChrome();
}

void TrampSession::mainMinimized(bool minimized) {
  if (!settings_.minimizeHidesSecondaries) return;
  if (minimized) {
    hiddenByMinimize_.clear();
    for (WindowId id : {WindowId::equalizer, WindowId::playlist, WindowId::settings,
                        WindowId::about}) {
      HostWindow* w = windowFor(id);
      if (w && w->isVisible()) {
        hiddenByMinimize_.insert(id);
        w->hide();
      }
    }
    applyFramesToWindows();
  } else {
    for (WindowId id : hiddenByMinimize_) {
      HostWindow* w = windowFor(id);
      if (w && docking_.layout().frameOf(id).visible) w->show();
    }
    hiddenByMinimize_.clear();
    applyAlwaysOnTop();
    applyFramesToWindows();
    if (settingsWin_ && settingsWin_->isVisible()) settingsWin_->raise();
  }
}

void TrampSession::mainActivated() {
  if (settingsWin_ && settingsWin_->isVisible()) settingsWin_->raise();
}

void TrampSession::playTrackAt(int index) { playback_->playIndex(index); }
void TrampSession::selectAllTracks() { playlist_.selectAll(); }
void TrampSession::removeSelectedTracks() { playlist_.removeSelected(); }

void TrampSession::windowMoved(WindowId id, QPoint nativeTopLeft, bool finalize) {
  if (applyingDock_) return;
  docking_.move(id, nativeToLogical(nativeTopLeft), false, finalize && id != WindowId::main);
  applyFramesToWindows();
  schedulePersist();
}

void TrampSession::titleDragBegan(WindowId id) {
  Q_UNUSED(id);
  syncLayoutFromWindows();
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
  applyFramesToWindows();
}

void TrampSession::syncLayoutFromWindows(std::optional<WindowId> skip) {
  for (WindowId id : {WindowId::main, WindowId::equalizer, WindowId::playlist, WindowId::settings,
                      WindowId::about}) {
    if (skip && id == *skip) continue;
    HostWindow* w = windowFor(id);
    if (!w || !w->isVisible()) continue;
    const QPointF logical = nativeToLogical(w->nativeTopLeft());
    WindowFrame& frame = docking_.layout().frameOf(id);
    frame.left = logical.x();
    frame.top = logical.y();
  }
}

void TrampSession::reapplyWindowFrames() {
  applyFramesToWindows();
}

void TrampSession::applyDockToWindows(std::optional<WindowId> skip) {
  if (applyingDock_) return;
  applyingDock_ = true;

  QVector<QRect> screenRects;
  struct Placement {
    HostWindow* widget = nullptr;
    QRect screen;
  };
  QVector<Placement> visible;

  for (WindowId id : {WindowId::main, WindowId::equalizer, WindowId::playlist, WindowId::settings,
                      WindowId::about}) {
    if (skip && id == *skip) continue;
    HostWindow* w = windowFor(id);
    if (!w) continue;
    const WindowFrame& f = docking_.layout().frameOf(id);
    if (id != WindowId::main) w->setShaded(f.shaded);
    if (id == WindowId::playlist && f.width && f.height) {
      w->setPlaylistLogicalSize(QSize(int(*f.width), int(*f.height)));
    }
    const bool show = f.visible && !hiddenByMinimize_.contains(id);
    if (!show) {
      w->hide();
      continue;
    }
    const QPoint native = logicalToNative(QPointF(f.left, f.top));
    const QRect screen(native, w->size());
    screenRects.push_back(screen);
    visible.push_back({w, screen});
  }

  const tramp::HostShellLayout layout = tramp::hostShellLayout(screenRects);
  if (shell_) shell_->applyLayout(layout);
  if (layout.screenRect.isNull()) {
    applyingDock_ = false;
    return;
  }

  const QPoint origin = layout.screenRect.topLeft();
  for (const Placement& place : visible) {
    place.widget->setGeometry(place.screen.translated(-origin));
    place.widget->show();
  }
  if (shell_) shell_->refreshMaskFromChildren();
  applyingDock_ = false;
}

void TrampSession::playlistResized(QSize native) {
  if (applyingDock_) return;
  const qreal z = settings_.zoomPercent / 100.0;
  docking_.resizePlaylist(QSizeF(native.width() / z, native.height() / z));
  settings_.playlist.width = native.width() / z;
  settings_.playlist.height = native.height() / z;
  applyFramesToWindows();
  schedulePersist();
  refreshChrome();
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
  if (!playlists.isEmpty()) {
    playlist_.openPlaylistFile(playlists.first());
    collection_.add(playlists.first());
    collection_.saveIndex(store_);
    if (playlist_.tracks().isEmpty() == false) playback_->playIndex(0);
  }
  const auto audio = tracksFromPaths(others);
  if (!audio.isEmpty()) {
    if (!enqueue && playlists.isEmpty()) playlist_.setTracks(audio);
    else playlist_.addTracks(audio);
    if (!playback_->playingIndex()) playback_->playIndex(0);
  }
  refreshChrome();
}

QString TrampSession::pickAudio(bool multiple) {
  const QString filter = QStringLiteral("Audio (*.mp3 *.m4a *.aac *.flac *.wav *.ogg *.opus)");
  if (multiple) {
    const QStringList files = QFileDialog::getOpenFileNames(main_, QStringLiteral("Add audio files"),
                                                            QString(), filter);
    return files.join(QLatin1Char('\n'));
  }
  return QFileDialog::getOpenFileName(main_, QStringLiteral("Open audio"), QString(), filter);
}

QString TrampSession::pickPlaylist(bool save) {
  const QString filter = QStringLiteral("Playlists (*.m3u *.m3u8)");
  if (save) {
    return QFileDialog::getSaveFileName(main_, QStringLiteral("Save playlist"),
                                        QStringLiteral("playlist.m3u"), filter);
  }
  return QFileDialog::getOpenFileName(main_, QStringLiteral("Open playlist"), QString(), filter);
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
    refreshChrome();
    return;
  }
  if (!confirmReplaceAltered()) return;
  playlist_.openPlaylistFile(e.path);
  collection_.select(e.path);
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
  trackScroll_ = std::max(0, std::min(trackScroll_ + step, std::max(0, int(playlist_.tracks().size()) - 1)));
  refreshChrome();
}

void TrampSession::handleDrag(WindowId id, ChromeHit hit, QPoint logical) {
  auto frac = [](const QRect& r, int x) {
    if (r.width() <= 1) return 0.0;
    return std::clamp((x - r.left()) / double(r.width()), 0.0, 1.0);
  };
  auto band = [](const QRect& r, int y) {
    if (r.height() <= 1) return 0.0;
    const double t = std::clamp((y - r.top()) / double(r.height()), 0.0, 1.0);
    return EqualizerSettings::kGainLimit - t * (EqualizerSettings::kGainLimit * 2);
  };
  if (sliderKind_ == ChromeHit::Kind::volume || hit.kind == ChromeHit::Kind::volume) {
    playback_->setVolume(frac(hit.rect, logical.x()));
    sliderKind_ = ChromeHit::Kind::volume;
  } else if (sliderKind_ == ChromeHit::Kind::seek || hit.kind == ChromeHit::Kind::seek) {
    if (playback_->durationMs() > 0) {
      playback_->seekMs(qint64(frac(hit.rect, logical.x()) * playback_->durationMs()));
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

void TrampSession::handleHit(WindowId id, ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical) {
  using K = ChromeHit::Kind;
  dragOrigin_ = logical;
  switch (hit.kind) {
    case K::options:
      showOptionsMenu(hit.rect);
      break;
    case K::timeToggle:
      showElapsed_ = !showElapsed_;
      refreshChrome();
      break;
    case K::mute:
      playback_->toggleMute();
      break;
    case K::volume:
    case K::seek:
    case K::eqPreamp:
    case K::eqBand:
      sliderKind_ = hit.kind;
      sliderIndex_ = hit.index;
      handleDrag(id, hit, hit.rect.center());
      break;
    case K::mono:
      settings_.forceMono = !settings_.forceMono;
      engine_->setForceMono(settings_.forceMono);
      schedulePersist();
      refreshChrome();
      break;
    case K::eqToggle:
      setWindowVisible(WindowId::equalizer, !(eq_ && eq_->isVisible()));
      break;
    case K::plToggle:
      setWindowVisible(WindowId::playlist, !(pl_ && pl_->isVisible()));
      break;
    case K::prev:
    case K::plPrev:
      playback_->previous();
      break;
    case K::play:
      if (!playback_->playing()) playback_->playPause();
      break;
    case K::pause:
      if (playback_->playing()) playback_->playPause();
      break;
    case K::plPlay:
      playback_->playPause();
      break;
    case K::stop:
      playback_->stop();
      break;
    case K::next:
    case K::plNext:
      playback_->next();
      break;
    case K::eject:
    case K::plAdd: {
      const QString picked = pickAudio(true);
      if (!picked.isEmpty()) openPaths(picked.split(QLatin1Char('\n')), true);
      break;
    }
    case K::shuffle:
      playback_->toggleShuffle();
      break;
    case K::repeat:
      playback_->cycleRepeatMode();
      break;
    case K::eqOn:
      settings_.equalizerCurve.enabled = !settings_.equalizerCurve.enabled;
      applyEq();
      schedulePersist();
      refreshEqChrome();
      break;
    case K::eqAuto:
      settings_.equalizerCurve.auto_ = !settings_.equalizerCurve.auto_;
      schedulePersist();
      refreshEqChrome();
      break;
    case K::eqPresets: {
      QMenu menu(eq_ ? static_cast<QWidget*>(eq_) : static_cast<QWidget*>(main_));
      for (const auto& preset : EqualizerPresets::builtIn()) {
        QAction* a = menu.addAction(preset.first);
        QObject::connect(a, &QAction::triggered, this, [this, preset]() {
          settings_.equalizerCurve =
              settings_.equalizerCurve.withPreset(preset.first, preset.second);
          settings_.equalizerCurve.enabled = true;
          applyEq();
          schedulePersist();
          refreshEqChrome();
        });
      }
      execAnchoredMenu(menu, eq_, hit.rect, false);
      break;
    }
    case K::plCollapse:
      settings_.playlistCollectionCollapsed = !settings_.playlistCollectionCollapsed;
      schedulePersist();
      refreshChrome();
      break;
    case K::plCollectionRow: {
      const auto entries = collection_.entries();
      if (hit.index < 0 || hit.index >= entries.size()) break;
      if (mods & Qt::ControlModifier) {
        collection_.select(entries[hit.index].path);
        refreshChrome();
      } else {
        loadCollectionRow(hit.index);
      }
      break;
    }
    case K::plAddCollection: {
      const QString path = pickPlaylist(false);
      if (!path.isEmpty()) {
        collection_.add(path);
        collection_.saveIndex(store_);
        refreshChrome();
      }
      break;
    }
    case K::plCreate: {
      QMenu menu(pl_ ? static_cast<QWidget*>(pl_) : static_cast<QWidget*>(main_));
      QAction* fromCurrent = menu.addAction(QStringLiteral("From current playlist"));
      fromCurrent->setEnabled(!playlist_.tracks().isEmpty());
      QAction* fromSel = menu.addAction(QStringLiteral("From selection"));
      fromSel->setEnabled(!playlist_.selectedIndices().isEmpty());
      QAction* chosen = execAnchoredMenu(menu, pl_, hit.rect, true);
      if (chosen == fromCurrent) {
        const QString path = pickPlaylist(true);
        if (!path.isEmpty()) {
          playlist_.savePlaylistFile(path);
          collection_.addWritten(path, playlist_.tracks());
          collection_.saveIndex(store_);
          collection_.saveTrackSets(store_);
        }
      } else if (chosen == fromSel) {
        const QString path = pickPlaylist(true);
        if (!path.isEmpty()) {
          QVector<Track> selected;
          QList<int> idx = playlist_.selectedIndices().values();
          std::sort(idx.begin(), idx.end());
          for (int i : idx) selected.push_back(playlist_.tracks()[i]);
          QFile f(path);
          if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(M3uCodec().encode(selected).toUtf8());
          }
          collection_.addWritten(path, selected);
          collection_.saveIndex(store_);
          collection_.saveTrackSets(store_);
        }
      }
      refreshChrome();
      break;
    }
    case K::plRename: {
      if (collection_.selectedPath().isEmpty()) break;
      QString current;
      for (const SavedPlaylist& e : collection_.entries()) {
        if (e.path == collection_.selectedPath()) {
          current = e.displayName();
          break;
        }
      }
      bool ok = false;
      const QString name = QInputDialog::getText(
          pl_ ? static_cast<QWidget*>(pl_) : static_cast<QWidget*>(main_),
          QStringLiteral("Rename playlist"), QStringLiteral("Name"), QLineEdit::Normal, current,
          &ok);
      if (!ok) break;
      collection_.rename(collection_.selectedPath(), name);
      collection_.saveIndex(store_);
      refreshChrome();
      break;
    }
    case K::plRemoveCollection:
      if (!collection_.selectedPath().isEmpty()) {
        collection_.remove(collection_.selectedPath());
        collection_.saveIndex(store_);
        refreshChrome();
      }
      break;
    case K::plTrackRow: {
      sliderKind_ = K::plTrackRow;
      sliderIndex_ = hit.index;
      if (mods & Qt::ShiftModifier) playlist_.selectRange(hit.index);
      else if (mods & Qt::ControlModifier) playlist_.toggleSelection(hit.index);
      else playlist_.select(hit.index);
      break;
    }
    case K::plRemove:
      playlist_.removeSelected();
      break;
    case K::plSort: {
      QMenu menu(pl_ ? static_cast<QWidget*>(pl_) : static_cast<QWidget*>(main_));
      auto add = [&](const QString& name, PlaylistSortKey key) {
        QAction* a = menu.addAction(name);
        QObject::connect(a, &QAction::triggered, this, [this, key]() { playlist_.sortBy(key); });
      };
      add(QStringLiteral("Title"), PlaylistSortKey::title);
      add(QStringLiteral("Artist"), PlaylistSortKey::artist);
      add(QStringLiteral("Duration"), PlaylistSortKey::duration);
      add(QStringLiteral("Path"), PlaylistSortKey::path);
      QAction* rev = menu.addAction(QStringLiteral("Reverse"));
      QObject::connect(rev, &QAction::triggered, this, [this]() { playlist_.reverseTracks(); });
      if (pl_) execAnchoredMenu(menu, pl_, hit.rect, true);
      break;
    }
    case K::plOptions: {
      QMenu menu(pl_ ? static_cast<QWidget*>(pl_) : static_cast<QWidget*>(main_));
      menu.addAction(QStringLiteral("Select all"), this, [this]() { playlist_.selectAll(); });
      menu.addAction(QStringLiteral("Invert selection"), this, [this]() { playlist_.invertSelection(); });
      menu.addAction(QStringLiteral("Save playlist…"), this, [this]() {
        const QString path = pickPlaylist(true);
        if (!path.isEmpty()) playlist_.savePlaylistFile(path);
      });
      menu.addAction(QStringLiteral("Clear"), this, [this]() { playlist_.clear(); });
      if (pl_) execAnchoredMenu(menu, pl_, hit.rect, true);
      break;
    }
    case K::settingsGeneral:
      settingsTab_ = 0;
      refreshChrome();
      break;
    case K::settingsSkins:
      settingsTab_ = 1;
      skins_.rescan();
      refreshChrome();
      break;
    case K::settingsSkinRow: {
      const auto catalog = skins_.catalog();
      if (hit.index >= 0 && hit.index < catalog.size()) {
        if (skins_.activate(catalog[hit.index].id, settings_)) {
          schedulePersist();
        }
        refreshChrome();
      }
      break;
    }
    case K::settingsInstallZip: {
      const QString path = QFileDialog::getOpenFileName(
          settingsWin_, QStringLiteral("Install skin"), QString(),
          QStringLiteral("Skin zip (*.zip)"));
      if (!path.isEmpty() && skins_.installZip(path, skinConflictPrompt())) {
        schedulePersist();
      }
      refreshChrome();
      break;
    }
    case K::settingsInstallFolder: {
      const QString path =
          QFileDialog::getExistingDirectory(settingsWin_, QStringLiteral("Install skin folder"));
      if (!path.isEmpty() && skins_.installDirectory(path, skinConflictPrompt())) {
        schedulePersist();
      }
      refreshChrome();
      break;
    }
    case K::settingsSkinsFolder: {
      const QString path =
          QFileDialog::getExistingDirectory(settingsWin_, QStringLiteral("Skins folder"));
      if (!path.isEmpty()) {
        skins_.setSkinsDirectory(path, settings_);
        schedulePersist();
      }
      refreshChrome();
      break;
    }
    case K::settingsResetSkinsFolder:
      skins_.setSkinsDirectory({}, settings_);
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsResume:
      settings_.resumeLastSession = !settings_.resumeLastSession;
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsConfirm:
      settings_.confirmBeforeQuit = !settings_.confirmBeforeQuit;
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsScroll:
      settings_.scrollTitle = !settings_.scrollTitle;
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsMinimize:
      settings_.minimizeHidesSecondaries = !settings_.minimizeHidesSecondaries;
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsSnapOff:
      settings_.dockSnapStrength = DockSnapStrength::off;
      docking_.setSnapThreshold(0);
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsSnapNormal:
      settings_.dockSnapStrength = DockSnapStrength::normal;
      docking_.setSnapThreshold(20);
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsSnapStrong:
      settings_.dockSnapStrength = DockSnapStrength::strong;
      docking_.setSnapThreshold(40);
      schedulePersist();
      refreshChrome();
      break;
    case K::settingsReset:
      settings_ = TrampSettings{};
      applyEq();
      engine_->setForceMono(false);
      docking_.setSnapThreshold(20);
      applyAlwaysOnTop();
      skins_.setSkinsDirectory({}, settings_);
      schedulePersist();
      refreshChrome();
      break;
    case K::aboutWeb:
      QDesktopServices::openUrl(QUrl(QStringLiteral("https://tramp.music")));
      break;
    case K::plResize:
    case K::plDivider:
    case K::none:
      break;
  }
}

void TrampSession::showOptionsMenu(QRect logicalHit) {
  QMenu menu(main_);
  QAction* aot = menu.addAction(settings_.alwaysOnTop ? QStringLiteral("Always on top ✓")
                                                      : QStringLiteral("Always on top"));
  QObject::connect(aot, &QAction::triggered, this, [this]() {
    settings_.alwaysOnTop = !settings_.alwaysOnTop;
    applyAlwaysOnTop();
    schedulePersist();
  });
  menu.addAction(QStringLiteral("Settings…"), this, [this]() {
    setWindowVisible(WindowId::settings, !windowShouldShow(WindowId::settings));
  });
  menu.addAction(QStringLiteral("Track info"), this, [this]() { showTrackInfo(); });
  menu.addAction(QStringLiteral("About Tramp"), this, [this]() {
    if (windowShouldShow(WindowId::about)) emit requestRaise(WindowId::about);
    else setWindowVisible(WindowId::about, true);
  });
  menu.addAction(QStringLiteral("Quit"), this, [this]() { quitFromMenu(); });
  if (logicalHit.isEmpty()) logicalHit = mainOptionsHit(kMainPlayer);
  execAnchoredMenu(menu, main_, logicalHit, false);
}

QAction* TrampSession::execAnchoredMenu(QMenu& menu, HostWindow* host, QRect logicalHit, bool above) {
  if (!host) return nullptr;
  if (logicalHit.isEmpty()) logicalHit = QRect(0, 0, 1, 1);
  menu.adjustSize();
  const QRect widget = host->widgetRectFromLogical(logicalHit);
  const QRect global(host->mapToGlobal(widget.topLeft()), widget.size());
  return menu.exec(popupMenuPos(global, menu.sizeHint(),
                                above ? PopupAnchor::aboveLeft : PopupAnchor::belowLeft));
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
  QMessageBox::information(main_, QStringLiteral("Track info"), message);
}

bool TrampSession::confirmReplaceAltered() {
  if (!playlist_.altered()) return true;
  QMessageBox box(main_);
  box.setWindowTitle(QStringLiteral("Save the current playlist?"));
  box.setText(QStringLiteral(
      "The current playlist has changes that are not in any file. "
      "Loading another playlist replaces it."));
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
    playlist_.savePlaylistFile(path);
  }
  Q_UNUSED(discard);
  return true;
}

void TrampSession::quitFromMenu() {
  if (main_) main_->close();
}

QString TrampSession::bundledSkinsDir() const { return tramp::bundledSkinsDir(); }

SkinController::ConflictFn TrampSession::skinConflictPrompt() {
  return [this](const SkinConflict& conflict) {
    QWidget* parent = settingsWin_ ? static_cast<QWidget*>(settingsWin_) : main_;
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
    const auto reply = QMessageBox::question(parent, QStringLiteral("Replace skin?"), text,
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return reply == QMessageBox::Yes ? SkinConflictChoice::replace : SkinConflictChoice::cancel;
  };
}

}  // namespace tramp
