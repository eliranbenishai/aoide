#pragma once

#include "title_chrome.h"
#include "window_spec.h"

#include <QImage>
#include <QPainter>
#include <QSize>

namespace tramp {

void paintMockupWindow(QPainter& painter,
                       QSize logical,
                       WindowId id,
                       const TitleChromeLayout& title,
                       const QImage* logo);

}  // namespace tramp
