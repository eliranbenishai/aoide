#pragma once

#include "chrome_hits.h"
#include "look.h"
#include "session_view.h"
#include "title_chrome.h"

#include <QPoint>
#include <QString>

namespace aoide {

inline constexpr int kTooltipWaitMs = 450;

enum class TooltipMotion { hide, restartWait, keep };

/// Hover label for a title-bar or chrome hit. Empty means no tip (sliders,
/// list rows, skins preview cells, title-bar drag, empty space).
QString chromeTooltip(TitleChromeLayout::Hit title, const ChromeHit& chrome,
                      const SessionView& view);

/// What to do when the hover label changes. `busy` is title-bar drag,
/// playlist resize, chrome drag, or the wait cursor.
TooltipMotion tooltipMotion(const QString& previous, const QString& next, bool busy,
                            bool sameControl = true);

void showChromeTooltip(QPoint globalAbove, const QString& text, qreal zoomPercent,
                       const ChromeTokens& look);
void hideChromeTooltip();

}  // namespace aoide
