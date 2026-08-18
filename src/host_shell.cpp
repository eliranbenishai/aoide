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

QPoint panelLocalTopLeft(QPoint screenTopLeft, QPoint actualHostGlobal) {
  return screenTopLeft - actualHostGlobal;
}

QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize) {
  if (logicalZoomed.width() > 0 && logicalZoomed.height() > 0) return logicalZoomed;
  return widgetSize;
}

}  // namespace tramp
