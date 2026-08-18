#include "host_shell.h"

#include <QGuiApplication>
#include <QScreen>

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

QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize) {
  if (logicalZoomed.width() > 0 && logicalZoomed.height() > 0) return logicalZoomed;
  return widgetSize;
}

QRect virtualDesktopGeometry() {
  QRect desktop;
  const auto screens = QGuiApplication::screens();
  for (QScreen* screen : screens) {
    if (screen) desktop = desktop.united(screen->geometry());
  }
  return desktop;
}

}  // namespace tramp
