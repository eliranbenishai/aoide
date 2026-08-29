#include "layout_sync.h"

#include "host_shell.h"
#include "panel_registry.h"

#include <QVector>
#include <cmath>
#include <iterator>

namespace aoide {
namespace {

QVector<WindowId> allPanelIds() {
  QVector<WindowId> ids;
  ids.reserve(kPanelCount);
  for (const PanelSpec& panel : panelSpecs()) ids.push_back(panel.id);
  return ids;
}

}  // namespace

LayoutSync::LayoutSync(DockLayout layout, qreal zoomPercent)
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

int LayoutSync::reservedTopLiftFor(QRect panelNative) const {
  const QRect work = surfaces_ ? surfaces_->workAreaFor(panelNative) : QRect();
  return reservedTopLift(panelNative, work);
}

QRect LayoutSync::clusterNativeRect() const {
  QRect united;
  for (WindowId id : visibleClusterMembers(docking_.layout())) {
    united = united.united(nativeFrameRect(id));
  }
  return united;
}

QSizeF LayoutSync::clusterLogicalSize() const {
  QRectF united;
  for (WindowId id : visibleClusterMembers(docking_.layout())) {
    united = united.united(docking_.rectFor(id));
  }
  return united.size();
}

bool LayoutSync::zoomStepAvailable(qreal percent) const {
  if (!zoomPercentsEqual(percent, snapZoomPercent(percent))) return false;
  if (zoomPercentsEqual(percent, zoomPercent_) || percent < zoomPercent_) return true;
  const QRect work = surfaces_ ? surfaces_->workAreaFor(clusterNativeRect()) : QRect();
  return zoomStepFits(clusterLogicalSize(), work.size(), percent);
}

std::optional<qreal> LayoutSync::zoomStepUp() const {
  const qreal up = nextZoomPercent(zoomPercent_);
  if (zoomPercentsEqual(up, zoomPercent_) || !zoomStepAvailable(up)) return std::nullopt;
  return up;
}

std::optional<qreal> LayoutSync::zoomStepDown() const {
  const qreal down = prevZoomPercent(zoomPercent_);
  if (zoomPercentsEqual(down, zoomPercent_)) return std::nullopt;
  return down;
}

bool LayoutSync::setZoomPercent(qreal percent) {
  if (!zoomStepAvailable(percent)) return false;
  zoomPercent_ = percent;
  return true;
}

void LayoutSync::clampToHost(WindowId id) {
  const QRect host = hostRect();
  if (host.isEmpty()) return;
  setNativeFrame(id, clampRectToHost(nativeFrameRect(id), host));
  // A sibling parked under the same strip is stranded the same way main is:
  // its title bar is the only handle, and the host clamp will not move it.
  const int lift = reservedTopLiftFor(nativeFrameRect(id));
  if (lift != 0) setNativeFrame(id, nativeFrameRect(id).translated(0, lift));
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
    if (!delta->isNull()) {
      // Only visible panels decide how far the cluster has to move, but a hidden
      // one still rides along: a main drag already carries it, and leaving it
      // behind here would walk it out of the cluster one correction at a time.
      for (WindowId id : allPanelIds()) {
        setNativeFrame(id, nativeFrameRect(id).translated(*delta));
      }
    }
  } else {
    // Host-only clamp: clampToHost also lifts, and doing that here would clear
    // main before the cluster lift below, so hidden panels would not ride along.
    for (WindowId id : ids) setNativeFrame(id, clampRectToHost(nativeFrameRect(id), host));
  }
  // The host is the virtual desktop, so y=0 is on-screen even when a menu bar
  // covers it. A title-bar drag is app-owned and clamped to that host, which is
  // how a Mac listener parks the cluster under the strip and cannot pick it up
  // again. The work area names the strip; lifting from main's own screen keeps
  // a cluster that spans two monitors from being measured against the wrong one.
  const int lift = reservedTopLiftFor(nativeFrameRect(WindowId::main));
  if (lift != 0) {
    for (WindowId id : allPanelIds()) {
      setNativeFrame(id, nativeFrameRect(id).translated(0, lift));
    }
  }
}

void LayoutSync::setMainMinimized(bool minimized) {
  suppressed_.clear();
  if (!minimized) return;
  for (WindowId id : allPanelIds()) {
    if (id != WindowId::main && docking_.layout().frameOf(id).visible) suppressed_.insert(id);
  }
}

void LayoutSync::place() {
  if (!surfaces_ || placing_) return;
  placing_ = true;
  docking_.ensureMainVisible();
  const QRect host = surfaces_->hostRect();

  QVector<PanelPlacement> panels;
  const QVector<WindowId> ids = allPanelIds();
  panels.reserve(ids.size());
  for (WindowId id : ids) {
    PanelPlacement panel;
    panel.id = id;
    {
      const WindowFrame& frame = docking_.layout().frameOf(id);
      panel.shaded = frame.shaded;
      panel.visible = frame.visible && !suppressed_.contains(id);
    }
    if (panel.visible) {
      const QRect wanted = nativeFrameRect(id);
      panel.screen = host.isEmpty() ? wanted : clampRectToHost(wanted, host);
      // The desktop having the last word is a correction to the layout, not a
      // note made on the way to the screen. Leaving the frame saying one thing
      // while the panel sat somewhere else is what made a read-back from the
      // widgets necessary in the first place. Only write when the clamp bit, so
      // an ordinary placement cannot round its way into a drift.
      if (panel.screen != wanted) setNativeFrame(id, panel.screen);
    }
    const QSizeF canvas = docking_.canvasSize(id);
    panel.logicalSize = QSize(qRound(canvas.width()), qRound(canvas.height()));
    panels.push_back(panel);
  }
  // Every geometry change funnels through here — a drag, a zoom step, a fit,
  // the desktop changing shape — so this is the one moment where a dock edge
  // can be checked against the frames it describes. The check comes after the
  // clamp above, because the desktop overruling a panel is one of the ways an
  // edge stops being true without anybody dragging anything.
  docking_.validateEdges();
  surfaces_->placePanels(panels);
  placing_ = false;
}

}  // namespace aoide
