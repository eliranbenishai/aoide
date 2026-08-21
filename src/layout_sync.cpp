#include "layout_sync.h"

#include "host_shell.h"

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

QRect LayoutSync::nativeFrameRect(WindowId id) const {
  const QSizeF logical = docking_.logicalSize(id);
  const QSize zoomedSize =
      zoomed(QSize(qRound(logical.width()), qRound(logical.height())), zoomPercent_);
  const QSize nativeSize = panelNativeSize(zoomedSize, QSize());
  const WindowFrame& f = docking_.layout().frameOf(id);
  return QRect(logicalToNative(QPointF(f.left, f.top)), nativeSize);
}

}  // namespace tramp
