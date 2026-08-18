#pragma once

#include <QPoint>
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

/// Child top-left in host-local pixels from actual host origin, not a requested bbox origin.
QPoint panelLocalTopLeft(QPoint screenTopLeft, QPoint actualHostGlobal);

/// Native size for a panel. Zoomed logical size wins over an unmapped 0×0 widget.
QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize);

}  // namespace tramp
