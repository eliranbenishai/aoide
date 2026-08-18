#include "host_shell_window.h"

#include "window_spec.h"

#include <QGuiApplication>
#include <QRegion>
#include <QScreen>

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
  setMask(layout.localMask);
  update();
}

void HostShell::placePanels(const QVector<HostPanelPlacement>& panels, bool updatePunch) {
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
  for (const HostPanelPlacement& place : panels) {
    if (!place.widget) continue;
    const QRect local(tramp::panelLocalTopLeft(place.screen.topLeft(), origin), place.screen.size());
    if (place.widget->geometry() != local) place.widget->setGeometry(local);
    place.widget->show();
    mask += local;
  }

  lastLayout_.screenRect = QRect(origin, size());
  lastLayout_.localMask = mask;
  if (updatePunch) setMask(mask);
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

void HostShell::applyStoredMask() {
  if (lastLayout_.localMask.isEmpty()) return;
  setMask(lastLayout_.localMask);
  update();
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

void HostShell::paintEvent(QPaintEvent*) {}
