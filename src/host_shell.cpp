#include "host_shell.h"

#include <algorithm>

namespace aoide {

HostShellLayout hostShellLayout(QRect hostScreenRect, const QVector<QRect>& visiblePanelScreenRects) {
  if (visiblePanelScreenRects.isEmpty() || hostScreenRect.isNull()) {
    return {};
  }

  QRegion localMask;
  const QPoint origin = hostScreenRect.topLeft();
  for (const QRect& panel : visiblePanelScreenRects) {
    localMask += panel.translated(-origin);
  }
  return {hostScreenRect, localMask};
}

QRect clampRectToHost(QRect panel, QRect host) {
  if (host.isEmpty()) return panel;
  const int w = std::min(panel.width(), host.width());
  const int h = std::min(panel.height(), host.height());
  const int x = std::clamp(panel.x(), host.x(), host.x() + host.width() - w);
  const int y = std::clamp(panel.y(), host.y(), host.y() + host.height() - h);
  return QRect(x, y, w, h);
}

std::optional<QPoint> clusterDeltaToFit(const QVector<QRect>& panels, QRect host) {
  if (host.isEmpty() || panels.isEmpty()) return QPoint(0, 0);
  QRect u;
  for (const QRect& panel : panels) u = u.united(panel);
  if (u.width() > host.width() || u.height() > host.height()) return std::nullopt;
  const int x = std::clamp(u.x(), host.x(), host.x() + host.width() - u.width());
  const int y = std::clamp(u.y(), host.y(), host.y() + host.height() - u.height());
  return QPoint(x - u.x(), y - u.y());
}

QRect clampPlaylistGripToWorkArea(QRect panel, QRect work) {
  if (work.isEmpty()) return panel;
  const int w = qMin(panel.width(), work.width());
  const int h = qMin(panel.height(), work.height());
  int x = panel.x();
  int y = panel.y();
  const int right = work.x() + work.width();
  const int bottom = work.y() + work.height();
  if (x + w > right) x = right - w;
  if (y + h > bottom) y = bottom - h;
  return QRect(x, y, w, h);
}

int reservedTopLift(QRect panel, QRect workArea) {
  if (workArea.isEmpty()) return 0;
  return std::max(0, workArea.top() - panel.top());
}

QPoint panelLocalTopLeft(QPoint screenTopLeft, QPoint actualHostGlobal) {
  return screenTopLeft - actualHostGlobal;
}

QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize) {
  if (logicalZoomed.width() > 0 && logicalZoomed.height() > 0) return logicalZoomed;
  return widgetSize;
}

}  // namespace aoide
