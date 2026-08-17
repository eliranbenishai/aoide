#pragma once

#include "chrome_hits.h"
#include "session_view.h"
#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QSize>

namespace tramp {

void paintWindowBody(QPainter& painter, WindowId id, QSize logical,
                     const QImage* logo = nullptr, const SessionView& view = {});

}  // namespace tramp
