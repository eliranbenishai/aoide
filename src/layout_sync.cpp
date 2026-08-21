#include "layout_sync.h"

#include <cmath>

namespace tramp {

LayoutSync::LayoutSync(DockLayout layout, int zoomPercent)
    : docking_(std::move(layout)), zoomPercent_(zoomPercent) {}

QPointF LayoutSync::nativeToLogical(QPoint native) const {
  const qreal z = zoomPercent_ / 100.0;
  return QPointF(native.x() / z, native.y() / z);
}

QPoint LayoutSync::logicalToNative(QPointF logical) const {
  const qreal z = zoomPercent_ / 100.0;
  return QPoint(int(std::lround(logical.x() * z)), int(std::lround(logical.y() * z)));
}

}  // namespace tramp
