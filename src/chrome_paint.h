#pragma once

#include "chrome_bodies.h"
#include "mockup_draw.h"
#include "title_chrome.h"
#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QSize>
#include <algorithm>

namespace aoide {

/// Where a title-bar button with nothing to do sits, and how much of its
/// glyph's ink is left. The lift is below a resting button's rather than level
/// with it: the two sit side by side in the same row, and a listener has to be
/// able to tell them apart without hovering either.
inline constexpr qreal kWinBtnDeadLift = 0.82;
inline constexpr int kWinBtnDeadGlyphAlpha = 77;

/// What a title-bar button paints as, once whether it has anything to do has
/// had its say.
///
/// A withdrawn zoom step keeps its button's geometry — the row has to stay a
/// row, or the title bar reflows to the eye every time a step goes — so the
/// face gives way instead: it drops to [kWinBtnDeadLift] and its glyph drains.
/// Both pointer channels are dropped here rather than left to the panel that
/// feeds them, because a dead button must not light up in *any* paint of it and
/// a live panel's pointer is only one of the things that paints one.
struct WinBtnFace {
  qreal hover = 0;
  qreal press = 0;
  qreal lift = 1;
  bool dead = false;
};

inline WinBtnFace winBtnFace(BtnFace phase, bool close, bool enabled) {
  if (!enabled) {
    return WinBtnFace{0, 0, kWinBtnDeadLift, true};
  }
  WinBtnFace face;
  face.hover = std::clamp(phase.hover, qreal(0), qreal(1));
  face.press = std::clamp(phase.press, qreal(0), qreal(1));
  // Close is already the loud one, so it lifts less than the neutral buttons or
  // it goes past the accent and stops reading as a warning.
  face.lift = 1 + (close ? 0.16 : 0.24) * face.hover - 0.18 * face.press;
  return face;
}

void paintMockupWindow(QPainter& painter,
                       QSize logical,
                       WindowId id,
                       const TitleChromeLayout& title,
                       const QImage* logo,
                       const SessionView& view = {},
                       BodyPaint pass = BodyPaint::full,
                       const ChromePhases& phases = ChromePhases());

}  // namespace aoide
