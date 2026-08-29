#pragma once

#include <QPoint>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QVector>
#include <optional>

namespace aoide {

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

/// Downward shift that clears [panel] of the strip a screen reserves at its top.
/// One-directional and vertical only: the furniture that can bury a title bar is
/// the menu bar / top bar / top taskbar. A title bar spans the panel, so a dock
/// along the side still leaves something to grab, and furniture along the bottom
/// cannot hide the handle. An unknown work area reserves nothing; a panel
/// already at or below the work-area top is left alone — including when that
/// top is negative, on a monitor above the primary.
int reservedTopLift(QRect panel, QRect workArea);

/// Child top-left in host-local pixels from actual host origin, not a requested bbox origin.
QPoint panelLocalTopLeft(QPoint screenTopLeft, QPoint actualHostGlobal);

/// Native size for a panel. Zoomed logical size wins over an unmapped 0×0 widget.
QSize panelNativeSize(QSize logicalZoomed, QSize widgetSize);

}  // namespace aoide
