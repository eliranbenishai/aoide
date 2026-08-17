#include "host_shell_window.h"

#include "window_spec.h"

#include <QRegion>

HostShell::HostShell(QWidget* parent) : QWidget(parent) {
  setWindowFlags(tramp::hostWindowFlags());
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowTitle(QStringLiteral("Tramp"));
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

void HostShell::refreshMaskFromChildren() {
  QRegion mask;
  const auto kids = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
  for (QWidget* child : kids) {
    if (!child->isHidden()) mask += child->geometry();
  }
  if (mask.isEmpty()) {
    hide();
    return;
  }
  lastLayout_.localMask = mask;
  setMask(mask);
  update();
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
  if (lastLayout_.screenRect.isNull() || lastLayout_.localMask.isEmpty()) return;
  setGeometry(lastLayout_.screenRect);
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
