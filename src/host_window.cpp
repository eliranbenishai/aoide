#include "host_window.h"

#include "chrome_layout.h"
#include "chrome_paint.h"
#include "mockup_draw.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QMimeData>
#include <QMoveEvent>
#include <QPainter>
#include <QUrl>
#include <cmath>

HostWindow::HostWindow(const tramp::WindowSpec& spec, QWidget* parent)
    : QWidget(parent),
      spec_(spec),
      title_(tramp::TitleChromeLayout::forWindow(spec.id, spec.logicalSize)) {
  setAttribute(Qt::WA_TranslucentBackground);
  setMouseTracking(true);
  setAcceptDrops(true);
  setWindowTitle(spec.title);
  if (!parent) {
    setWindowFlags(tramp::hostWindowFlags());
    move(spec.origin);
    winId();
  }
  logo_ = tramp::loadTrampLogo();
  applyNativeSize();
  if (parent && spec.id != tramp::WindowId::main) hide();
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
  invalidateChassis();
  update();
}

void HostWindow::setZoomPercent(int percent) {
  if (zoomPercent_ == percent && view_.zoomPercent == percent) return;
  zoomPercent_ = percent;
  view_.zoomPercent = percent;
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
  view_.zoomPercent = zoomPercent_;
  titleMarqueeLive_ = view_.scrollTitle && !view_.goldenDemo &&
                      view_.titleScrollMs > tramp::kMarqueeHoldMs;
  invalidateChassis();
  if (collectionChanged) applyNativeSize();
  update();
}

void HostWindow::applyEqualizer(const tramp::EqualizerSettings& eq) {
  const bool chrome = view_.eq.enabled != eq.enabled || view_.eq.auto_ != eq.auto_ ||
                      view_.eq.presetName != eq.presetName;
  view_.eq = eq;
  if (chrome) invalidateChassis();
  update();
}

void HostWindow::applyLiveReadouts(const tramp::MainLiveReadouts& live) {
  view_.positionMs = live.positionMs;
  view_.durationMs = live.durationMs;
  view_.showElapsed = live.showElapsed;
  view_.titleScrollMs = live.titleScrollMs;
  view_.spectrum = live.spectrum;
  view_.spectrumPeaks = live.spectrumPeaks;
  const bool marqueeLive = view_.scrollTitle && !view_.goldenDemo &&
                           live.titleScrollMs > tramp::kMarqueeHoldMs;
  if (titleMarqueeLive_ != marqueeLive) {
    titleMarqueeLive_ = marqueeLive;
    invalidateChassis();
  }
  if (spec_.id != tramp::WindowId::main || !chassisValid_ || shaded_) {
    update();
    return;
  }
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  auto mapRect = [&](qreal x, qreal y, qreal w, qreal h) {
    return QRect(int(std::floor(x * sx)), int(std::floor(y * sy)),
                 int(std::ceil(w * sx)) + 2, int(std::ceil(h * sy)) + 2);
  };
  // CRT well (clock + bars) and seek row — the only live pixels.
  update(mapRect(90, tramp::kTitleBar + 8, 720, 150));
  update(mapRect(16, tramp::kTitleBar + 198, 800, 52));
}

void HostWindow::invalidateChassis() { chassisValid_ = false; }

void HostWindow::grabPointerIfAllowed() {
  if (QGuiApplication::platformName() == QLatin1String("wayland")) return;
  grabMouse();
  grabbedPointer_ = true;
}

void HostWindow::releasePointerIfHeld() {
  if (!grabbedPointer_) return;
  grabbedPointer_ = false;
  releaseMouse();
}

void HostWindow::rebuildChassis() {
  const QSize logical = paintLogical();
  const QSize widget = size();
  const qreal dpr = qMax(devicePixelRatioF(), qreal(0.5));
  chassis_ = QImage(tramp::chromePaintBufferSize(widget, dpr), QImage::Format_ARGB32_Premultiplied);
  chassis_.setDevicePixelRatio(dpr);
  chassis_.fill(Qt::transparent);
  QPainter p(&chassis_);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  const qreal sx = qreal(qMax(1, widget.width())) / qMax(1, logical.width());
  const qreal sy = qreal(qMax(1, widget.height())) / qMax(1, logical.height());
  p.scale(sx, sy);
  tramp::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_,
                           tramp::BodyPaint::chassis);
  p.end();
  chassisValid_ = true;
}

