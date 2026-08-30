#include "layout_sync.h"

#include "chrome_layout.h"
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

DockSide oppositeSide(DockSide side) {
  switch (side) {
    case DockSide::left:
      return DockSide::right;
    case DockSide::right:
      return DockSide::left;
    case DockSide::top:
      return DockSide::bottom;
    case DockSide::bottom:
      return DockSide::top;
  }
  return DockSide::bottom;
}

QRect placedOnSide(QRect sibling, QRect keep, DockSide side) {
  switch (side) {
    case DockSide::right:
      return QRect(keep.x() + keep.width(), sibling.y(), sibling.width(), sibling.height());
    case DockSide::left:
      return QRect(keep.x() - sibling.width(), sibling.y(), sibling.width(), sibling.height());
    case DockSide::bottom:
      return QRect(sibling.x(), keep.y() + keep.height(), sibling.width(), sibling.height());
    case DockSide::top:
      return QRect(sibling.x(), keep.y() - sibling.height(), sibling.width(), sibling.height());
  }
  return sibling;
}

bool fullyInside(QRect panel, QRect bounds) {
  return !bounds.isEmpty() && clampRectToHost(panel, bounds) == panel;
}

QRect tryBeside(QRect sibling, QRect keep, QRect bounds, DockSide side) {
  QRect at = clampRectToHost(placedOnSide(sibling, keep, side), bounds);
  if (!at.intersects(keep) && fullyInside(at, bounds)) return at;
  return {};
}

QRect tryBesideShrunk(QRect sibling, QRect keep, QRect bounds, DockSide side, QSize minSize) {
  int w = sibling.width();
  int h = sibling.height();
  QRect at;
  switch (side) {
    case DockSide::right: {
      const int room = bounds.x() + bounds.width() - (keep.x() + keep.width());
      if (room < minSize.width()) return {};
      w = qMin(w, room);
      at = QRect(keep.x() + keep.width(), sibling.y(), w, h);
      break;
    }
    case DockSide::left: {
      const int room = keep.x() - bounds.x();
      if (room < minSize.width()) return {};
      w = qMin(w, room);
      at = QRect(keep.x() - w, sibling.y(), w, h);
      break;
    }
    case DockSide::bottom: {
      const int room = bounds.y() + bounds.height() - (keep.y() + keep.height());
      if (room < minSize.height()) return {};
      h = qMin(h, room);
      at = QRect(sibling.x(), keep.y() + keep.height(), w, h);
      break;
    }
    case DockSide::top: {
      const int room = keep.y() - bounds.y();
      if (room < minSize.height()) return {};
      h = qMin(h, room);
      at = QRect(sibling.x(), keep.y() - h, w, h);
      break;
    }
  }
  at = clampRectToHost(at, bounds);
  if (!at.intersects(keep) && fullyInside(at, bounds)) return at;
  return {};
}

QRect clearKeep(QRect sibling, QRect keep, QRect bounds, QSize minSize, bool mayShrink,
                DockSide prefer) {
  if (bounds.isEmpty() || !sibling.intersects(keep)) return sibling;
  DockSide sides[4] = {prefer, oppositeSide(prefer), DockSide::bottom, DockSide::right};
  if (prefer == DockSide::bottom || prefer == DockSide::top) {
    sides[2] = DockSide::right;
    sides[3] = DockSide::left;
  } else {
    sides[2] = DockSide::bottom;
    sides[3] = DockSide::top;
  }
  for (DockSide side : sides) {
    const QRect at = tryBeside(sibling, keep, bounds, side);
    if (!at.isNull()) return at;
  }
  if (!mayShrink) return sibling;
  for (DockSide side : sides) {
    const QRect at = tryBesideShrunk(sibling, keep, bounds, side, minSize);
    if (!at.isNull()) return at;
  }
  return sibling;
}

