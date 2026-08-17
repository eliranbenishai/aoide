#pragma once

#include <QRect>
#include <QRegion>
#include <QVector>

namespace tramp {

struct HostShellLayout {
  QRect screenRect;   // native px, global; null if no panels
  QRegion localMask;  // host-local union of panels; empty if none
};

HostShellLayout hostShellLayout(const QVector<QRect>& visiblePanelScreenRects);

}  // namespace tramp
