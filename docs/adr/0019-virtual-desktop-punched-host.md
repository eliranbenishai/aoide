# Virtual-desktop punched host

Child-panel drag and playlist resize were janky because the host resized to the tight union and rebuilt the punch mask on every mouse-move. A KWin prototype — frameless host equal to the virtual desktop, `setMask` of panel rects, no host resize during drag — mapped and felt right. The host is now that overlay; panels move inside it.

## Status

Accepted

Supersedes [ADR 0017](0017-one-host-window-internal-panels.md) on host geometry, main-drag mechanism, and the rejected screen-sized overlay. One OS host, five internal panels, and punched gaps remain.

## Decision

The host window’s geometry is the **virtual desktop**: the bounding rectangle of every screen. It resizes only when that rectangle changes (monitor plug, resolution). It does not move or resize while a panel is dragged or the playlist is resized.

The punched mask is still the union of visible panel rects. Clicks in the gaps go through to whatever is underneath. An empty mask is not a punch: Qt Wayland treats it as the entire surface taking input, so the host never clears the mask while mapped.

Dragging the **main** title bar translates every panel frame by the same delta (cluster as a unit), including hidden settings/about. That is app-owned; not compositor `startSystemMove` (that would slide a virtual-desktop-sized window). Main still does not snap. Dragging EQ, playlist, settings, or about still moves only that panel.

Every panel stays fully on the virtual desktop (position and size clamped). On monitor unplug, if the cluster still fits, it is translated onto the remainder; otherwise each panel is clamped.

Always-on-top remains a host flag.

## Considered options

- Tight union at rest, skip per-move resize — rejected; panels must roam the virtual desktop without a host resize. Dragging is cardinal for v1.
- One screen’s work area — rejected; a panel on another monitor is in scope.
- Hang off the outer edge — rejected; the whole panel stays on the virtual desktop.
