#pragma once

#include <QRect>
#include <QRegion>
#include <QSize>
#include <QVector>

namespace tramp {

struct HostShellLayout {
  QRect screenRect;   // native px, global; null if no panels
  QRegion localMask;  // host-local union of panels; empty if none
};

HostShellLayout hostShellLayout(const QVector<QRect>& visiblePanelScreenRects);

/// Native size for a panel. Zoomed logical size wins over an unmapped 0×0 widget.
QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize);

/// Union of `QGuiApplication::screens()` geometries. Null if there are no screens.
QRect virtualDesktopGeometry();

}  // namespace tramp
