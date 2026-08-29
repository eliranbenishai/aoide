#include "host_window.h"

#include "chrome_layout.h"
#include "chrome_paint.h"
#include "chrome_tooltip.h"
#include "mockup_draw.h"
#include "track.h"
#include "aoide_fonts.h"
#include "aoide_metrics.h"
#include "wait_cursor.h"

#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QMimeData>
#include <QMoveEvent>
#include <QPainter>
#include <QUrl>
#include <cmath>
#include <utility>

namespace {

QSize playlistMinNative(const aoide::SessionView& view, qreal zoomPercent) {
  const qreal totalW = aoide::playlistStripTotalWidth(
      aoide::textWidth(aoide::condensedFont(11, 0.2), QStringLiteral("TOTAL")),
      aoide::textWidth(aoide::monoFont(18), aoide::formatClock(view.playlistTotalMs)));
  const qreal col = view.collectionCollapsed ? 0 : view.collectionWidth;
  return aoide::zoomed(aoide::playlistMinLogical(col, totalW), zoomPercent);
}

bool platformAllowsPointerGrab() {
  const QString name = QGuiApplication::platformName();
  // Qt Wayland prints a warning and does nothing on a non-popup toplevel.
  if (name == QLatin1String("wayland")) return false;
  // Unverified on a punched virtual-desktop host. Enable if a title-bar drag
  // loses the pointer off the panel; keep skipped if grabMouse sticks.
  if (name == QLatin1String("cocoa")) return false;
  return true;
}

}  // namespace

HostWindow::HostWindow(const aoide::WindowSpec& spec, QWidget* parent)
    : QWidget(parent),
      spec_(spec),
      title_(aoide::TitleChromeLayout::forWindow(spec.id, spec.logicalSize)) {
  setAttribute(Qt::WA_TranslucentBackground);
  setMouseTracking(true);
  setAcceptDrops(true);
  setWindowTitle(spec.title);
  if (!parent) {
    setWindowFlags(aoide::hostWindowFlags());
    move(spec.origin);
    winId();
  }
  logo_ = aoide::loadAoideLogo();
  phases_.setLive(true);
  // A transition rebuilds the chassis per frame, which the paint budget only
  // affords because a pointer cannot hover a button and drag a panel at once.
  // The step is driven by the wall clock, so a panel too slow to hit this
  // interval still finishes in kBtnTransitionMs — it just draws fewer frames.
  animTimer_.setInterval(16);
  connect(&animTimer_, &QTimer::timeout, this, &HostWindow::stepButtonAnimation);
  tooltipTimer_.setSingleShot(true);
  connect(&tooltipTimer_, &QTimer::timeout, this, [this]() {
    if (tooltipCandidate_.isEmpty()) return;
    aoide::showChromeTooltip(tooltipGlobal_, tooltipCandidate_, zoomPercent_, view_.look);
  });
  collectionClickTimer_.setSingleShot(true);
  connect(&collectionClickTimer_, &QTimer::timeout, this, &HostWindow::onDeferredCollectionClick);
  applyNativeSize();
  if (parent && spec.id != aoide::WindowId::main) hide();
}

QSize HostWindow::paintLogical() const {
  if (shaded_) {
    return QSize(spec_.logicalSize.width(), aoide::kTitleBar);
  }
  return spec_.logicalSize;
}

