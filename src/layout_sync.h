#pragma once

#include "docking.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QPoint>
#include <QPointF>
#include <QRect>

namespace tramp {

/// The surfaces a layout is pushed onto. In the app the host shell and its five
/// panels satisfy this; the geometry tests satisfy it with a recorder, which is
/// the point — placement can be checked without a compositor.
class PanelSurfaces {
 public:
  virtual ~PanelSurfaces() = default;
  /// The virtual desktop every panel has to stay inside. Empty before there is
  /// a host, and nothing is clamped against an empty rectangle.
  virtual QRect hostRect() const = 0;
};

/// Owns the layout: where each panel sits in logical space, and what rectangle
/// that becomes on the screen at the current zoom step. It knows nothing about
/// playback, skins or persistence, and it never touches a widget — which is what
/// makes the geometry checkable without a compositor.
class LayoutSync {
 public:
  explicit LayoutSync(DockLayout layout = {}, int zoomPercent = kDefaultZoomPercent);

  void setSurfaces(PanelSurfaces* surfaces) { surfaces_ = surfaces; }

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

  /// Pull [id] back onto the virtual desktop, shrinking it if it is larger.
  void clampToHost(WindowId id);

 private:
  QRect hostRect() const;

  DockingCoordinator docking_;
  PanelSurfaces* surfaces_ = nullptr;
  int zoomPercent_ = kDefaultZoomPercent;
};

}  // namespace tramp