QSizeF clusterLogicalSizeAtPlaylistFloor(const DockingCoordinator& docking, QSize minL) {
  QRectF united;
  for (WindowId id : visibleClusterMembers(docking.layout())) {
    QRectF r = docking.rectFor(id);
    if (id == WindowId::playlist) {
      const QSizeF chosen = docking.logicalSize(id);
      const qreal h = docking.layout().playlist.shaded
                          ? chosen.height()
                          : qMin(qreal(minL.height()), chosen.height());
      r.setSize(QSizeF(qMin(qreal(minL.width()), chosen.width()), h));
    }
    united = united.united(r);
  }
  return united.size();
}

void shrinkPlaylistToFitWork(QVector<PanelPlacement>& panels, QRect work, QSize minNative) {
  if (work.isEmpty()) return;
  int playlistIndex = -1;
  QRect others;
  for (int i = 0; i < panels.size(); ++i) {
    if (!panels[i].visible) continue;
    if (panels[i].id == WindowId::playlist) {
      playlistIndex = i;
      continue;
    }
    // Only panels on this work area count. A neighbour on another display is
    // not pressure this screen's playlist has to absorb.
    if (work.intersects(panels[i].screen)) others = others.united(panels[i].screen);
  }
  if (playlistIndex < 0) return;
  PanelPlacement& playlist = panels[playlistIndex];
  // When the playlist is already parked past a sibling, shrinking into the
  // remaining work is what absorbs a zoom step. Moving it first would climb
  // onto the equalizer; a grip that is merely past the taskbar is a move,
  // not a shrink, and is handled after this.
  if (!others.isEmpty() && playlist.screen.left() >= others.right() + 1) {
    const int room = work.x() + work.width() - others.x() - others.width();
    if (room >= minNative.width() && playlist.screen.width() > room) {
      playlist.screen.setWidth(room);
    }
  }
  if (!others.isEmpty() && playlist.screen.top() >= others.bottom() + 1) {
    const int room = work.y() + work.height() - others.y() - others.height();
    if (room >= minNative.height() && playlist.screen.height() > room) {
      playlist.screen.setHeight(room);
    }
  }
}

void clearDockedSiblingsOfMain(QVector<PanelPlacement>& panels, QRect host, QSize playlistMin,
                               PanelSurfaces* surfaces) {
  QRect main;
  for (const PanelPlacement& panel : panels) {
    if (panel.id == WindowId::main && panel.visible) main = panel.screen;
  }
  if (main.isEmpty()) return;
  for (PanelPlacement& panel : panels) {
    if (!panel.visible) continue;
    if (panel.id != WindowId::equalizer && panel.id != WindowId::playlist) continue;
    QRect bounds = host;
    if (panel.id == WindowId::playlist && surfaces) {
      const QRect work = surfaces->workAreaFor(panel.screen);
      if (!work.isEmpty()) bounds = work;
    }
    const bool mayShrink = panel.id == WindowId::playlist;
    panel.screen = clearKeep(panel.screen, main, bounds, playlistMin, mayShrink,
                             panelSpec(panel.id).parkSide);
  }
}

}  // namespace

LayoutSync::LayoutSync(DockLayout layout, qreal zoomPercent)
    : docking_(std::move(layout)), zoomPercent_(zoomPercent) {}

void LayoutSync::setPlaylistMinLogical(QSize min) { playlistMinLogical_ = min; }

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
  QSize nativeSize = panelNativeSize(zoomedSize, QSize());
  const WindowFrame& f = docking_.layout().frameOf(id);
  const QRect wanted(logicalToNative(QPointF(f.left, f.top)), nativeSize);
  if (id != WindowId::playlist) return wanted;
  // Position stays the frame's. Size is derived so a chosen canvas larger
  // than this desktop still reads as a rectangle that fits — a clamp is a
  // placement, not a resize.
  // Work-area fitting is not done here: asking the display on every frame
  // read would measure a hidden playlist against the wrong screen, and the
  // reserved-top lift has to see main's own rectangle as the last ask.
  const QRect host = hostRect();
  if (host.isEmpty()) return wanted;
  nativeSize.setWidth(qMin(nativeSize.width(), host.width()));
  nativeSize.setHeight(qMin(nativeSize.height(), host.height()));
  return QRect(wanted.topLeft(), nativeSize);
}

