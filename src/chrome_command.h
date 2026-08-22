#pragma once

#include "chrome_hits.h"
#include "playback.h"

#include <QPoint>
#include <Qt>

namespace tramp {

/// What the router cannot do itself: a dialog or a menu needs a parent widget,
/// so the session presents these. Everything else the router does through the
/// controllers it was given.
enum class ChromeIntent {
  none,
  pickAudio,
};

struct ChromeCommandOutcome {
  bool handled = false;
  bool persist = false;
  ChromeIntent intent = ChromeIntent::none;
};

/// Routes a chrome hit into playback, playlist, settings and the rest, without
/// owning a window. Constructed on the stack for one hit — it holds references,
/// not the controllers.
class ChromeCommandRouter {
 public:
  explicit ChromeCommandRouter(PlaybackController& playback);

  ChromeCommandOutcome handle(WindowId id, const ChromeHit& hit, Qt::KeyboardModifiers mods,
                              QPoint logical);

 private:
  PlaybackController& playback_;
};

}  // namespace tramp
