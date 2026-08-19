#pragma once

#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QVector>
#include <optional>

namespace tramp {

struct HostShellLayout {
  QRect screenRect;   // native px, global host (virtual desktop); null if no panels
  QRegion localMask;  // host-local union of panels; empty if none
};

HostShellLayout hostShellLayout(QRect hostScreenRect, const QVector<QRect>& visiblePanelScreenRects);

/// Keep [panel] fully inside [host], shrinking it if it is larger than the host.
QRect clampRectToHost(QRect panel, QRect host);

/// Translation that keeps the union of [panels] inside [host] when the union fits.
/// Null if the union is larger than the host in either dimension.
std::optional<QPoint> clusterDeltaToFit(const QVector<QRect>& panels, QRect host);

/// Child top-left in host-local pixels from actual host origin, not a requested bbox origin.
QPoint panelLocalTopLeft(QPoint screenTopLeft, QPoint actualHostGlobal);

/// Native size for a panel. Zoomed logical size wins over an unmapped 0×0 widget.
QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize);

/// `TRAMP_LEGACY_DRAG=1` restores per-move punch and live paint during title drag.
bool legacyTitleDragPath();

/// Punch after place unless a title drag / playlist resize is holding the pointer.
/// Wayland cannot grabMouse on a normal window, so this is not `mouseGrabber()`.
inline bool placePanelsShouldPunch(bool legacyDrag, bool pointerBusy) {
  return legacyDrag || !pointerBusy;
}

}  // namespace tramp
