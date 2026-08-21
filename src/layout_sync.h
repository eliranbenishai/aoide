#pragma once

#include "docking.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSet>
#include <QSize>
#include <QVector>

namespace tramp {

/// One panel's share of a placement pass.
struct PanelPlacement {
  WindowId id = WindowId::main;
  /// Native pixels, global. Meaningless when the panel is not visible.
  QRect screen;
  /// The logical canvas, before zoom and ignoring windowshade — the size the
  /// panel paints at and goes back to when it is unshaded.
  QSize logicalSize;
  bool shaded = false;
  /// False means hide the surface this pass. The panel keeps its frame: this is
  /// where it will reappear.
  bool visible = false;
};

/// The surfaces a layout is pushed onto. In the app the host shell and its five
/// panels satisfy this; the geometry tests satisfy it with a recorder, which is
/// the point — placement can be checked without a compositor.
class PanelSurfaces {
 public:
  virtual ~PanelSurfaces() = default;
  /// The virtual desktop every panel has to stay inside. Empty before there is
  /// a host, and nothing is clamped against an empty rectangle.
  virtual QRect hostRect() const = 0;
  /// All five panels arrive every pass, hidden ones included, because hiding is
  /// something this call has to do rather than something it can skip. A panel
  /// missing from the list would keep its pixels on the canvas while dropping
  /// out of the punch the shell builds from the visible ones.
  virtual void placePanels(const QVector<PanelPlacement>& panels) = 0;
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
  /// Bring the cluster back onto the virtual desktop after it, or the desktop,
  /// changed shape. Translating keeps the panels' relationship to each other;
  /// only a cluster that cannot fit at all falls back to clamping one at a time.
  void fitClusterToHost();

  /// Panels the listener cannot see because main is minimized. They keep their
  /// frames, so this is a veto over the placement rather than a second copy of
  /// what is visible.
  void setMainMinimized(bool minimized);

  /// Push every panel to its surface. Placing can re-enter — moving a widget
  /// makes it report a move — so a pass already running wins.
  void place();
  bool placing() const { return placing_; }

 private:
  QRect hostRect() const;

  DockingCoordinator docking_;
  PanelSurfaces* surfaces_ = nullptr;
  QSet<WindowId> suppressed_;
  int zoomPercent_ = kDefaultZoomPercent;
  bool placing_ = false;
};

}  // namespace tramp
