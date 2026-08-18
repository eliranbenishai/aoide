#include "host_shell_window.h"

#include "window_spec.h"

#include <QGuiApplication>
#include <QPainter>
#include <QRegion>
#include <QScreen>
#include <QWindow>

HostShell::HostShell(QWidget* parent) : QWidget(parent) {
  setWindowFlags(tramp::hostWindowFlags());
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowTitle(QStringLiteral("Tramp"));
  bindDesktopScreens();
}

void HostShell::bindDesktopScreens() {
  auto hook = [this](QScreen* screen) {
    if (!screen) return;
    connect(screen, &QScreen::geometryChanged, this, &HostShell::desktopGeometryChanged);
  };
  connect(qApp, &QGuiApplication::screenAdded, this, [this, hook](QScreen* screen) {
    hook(screen);
    emit desktopGeometryChanged();
  });
  connect(qApp, &QGuiApplication::screenRemoved, this, [this](QScreen*) {
    emit desktopGeometryChanged();
  });
  for (QScreen* screen : QGuiApplication::screens()) hook(screen);
}

void HostShell::applyLayout(const tramp::HostShellLayout& layout) {
  lastLayout_ = layout;
  if (layout.screenRect.isNull() || layout.localMask.isEmpty()) {
    hide();
    return;
  }
  setGeometry(layout.screenRect);
  applyPunch(layout.localMask);
  update();
}

void HostShell::placePanels(const QVector<HostPanelPlacement>& panels, bool updatePunch) {
  Q_UNUSED(updatePunch);
  QVector<QRect> screenRects;
  screenRects.reserve(panels.size());
  for (const HostPanelPlacement& place : panels) {
    if (place.widget) screenRects.push_back(place.screen);
  }
  if (screenRects.isEmpty()) {
    applyLayout({});
    return;
  }

  const QRect virt = virtualDesktop();
  if (!virt.isNull() && lastRequestedVirtual_ != virt) {
    lastRequestedVirtual_ = virt;
    setGeometry(virt);
  }
  if (!isVisible()) show();

  const QPoint origin = mapToGlobal(QPoint(0, 0));
  QRegion mask;
  QRegion dirty;
  for (const HostPanelPlacement& place : panels) {
    if (!place.widget) continue;
    const QRect local(tramp::panelLocalTopLeft(place.screen.topLeft(), origin), place.screen.size());
    const QRect old = place.widget->geometry();
    if (old != local) {
      if (!old.isEmpty()) dirty += old;
      dirty += local;
      place.widget->setGeometry(local);
    }
    place.widget->show();
    mask += local;
  }

  lastLayout_.screenRect = QRect(origin, size());
  lastLayout_.localMask = mask;
  applyPunch(mask);
  if (!dirty.isEmpty()) update(dirty);
}

QRect HostShell::virtualDesktop() const {
  QRect box;
  for (QScreen* screen : QGuiApplication::screens()) {
    if (screen) box = box.united(screen->geometry());
  }
  return box;
}

void HostShell::setAlwaysOnTop(bool on) {
  const bool have = windowFlags().testFlag(Qt::WindowStaysOnTopHint);
  if (have == on) return;
  const bool vis = isVisible();
  setWindowFlag(Qt::WindowStaysOnTopHint, on);
  if (vis) show();
  applyStoredMask();
}

void HostShell::setPrimaryPanel(QWidget* panel) { primaryPanel_ = panel; }

void HostShell::applyPunch(const QRegion& mask) {
  lastLayout_.localMask = mask;
  // Qt Wayland: empty mask → wl_surface.set_input_region(nullptr) → the whole
  // surface takes clicks. Never punch-to-everything while mapped.
  if (mask.isEmpty()) {
    if (isVisible()) return;
    clearMask();
    if (QWindow* native = windowHandle()) native->setMask(QRegion());
    return;
  }
  setMask(mask);
  if (QWindow* native = windowHandle()) native->setMask(mask);
}

void HostShell::applyStoredMask() {
  applyPunch(lastLayout_.localMask);
  update();
}

void HostShell::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  applyStoredMask();
}

void HostShell::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::DevicePixelRatioChange) {
    applyStoredMask();
  }
  if (event->type() == QEvent::WindowStateChange) {
    emit minimizedChanged(windowState() & Qt::WindowMinimized);
  } else if (event->type() == QEvent::WindowActivate) {
    emit activated();
  }
}

void HostShell::closeEvent(QCloseEvent* event) {
  if (primaryPanel_ && !primaryPanel_->close()) {
    event->ignore();
    return;
  }
  QWidget::closeEvent(event);
}

void HostShell::paintEvent(QPaintEvent* event) {
  QPainter p(this);
  p.setCompositionMode(QPainter::CompositionMode_Source);
  p.fillRect(event->rect(), Qt::transparent);
}
