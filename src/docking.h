#pragma once

#include "settings.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <QVector>
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

/// Panels that hold the cluster against the edges of the desktop. A hidden panel
/// keeps the position it will reappear at, so counting it here reserves ghost
/// space — a closed About parked to main's left stopped main reaching the left
/// edge of the screen. Main is always a member; it cannot be hidden.
QVector<WindowId> visibleClusterMembers(const DockLayout& layout);

class DockingCoordinator {
 public:
  /// How far a drag has to travel to peel a dock edge. Measured per move event,
  /// not from where the panel was docked, so a drag slow enough to stay under it
  /// keeps its edges however far it goes — which is what Shift-undock is for.
  static constexpr double kPeelDelta = 8.0;

  explicit DockingCoordinator(DockLayout layout = {});

  DockLayout& layout() { return layout_; }
  const DockLayout& layout() const { return layout_; }
  void setSnapThreshold(double px) { snapThreshold_ = px; }
  double snapThreshold() const { return snapThreshold_; }

  /// Moves one panel, or the whole cluster when [id] is main. `shiftUndock`
  /// breaks [id]'s dock edges whatever the distance, and leaves the panel where
  /// it was dropped instead of snapping it back.
  void move(WindowId id, QPointF topLeft, bool shiftUndock, bool snap);
  void resizePlaylist(QSizeF logical);
  void setShaded(WindowId id, bool shaded);
  void setVisible(WindowId id, bool visible);
  /// Main is the host's reason to exist; closing it quits. It cannot be hidden.
  /// If main was stored invisible (corrupt persist), restore the default chrome trio.
  void ensureMainVisible();
  /// If [id] sits on top of main (same origin or heavy overlap), park it at the
  /// default offset: EQ flush below main, playlist flush to main's right.
  void nudgeOffMainIfStacked(WindowId id);
  QRectF rectFor(WindowId id) const;
  /// The panel's logical canvas, ignoring windowshade — the size it goes back
  /// to. Only the playlist's varies.
  QSizeF canvasSize(WindowId id) const;
  /// What docking measures the panel by: its canvas, collapsed to the title bar
  /// while it is shaded.
  QSizeF logicalSize(WindowId id) const;

 private:
  bool hasEdge(WindowId id) const;
  DockSide opposite(DockSide side) const;
  void trySnap(WindowId id);
  bool overlapsOrNear1D(double a0, double a1, double b0, double b1) const;
  QSet<WindowId> groupOf(WindowId id) const;

  DockLayout layout_;
  double snapThreshold_ = 20;
};

}  // namespace tramp