void LayoutSync::setNativeFrame(WindowId id, QRect native) {
  WindowFrame& f = docking_.layout().frameOf(id);
  const QPointF logical = nativeToLogical(native.topLeft());
  f.left = logical.x();
  f.top = logical.y();
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
  return zoomStepFits(clusterLogicalSizeAtPlaylistFloor(docking_, playlistMinLogical_),
                      work.size(), percent);
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
  if (id == WindowId::playlist) {
    const QRect hosted = nativeFrameRect(id);
    const QRect work = surfaces_ ? surfaces_->workAreaFor(hosted) : QRect();
    const QRect gripped = clampPlaylistGripToWorkArea(hosted, work);
    if (gripped.topLeft() != hosted.topLeft()) setNativeFrame(id, gripped);
  }
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
    // The playlist is left for place(): clamping it here walks it onto main
    // before the fitted shrink can absorb a zoom or a short work area.
    for (WindowId id : ids) {
      if (id == WindowId::playlist) continue;
      setNativeFrame(id, clampRectToHost(nativeFrameRect(id), host));
    }
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
  if (placing_) return;
  if (!surfaces_) {
    docking_.takeUserMoved();
    return;
  }
  placing_ = true;
  docking_.ensureMainVisible();
  const QRect host = surfaces_->hostRect();
  // A title-bar drag always moves then places. Clearing main here would pull
  // the sibling off the rectangle the listener just chose.
  const bool clearMain = !docking_.takeUserMoved();

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
    if (panel.visible) panel.screen = nativeFrameRect(id);
    panels.push_back(panel);
  }
  if (clearMain) {
    const QSize playlistMin = zoomed(playlistMinLogical_, zoomPercent_);
    QRect playlistScreen;
    for (const PanelPlacement& panel : panels) {
      if (panel.id == WindowId::playlist && panel.visible) playlistScreen = panel.screen;
    }
    const QRect work = playlistScreen.isEmpty() ? QRect() : surfaces_->workAreaFor(playlistScreen);
    shrinkPlaylistToFitWork(panels, work, playlistMin);
  }
  for (PanelPlacement& panel : panels) {
    if (!panel.visible) continue;
    if (!host.isEmpty()) panel.screen = clampRectToHost(panel.screen, host);
    if (panel.id != WindowId::playlist) continue;
    const QRect work = surfaces_->workAreaFor(panel.screen);
    panel.screen = clampPlaylistGripToWorkArea(panel.screen, work);
  }
  if (clearMain && !host.isEmpty()) {
    clearDockedSiblingsOfMain(panels, host, zoomed(playlistMinLogical_, zoomPercent_),
                             surfaces_);
  }
  for (PanelPlacement& panel : panels) {
    const QSizeF canvas = docking_.canvasSize(panel.id);
    if (panel.visible) {
      const QRect wanted = nativeFrameRect(panel.id);
      // Position is a fact about where the panel sits: leaving the frame at one
      // origin while the desktop parked it at another is what made a read-back
      // from the widgets necessary in the first place. Size is not. The
      // playlist's chosen size is what the listener dragged it to, and a clamp
      // that writes the fit back is why shrinking the desktop, or zooming in,
      // destroyed that size forever. Only write when the clamp moved the
      // origin, and never write a fitted size — resizePlaylist is the one
      // author of the chosen size. An ordinary placement still must not round
      // its way into a drift, which is why the write stays conditional.
      if (panel.screen.topLeft() != wanted.topLeft()) setNativeFrame(panel.id, panel.screen);
    }
    if (panel.id == WindowId::playlist && panel.visible) {
      // The playlist paints at whatever logical size it is handed. The fitted
      // screen rectangle is what actually sits on the desktop, so the canvas
      // has to be that size too — otherwise the panel paints at the chosen
      // size and is placed at another.
      const qreal z = zoomPercent_ / 100.0;
      const int fittedW = qRound(panel.screen.width() / z);
      const int fittedH =
          panel.shaded ? qRound(canvas.height()) : qRound(panel.screen.height() / z);
      panel.logicalSize = QSize(fittedW, fittedH);
    } else {
      panel.logicalSize = QSize(qRound(canvas.width()), qRound(canvas.height()));
    }
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
