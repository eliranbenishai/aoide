#pragma once

#include "look.h"
#include "popup_anchor.h"

#include <QRect>
#include <QSize>
#include <QString>
#include <QVector>
#include <QtGlobal>
#include <cmath>
#include <utility>

class QWidget;

namespace aoide {

enum class ChromeMenuKind { action, separator };

/// One row of a chrome menu. Separators are inert rules; disabled rows paint
/// dim and take neither the pointer nor the keyboard.
struct ChromeMenuItem {
  ChromeMenuKind kind = ChromeMenuKind::action;
  QString label;
  bool enabled = true;
  bool checkable = false;
  bool checked = false;

  static ChromeMenuItem action(QString label, bool enabled = true) {
    ChromeMenuItem item;
    item.label = std::move(label);
    item.enabled = enabled;
    return item;
  }

  static ChromeMenuItem check(QString label, bool checked, bool enabled = true) {
    ChromeMenuItem item = action(std::move(label), enabled);
    item.checkable = true;
    item.checked = checked;
    return item;
  }

  static ChromeMenuItem separator() {
    ChromeMenuItem item;
    item.kind = ChromeMenuKind::separator;
    item.enabled = false;
    return item;
  }
};

/// Rows the playlist "+" create menu presents. From-files is always live: an
/// empty list used to open a menu with every row dead.
inline QVector<ChromeMenuItem> createPlaylistMenuItems(bool hasTracks, bool hasSelection) {
  return {
      ChromeMenuItem::action(QStringLiteral("From files…")),
      ChromeMenuItem::action(QStringLiteral("From current playlist…"), hasTracks),
      ChromeMenuItem::action(QStringLiteral("From selection…"), hasSelection),
  };
}

/// No row: the menu was dismissed, or the point is not over a selectable row.
inline constexpr int kChromeMenuNone = -1;

inline bool chromeMenuSelectable(const ChromeMenuItem& item) {
  return item.kind == ChromeMenuKind::action && item.enabled;
}

/// Popup geometry at one zoom factor, in the popup's own pixels. Kept clear of
/// QWidget so layout, hit-testing and highlight movement stay testable alone.
struct ChromeMenuMetrics {
  int rowHeight = 0;
  int ruleHeight = 0;
  int padX = 0;
  int padY = 0;
  int checkColumn = 0;
  int trailing = 0;
  int labelPx = 0;
};

inline ChromeMenuMetrics chromeMenuMetrics(qreal zoom) {
  const auto scale = [zoom](int logical) {
    return qMax(1, int(std::lround(logical * zoom)));
  };
  ChromeMenuMetrics m;
  m.rowHeight = scale(26);
  m.ruleHeight = scale(7);
  m.padX = scale(8);
  m.padY = scale(5);
  m.checkColumn = scale(18);
  m.trailing = scale(22);
  m.labelPx = scale(13);
  return m;
}

inline int chromeMenuRowHeight(const ChromeMenuItem& item, const ChromeMenuMetrics& m) {
  return item.kind == ChromeMenuKind::separator ? m.ruleHeight : m.rowHeight;
}

inline int chromeMenuHeight(const QVector<ChromeMenuItem>& items, const ChromeMenuMetrics& m) {
  int height = m.padY * 2;
  for (const ChromeMenuItem& item : items) height += chromeMenuRowHeight(item, m);
  return height;
}

/// `widestLabel` is the widest label already measured in the label font, which
/// is the one thing here that needs a live QFont.
inline QSize chromeMenuSize(const QVector<ChromeMenuItem>& items, qreal widestLabel,
                            const ChromeMenuMetrics& m) {
  const int label = qMax(0, int(std::ceil(widestLabel)));
  return QSize(m.padX * 2 + m.checkColumn + label + m.trailing, chromeMenuHeight(items, m));
}

inline int chromeMenuRowTop(const QVector<ChromeMenuItem>& items, int index,
                            const ChromeMenuMetrics& m) {
  const int last = qMin(index, int(items.size()));
  int top = m.padY;
  for (int i = 0; i < last; ++i) top += chromeMenuRowHeight(items[i], m);
  return top;
}

/// Selectable row containing `y`, or `kChromeMenuNone` for the chassis padding,
/// rules and disabled rows. Rows are hot across the full popup width.
inline int chromeMenuRowAt(const QVector<ChromeMenuItem>& items, int y,
                           const ChromeMenuMetrics& m) {
  int top = m.padY;
  for (int i = 0; i < int(items.size()); ++i) {
    const int height = chromeMenuRowHeight(items[i], m);
    if (y >= top && y < top + height) {
      return chromeMenuSelectable(items[i]) ? i : kChromeMenuNone;
    }
    top += height;
  }
  return kChromeMenuNone;
}

/// Highlight one step from `from` in `direction` (+1 Down, -1 Up), skipping
/// rules and disabled rows and wrapping at the ends. `kChromeMenuNone` for
/// `from` enters the list at the near end.
inline int chromeMenuStep(const QVector<ChromeMenuItem>& items, int from, int direction) {
  const int count = int(items.size());
  if (count == 0) return kChromeMenuNone;
  const int step = direction < 0 ? -1 : 1;
  int cursor = (from >= 0 && from < count) ? from : (step > 0 ? -1 : count);
  for (int tried = 0; tried < count; ++tried) {
    cursor += step;
    if (cursor < 0) cursor = count - 1;
    else if (cursor >= count) cursor = 0;
    if (chromeMenuSelectable(items[cursor])) return cursor;
  }
  return kChromeMenuNone;
}

/// Blocking menu painted in the app's own chrome. Returns the chosen row, or
/// `kChromeMenuNone` when it was dismissed. `anchorGlobal` is the trigger
/// button in screen coordinates; `owner` is the panel the menu belongs to, and
/// its top-level window becomes the popup's transient parent.
int execChromeMenu(QWidget* owner, const QVector<ChromeMenuItem>& items,
                   const QRect& anchorGlobal, PopupAnchor anchor, qreal zoomPercent,
                   const ChromeTokens& look);

}  // namespace aoide
