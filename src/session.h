#pragma once

#include "chrome_command.h"
#include "chrome_hits.h"
#include "chrome_menu.h"
#include "collection.h"
#include "layout_sync.h"
#include "look.h"
#include "panel_registry.h"
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

/// Rows the options cog presents. Always-on-top and Quit stay off the openers;
/// the check follows [TrampSettings::alwaysOnTop].
inline QVector<ChromeMenuItem> optionsMenuItems(const TrampSettings& settings) {
  return {
      ChromeMenuItem::check(QStringLiteral("Always on top"), settings.alwaysOnTop),
      ChromeMenuItem::separator(),
      ChromeMenuItem::action(QStringLiteral("Open files…")),
      ChromeMenuItem::action(QStringLiteral("Settings…")),
      ChromeMenuItem::separator(),
      ChromeMenuItem::action(QStringLiteral("About Tramp")),
      ChromeMenuItem::action(QStringLiteral("Quit")),
  };
}

class TrampSession : public QObject, public PanelSurfaces {
  Q_OBJECT

 public:
  explicit TrampSession(QObject* parent = nullptr);
  ~TrampSession() override;

  void setWindows(const PanelWindows& windows);
  void setShell(HostShell* shell);
  void bootstrap(const QStringList& argvFiles);
  SessionView view() const;
  MainLiveReadouts mainLive() const;
  int zoomPercent() const { return layout_.zoomPercent(); }
  /// The step each zoom button would take, or nothing when it would take none.
  /// What decides whether the button paints enabled, and what a caller should
  /// ask before handing [setZoomPercent] anything.
  std::optional<int> zoomStepUp() const { return layout_.zoomStepUp(); }
  std::optional<int> zoomStepDown() const { return layout_.zoomStepDown(); }
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
  QWidget* dialogParent(WindowId id) const;
  void raiseSettingsIfShowing();
  QString pickAudio(bool multiple);
  QString pickPlaylist(bool save);
  void openPaths(const QStringList& paths, bool enqueue);
  void loadCollectionRow(int index);
  QRect hostRect() const override;
  QRect workAreaFor(QRect clusterNative) const override;
  void placePanels(const QVector<PanelPlacement>& panels) override;
  void presentChromeOutcome(const ChromeCommandOutcome& out, WindowId id, const ChromeHit& hit,
                            QPoint logical);
  void presentPlCreateMenu(const ChromeHit& hit);
  void presentPlRename();
  void presentPlSortMenu(const ChromeHit& hit);
  void presentPlOptionsMenu(const ChromeHit& hit);
  void presentEqPresets(const ChromeHit& hit);
  void presentResetSettings();
  void presentSkinZipInstall();
  void presentSkinFolderInstall();
  void presentSkinsDirectoryPick();
  void presentSkinRemove(int index);
  void refreshSkinPreviews();
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
  PersistHealth persistHealth_;
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
  PanelWindows windows_;
  HostShell* shell_ = nullptr;
  int settingsTab_ = 0;
  int trackScroll_ = 0;
  ChromeHit::Kind sliderKind_ = ChromeHit::Kind::none;
  int sliderIndex_ = -1;
  QPoint dragOrigin_;
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
  bool titleDragging_ = false;
  int skinsScroll_ = 0;
  /// A playlist is still taking on track data — what the Refresh lamp reports.
  bool ingesting_ = false;
  /// True when the session installed `MissingAudioEngine`.
  bool noAudioEngine_ = false;
  /// Set while a batch of probe answers is being applied: every answer touches
  /// the list, and every touch would otherwise rebuild the view and hand it to
  /// panels. One batch is one change as far as the chrome is concerned.
  bool holdChrome_ = false;
  bool chromeHeld_ = false;
  /// Last, so that even a teardown path that forgets to stop the workers joins
  /// them before any member they touch is destroyed.
  WorkerCrew workers_;
};

}  // namespace tramp
