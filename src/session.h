#pragma once

#include "chrome_hits.h"
#include "chrome_menu.h"
#include "collection.h"
#include "layout_sync.h"
#include "look.h"
#include "persist.h"
#include "playback.h"
#include "player_engine.h"
#include "playlist.h"
#include "session_view.h"
#include "settings.h"
#include "spectrum.h"
#include "worker_crew.h"

#include <QElapsedTimer>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <atomic>
#include <memory>
#include <optional>

class HostWindow;
class HostShell;

namespace tramp {

class TrampSession : public QObject {
  Q_OBJECT

 public:
  explicit TrampSession(QObject* parent = nullptr);
  ~TrampSession() override;

  void setWindows(HostWindow* main, HostWindow* eq, HostWindow* pl, HostWindow* settings,
                  HostWindow* about);
  void setShell(HostShell* shell);
  void bootstrap(const QStringList& argvFiles);
  SessionView view() const;
  MainLiveReadouts mainLive() const;
  int zoomPercent() const { return layout_.zoomPercent(); }
  bool confirmQuit() const;
  bool windowShouldShow(WindowId id) const;
  void persistNow();
  void detachWindows();
  void reapplyWindowFrames();
  void applyDroppedPaths(const QStringList& paths, bool replace);
  void extraClosed(WindowId id);
  void mainMinimized(bool minimized);
  void mainActivated();
  void playTrackAt(int index);
  void togglePlayPause();
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
  void titleDragBegan(WindowId id);
  void titleDragEnded(WindowId id);
  void extraWasMapped(WindowId id);
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
  void persistCollectionCache();
  /// One probe answer on its way back to the GUI thread. Answers travel in
  /// batches: a thousand-track open landing one row at a time would rebuild
  /// every panel a thousand times.
  struct ProbedTrack {
    QString path;
    QString title;
    QString artist;
    QString album;
    qint64 durationMs = 0;
  };

  QVector<Track> ingestPlaylistFile(const QString& path);
  void schedulePathVerify();
  void refreshCurrentPlaylist();
  /// Ask about every track this list still needs, on a worker. [overwrite] is
  /// Refresh: believe the files over the file that listed them.
  void startDurationProbe(const QVector<Track>& tracks, bool overwrite = false);
  void applyProbedBatch(const QVector<ProbedTrack>& batch, int gen, bool overwrite);
  void probeFinished(int gen);
  void setIngesting(bool ingesting);
  HostWindow* windowFor(WindowId id) const;
  QString pickAudio(bool multiple);
  QString pickPlaylist(bool save);
  void openPaths(const QStringList& paths, bool enqueue);
  void loadCollectionRow(int index);
  void applyDockToWindows(std::optional<WindowId> skip = {});
  void syncLayoutFromWindows(std::optional<WindowId> skip = {});
  void writeNativeFrame(WindowId id, QRect native);
  void clampOneToHost(WindowId id);
  void fitClusterToHost();
  void showOptionsMenu(QRect logicalHit);
  int execAnchoredMenu(const QVector<ChromeMenuItem>& items, HostWindow* host, QRect logicalHit,
                       PopupAnchor anchor);
  void showTrackInfo();
  bool reportPlaylistWriteFailure(bool wrote, const QString& path);
  bool confirmReplaceAltered(const QString& consequence = {});
  void quitFromMenu();
  void syncSpectrum();
  void tickSpectrum();
  void startSpectrumDecode(const QString& path, int gen);
  void syncTitleMarquee();
  qint64 titleScrollMs() const;

  SupportStore store_;
  TrampSettings settings_;
  PlaylistController playlist_;
  PlaylistCollection collection_;
  // The engine is declared first so it is destroyed last: MpvEngine's callbacks
  // capture the controller, and an engine draining events after the controller
  // had gone would be calling into a corpse.
  std::unique_ptr<PlayerEngine> engine_;
  std::unique_ptr<PlaybackController> playback_;
  SpectrumAnalyzer analyzer_;
  Spectrogram spectrogram_;
  SpectrumHold spectrumHold_;
  QTimer spectrumTimer_;
  QString spectrumPath_;
  // Workers read the generation they were started with against the live one on
  // every loop iteration, so all three have to be atomic.
  std::atomic<int> spectrumGen_{0};
  bool spectrumReady_ = false;
  LayoutSync layout_;
  SkinController skins_;
  HostWindow* main_ = nullptr;
  HostWindow* eq_ = nullptr;
  HostWindow* pl_ = nullptr;
  HostWindow* settingsWin_ = nullptr;
  HostWindow* about_ = nullptr;
  HostShell* shell_ = nullptr;
  int settingsTab_ = 0;
  int trackScroll_ = 0;
  ChromeHit::Kind sliderKind_ = ChromeHit::Kind::none;
  int sliderIndex_ = -1;
  QPoint dragOrigin_;
  QSet<WindowId> hiddenByMinimize_;
  QTimer marqueeTimer_;
  QElapsedTimer marqueeClock_;
  QString marqueeIdentity_;
  QTimer persistTimer_;
  QTimer alteredTimer_;
  QTimer usageTimer_;
  QTimer aboutTimer_;
  QTimer eqApplyTimer_;
  QTimer collectionPersistTimer_;
  CollectionFigures figures_;
  std::atomic<int> durationGen_{0};
  std::atomic<int> verifyGen_{0};
  /// Paths the live probe generation still owes an answer for. A second ingest
  /// supersedes the first worker, so these ride along into the new run rather
  /// than staying at --:-- until the list is opened again.
  QSet<QString> probeOutstanding_;
  bool figuresLoaded_ = false;
  bool applyingDock_ = false;
  bool titleDragging_ = false;
  int skinsScroll_ = 0;
  /// A playlist is still taking on track data — what the Refresh lamp reports.
  bool ingesting_ = false;
  /// Set while a batch of probe answers is being applied: every answer touches
  /// the list, and every touch would otherwise rebuild the view and hand it to
  /// five panels. One batch is one change as far as the chrome is concerned.
  bool holdChrome_ = false;
  bool chromeHeld_ = false;
  /// Last, so that even a teardown path that forgets to stop the workers joins
  /// them before any member they touch is destroyed.
  WorkerCrew workers_;
};

}  // namespace tramp
