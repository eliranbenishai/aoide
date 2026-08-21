#pragma once

#include "chrome_hits.h"
#include "mockup_draw.h"
#include "title_chrome.h"

#include <QVector>

namespace tramp {

/// How long a button takes to walk between faces. Long enough to read as a
/// fade, short enough that a click still feels immediate.
inline constexpr qreal kBtnTransitionMs = 200;

/// The three things that change a button's appearance. `on` is latched state the
/// session owns (shuffle, EQ, mute); `hover` and `press` belong to the panel's
/// own pointer.
enum class BtnChannel { on, hover, press };

/// Whether a control takes pointer feedback. Sliders, list rows, dividers and
/// drag handles are excluded: they are not buttons, and hovering a track list
/// would otherwise re-rasterise the whole playlist chassis on every mouse move.
bool takesPointerFeedback(ChromeHit::Kind kind);

/// Eased 0..1 phases for chrome whose appearance changes.
///
/// Nothing in the chrome snaps between states. Each visual walks toward its
/// target and the owning panel repaints until [moving] goes false, so the cost
/// is bounded to the length of one transition. That cost is a chassis rebuild
/// per frame, which is only affordable because a pointer cannot be hovering a
/// button and dragging a panel at the same time — see the paint budget in
/// `docs/architecture.md`.
///
/// Body controls key on [ChromeHit::Kind]; the title bar has its own hit map, so
/// its buttons key on [TitleChromeLayout::Hit].
class ChromePhases {
 public:
  void setTarget(ChromeHit::Kind kind, int index, BtnChannel channel, qreal target);
  /// Jump straight to the target. Used for the first view a panel is handed, so
  /// opening a window does not play every latched button's fade-in at once.
  void snapTo(ChromeHit::Kind kind, int index, BtnChannel channel, qreal target);
  qreal value(ChromeHit::Kind kind, int index, BtnChannel channel) const;
  BtnFace face(ChromeHit::Kind kind, int index = -1) const;

  void setTitleTarget(TitleChromeLayout::Hit hit, BtnChannel channel, qreal target);
  BtnFace titleFace(TitleChromeLayout::Hit hit) const;

  /// Send every phase on this channel to 0 — the pointer left the panel, or a
  /// held button was released.
  void releaseChannel(BtnChannel channel);

  /// Step by real elapsed time, so a transition lasts [kBtnTransitionMs]
  /// whatever frame rate the panel can actually manage. Returns [moving].
  bool advance(qreal dtMs);
  bool moving() const;

  /// A default-constructed store is inert. Golden dumps and tests paint without
  /// a panel behind them and must still show latched buttons lit, so painters
  /// fall back to plain session state unless something is actually driving
  /// phases. Only a live panel calls this.
  void setLive(bool live) { live_ = live; }
  bool live() const { return live_; }

 private:
  /// Body controls and title-bar buttons are separate hit enums whose values
  /// overlap, so the key carries which one it came from.
  enum class Group { body, title };

  struct Entry {
    Group group = Group::body;
    int key = 0;
    int index = -1;
    BtnChannel channel = BtnChannel::on;
    qreal value = 0;
    qreal target = 0;
  };

  const Entry* find(Group group, int key, int index, BtnChannel channel) const;
  void write(Group group, int key, int index, BtnChannel channel, qreal target, bool snap);
  qreal read(Group group, int key, int index, BtnChannel channel) const;

  QVector<Entry> entries_;
  bool live_ = false;
};

/// Smoothstep. A linear phase reads as mechanical on a face this glossy.
inline qreal easeBtnPhase(qreal t) {
  const qreal k = t < 0 ? 0 : (t > 1 ? 1 : t);
  return k * k * (3 - 2 * k);
}

}  // namespace tramp