void HostWindow::applyNativeSize() {
  title_ = aoide::TitleChromeLayout::forWindow(spec_.id, paintLogical());
  const QSize native = aoide::zoomed(paintLogical(), zoomPercent_);
  if (spec_.id == aoide::WindowId::playlist && !shaded_) {
    const QSize min = playlistMinNative(view_, zoomPercent_);
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

void HostWindow::setZoomPercent(qreal percent) {
  if (aoide::zoomPercentsEqual(zoomPercent_, percent) &&
      aoide::zoomPercentsEqual(view_.zoomPercent, percent)) {
    return;
  }
  zoomPercent_ = percent;
  view_.zoomPercent = percent;
  hideChromeTooltipNow();
  applyNativeSize();
}

void HostWindow::setShaded(bool shaded) {
  if (spec_.id == aoide::WindowId::main || shaded_ == shaded) return;
  shaded_ = shaded;
  applyNativeSize();
  emit shadedChanged(shaded_);
}

void HostWindow::setSessionView(const aoide::SessionView& view) {
  const bool playlistMinDirty =
      spec_.id == aoide::WindowId::playlist &&
      (view_.collectionCollapsed != view.collectionCollapsed ||
       view_.collectionWidth != view.collectionWidth ||
       view_.playlistTotalMs != view.playlistTotalMs);
  // Every snapshot goes to every panel, so most of what arrives here is a
  // change some other panel asked for. Keep the cached raster when this one
  // paints the same either way — the raster only. The view, the latched phases
  // and the repaint below are what the rest of the panel reads, and none of
  // them is conditional.
  //
  // Compared against the view this panel already holds, zoom included, because
  // that is what its raster was built from. `applyLiveReadouts` and
  // `applyEqualizer` do move that view ahead of the raster, but only in fields
  // that are painted outside it, which is what lets them.
  aoide::SessionView incoming = view;
  incoming.zoomPercent = zoomPercent_;
  const bool rerasterise = !sawFirstView_ || !aoide::paintsSame(spec_.id, view_, incoming);
  view_ = std::move(incoming);
  titleMarqueeLive_ = aoide::titleMarqueeRunning(view_);
  syncLatchedPhases(!sawFirstView_);
  sawFirstView_ = true;
  if (rerasterise) invalidateChassis();
  if (playlistMinDirty) applyNativeSize();
  // A wait cursor means a blocking load is about to start and this is the last
  // chance the chrome has to reach the screen before it does. Synchronous
  // whether or not the raster was kept: the point is the pixels arriving now,
  // and a skipped repaint here is the pre-load chrome never showing at all.
  if (aoide::WaitCursorScope::showing()) {
    repaint();
    return;
  }
  update();
}

void HostWindow::applyEqualizer(const aoide::EqualizerSettings& eq) {
  const bool chrome = view_.eq.enabled != eq.enabled || view_.eq.auto_ != eq.auto_ ||
                      view_.eq.presetName != eq.presetName;
  view_.eq = eq;
  if (chrome) {
    syncLatchedPhases(false);
    invalidateChassis();
  }
  update();
}

void HostWindow::applyLiveReadouts(const aoide::MainLiveReadouts& live) {
  view_.positionMs = live.positionMs;
  view_.durationMs = live.durationMs;
  view_.showElapsed = live.showElapsed;
  view_.titleScrollMs = live.titleScrollMs;
  view_.spectrum = live.spectrum;
  view_.spectrumPeaks = live.spectrumPeaks;
  const bool marqueeLive = aoide::titleMarqueeRunning(view_);
  if (titleMarqueeLive_ != marqueeLive) {
    titleMarqueeLive_ = marqueeLive;
    invalidateChassis();
  }
  if (spec_.id != aoide::WindowId::main || !chassisValid_ || shaded_) {
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
  update(mapRect(90, aoide::kTitleBar + 8, 720, 150));
  update(mapRect(16, aoide::kTitleBar + 198, 800, 52));
}

void HostWindow::invalidateChassis() { chassisValid_ = false; }

void HostWindow::startButtonAnimation() {
  if (!phases_.moving()) return;
  if (animTimer_.isActive()) return;
  animClock_.restart();
  animTimer_.start();
}

void HostWindow::stepButtonAnimation() {
  const qreal dtMs = qreal(animClock_.restart());
  const bool busy = phases_.advance(dtMs);
  invalidateChassis();
  update();
  if (!busy) animTimer_.stop();
}

void HostWindow::syncLatchedPhases(bool snap) {
  using K = aoide::ChromeHit::Kind;
  using aoide::BtnChannel;
  auto aim = [&](K kind, bool on, bool immediate = false) {
    if (snap || immediate) {
      phases_.snapTo(kind, -1, BtnChannel::on, on ? 1 : 0);
    } else {
      phases_.setTarget(kind, -1, BtnChannel::on, on ? 1 : 0);
    }
  };
  switch (spec_.id) {
    case aoide::WindowId::main:
      aim(K::mute, view_.muted);
      aim(K::mono, view_.forceMono);
      aim(K::eqToggle, view_.eqOn);
      aim(K::plToggle, view_.plOn);
      aim(K::skins, view_.skinsOn);
      aim(K::play, view_.playing);
      aim(K::pause, view_.paused);
      aim(K::shuffle, view_.shuffle);
      aim(K::repeat, view_.repeat != aoide::RepeatMode::off);
      break;
    case aoide::WindowId::equalizer:
      aim(K::eqOn, view_.eq.enabled);
      aim(K::eqAuto, view_.eq.auto_);
      break;
    case aoide::WindowId::playlist:
      aim(K::plPlay, view_.playing);
      // Refresh's `on` tracks an ingest, not a press on this button — a drop or
      // an open lights it too — and an ingest can be over inside
      // `kBtnTransitionMs`. Eased, the short ones would manage a dim blip and
      // nothing else. Hover and press still fade.
      aim(K::plRefresh, view_.playlistRefreshing, true);
      break;
    case aoide::WindowId::settings:
      aim(K::settingsGeneral, view_.settingsTab == 0);
      aim(K::settingsAudio, view_.settingsTab == 1);
      aim(K::settingsResume, view_.resumeLastSession);
      aim(K::settingsConfirm, view_.confirmBeforeQuit);
      aim(K::settingsScroll, view_.scrollTitle);
      aim(K::settingsMinimize, view_.minimizeHidesSecondaries);
      aim(K::settingsSnapOff, view_.dockSnap == 0);
      aim(K::settingsSnapNormal, view_.dockSnap == 1);
      aim(K::settingsSnapStrong, view_.dockSnap == 2);
      aim(K::settingsExclusive, view_.audioExclusive);
      break;
    case aoide::WindowId::about:
      break;
    case aoide::WindowId::skins:
      break;
  }
  if (!snap) startButtonAnimation();
}

void HostWindow::trackPointer(std::optional<QPointF> widgetPos, bool pressed) {
  using aoide::BtnChannel;
  phases_.releaseChannel(BtnChannel::hover);
  phases_.releaseChannel(BtnChannel::press);
  // A held button keeps its press even if the pointer slides off, matching how
  // a real button behaves under a finger; a wait cursor means nothing is live.
  if (widgetPos && !aoide::WaitCursorScope::showing()) {
    const QPoint logical = logicalFrom(*widgetPos);
    const auto titleHit = title_.hit(logical);
    if (titleHit != aoide::TitleChromeLayout::Hit::none &&
        titleHit != aoide::TitleChromeLayout::Hit::drag) {
      phases_.setTitleTarget(titleHit, BtnChannel::hover, 1);
      if (pressed) phases_.setTitleTarget(titleHit, BtnChannel::press, 1);
    } else if (titleHit == aoide::TitleChromeLayout::Hit::none) {
      const auto hit = aoide::hitTest(spec_.id, spec_.logicalSize, logical, view_);
      if (aoide::takesPointerFeedback(hit.kind) && aoide::chromeHitEnabled(hit, view_)) {
        phases_.setTarget(hit.kind, hit.index, BtnChannel::hover, 1);
        if (pressed) phases_.setTarget(hit.kind, hit.index, BtnChannel::press, 1);
      }
    }
  }
  startButtonAnimation();
}

void HostWindow::grabPointerIfAllowed() {
  if (!platformAllowsPointerGrab()) return;
  grabMouse();
  grabbedPointer_ = true;
}

void HostWindow::releasePointerIfHeld() {
  if (!grabbedPointer_) return;
  grabbedPointer_ = false;
  releaseMouse();
}

bool HostWindow::hasLiveBody() const {
  return (spec_.id == aoide::WindowId::main || spec_.id == aoide::WindowId::equalizer) && !shaded_;
}

void HostWindow::rebuildChassis() {
  const QSize logical = paintLogical();
  const QSize widget = size();
  const qreal dpr = qMax(devicePixelRatioF(), qreal(0.5));
  chassis_ = QImage(aoide::chromePaintBufferSize(widget, dpr), QImage::Format_ARGB32_Premultiplied);
  chassis_.setDevicePixelRatio(dpr);
  chassis_.fill(Qt::transparent);
  QPainter p(&chassis_);
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  const qreal sx = qreal(qMax(1, widget.width())) / qMax(1, logical.width());
  const qreal sy = qreal(qMax(1, widget.height())) / qMax(1, logical.height());
  p.scale(sx, sy);
  // Panels with a live body cache only their static chrome; the rest have no
  // per-frame content at all, so the whole paint is cacheable.
  chassisIsFullPaint_ = !hasLiveBody();
  aoide::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_,
                           chassisIsFullPaint_ ? aoide::BodyPaint::full : aoide::BodyPaint::chassis,
                           phases_);
  p.end();
  chassisValid_ = true;
}

/// Rebuilds when the cache is stale, the widget was resized, or the split
/// changed. Keying on the buffer size means a resize cannot show stale pixels
/// even if some path forgets to invalidate.
void HostWindow::ensureChassis() {
  const qreal dpr = qMax(devicePixelRatioF(), qreal(0.5));
  if (chassisValid_ && chassisIsFullPaint_ == !hasLiveBody() &&
      chassis_.size() == aoide::chromePaintBufferSize(size(), dpr)) {
    return;
  }
  rebuildChassis();
  paintStats_.chassisBuilds += 1;
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
  if (spec_.id != aoide::WindowId::playlist) return;
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

void HostWindow::hideChromeTooltipNow() {
  tooltipTimer_.stop();
  tooltipCandidate_.clear();
  tooltipTitle_ = aoide::TitleChromeLayout::Hit::none;
  tooltipChrome_ = {};
  aoide::hideChromeTooltip();
}

QRect HostWindow::tooltipAnchorRect(aoide::TitleChromeLayout::Hit title,
                                    const aoide::ChromeHit& chrome) const {
  using Hit = aoide::TitleChromeLayout::Hit;
  switch (title) {
    case Hit::minimize:
    case Hit::collapse:
      return title_.minimize;
    case Hit::zoomOut:
      return title_.zoomOut;
    case Hit::zoomIn:
      return title_.zoomIn;
    case Hit::close:
      return title_.close;
    case Hit::drag:
    case Hit::none:
      break;
  }
  return chrome.rect;
}

void HostWindow::applyChromeTooltip(const QPointF& widgetPos) {
  const bool busy = draggingTitle_ || resizingPlaylist_ || draggingChrome_ ||
                    aoide::WaitCursorScope::showing();
  const QPoint logical = logicalFrom(widgetPos);
  const auto titleHit = title_.hit(logical);
  const auto chrome = aoide::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  const QString next = aoide::chromeTooltip(titleHit, chrome, view_);
  const bool sameControl = titleHit == tooltipTitle_ && chrome.kind == tooltipChrome_.kind &&
                           chrome.index == tooltipChrome_.index;
  switch (aoide::tooltipMotion(tooltipCandidate_, next, busy, sameControl)) {
    case aoide::TooltipMotion::hide:
      hideChromeTooltipNow();
      break;
    case aoide::TooltipMotion::restartWait: {
      aoide::hideChromeTooltip();
      tooltipCandidate_ = next;
      tooltipTitle_ = titleHit;
      tooltipChrome_ = chrome;
      QRect logicalRect = tooltipAnchorRect(titleHit, chrome);
      if (logicalRect.isEmpty()) logicalRect = QRect(logical, QSize(1, 1));
      const QRect widgetRect = widgetRectFromLogical(logicalRect);
      tooltipGlobal_ = mapToGlobal(QPoint(widgetRect.center().x(), widgetRect.top()));
      tooltipTimer_.start(aoide::kTooltipWaitMs);
      break;
    }
    case aoide::TooltipMotion::keep:
      break;
  }
}

void HostWindow::applyHitCursor(const QPointF& widgetPos) {
  if (aoide::WaitCursorScope::showing()) {
    setCursor(Qt::WaitCursor);
    hideChromeTooltipNow();
    return;
  }
  const QPoint logical = logicalFrom(widgetPos);
  const auto titleHit = title_.hit(logical);
  if (titleHit == aoide::TitleChromeLayout::Hit::drag) {
    setCursor(Qt::OpenHandCursor);
  } else if (titleHit != aoide::TitleChromeLayout::Hit::none) {
    setCursor(Qt::PointingHandCursor);
  } else {
    const auto hit = aoide::hitTest(spec_.id, spec_.logicalSize, logical, view_);
    if (hit.kind == aoide::ChromeHit::Kind::plResize) {
      setCursor(Qt::SizeFDiagCursor);
    } else if (hit.kind == aoide::ChromeHit::Kind::plDivider) {
      setCursor(Qt::SplitHCursor);
    } else if (hit.kind == aoide::ChromeHit::Kind::volume || hit.kind == aoide::ChromeHit::Kind::seek ||
               hit.kind == aoide::ChromeHit::Kind::eqPreamp ||
               hit.kind == aoide::ChromeHit::Kind::eqBand) {
      setCursor(Qt::ArrowCursor);
    } else if (hit.kind != aoide::ChromeHit::Kind::none &&
               aoide::chromeHitEnabled(hit, view_)) {
      setCursor(Qt::PointingHandCursor);
    } else {
      setCursor(Qt::ArrowCursor);
    }
  }
  applyChromeTooltip(widgetPos);
}

void HostWindow::paintChrome(QPainter& p) {
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  const QSize logical = paintLogical();
  const qreal sx = qreal(width()) / qMax(1, logical.width());
  const qreal sy = qreal(height()) / qMax(1, logical.height());
  // The golden demo is the fidelity reference: paint it straight, uncached.
  if (view_.goldenDemo) {
    p.scale(sx, sy);
    aoide::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_);
    return;
  }
  ensureChassis();
  p.drawImage(QPointF(0, 0), chassis_);
  if (chassisIsFullPaint_) return;
  p.scale(sx, sy);
  aoide::paintMockupWindow(p, logical, spec_.id, title_, &logo_, view_, aoide::BodyPaint::live,
                           phases_);
}

void HostWindow::paintEvent(QPaintEvent*) {
  const aoide::BlurCost blurBefore = aoide::blurCost();
  QElapsedTimer timer;
  timer.start();
  {
    QPainter p(this);
    paintChrome(p);
  }
  const aoide::BlurCost blurAfter = aoide::blurCost();
  paintStats_.paints += 1;
  paintStats_.nanos += timer.nsecsElapsed();
  paintStats_.blurCalls += blurAfter.calls - blurBefore.calls;
  paintStats_.blurNanos += blurAfter.nanos - blurBefore.nanos;
  paintStats_.blurPixels += blurAfter.pixels - blurBefore.pixels;
  paintStats_.layers += blurAfter.layers - blurBefore.layers;
  paintStats_.layerNanos += blurAfter.layerNanos - blurBefore.layerNanos;
  paintStats_.fonts += blurAfter.fonts - blurBefore.fonts;
  paintStats_.fontNanos += blurAfter.fontNanos - blurBefore.fontNanos;
}

void HostWindow::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (spec_.id != aoide::WindowId::main) emit extraMapped();
}

void HostWindow::hideEvent(QHideEvent* event) {
  hideChromeTooltipNow();
  // A panel hidden under the pointer gets no leaveEvent, so without this the
  // button the cursor was over is still lit when the panel comes back.
  phases_.releaseChannel(aoide::BtnChannel::hover);
  phases_.releaseChannel(aoide::BtnChannel::press);
  phases_.advance(aoide::kBtnTransitionMs);
  animTimer_.stop();
  QWidget::hideEvent(event);
}

void HostWindow::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::DevicePixelRatioChange) {
    invalidateChassis();
    update();
  }
}

