#include "host_window.h"

#include "chrome_paint.h"
#include "mockup_draw.h"
#include "skip_taskbar.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QCoreApplication>
#include <QMimeData>
#include <QMoveEvent>
#include <QPainter>
#include <QUrl>
#include <QWindow>

HostWindow::HostWindow(const tramp::WindowSpec& spec, QWidget* parent)
    : QWidget(parent),
      spec_(spec),
      title_(tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize)) {
  setAttribute(Qt::WA_TranslucentBackground);
  setMouseTracking(true);
  setAcceptDrops(true);
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
  if (spec_.id == tramp::WindowId::playlist && !shaded_) {
    const QSize min = tramp::zoomed(view_.collectionCollapsed ? tramp::kPlaylistMin
                                                              : tramp::kPlaylistMinWithCollection,
                                    zoomPercent_);
    setMinimumSize(min);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    const QSize target = native.expandedTo(min);
    if (size() != target) resize(target);
  } else {
    setFixedSize(native);
  }
  update();
}

void HostWindow::setZoomPercent(int percent) {
  if (zoomPercent_ == percent) return;
  zoomPercent_ = percent;
  applyNativeSize();
}

void HostWindow::setShaded(bool shaded) {
  if (spec_.id == tramp::WindowId::main || shaded_ == shaded) return;
  shaded_ = shaded;
  applyNativeSize();
  emit shadedChanged(shaded_);
}

void HostWindow::setSessionView(const tramp::SessionView& view) {
  const bool collectionChanged =
      spec_.id == tramp::WindowId::playlist && view_.collectionCollapsed != view.collectionCollapsed;
  view_ = view;
  if (collectionChanged) applyNativeSize();
  update();
}

void HostWindow::setAlwaysOnTop(bool on) {
  const bool have = windowFlags().testFlag(Qt::WindowStaysOnTopHint);
  if (have == on) return;
  const bool vis = isVisible();
  setWindowFlag(Qt::WindowStaysOnTopHint, on);
  if (vis) {
    show();
    if (spec_.skipTaskbar) tramp::applySkipTaskbar(windowHandle());
  }
}

void HostWindow::setPlaylistLogicalSize(QSize logical) {
  if (spec_.id != tramp::WindowId::playlist) return;
  spec_.logicalSize = logical;
  applyNativeSize();
}

QPoint HostWindow::logicalFrom(const QPointF& widgetPos) const {
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  return QPoint(int(widgetPos.x() / sx), int(widgetPos.y() / sy));
}

void HostWindow::applyHitCursor(const QPointF& widgetPos) {
  const QPoint logical = logicalFrom(widgetPos);
  const auto titleHit = title_.hit(logical);
  if (titleHit == tramp::TitleChromeLayout::Hit::drag) {
    setCursor(Qt::OpenHandCursor);
    return;
  }
  if (titleHit != tramp::TitleChromeLayout::Hit::none) {
    setCursor(Qt::PointingHandCursor);
    return;
  }
  const auto hit = tramp::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  if (hit.kind == tramp::ChromeHit::Kind::plResize) {
    setCursor(Qt::SizeFDiagCursor);
  } else if (hit.kind == tramp::ChromeHit::Kind::plDivider) {
    setCursor(Qt::SplitHCursor);
  } else if (hit.kind == tramp::ChromeHit::Kind::volume || hit.kind == tramp::ChromeHit::Kind::seek ||
             hit.kind == tramp::ChromeHit::Kind::eqPreamp || hit.kind == tramp::ChromeHit::Kind::eqBand) {
    setCursor(Qt::SizeHorCursor);
  } else if (hit.kind != tramp::ChromeHit::Kind::none) {
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
  tramp::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_);
}

void HostWindow::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (spec_.skipTaskbar) {
    tramp::applySkipTaskbar(windowHandle());
  }
}

void HostWindow::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (spec_.id != tramp::WindowId::main) return;
  if (event->type() == QEvent::WindowStateChange) {
    emit mainMinimized(windowState() & Qt::WindowMinimized);
  } else if (event->type() == QEvent::WindowActivate) {
    emit mainActivated();
  }
}

void HostWindow::closeEvent(QCloseEvent* event) {
  if (spec_.id == tramp::WindowId::main) {
    if (quitConfirmer_ && !quitConfirmer_()) {
      event->ignore();
      return;
    }
    emit aboutToQuit();
    QCoreApplication::quit();
    event->accept();
    return;
  }
  event->ignore();
  hide();
  emit extraHidden();
}

void HostWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) return;
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

  const auto chrome = tramp::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  if (chrome.kind == tramp::ChromeHit::Kind::plResize) {
    if (QWindow* win = windowHandle()) {
      win->startSystemResize(Qt::BottomEdge | Qt::RightEdge);
    }
    event->accept();
    return;
  }
  if (chrome.kind != tramp::ChromeHit::Kind::none) {
    draggingChrome_ = true;
    dragHit_ = chrome;
    emit chromePressed(chrome, event->modifiers(), logical);
    event->accept();
  }
}

void HostWindow::mouseMoveEvent(QMouseEvent* event) {
  applyHitCursor(event->position());
  if (draggingChrome_ && (event->buttons() & Qt::LeftButton)) {
    const QPoint logical = logicalFrom(event->position());
    emit chromeDragged(dragHit_, logical);
  }
  QWidget::mouseMoveEvent(event);
}

void HostWindow::mouseReleaseEvent(QMouseEvent* event) {
  if (draggingChrome_) {
    draggingChrome_ = false;
    emit chromeReleased();
  }
  QWidget::mouseReleaseEvent(event);
}

void HostWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) return;
  const QPoint logical = logicalFrom(event->position());
  const auto chrome = tramp::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  if (chrome.kind == tramp::ChromeHit::Kind::plTrackRow) {
    emit trackActivated(chrome.index);
    event->accept();
    return;
  }
  QWidget::mouseDoubleClickEvent(event);
}

void HostWindow::wheelEvent(QWheelEvent* event) {
  emit wheelScrolled(event->angleDelta().y());
}

void HostWindow::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  emit nativeMoved(event->pos());
}

void HostWindow::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (spec_.id == tramp::WindowId::playlist && !shaded_) {
    const qreal z = zoomPercent_ / 100.0;
    spec_.logicalSize = QSize(int(width() / z), int(height() / z));
    title_ = tramp::TitleChromeLayout::forWindow(spec_.id, paintLogical());
    emit nativeResized(size());
  }
}

void HostWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void HostWindow::dropEvent(QDropEvent* event) {
  QStringList paths;
  for (const QUrl& url : event->mimeData()->urls()) {
    if (url.isLocalFile()) paths << url.toLocalFile();
  }
  if (!paths.isEmpty()) {
    emit filesDropped(paths);
    event->acceptProposedAction();
  }
}
