#include "host_window.h"

#include "chrome_paint.h"
#include "mockup_draw.h"
#include "skip_taskbar.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

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
  move(spec.origin);
  logo_ = tramp::loadTrampLogo();
  applyNativeSize();
  winId();
}

QSize HostWindow::paintLogical() const {
  if (shaded_) {
    return QSize(spec_.logicalSize.width(), tramp::kTitleBar);
  }
  return spec_.logicalSize;
}

void HostWindow::applyNativeSize() {
  title_ = tramp::TitleChromeLayout::forWindow(spec_.id, paintLogical());
  const QSize native = tramp::zoomed(paintLogical(), zoomPercent_);
  setFixedSize(native);
  update();
}

void HostWindow::setZoomPercent(int percent) {
  if (zoomPercent_ == percent) {
    return;
  }
  zoomPercent_ = percent;
  applyNativeSize();
}

void HostWindow::setShaded(bool shaded) {
  if (spec_.id == tramp::WindowId::main || shaded_ == shaded) {
    return;
  }
  shaded_ = shaded;
  applyNativeSize();
}

void HostWindow::setBodyChrome(const tramp::BodyChrome& chrome) {
  chrome_ = chrome;
  update();
}

QPoint HostWindow::logicalFrom(const QPointF& widgetPos) const {
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  return QPoint(int(widgetPos.x() / sx), int(widgetPos.y() / sy));
}

void HostWindow::applyHitCursor(const QPointF& widgetPos) {
  const QPoint logical = logicalFrom(widgetPos);
  const auto hit = title_.hit(logical);
  if (hit == tramp::TitleChromeLayout::Hit::drag) {
    setCursor(Qt::OpenHandCursor);
  } else if (hit != tramp::TitleChromeLayout::Hit::none) {
    setCursor(Qt::PointingHandCursor);
  } else if (spec_.id == tramp::WindowId::main &&
             (tramp::mainEqHit(spec_.logicalSize).contains(logical) ||
              tramp::mainPlHit(spec_.logicalSize).contains(logical) ||
              tramp::mainOptionsHit(spec_.logicalSize).contains(logical))) {
    setCursor(Qt::PointingHandCursor);
  } else {
    setCursor(Qt::ArrowCursor);
  }
}

void HostWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  p.scale(sx, sy);
  tramp::paintMockupWindow(p, logical, spec_.id, title_, &logo_, chrome_);
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
    event->accept();
    return;
  }
  event->ignore();
  hide();
  emit extraHidden();
}

void HostWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) {
    return;
  }
  const QPoint logical = logicalFrom(event->position());
  const auto hit = title_.hit(logical);
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
      setShaded(!shaded_);
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::zoomOut:
      emit zoomOutRequested();
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::zoomIn:
      emit zoomInRequested();
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

  if (spec_.id == tramp::WindowId::main) {
    if (tramp::mainEqHit(spec_.logicalSize).contains(logical)) {
      emit toggleEqualizer();
      event->accept();
      return;
    }
    if (tramp::mainPlHit(spec_.logicalSize).contains(logical)) {
      emit togglePlaylist();
      event->accept();
      return;
    }
    if (tramp::mainOptionsHit(spec_.logicalSize).contains(logical)) {
      emit openSettings();
      event->accept();
      return;
    }
  }
}

void HostWindow::mouseMoveEvent(QMouseEvent* event) {
  applyHitCursor(event->position());
  QWidget::mouseMoveEvent(event);
}
