#pragma once

#include "chrome_anim.h"
#include "chrome_hits.h"
#include "session_view.h"
#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QSize>

namespace aoide {

enum class BodyPaint { full, chassis, live };

void paintWindowBody(QPainter& painter, WindowId id, QSize logical,
                     const QImage* logo = nullptr, const SessionView& view = {},
                     BodyPaint pass = BodyPaint::full,
                     const ChromePhases& phases = ChromePhases());

}  // namespace aoide