void HostWindow::closeEvent(QCloseEvent* event) {
  if (spec_.id == aoide::WindowId::main) {
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

void HostWindow::deliverPendingCollectionClick() {
  if (!pendingCollectionClick_) return;
  pendingCollectionClick_ = false;
  collectionClickTimer_.stop();
  emit chromePressed(pendingCollectionHit_, pendingCollectionMods_, pendingCollectionLogical_);
}

void HostWindow::onDeferredCollectionClick() {
  if (!pendingCollectionClick_) return;
  const aoide::ChromeHit hit = pendingCollectionHit_;
  deliverPendingCollectionClick();
  // A click that already released must not leave draggingChrome_ set: the
  // release already ran, and a later release would fire chromeReleased for a
  // press the session never paired. Still holding means the usual press path
  // should own the pointer so the matching release can clear it.
  if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
    draggingChrome_ = true;
    dragHit_ = hit;
  }
}

void HostWindow::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) return;
  // A second press must not overtake a collection click that is still waiting
  // to see if it was the first half of a double-click.
  deliverPendingCollectionClick();
  hideChromeTooltipNow();
  trackPointer(event->position(), true);
  const QPoint logical = logicalFrom(event->position());
  const auto hit = title_.hit(logical);
  switch (hit) {
    case aoide::TitleChromeLayout::Hit::close:
      close();
      event->accept();
      return;
    case aoide::TitleChromeLayout::Hit::minimize:
      if (QWidget* top = window()) top->showMinimized();
      event->accept();
      return;
    case aoide::TitleChromeLayout::Hit::collapse:
      setShaded(!shaded_);
      event->accept();
      return;
    case aoide::TitleChromeLayout::Hit::zoomOut:
      emit zoomOutRequested();
      event->accept();
      return;
    case aoide::TitleChromeLayout::Hit::zoomIn:
      emit zoomInRequested();
      event->accept();
      return;
    case aoide::TitleChromeLayout::Hit::drag:
      emit titleDragStarted();
      draggingTitle_ = true;
      grabOffset_ = event->globalPosition().toPoint() - mapToGlobal(QPoint(0, 0));
      grabPointerIfAllowed();
      event->accept();
      return;
    case aoide::TitleChromeLayout::Hit::none:
      break;
  }

  const auto chrome = aoide::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  if (chrome.kind == aoide::ChromeHit::Kind::plResize) {
    resizingPlaylist_ = true;
    grabPointerIfAllowed();
    event->accept();
    return;
  }
  if (chrome.kind != aoide::ChromeHit::Kind::none && aoide::chromeHitEnabled(chrome, view_)) {
    if (chrome.kind == aoide::ChromeHit::Kind::plCollectionRow &&
        !(event->modifiers() & Qt::ControlModifier)) {
      // Do not set draggingChrome_: a release before the timer would emit
      // chromeReleased with no chromePressed, and collection rows take no
      // press face to preserve.
      pendingCollectionClick_ = true;
      pendingCollectionHit_ = chrome;
      pendingCollectionMods_ = event->modifiers();
      pendingCollectionLogical_ = logical;
      collectionClickTimer_.start(QApplication::doubleClickInterval());
      event->accept();
      return;
    }
    draggingChrome_ = true;
    dragHit_ = chrome;
    emit chromePressed(chrome, event->modifiers(), logical);
    event->accept();
  }
}

