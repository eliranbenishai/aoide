#include "chrome_command.h"

namespace tramp {

ChromeCommandRouter::ChromeCommandRouter(PlaybackController& playback) : playback_(playback) {}

ChromeCommandOutcome ChromeCommandRouter::handle(WindowId id, const ChromeHit& hit,
                                                 Qt::KeyboardModifiers mods, QPoint logical) {
  Q_UNUSED(id);
  Q_UNUSED(mods);
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
    default:
      break;
  }
  return out;
}

}  // namespace tramp
