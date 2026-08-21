#include "chrome_menu.h"

#include "mockup_draw.h"
#include "tramp_metrics.h"

#include <QCloseEvent>
#include <QColor>
#include <QEventLoop>
#include <QFont>
#include <QGuiApplication>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QScreen>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace tramp {
namespace {

QPoint clampToWorkArea(QPoint pos, const QSize& size, const QRect& anchorGlobal) {
  const QScreen* screen = QGuiApplication::screenAt(anchorGlobal.center());
  if (!screen) screen = QGuiApplication::primaryScreen();
  if (!screen) return pos;
  const QRect avail = screen->availableGeometry();
  // qMax guards a menu taller or wider than the work area, where the low bound
  // would otherwise pass the high one and std::clamp is undefined.
  pos.setX(std::clamp(pos.x(), avail.left(),
                      qMax(avail.left(), avail.right() - size.width() + 1)));
  pos.setY(std::clamp(pos.y(), avail.top(),
                      qMax(avail.top(), avail.bottom() - size.height() + 1)));
  return pos;
}

void drawCheck(QPainter& p, QPointF centre, const QColor& colour, qreal zoom) {
  const qreal size = 9 * zoom;
  QPainterPath tick;
  tick.moveTo(centre.x() - size * 0.38, centre.y() + size * 0.02);
  tick.lineTo(centre.x() - size * 0.10, centre.y() + size * 0.30);
  tick.lineTo(centre.x() + size * 0.40, centre.y() - size * 0.30);
  p.strokePath(tick, QPen(colour, qMax(qreal(1), 1.6 * zoom), Qt::SolidLine, Qt::RoundCap,
                          Qt::RoundJoin));
}

class ChromeMenuWindow : public QWidget {
 public:
  ChromeMenuWindow(QWidget* owner, const QVector<ChromeMenuItem>& items, int zoomPercent,
                   const ChromeTokens& look)
      // Qt::Popup is the only window type Qt can grab the pointer for on
      // Wayland ("This plugin supports grabbing the mouse only for popup
      // windows"), and it is what maps to xdg_popup, so the compositor both
      // stacks the menu over the panel and dismisses it on an outside click.
      // window_spec.h's Qt::Window-only rule is about the host panels, which
      // have to stay ordinary toplevels; a transient menu is what it excludes.
      //
      // The widget parent is load-bearing, not decoration. Qt picks the surface
      // role while it creates the platform window, and it reads the transient
      // parent to do it — an unparented popup measures NULL there and comes out
      // an ordinary toplevel, with no grab, no dismiss and no keys, whatever you
      // set afterwards. Do not reparent this to nullptr to make the lifetime
      // tidier; it holds the same shape as QMenu::exec for the same reason.
      //
      // Panels are non-native children of the host shell, so the surface the
      // compositor ends up hanging this off is the shell, which is what we want.
      : QWidget(owner, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
        items_(items),
        zoom_(qMax(1, zoomPercent) / 100.0),
        metrics_(chromeMenuMetrics(zoom_)),
        look_(look) {
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
  }

  int run(const QRect& anchorGlobal, PopupAnchor anchor) {
    const QSize wanted = chromeMenuSize(items_, widestLabel(), metrics_);
    setGeometry(QRect(clampToWorkArea(popupMenuPos(anchorGlobal, wanted, anchor), wanted,
                                      anchorGlobal),
                      wanted));
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
    // A compositor can be done with the popup before the loop starts — a
    // refused xdg_popup grab arrives as popup_done — and quit() on a loop that
    // is not running is dropped, which would hang here forever.
    if (!done_) loop_.exec();
    return choice_;
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    // mockup_draw helpers resolve skin colour through the thread-local look,
    // which is only live inside a panel paint. This popup is its own window.
    const LookPaintScope scope(look_);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const qreal radius = kShellRadius * zoom_;
    p.setBrush(look_.shellMid);
    p.setPen(QPen(withAlpha(look_.coolSheen, 0x33), 1));
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

    const QFont font = labelFont();
    // Rows stop short of the chassis edge so a full-width highlight cannot
    // spill past the rounded corners.
    const qreal bleed = qMax(qreal(1), 3 * zoom_);
    const qreal labelLeft = metrics_.padX + metrics_.checkColumn;
    for (int i = 0; i < int(items_.size()); ++i) {
      const ChromeMenuItem& item = items_[i];
      const QRectF row(bleed, chromeMenuRowTop(items_, i, metrics_), width() - bleed * 2,
                       chromeMenuRowHeight(item, metrics_));
      if (item.kind == ChromeMenuKind::separator) {
        const qreal y = std::round(row.center().y()) + 0.5;
        p.setPen(QPen(withAlpha(look_.coolSheen, 0x2e), 1));
        p.drawLine(QPointF(metrics_.padX, y), QPointF(width() - metrics_.padX, y));
        continue;
      }
      if (i == highlight_) {
        // Same lift as a selected playlist row (chrome_bodies.cpp).
        QLinearGradient lift(row.topLeft(), row.bottomLeft());
        lift.setColorAt(0, withAlpha(look_.phos, 33));
        lift.setColorAt(1, withAlpha(look_.phos, 10));
        p.fillRect(row, lift);
      }
      if (item.checkable && item.checked) {
        drawCheck(p, QPointF(metrics_.padX + metrics_.checkColumn / 2.0, row.center().y()),
                  item.enabled ? look_.phos : look_.inkFaint, zoom_);
      }
      p.setFont(font);
      p.setPen(item.enabled ? look_.ink : look_.inkFaint);
      p.drawText(QRectF(labelLeft, row.top(), width() - labelLeft - metrics_.padX, row.height()),
                 Qt::AlignVCenter | Qt::AlignLeft, item.label);
    }
  }

  void mouseMoveEvent(QMouseEvent* event) override {
    armed_ = true;
    setHighlight(rowUnder(event));
    event->accept();
  }

  void mousePressEvent(QMouseEvent* event) override {
    if (!rect().contains(event->position().toPoint())) {
      finish(kChromeMenuNone);
      return;
    }
    // The popup only exists from partway through the press that opened it, so a
    // press it does see is always a second, deliberate one.
    armed_ = true;
    setHighlight(rowUnder(event));
    event->accept();
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    // The release of the press that opened the menu lands in here. Clamping near
    // a screen edge can slide the popup under that press, so a release only
    // chooses once the pointer has moved or pressed again.
    const int row = armed_ ? rowUnder(event) : kChromeMenuNone;
    if (row != kChromeMenuNone) finish(row);
    event->accept();
  }

  void keyPressEvent(QKeyEvent* event) override {
    switch (event->key()) {
      case Qt::Key_Up:
        setHighlight(chromeMenuStep(items_, highlight_, -1));
        break;
      case Qt::Key_Down:
        setHighlight(chromeMenuStep(items_, highlight_, 1));
        break;
      case Qt::Key_Return:
      case Qt::Key_Enter:
        if (highlight_ != kChromeMenuNone) finish(highlight_);
        break;
      case Qt::Key_Escape:
        finish(kChromeMenuNone);
        break;
      default:
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
  }

  void hideEvent(QHideEvent* event) override {
    finish(kChromeMenuNone);
    QWidget::hideEvent(event);
  }

  void closeEvent(QCloseEvent* event) override {
    // Wayland delivers xdg_popup.popup_done as a close.
    finish(kChromeMenuNone);
    event->accept();
  }

 private:
  QFont labelFont() const { return condensedFont(metrics_.labelPx, 0.1); }

  qreal widestLabel() const {
    const QFont font = labelFont();
    qreal widest = 0;
    for (const ChromeMenuItem& item : items_) {
      if (item.kind == ChromeMenuKind::separator) continue;
      widest = qMax(widest, textWidth(font, item.label));
    }
    return widest;
  }

  int rowUnder(const QMouseEvent* event) const {
    const QPoint pos = event->position().toPoint();
    if (!rect().contains(pos)) return kChromeMenuNone;
    return chromeMenuRowAt(items_, pos.y(), metrics_);
  }

  void setHighlight(int row) {
    if (row == highlight_) return;
    highlight_ = row;
    update();
  }

  void finish(int choice) {
    if (done_) return;
    done_ = true;
    choice_ = choice;
    hide();
    loop_.quit();
  }

  QVector<ChromeMenuItem> items_;
  qreal zoom_ = 0.75;
  ChromeMenuMetrics metrics_{};
  ChromeTokens look_{};
  QEventLoop loop_;
  int highlight_ = kChromeMenuNone;
  int choice_ = kChromeMenuNone;
  bool armed_ = false;
  bool done_ = false;
};

}  // namespace

int execChromeMenu(QWidget* owner, const QVector<ChromeMenuItem>& items,
                   const QRect& anchorGlobal, PopupAnchor anchor, int zoomPercent,
                   const ChromeTokens& look) {
  if (!owner || items.isEmpty()) return kChromeMenuNone;
  ChromeMenuWindow menu(owner, items, zoomPercent, look);
  return menu.run(anchorGlobal, anchor);
}

}  // namespace tramp
