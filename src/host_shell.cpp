#include "host_shell.h"

namespace tramp {

HostShellLayout hostShellLayout(const QVector<QRect>& visiblePanelScreenRects) {
  if (visiblePanelScreenRects.isEmpty()) {
    return {};
  }

  QRect screenRect;
  for (const QRect& panel : visiblePanelScreenRects) {
    screenRect = screenRect.united(panel);
  }

  QRegion localMask;
  const QPoint origin = screenRect.topLeft();
  for (const QRect& panel : visiblePanelScreenRects) {
    localMask += panel.translated(-origin);
  }

  return {screenRect, localMask};
}

}  // namespace tramp