void HostWindow::setAlwaysOnTop(bool on) {
  if (parentWidget()) return;
  const bool have = windowFlags().testFlag(Qt::WindowStaysOnTopHint);
  if (have == on) return;
  const bool vis = isVisible();
  setWindowFlag(Qt::WindowStaysOnTopHint, on);
  if (vis) show();
}

QPoint HostWindow::nativeTopLeft() const { return mapToGlobal(QPoint(0, 0)); }

void HostWindow::setPlaylistLogicalSize(QSize logical) {
  if (spec_.id != tramp::WindowId::playlist) return;
  if (spec_.logicalSize == logical) return;
  spec_.logicalSize = logical;
  applyNativeSize();
}

QPoint HostWindow::logicalFrom(const QPointF& widgetPos) const {
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  return QPoint(int(widgetPos.x() / sx), int(widgetPos.y() / sy));
}

QRect HostWindow::widgetRectFromLogical(const QRect& logical) const {
  const QSize ls = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, ls.width());
  const qreal sy = qreal(height()) / qMax(1, ls.height());
  return QRect(int(std::lround(logical.x() * sx)), int(std::lround(logical.y() * sy)),
               qMax(1, int(std::lround(logical.width() * sx))),
               qMax(1, int(std::lround(logical.height() * sy))));
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
    setCursor(Qt::ArrowCursor);
  } else if (hit.kind != tramp::ChromeHit::Kind::none) {
    setCursor(Qt::PointingHandCursor);
  } else {
    setCursor(Qt::ArrowCursor);
  }
}

void HostWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  if ((spec_.id == tramp::WindowId::main || spec_.id == tramp::WindowId::equalizer) &&
      !view_.goldenDemo && !shaded_) {
    if (!chassisValid_) rebuildChassis();
    p.drawImage(QPointF(0, 0), chassis_);
    p.scale(sx, sy);
    tramp::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_,
                             tramp::BodyPaint::live);
    return;
  }
  p.scale(sx, sy);
  tramp::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_);
}

void HostWindow::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (spec_.id != tramp::WindowId::main) emit extraMapped();
}

void HostWindow::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::DevicePixelRatioChange) {
    invalidateChassis();
    update();
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
      if (QWidget* top = window()) top->showMinimized();
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
      emit titleDragStarted();
      draggingTitle_ = true;
      grabOffset_ = event->globalPosition().toPoint() - mapToGlobal(QPoint(0, 0));
      grabPointerIfAllowed();
      event->accept();
      return;
    case tramp::TitleChromeLayout::Hit::none:
      break;
  }

  const auto chrome = tramp::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  if (chrome.kind == tramp::ChromeHit::Kind::plResize) {
    resizingPlaylist_ = true;
    grabPointerIfAllowed();
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
  if (draggingTitle_ && (event->buttons() & Qt::LeftButton)) {
    const QPoint newTopLeft = event->globalPosition().toPoint() - grabOffset_;
    emit nativeMoved(newTopLeft);
    event->accept();
    return;
  }
  if (resizingPlaylist_ && (event->buttons() & Qt::LeftButton)) {
    const QPoint global = event->globalPosition().toPoint();
    const QPoint origin = mapToGlobal(QPoint(0, 0));
    const QSize next(qMax(minimumWidth(), global.x() - origin.x()),
                     qMax(minimumHeight(), global.y() - origin.y()));
    emit nativeResized(next);
    event->accept();
    return;
  }
  if (draggingChrome_ && (event->buttons() & Qt::LeftButton)) {
    const QPoint logical = logicalFrom(event->position());
    emit chromeDragged(dragHit_, logical);
  }
  QWidget::mouseMoveEvent(event);
}

void HostWindow::mouseReleaseEvent(QMouseEvent* event) {
  const bool wasResizing = resizingPlaylist_;
  if (draggingTitle_ || resizingPlaylist_) releasePointerIfHeld();
  if (draggingTitle_) {
    draggingTitle_ = false;
    emit titleDragFinished();
  }
  resizingPlaylist_ = false;
  if (wasResizing) emit nativeResized(size());
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
  if (parentWidget()) return;
  emit nativeMoved(mapToGlobal(QPoint(0, 0)));
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
