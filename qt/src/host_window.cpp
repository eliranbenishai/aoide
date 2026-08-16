#include "host_window.h"

#include "chrome_paint.h"
#include "skip_taskbar.h"
#include "tramp_fonts.h"

#include <QCoreApplication>
#include <QPainter>
#include <QWindow>

HostWindow::HostWindow(const tramp::WindowSpec& spec, QWidget* parent)
    : QWidget(parent),
      spec_(spec),
      title_(tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize)) {
  setAttribute(Qt::WA_TranslucentBackground);
  setMouseTracking(true);
  setWindowTitle(spec.title);
  setWindowFlags(tramp::hostWindowFlags(spec.skipTaskbar));
  setFixedSize(spec.size);
  move(spec.origin);

  logo_.load(tramp::assetPath("branding/app_icon.png"));

  winId();
}

QPoint HostWindow::logicalFrom(const QPointF& widgetPos) const {
  const qreal sx = qreal(width()) / qMax(1, spec_.logicalSize.width());
  const qreal sy = qreal(height()) / qMax(1, spec_.logicalSize.height());
  return QPoint(int(widgetPos.x() / sx), int(widgetPos.y() / sy));
}

void HostWindow::applyHitCursor(const QPointF& widgetPos) {
  const auto hit = title_.hit(logicalFrom(widgetPos));
  if (hit == tramp::TitleChromeLayout::Hit::drag) {
    setCursor(Qt::OpenHandCursor);
  } else if (hit == tramp::TitleChromeLayout::Hit::none) {
    setCursor(Qt::ArrowCursor);
  } else {
    setCursor(Qt::PointingHandCursor);
  }
}

void HostWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  const qreal sx = qreal(width()) / qMax(1, spec_.logicalSize.width());
  const qreal sy = qreal(height()) / qMax(1, spec_.logicalSize.height());
  p.scale(sx, sy);
  tramp::paintMockupWindow(p, spec_.logicalSize, spec_.id, title_, &logo_);
}

void HostWindow::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (spec_.skipTaskbar) {
    tramp::applySkipTaskbar(windowHandle());
  }
}

void HostWindow::closeEvent(QCloseEvent* event) {
  if (spec_.id == tramp::WindowId::main) {
    QCoreApplication::quit();
  }
  event->accept();
}

void HostWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  const auto hit = title_.hit(logicalFrom(event->position()));
  switch (hit) {
    case tramp::TitleChromeLayout::Hit::close:
      close();
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::minimize:
      showMinimized();
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::collapse:
    case tramp::TitleChromeLayout::Hit::zoomOut:
    case tramp::TitleChromeLayout::Hit::zoomIn:
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::drag:
      if (QWindow* win = windowHandle()) {
        win->startSystemMove();
      }
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::none:
      break;
  }
}

void HostWindow::mouseMoveEvent(QMouseEvent* event) {
  applyHitCursor(event->position());
  QWidget::mouseMoveEvent(event);
}
