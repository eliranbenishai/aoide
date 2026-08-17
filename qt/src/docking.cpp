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

QSet<WindowId> DockingCoordinator::moveCohortOf(WindowId id) const {
  if (!stickyMoveGroups_) return {id};
  if (id == WindowId::settings || id == WindowId::about) return {id};
  if (id != WindowId::main) return groupOf(id);
  QSet<WindowId> cohort = groupOf(WindowId::main);
  if (snapThreshold_ <= 0) return cohort;
  const QRectF main = rectFor(WindowId::main);
  for (WindowId other : {WindowId::equalizer, WindowId::playlist}) {
    if (cohort.contains(other) || !layout_.frameOf(other).visible) continue;
    const QRectF r = rectFor(other);
    const bool flushV = std::abs(main.bottom() - r.top()) <= snapThreshold_ ||
                        std::abs(main.top() - r.bottom()) <= snapThreshold_;
    const bool flushH = std::abs(main.right() - r.left()) <= snapThreshold_ ||
                        std::abs(main.left() - r.right()) <= snapThreshold_;
    const bool alignedH = overlapsOrNear1D(main.left(), main.right(), r.left(), r.right());
    const bool alignedV = overlapsOrNear1D(main.top(), main.bottom(), r.top(), r.bottom());
    if ((flushV && alignedH) || (flushH && alignedV)) {
      cohort.insert(other);
    }
  }
  return cohort;
}

void DockingCoordinator::setShaded(WindowId id, bool shaded) {
  layout_.frameOf(id).shaded = shaded;
}

void DockingCoordinator::setVisible(WindowId id, bool visible) {
  layout_.frameOf(id).visible = visible;
  if (!visible) {
    QVector<DockEdge> kept;
    for (const DockEdge& e : layout_.dockEdges) {
      if (e.a != id && e.b != id) kept.push_back(e);
    }
    layout_.dockEdges = kept;
  }
}

void DockingCoordinator::resizePlaylist(QSizeF logical) {
  layout_.playlist.width = logical.width();
  layout_.playlist.height = logical.height();
}

void DockingCoordinator::move(WindowId id, QPointF topLeft, bool shiftUndock, bool snap) {
  if (id == WindowId::settings || id == WindowId::about) {
    layout_.frameOf(id).left = topLeft.x();
    layout_.frameOf(id).top = topLeft.y();
    return;
  }
  const QPointF current(layout_.frameOf(id).left, layout_.frameOf(id).top);
  const QPointF delta = topLeft - current;
  bool shouldUndock = shiftUndock;
  const double dist2 = QPointF::dotProduct(delta, delta);
  if (!shouldUndock && id != WindowId::main && dist2 > kPeelDelta * kPeelDelta && hasEdge(id)) {
    shouldUndock = true;
  }
  if (shouldUndock) {
    QVector<DockEdge> kept;
    for (const DockEdge& e : layout_.dockEdges) {
      if (e.a != id && e.b != id) kept.push_back(e);
    }
    layout_.dockEdges = kept;
  }
  const QSet<WindowId> cohort = moveCohortOf(id);
  for (WindowId member : cohort) {
    if (member == id) {
      layout_.frameOf(member).left = topLeft.x();
      layout_.frameOf(member).top = topLeft.y();
    } else {
      layout_.frameOf(member).left += delta.x();
      layout_.frameOf(member).top += delta.y();
    }
  }
  if (snap && !shiftUndock && id != WindowId::main) {
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
  } best;
  bool found = false;

  auto consider = [&](WindowId targetId, DockSide side, double distance, QPointF delta, bool aligned) {
    if (distance > snapThreshold_ || !aligned) return;
    if (!found || distance < best.distance) {
      best = {targetId, side, distance, delta};
      found = true;
    }
  };

  for (WindowId otherId : {WindowId::main, WindowId::equalizer, WindowId::playlist}) {
    if (otherId == id || group.contains(otherId) || !layout_.frameOf(otherId).visible) continue;
    const QRectF other = rectFor(otherId);
    consider(otherId, DockSide::bottom, std::abs(moving.bottom() - other.top()),
             QPointF(0, other.top() - moving.bottom()),
             overlapsOrNear1D(moving.left(), moving.right(), other.left(), other.right()));
    consider(otherId, DockSide::top, std::abs(moving.top() - other.bottom()),
             QPointF(0, other.bottom() - moving.top()),
             overlapsOrNear1D(moving.left(), moving.right(), other.left(), other.right()));
    if (id != WindowId::playlist) {
      consider(otherId, DockSide::right, std::abs(moving.right() - other.left()),
               QPointF(other.left() - moving.right(), 0),
               overlapsOrNear1D(moving.top(), moving.bottom(), other.top(), other.bottom()));
      consider(otherId, DockSide::left, std::abs(moving.left() - other.right()),
               QPointF(other.right() - moving.left(), 0),
               overlapsOrNear1D(moving.top(), moving.bottom(), other.top(), other.bottom()));
    }
  }
  if (!found) return;

  QPointF snapDelta = best.delta;
  const QRectF snapped = moving.translated(best.delta);
  const QRectF target = rectFor(best.target);
  std::optional<DockSide> ortho;
  if (best.side == DockSide::top || best.side == DockSide::bottom) {
    const double leftDist = std::abs(snapped.left() - target.left());
    const double rightDist = std::abs(snapped.right() - target.right());
    if (leftDist <= snapThreshold_ && leftDist <= rightDist) {
      snapDelta += QPointF(target.left() - snapped.left(), 0);
      ortho = DockSide::left;
    } else if (rightDist <= snapThreshold_) {
      snapDelta += QPointF(target.right() - snapped.right(), 0);
      ortho = DockSide::right;
    }
  } else {
    const double topDist = std::abs(snapped.top() - target.top());
    const double bottomDist = std::abs(snapped.bottom() - target.bottom());
    if (topDist <= snapThreshold_ && topDist <= bottomDist) {
      snapDelta += QPointF(0, target.top() - snapped.top());
      ortho = DockSide::top;
    } else if (bottomDist <= snapThreshold_) {
      snapDelta += QPointF(0, target.bottom() - snapped.bottom());
      ortho = DockSide::bottom;
    }
  }

  for (WindowId member : group) {
    layout_.frameOf(member).left += snapDelta.x();
    layout_.frameOf(member).top += snapDelta.y();
  }

  auto addEdge = [&](DockSide side) {
    for (const DockEdge& e : layout_.dockEdges) {
      const bool pair = (e.a == id && e.b == best.target) || (e.a == best.target && e.b == id);
      if (pair && (e.side == side || e.side == opposite(side))) return;
    }
    layout_.dockEdges.push_back({id, best.target, side});
  };
  addEdge(best.side);
  if (ortho) addEdge(*ortho);
}

}  // namespace tramp
