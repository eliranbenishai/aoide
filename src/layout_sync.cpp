#include "layout_sync.h"

#include "host_shell.h"

#include <cmath>
#include <iterator>

namespace tramp {
namespace {

constexpr WindowId kAllPanels[] = {WindowId::main, WindowId::equalizer, WindowId::playlist,
                                   WindowId::settings, WindowId::about};

}  // namespace

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

void LayoutSync::setNativeFrame(WindowId id, QRect native) {
  WindowFrame& f = docking_.layout().frameOf(id);
  const QPointF logical = nativeToLogical(native.topLeft());
  f.left = logical.x();
  f.top = logical.y();
  if (id == WindowId::playlist) {
    const qreal z = zoomPercent_ / 100.0;
    f.width = native.width() / z;
    f.height = native.height() / z;
  }
}

QRect LayoutSync::hostRect() const { return surfaces_ ? surfaces_->hostRect() : QRect(); }

void LayoutSync::clampToHost(WindowId id) {
  const QRect host = hostRect();
  if (host.isEmpty()) return;
  setNativeFrame(id, clampRectToHost(nativeFrameRect(id), host));
}

void LayoutSync::fitClusterToHost() {
  const QRect host = hostRect();
  if (host.isEmpty()) return;
  const QVector<WindowId> ids = visibleClusterMembers(docking_.layout());
  QVector<QRect> rects;
  rects.reserve(ids.size());
  for (WindowId id : ids) rects.push_back(nativeFrameRect(id));
  const auto delta = clusterDeltaToFit(rects, host);
  if (delta) {
    if (delta->isNull()) return;
    // Only visible panels decide how far the cluster has to move, but a hidden
    // one still rides along: a main drag already carries it, and leaving it
    // behind here would walk it out of the cluster one correction at a time.
    for (WindowId id : kAllPanels) {
      setNativeFrame(id, nativeFrameRect(id).translated(*delta));
    }
    return;
  }
  for (WindowId id : ids) clampToHost(id);
}

void LayoutSync::setMainMinimized(bool minimized) {
  suppressed_.clear();
  if (!minimized) return;
  for (WindowId id : kAllPanels) {
    if (id != WindowId::main && docking_.layout().frameOf(id).visible) suppressed_.insert(id);
  }
}

void LayoutSync::place() {
  if (!surfaces_ || placing_) return;
  placing_ = true;
  docking_.ensureMainVisible();
  const QRect host = surfaces_->hostRect();

  QVector<PanelPlacement> panels;
  panels.reserve(int(std::size(kAllPanels)));
  for (WindowId id : kAllPanels) {
    const WindowFrame& frame = docking_.layout().frameOf(id);
    const QSizeF canvas = docking_.canvasSize(id);
    PanelPlacement panel;
    panel.id = id;
    panel.logicalSize = QSize(qRound(canvas.width()), qRound(canvas.height()));
    panel.shaded = frame.shaded;
    panel.visible = frame.visible && !suppressed_.contains(id);
    if (panel.visible) {
      panel.screen = nativeFrameRect(id);
      if (!host.isEmpty()) panel.screen = clampRectToHost(panel.screen, host);
    }
    panels.push_back(panel);
  }
  surfaces_->placePanels(panels);
  placing_ = false;
}

}  // namespace tramp
