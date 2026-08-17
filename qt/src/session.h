#pragma once

#include "chrome_hits.h"
#include "collection.h"
#include "docking.h"
#include "look.h"
#include "persist.h"
#include "playback.h"
#include "player_engine.h"
#include "playlist.h"
#include "session_view.h"
#include "settings.h"
#include "spectrum.h"

#include <QObject>
#include <QPoint>
#include <QSet>
#include <QTimer>
#include <memory>

class HostWindow;

namespace tramp {

class TrampSession : public QObject {
  Q_OBJECT

 public:
  explicit TrampSession(QObject* parent = nullptr);
  ~TrampSession() override;

  void setWindows(HostWindow* main, HostWindow* eq, HostWindow* pl, HostWindow* settings,
                  HostWindow* about);
  void bootstrap(const QStringList& argvFiles);
  SessionView view() const;
  MainLiveReadouts mainLive() const;
  int zoomPercent() const { return settings_.zoomPercent; }
  bool confirmQuit() const;
  bool windowShouldShow(WindowId id) const;
  void persistNow();
  void detachWindows();
  void applyDroppedPaths(const QStringList& paths, bool replace);
  void extraClosed(WindowId id);
  void mainMinimized(bool minimized);
  void mainActivated();
  void playTrackAt(int index);
  void selectAllTracks();
  void removeSelectedTracks();

 public slots:
  void handleHit(WindowId id, ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical);
  void handleDrag(WindowId id, ChromeHit hit, QPoint logical);
  void handleRelease(WindowId id);
  void handleWheel(WindowId id, int delta);
  void setZoomPercent(int percent);
  void setWindowVisible(WindowId id, bool visible);
  void setShaded(WindowId id, bool shaded);
  void windowMoved(WindowId id, QPoint nativeTopLeft, bool finalize);
  void playlistResized(QSize native);
  void refreshChrome();

 signals:
  void chromeChanged();
  void mainChromeChanged();
  void zoomChanged(int percent);
  void requestShow(WindowId id);
  void requestHide(WindowId id);
  void requestRaise(WindowId id);

 private:
  void bindPlayback();
  void applyEq();
  void scheduleApplyEq();
  void refreshEqChrome();
  void applyAlwaysOnTop();
  void applyFramesToWindows();
  void schedulePersist();
  QString bundledSkinsDir() const;
  SkinController::ConflictFn skinConflictPrompt();
  void scheduleAltered();
  void scheduleUsage();
  void refreshAboutFigures();
  HostWindow* windowFor(WindowId id) const;
  QString pickAudio(bool multiple);
  QString pickPlaylist(bool save);
  void openPaths(const QStringList& paths, bool enqueue);
  void loadCollectionRow(int index);
  void applyDockToWindows();
  QPointF nativeToLogical(QPoint native) const;
  QPoint logicalToNative(QPointF logical) const;
  void showOptionsMenu();
  void showTrackInfo();
  bool confirmReplaceAltered();
  void quitFromMenu();
  void syncSpectrum();
  void tickSpectrum();
  void startSpectrumDecode(const QString& path, int gen);

  SupportStore store_;
  TrampSettings settings_;
  PlaylistController playlist_;
  PlaylistCollection collection_;
  std::unique_ptr<PlayerEngine> engine_;
  std::unique_ptr<PlaybackController> playback_;
  SpectrumAnalyzer analyzer_;
  Spectrogram spectrogram_;
  SpectrumHold spectrumHold_;
  QTimer spectrumTimer_;
  QString spectrumPath_;
  int spectrumGen_ = 0;
  bool spectrumReady_ = false;
  DockingCoordinator docking_;
  SkinController skins_;
  HostWindow* main_ = nullptr;
  HostWindow* eq_ = nullptr;
  HostWindow* pl_ = nullptr;
  HostWindow* settingsWin_ = nullptr;
  HostWindow* about_ = nullptr;
  bool showElapsed_ = true;
  int settingsTab_ = 0;
  int trackScroll_ = 0;
  ChromeHit::Kind sliderKind_ = ChromeHit::Kind::none;
  int sliderIndex_ = -1;
  QPoint dragOrigin_;
  QSet<WindowId> hiddenByMinimize_;
  QTimer persistTimer_;
  QTimer alteredTimer_;
  QTimer usageTimer_;
  QTimer aboutTimer_;
  QTimer eqApplyTimer_;
  CollectionFigures figures_;
  bool figuresLoaded_ = false;
};

}  // namespace tramp
