#include "chrome_command.h"

namespace tramp {

ChromeCommandRouter::ChromeCommandRouter(PlaybackController& playback, PlaylistController& playlist,
                                         TrampSettings& settings, PlaylistCollection& collection,
                                         PlayerEngine& engine, DockingCoordinator& docking)
    : playback_(playback),
      playlist_(playlist),
      settings_(settings),
      collection_(collection),
      engine_(engine),
      docking_(docking) {}

ChromeCommandOutcome ChromeCommandRouter::handle(WindowId id, const ChromeHit& hit,
                                                 Qt::KeyboardModifiers mods, QPoint logical) {
  Q_UNUSED(id);
  Q_UNUSED(logical);
  using K = ChromeHit::Kind;
  ChromeCommandOutcome out;
  switch (hit.kind) {
    case K::mute:
      playback_.toggleMute();
      out.handled = true;
      break;
    case K::prev:
    case K::plPrev:
      playback_.previous();
      out.handled = true;
      break;
    case K::play:
      if (!playback_.playing()) playback_.playPause();
      out.handled = true;
      break;
    case K::pause:
      if (playback_.playing()) playback_.playPause();
      out.handled = true;
      break;
    case K::plPlay:
      playback_.playPause();
      out.handled = true;
      break;
    case K::stop:
      playback_.stop();
      out.handled = true;
      break;
    case K::next:
    case K::plNext:
      playback_.next();
      out.handled = true;
      break;
    case K::shuffle:
      playback_.toggleShuffle();
      out.handled = true;
      break;
    case K::repeat:
      playback_.cycleRepeatMode();
      out.handled = true;
      break;
    case K::eject:
      out.handled = true;
      out.intent = ChromeIntent::pickAudio;
      break;
    case K::volume:
    case K::seek:
    case K::eqPreamp:
    case K::eqBand:
      out.handled = true;
      out.beginSlider = true;
      out.sliderKind = hit.kind;
      out.sliderIndex = hit.index;
      break;
    case K::plAdd:
      out.handled = true;
      out.intent = ChromeIntent::pickAudio;
      break;
    case K::plCollapse:
      settings_.playlistCollectionCollapsed = !settings_.playlistCollectionCollapsed;
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::plCollectionRow: {
      out.handled = true;
      const auto entries = collection_.entries();
      if (hit.index < 0 || hit.index >= entries.size()) break;
      if (mods & Qt::ControlModifier) {
        collection_.select(entries[hit.index].path);
        out.refreshChrome = true;
      } else {
        out.intent = ChromeIntent::loadCollectionRow;
        out.collectionRow = hit.index;
      }
      break;
    }
    case K::plAddCollection:
      out.handled = true;
      out.intent = ChromeIntent::pickPlaylistFile;
      break;
    case K::plCreate:
      out.handled = true;
      out.intent = ChromeIntent::showPlCreateMenu;
      break;
    case K::plRename:
      out.handled = true;
      out.intent = ChromeIntent::renameCollectionEntry;
      break;
    case K::plRemoveCollection:
      out.handled = true;
      if (!collection_.selectedPath().isEmpty()) {
        collection_.remove(collection_.selectedPath());
        out.persistCollection = true;
        out.refreshChrome = true;
      }
      break;
    case K::plTrackRow:
      out.handled = true;
      out.beginSlider = true;
      out.sliderKind = K::plTrackRow;
      out.sliderIndex = hit.index;
      if (mods & Qt::ShiftModifier) playlist_.selectRange(hit.index);
      else if (mods & Qt::ControlModifier) playlist_.toggleSelection(hit.index);
      else playlist_.select(hit.index);
      break;
    case K::plRemove:
      playlist_.removeSelected();
      out.handled = true;
      break;
    case K::plSave:
      out.handled = true;
      if (playlist_.altered()) out.intent = ChromeIntent::saveCurrentPlaylist;
      break;
    case K::plSort:
      out.handled = true;
      out.intent = ChromeIntent::showPlSortMenu;
      break;
    case K::plOptions:
      out.handled = true;
      out.intent = ChromeIntent::showPlOptionsMenu;
      break;
    case K::plRefresh:
      out.handled = true;
      out.intent = ChromeIntent::refreshCurrentPlaylist;
      break;
    case K::options:
      out.handled = true;
      out.intent = ChromeIntent::showOptionsMenu;
      break;
    case K::skins:
      out.handled = true;
      out.toggleVisible = WindowId::skins;
      break;
    case K::trackInfo:
      out.handled = true;
      if (playback_.currentTrack()) out.intent = ChromeIntent::showTrackInfo;
      break;
    case K::timeToggle:
      settings_.showElapsed = !settings_.showElapsed;
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::mono:
      settings_.forceMono = !settings_.forceMono;
      engine_.setForceMono(settings_.forceMono);
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::eqToggle:
      out.handled = true;
      out.toggleVisible = WindowId::equalizer;
      break;
    case K::plToggle:
      out.handled = true;
      out.toggleVisible = WindowId::playlist;
      break;
    case K::eqOn:
      settings_.equalizerCurve.enabled = !settings_.equalizerCurve.enabled;
      out.handled = true;
      out.applyEq = true;
      out.persist = true;
      out.refreshEq = true;
      break;
    case K::eqAuto:
      settings_.equalizerCurve.auto_ = !settings_.equalizerCurve.auto_;
      out.handled = true;
      out.persist = true;
      out.refreshEq = true;
      break;
    case K::eqPresets:
      out.handled = true;
      out.intent = ChromeIntent::showEqPresets;
      break;
    case K::settingsGeneral:
      out.handled = true;
      out.settingsTab = 0;
      out.refreshChrome = true;
      break;
    case K::settingsResume:
      settings_.resumeLastSession = !settings_.resumeLastSession;
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsConfirm:
      settings_.confirmBeforeQuit = !settings_.confirmBeforeQuit;
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsScroll:
      settings_.scrollTitle = !settings_.scrollTitle;
      out.handled = true;
      out.syncTitleMarquee = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsMinimize:
      settings_.minimizeHidesSecondaries = !settings_.minimizeHidesSecondaries;
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsSnapOff:
      settings_.dockSnapStrength = DockSnapStrength::off;
      docking_.setSnapThreshold(snapPixels(settings_.dockSnapStrength));
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsSnapNormal:
      settings_.dockSnapStrength = DockSnapStrength::normal;
      docking_.setSnapThreshold(snapPixels(settings_.dockSnapStrength));
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsSnapStrong:
      settings_.dockSnapStrength = DockSnapStrength::strong;
      docking_.setSnapThreshold(snapPixels(settings_.dockSnapStrength));
      out.handled = true;
      out.persist = true;
      out.refreshChrome = true;
      break;
    case K::settingsReset:
      out.handled = true;
      out.intent = ChromeIntent::resetSettings;
      break;
    case K::aboutWeb:
      out.handled = true;
      out.intent = ChromeIntent::openWebsite;
      break;
    case K::settingsAudio:
      out.handled = true;
      out.settingsTab = 1;
      out.refreshChrome = true;
      break;
    case K::settingsSkinRow:
      out.handled = true;
      out.intent = ChromeIntent::activateSkin;
      out.collectionRow = hit.index;
      break;
    case K::settingsSkinRemove:
      out.handled = true;
      out.intent = ChromeIntent::removeSkin;
      out.collectionRow = hit.index;
      break;
    case K::settingsSkinScroll:
      out.handled = true;
      out.beginSlider = true;
      out.sliderKind = K::settingsSkinScroll;
      break;
    case K::settingsSkinAdd:
      out.handled = true;
      out.intent = ChromeIntent::showSkinInstallMenu;
      break;
    case K::settingsSkinsFolder:
      out.handled = true;
      out.intent = ChromeIntent::openSkinsDirectory;
      break;
    case K::settingsSkinsRefresh:
      out.handled = true;
      out.intent = ChromeIntent::rescanSkins;
      break;
    case K::plResize:
    case K::plDivider:
    case K::none:
      out.handled = true;
      break;
    default:
      break;
  }
  return out;
}

}  // namespace tramp
