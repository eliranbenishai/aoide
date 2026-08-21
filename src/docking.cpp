#include "docking.h"

#include <cmath>

namespace tramp {

WindowFrame& DockLayout::frameOf(WindowId id) {
  switch (id) {
    case WindowId::main:
      return main;
    case WindowId::equalizer:
      return equalizer;
    case WindowId::playlist:
      return playlist;
    case WindowId::settings:
      return settings;
    case WindowId::about:
      return about;
  }
  return main;
}

const WindowFrame& DockLayout::frameOf(WindowId id) const {
  return const_cast<DockLayout*>(this)->frameOf(id);
}

DockingCoordinator::DockingCoordinator(DockLayout layout) : layout_(std::move(layout)) {}

QSizeF DockingCoordinator::logicalSize(WindowId id) const {
  const WindowFrame& frame = layout_.frameOf(id);
  QSizeF base;
  switch (id) {
    case WindowId::main:
      base = QSizeF(kMainPlayer);
      break;
    case WindowId::equalizer:
      base = QSizeF(kEqualizer);
      break;
    case WindowId::playlist:
      base = QSizeF(frame.width.value_or(kPlaylistDefault.width()),
                    frame.height.value_or(kPlaylistDefault.height()));
      break;
    case WindowId::settings:
      base = QSizeF(kSettings);
      break;
    case WindowId::about:
      base = QSizeF(kAbout);
      break;
  }
  if (frame.shaded) {
    base.setHeight(kTitleBar);
  }
  return base;
}

QRectF DockingCoordinator::rectFor(WindowId id) const {
  const WindowFrame& frame = layout_.frameOf(id);
  const QSizeF size = logicalSize(id);
  return QRectF(frame.left, frame.top, size.width(), size.height());
}

bool DockingCoordinator::hasEdge(WindowId id) const {
  for (const DockEdge& e : layout_.dockEdges) {
    if (e.a == id || e.b == id) return true;
  }
  return false;
}

DockSide DockingCoordinator::opposite(DockSide side) const {
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

QSet<WindowId> DockingCoordinator::groupOf(WindowId id) const {
  QSet<WindowId> group{id};
  if (!layout_.frameOf(id).visible) return group;
  bool grew = true;
  while (grew) {
    grew = false;
    for (const DockEdge& edge : layout_.dockEdges) {
      const bool aIn = group.contains(edge.a);
      const bool bIn = group.contains(edge.b);
      if (aIn && !bIn && layout_.frameOf(edge.b).visible) {
        group.insert(edge.b);
        grew = true;
      } else if (bIn && !aIn && layout_.frameOf(edge.a).visible) {
        group.insert(edge.a);
        grew = true;
      }
    }
  }
  return group;
}

bool DockingCoordinator::overlapsOrNear1D(double a0, double a1, double b0, double b1) const {
  return std::abs(a0 - b0) <= snapThreshold_ || std::abs(a1 - b1) <= snapThreshold_ ||
         std::abs(a0 - b1) <= snapThreshold_ || std::abs(a1 - b0) <= snapThreshold_ ||
         (a0 < b1 && a1 > b0);
}

QVector<WindowId> visibleClusterMembers(const DockLayout& layout) {
  QVector<WindowId> members;
  for (WindowId id : {WindowId::main, WindowId::equalizer, WindowId::playlist, WindowId::settings,
                      WindowId::about}) {
    if (id == WindowId::main || layout.frameOf(id).visible) members.push_back(id);
  }
  return members;
}

void DockingCoordinator::setShaded(WindowId id, bool shaded) {
  layout_.frameOf(id).shaded = shaded;
}

void DockingCoordinator::setVisible(WindowId id, bool visible) {
  if (id == WindowId::main && !visible) return;
  layout_.frameOf(id).visible = visible;
  if (!visible) {
    QVector<DockEdge> kept;
    for (const DockEdge& e : layout_.dockEdges) {
      if (e.a != id && e.b != id) kept.push_back(e);
    }
    layout_.dockEdges = kept;
  }
}

void DockingCoordinator::ensureMainVisible() {
  const bool hostWasEmpty = !layout_.main.visible;
  layout_.main.visible = true;
  if (hostWasEmpty) {
    layout_.equalizer.visible = true;
    layout_.playlist.visible = true;
  }
}

void DockingCoordinator::resizePlaylist(QSizeF logical) {
  layout_.playlist.width = logical.width();
  layout_.playlist.height = logical.height();
}

void DockingCoordinator::nudgeOffMainIfStacked(WindowId id) {
  if (id == WindowId::main || id == WindowId::settings || id == WindowId::about) return;
  const QRectF mainR = rectFor(WindowId::main);
  const QRectF r = rectFor(id);
  const QRectF inter = mainR.intersected(r);
  const double cover = inter.width() * inter.height();
  const double otherArea = std::max(1.0, r.width() * r.height());
  const bool sameOrigin = std::abs(r.left() - mainR.left()) <= 8 && std::abs(r.top() - mainR.top()) <= 8;
  if (!sameOrigin && cover < 0.6 * otherArea) return;
  WindowFrame& frame = layout_.frameOf(id);
  frame.left = mainR.left();
  if (id == WindowId::equalizer) {
    frame.top = mainR.bottom();
  } else {
    frame.left = mainR.right();
    frame.top = mainR.top();
  }
}

void DockingCoordinator::move(WindowId id, QPointF topLeft, bool shiftUndock, bool snap) {
  if (id == WindowId::settings || id == WindowId::about) {
    layout_.frameOf(id).left = topLeft.x();
    layout_.frameOf(id).top = topLeft.y();
    return;
  }
  const QPointF current(layout_.frameOf(id).left, layout_.frameOf(id).top);
  const QPointF delta = topLeft - current;
  if (id == WindowId::main) {
    for (WindowId w : {WindowId::main, WindowId::equalizer, WindowId::playlist, WindowId::settings,
                       WindowId::about}) {
      WindowFrame& frame = layout_.frameOf(w);
      frame.left += delta.x();
      frame.top += delta.y();
    }
    return;
  }
  bool shouldUndock = shiftUndock;
  const double dist2 = QPointF::dotProduct(delta, delta);
  if (!shouldUndock && dist2 > kPeelDelta * kPeelDelta && hasEdge(id)) {
    shouldUndock = true;
  }
  if (shouldUndock) {
    QVector<DockEdge> kept;
    for (const DockEdge& e : layout_.dockEdges) {
      if (e.a != id && e.b != id) kept.push_back(e);
    }
    layout_.dockEdges = kept;
  }
  layout_.frameOf(id).left = topLeft.x();
  layout_.frameOf(id).top = topLeft.y();
  if (snap && !shiftUndock) {
    trySnap(id);
  }
}

void DockingCoordinator::trySnap(WindowId id) {
  if (id == WindowId::settings || id == WindowId::about || snapThreshold_ <= 0) return;
  const QSet<WindowId> group = groupOf(id);
  const QRectF moving = rectFor(id);

  struct Cand {
    WindowId target = WindowId::main;
    DockSide side = DockSide::bottom;
    double distance = 1e9;
    QPointF delta;
  };
  std::optional<Cand> bestV;
  std::optional<Cand> bestH;

  auto consider = [&](std::optional<Cand>& slot, Cand next, bool aligned) {
    if (next.distance > snapThreshold_ || !aligned) return;
    if (!slot || next.distance < slot->distance) slot = next;
  };

  for (WindowId otherId : {WindowId::main, WindowId::equalizer, WindowId::playlist}) {
    if (otherId == id || group.contains(otherId) || !layout_.frameOf(otherId).visible) continue;
    const QRectF other = rectFor(otherId);
    consider(bestV,
             {otherId, DockSide::bottom, std::abs(moving.bottom() - other.top()),
              QPointF(0, other.top() - moving.bottom())},
             overlapsOrNear1D(moving.left(), moving.right(), other.left(), other.right()));
    consider(bestV,
             {otherId, DockSide::top, std::abs(moving.top() - other.bottom()),
              QPointF(0, other.bottom() - moving.top())},
             overlapsOrNear1D(moving.left(), moving.right(), other.left(), other.right()));
    consider(bestH,
             {otherId, DockSide::right, std::abs(moving.right() - other.left()),
              QPointF(other.left() - moving.right(), 0)},
             overlapsOrNear1D(moving.top(), moving.bottom(), other.top(), other.bottom()));
    consider(bestH,
             {otherId, DockSide::left, std::abs(moving.left() - other.right()),
              QPointF(other.right() - moving.left(), 0)},
             overlapsOrNear1D(moving.top(), moving.bottom(), other.top(), other.bottom()));
  }

  if (bestV && !bestH) {
    const QRectF snapped = moving.translated(bestV->delta);
    const QRectF target = rectFor(bestV->target);
    const double leftDist = std::abs(snapped.left() - target.left());
    const double rightDist = std::abs(snapped.right() - target.right());
    if (leftDist <= snapThreshold_ && leftDist <= rightDist) {
      bestH = Cand{bestV->target, DockSide::left, leftDist,
                   QPointF(target.left() - snapped.left(), 0)};
    } else if (rightDist <= snapThreshold_) {
      bestH = Cand{bestV->target, DockSide::right, rightDist,
                   QPointF(target.right() - snapped.right(), 0)};
    }
  } else if (bestH && !bestV) {
    const QRectF snapped = moving.translated(bestH->delta);
    const QRectF target = rectFor(bestH->target);
    const double topDist = std::abs(snapped.top() - target.top());
    const double bottomDist = std::abs(snapped.bottom() - target.bottom());
    if (topDist <= snapThreshold_ && topDist <= bottomDist) {
      bestV = Cand{bestH->target, DockSide::top, topDist,
                   QPointF(0, target.top() - snapped.top())};
    } else if (bottomDist <= snapThreshold_) {
      bestV = Cand{bestH->target, DockSide::bottom, bottomDist,
                   QPointF(0, target.bottom() - snapped.bottom())};
    }
  }

  if (!bestV && !bestH) return;

  QPointF snapDelta;
  if (bestV) snapDelta += bestV->delta;
  if (bestH) snapDelta += bestH->delta;

  for (WindowId member : group) {
    layout_.frameOf(member).left += snapDelta.x();
    layout_.frameOf(member).top += snapDelta.y();
  }

  auto addEdge = [&](const Cand& cand) {
    for (const DockEdge& e : layout_.dockEdges) {
      const bool pair = (e.a == id && e.b == cand.target) || (e.a == cand.target && e.b == id);
      if (pair && (e.side == cand.side || e.side == opposite(cand.side))) return;
    }
    layout_.dockEdges.push_back({id, cand.target, cand.side});
  };
  if (bestV) addEdge(*bestV);
  if (bestH) addEdge(*bestH);
}

}  // namespace tramp