void HostWindow::leaveEvent(QEvent* event) {
  hideChromeTooltipNow();
  trackPointer(std::nullopt, false);
  QWidget::leaveEvent(event);
}

void HostWindow::mouseMoveEvent(QMouseEvent* event) {
  applyHitCursor(event->position());
  // Not while a gesture owns the pointer: a drag repaints the panel already,
  // and a chassis rebuild per move is exactly what the paint budget forbids.
  if (!draggingTitle_ && !resizingPlaylist_) {
    trackPointer(event->position(), draggingChrome_ && (event->buttons() & Qt::LeftButton));
  }
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
  if (underMouse()) {
    trackPointer(event->position(), false);
  } else {
    trackPointer(std::nullopt, false);
  }
  QWidget::mouseReleaseEvent(event);
}

void HostWindow::mouseDoubleClickEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) return;
  const QPoint logical = logicalFrom(event->position());
  const auto chrome = aoide::hitTest(spec_.id, spec_.logicalSize, logical, view_);
  if (chrome.kind == aoide::ChromeHit::Kind::plTrackRow) {
    emit trackActivated(chrome.index);
    event->accept();
    return;
  }
  if (chrome.kind == aoide::ChromeHit::Kind::plCollectionRow) {
    pendingCollectionClick_ = false;
    collectionClickTimer_.stop();
    emit collectionRowActivated(chrome.index);
    event->accept();
    return;
  }
  QWidget::mouseDoubleClickEvent(event);
}

void HostWindow::wheelEvent(QWheelEvent* event) {
  const int angleY = event->angleDelta().y();
  const int rows = aoide::wheelRowSteps(event->pixelDelta().y(), angleY, wheelPixelCarry_,
                                        int(qRound(aoide::kPlaylistRowStride)));
  if (rows == 0) return;
  if (angleY != 0) {
    emit wheelScrolled(angleY);
    return;
  }
  const int notch = rows < 0 ? 120 : -120;
  for (int i = 0; i < qAbs(rows); ++i) emit wheelScrolled(notch);
}

void HostWindow::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  if (parentWidget()) return;
  emit nativeMoved(mapToGlobal(QPoint(0, 0)));
}

void HostWindow::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (spec_.id == aoide::WindowId::playlist && !shaded_) {
    const qreal z = zoomPercent_ / 100.0;
    spec_.logicalSize = QSize(int(width() / z), int(height() / z));
    title_ = aoide::TitleChromeLayout::forWindow(spec_.id, paintLogical());
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
