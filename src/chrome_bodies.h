#pragma once

#include "chrome_anim.h"
#include "chrome_hits.h"
#include "session_view.h"
#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QSize>

namespace tramp {

enum class BodyPaint { full, chassis, live };

/// The demo state every golden dump and paint benchmark is measured in.
///
/// It is data rather than something the painters synthesise, so a caller can
/// photograph a state the demo does not open on — another settings tab, a
/// scrolled list, a disabled row — by saying so on the view. A painter that
/// overrode the view instead would make that state unreachable, and an
/// unreachable state is an unwatched one. `SessionView::golden()` only raises
/// the flag that marks a paint as the fidelity reference and holds animation
/// still; this fills that view in, and is what callers want.
SessionView goldenDemoView();

void paintWindowBody(QPainter& painter, WindowId id, QSize logical,
                     const QImage* logo = nullptr, const SessionView& view = {},
                     BodyPaint pass = BodyPaint::full,
                     const ChromePhases& phases = ChromePhases());

}  // namespace tramp
