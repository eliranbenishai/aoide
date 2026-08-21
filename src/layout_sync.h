#pragma once

#include "docking.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QPoint>
#include <QPointF>
#include <QRect>

namespace tramp {

/// Owns the layout: where each panel sits in logical space, and what rectangle
/// that becomes on the screen at the current zoom step. It knows nothing about
/// playback, skins or persistence, and it never touches a widget — which is what
/// makes the geometry checkable without a compositor.
class LayoutSync {
 public:
  explicit LayoutSync(DockLayout layout = {}, int zoomPercent = kDefaultZoomPercent);

  DockingCoordinator& docking() { return docking_; }
  const DockingCoordinator& docking() const { return docking_; }
  const DockLayout& layout() const { return docking_.layout(); }

  int zoomPercent() const { return zoomPercent_; }
  void setZoomPercent(int percent) { zoomPercent_ = percent; }

  QPointF nativeToLogical(QPoint native) const;
  QPoint logicalToNative(QPointF logical) const;

  /// Where [id] sits on the screen at the current zoom step.
  QRect nativeFrameRect(WindowId id) const;
  /// Put [id] at a screen rectangle. The frame is the only place it is kept —
  /// the widget and the settings file are both derived from it later.
  void setNativeFrame(WindowId id, QRect native);

 private:
  DockingCoordinator docking_;
  int zoomPercent_ = kDefaultZoomPercent;
};

}  // namespace tramp
