#pragma once

#include "settings.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <optional>

namespace tramp {

struct DockLayout {
  WindowFrame main = WindowFrame::mainDefault();
  WindowFrame equalizer = WindowFrame::equalizerDefault();
  WindowFrame playlist = WindowFrame::playlistDefault();
  WindowFrame settings = WindowFrame::settingsDefault();
  WindowFrame about = WindowFrame::aboutDefault();
  QVector<DockEdge> dockEdges;

  WindowFrame& frameOf(WindowId id);
  const WindowFrame& frameOf(WindowId id) const;
};

class DockingCoordinator {
 public:
  static constexpr double kPeelDelta = 8.0;
  static constexpr double kUndockSeparation = 48.0;

  explicit DockingCoordinator(DockLayout layout = {});

  DockLayout& layout() { return layout_; }
  const DockLayout& layout() const { return layout_; }
  void setSnapThreshold(double px) { snapThreshold_ = px; }
  double snapThreshold() const { return snapThreshold_; }

  void move(WindowId id, QPointF topLeft, bool shiftUndock, bool snap);
  void resizePlaylist(QSizeF logical);
  void setShaded(WindowId id, bool shaded);
  void setVisible(WindowId id, bool visible);
  QSet<WindowId> moveCohortOf(WindowId id) const;
  QSet<WindowId> groupOf(WindowId id) const;
  QRectF rectFor(WindowId id) const;
  QSizeF logicalSize(WindowId id) const;

 private:
  bool hasEdge(WindowId id) const;
  DockSide opposite(DockSide side) const;
  void trySnap(WindowId id);
  bool overlapsOrNear1D(double a0, double a1, double b0, double b1) const;

  DockLayout layout_;
  double snapThreshold_ = 20;
  bool stickyMoveGroups_ = true;
};

}  // namespace tramp
