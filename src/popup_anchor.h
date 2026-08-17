#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>

namespace tramp {

enum class PopupAnchor { belowLeft, aboveLeft };

/// Screen position for a QMenu: left-aligned to the trigger, either flush
/// under it or flush above it. Uses y+height (not QRect::bottom, which is
/// inclusive) so the menu does not cover the button.
inline QPoint popupMenuPos(const QRect& buttonGlobal, const QSize& menuSize,
                           PopupAnchor anchor) {
  if (anchor == PopupAnchor::aboveLeft) {
    return {buttonGlobal.left(), buttonGlobal.top() - menuSize.height()};
  }
  return {buttonGlobal.left(), buttonGlobal.y() + buttonGlobal.height()};
}

}  // namespace tramp
