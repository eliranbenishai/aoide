#pragma once

#include "chrome_hits.h"
#include "collection.h"
#include "docking.h"
#include "playback.h"
#include "player_engine.h"
#include "playlist.h"
#include "settings.h"

#include <QPoint>
#include <Qt>
#include <optional>

namespace tramp {

/// What the router cannot do itself: a dialog or a menu needs a parent widget,
/// so the session presents these. Everything else the router does through the
/// controllers it was given.
enum class ChromeIntent {
  none,
  pickAudio,
  pickPlaylistFile,
  showPlCreateMenu,
  renameCollectionEntry,
  showPlSortMenu,
  showPlOptionsMenu,
  saveCurrentPlaylist,
  refreshCurrentPlaylist,
  loadCollectionRow,
  showOptionsMenu,
  showTrackInfo,
  showEqPresets,
  openWebsite,
  resetSettings,
  rescanSkins,
  activateSkin,
  removeSkin,
  showSkinInstallMenu,
  pickSkinZip,
  pickSkinFolder,
  openSkinsDirectory,
};

struct ChromeCommandOutcome {
  bool handled = false;
  bool persist = false;
  bool refreshChrome = false;
  bool persistCollection = false;
  bool applyEq = false;
  bool refreshEq = false;
  bool applyAlwaysOnTop = false;
  bool syncTitleMarquee = false;
  std::optional<WindowId> toggleVisible;
  std::optional<int> settingsTab;
  bool beginSlider = false;
  ChromeHit::Kind sliderKind = ChromeHit::Kind::none;
  int sliderIndex = -1;
  int collectionRow = -1;
  ChromeIntent intent = ChromeIntent::none;
};

/// Routes a chrome hit into playback, playlist, settings and the rest, without
/// owning a window. Constructed on the stack for one hit — it holds references,
/// not the controllers.
class ChromeCommandRouter {
 public:
  ChromeCommandRouter(PlaybackController& playback, PlaylistController& playlist,
                      TrampSettings& settings, PlaylistCollection& collection, PlayerEngine& engine,
                      DockingCoordinator& docking);

  ChromeCommandOutcome handle(WindowId id, const ChromeHit& hit, Qt::KeyboardModifiers mods,
                              QPoint logical);

 private:
  PlaybackController& playback_;
  PlaylistController& playlist_;
  TrampSettings& settings_;
  PlaylistCollection& collection_;
  PlayerEngine& engine_;
  DockingCoordinator& docking_;
};

}  // namespace tramp
