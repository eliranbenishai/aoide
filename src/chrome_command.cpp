#include "chrome_command.h"

namespace tramp {

ChromeCommandRouter::ChromeCommandRouter(PlaybackController& playback, PlaylistController& playlist,
                                         TrampSettings& settings, PlaylistCollection& collection)
    : playback_(playback), playlist_(playlist), settings_(settings), collection_(collection) {}

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
    default:
      break;
  }
  return out;
}

}  // namespace tramp
