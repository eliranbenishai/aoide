#include "host_shell_window.h"

#include "app_icon.h"
#include "compositor_keep_above.h"
#include "window_spec.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QMoveEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QWindow>

HostShell::HostShell(QWidget* parent)
    : HostShell(aoide::panelPresentationFor(QGuiApplication::platformName()), parent) {}

HostShell::HostShell(aoide::PanelPresentation presentation, QWidget* parent)
    : QWidget(parent), presentation_(presentation) {
  setWindowTitle(QStringLiteral("Aoide"));
  setWindowIcon(aoide::appIcon());
  if (embedsPanels()) {
    // A normal desktop window: the compositor owns its position and decoration.
    // No translucent desktop overlay or input mask is needed.
    setWindowFlags(Qt::Window);
    QPalette colors = palette();
    colors.setColor(QPalette::Window, QColor(16, 18, 24));
    setPalette(colors);
    setAutoFillBackground(true);
    const QScreen* display = QGuiApplication::primaryScreen();
    const QSize work = display ? display->availableGeometry().size() : QSize(1280, 900);
    resize(QSize(1280, 900).boundedTo(work - QSize(64, 64)).expandedTo(QSize(320, 240)));
  } else {
    setWindowFlags(aoide::hostWindowFlags());
    setAttribute(Qt::WA_TranslucentBackground);
  }
  bindDesktopScreens();
  connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
    if (state == Qt::ApplicationActive && isVisible() && !isMinimized()) bringPanelsForward();
  });
}

void HostShell::preparePanel(QWidget* panel, bool primary) {
  if (!panel) return;
  if (primary) primaryPanel_ = panel;
  if (panels_.contains(panel)) return;
  panels_.push_back(panel);
  if (embedsPanels() || primary) {
    if (panel->parentWidget() != this || panel->isWindow()) panel->setParent(this, Qt::Widget);
  } else {
    panel->setParent(nullptr, aoide::hostWindowFlags());
    panel->setWindowIcon(windowIcon());
    panel->setAttribute(Qt::WA_QuitOnClose, false);
    panel->installEventFilter(this);
    applyTopHint(panel);
  }
}

void HostShell::bindDesktopScreens() {
  auto hook = [this](QScreen* screen) {
    if (!screen) return;
    connect(screen, &QScreen::geometryChanged, this, &HostShell::notifyBoundsChanged);
    connect(screen, &QScreen::availableGeometryChanged, this, &HostShell::notifyBoundsChanged);
  };
  connect(qApp, &QGuiApplication::screenAdded, this, [this, hook](QScreen* screen) {
    hook(screen);
    notifyBoundsChanged();
  });
  connect(qApp, &QGuiApplication::screenRemoved, this, &HostShell::notifyBoundsChanged);
  for (QScreen* screen : QGuiApplication::screens()) hook(screen);
}

void HostShell::notifyBoundsChanged() {
  if (boundsPending_) return;
  boundsPending_ = true;
  QTimer::singleShot(0, this, [this]() {
    boundsPending_ = false;
    emit desktopGeometryChanged();
  });
}

void HostShell::placePanels(const QVector<HostPanelPlacement>& panels) {
  if (placing_) return;
  QScopedValueRollback<bool> placing(placing_, true);
  if (panels.isEmpty()) {
    hide();
    return;
  }
  if (!primaryPanel_) setPrimaryPanel(panels.front().widget);
  for (const HostPanelPlacement& place : panels) preparePanel(place.widget);

  if (embedsPanels()) {
    QSize minimum;
    for (const HostPanelPlacement& place : panels) {
      if (place.widget) minimum = minimum.expandedTo(place.widget->minimumSize());
    }
    setMinimumSize(minimum);
  }
  for (const HostPanelPlacement& place : panels) {
    if (!place.widget) continue;
    if (!embedsPanels() && place.widget == primaryPanel_) {
      // The only child of the native primary window always starts at (0, 0).
      // An OS-adjusted window origin can no longer strand main outside its host.
      setGeometry(place.screen);
      place.widget->setGeometry(QRect(QPoint(), place.screen.size()));
    } else {
      place.widget->setGeometry(place.screen);
    }
    if (place.widget->isHidden()) place.widget->show();
  }
  if (isHidden()) show();
}

QRect HostShell::layoutBounds() const {
  return embedsPanels() ? rect() : virtualDesktop();
}

QRect HostShell::virtualDesktop() const {
  QRect box;
  for (QScreen* screen : QGuiApplication::screens()) {
    if (screen) box = box.united(screen->geometry());
  }
  return box;
}

void HostShell::applyTopHint(QWidget* window) {
  if (window->windowFlags().testFlag(Qt::WindowStaysOnTopHint) == alwaysOnTop_) return;
  const bool visible = window->isVisible();
  const Qt::WindowStates state = window->windowState();
  window->setWindowFlag(Qt::WindowStaysOnTopHint, alwaysOnTop_);
  window->setWindowState(state);
  if (visible) window->show();
}

void HostShell::setAlwaysOnTop(bool on) {
  alwaysOnTop_ = on;
  // xdg-shell has no keep-above request. KWin's integration handles it there;
  // the native window flag is supported by Cocoa, Windows and X11.
  if (aoide::panelPresentationFor(QGuiApplication::platformName()) !=
      aoide::PanelPresentation::embedded) {
    QScopedValueRollback<bool> placing(placing_, true);
    applyTopHint(this);
    if (!embedsPanels()) {
      for (const auto& panel : panels_) {
        if (panel && panel->isWindow()) applyTopHint(panel);
      }
    }
  }
  scheduleCompositorKeepAbove();
}

void HostShell::scheduleCompositorKeepAbove() {
  aoide::applyCompositorKeepAbove(windowHandle(), alwaysOnTop_);
  // KWin can match only a mapped window. Retry after the first map.
  for (int delay : {0, 150}) {
    QTimer::singleShot(delay, this, [this]() {
      aoide::applyCompositorKeepAbove(windowHandle(), alwaysOnTop_);
    });
  }
}

void HostShell::bringPanelsForward() {
  if (raising_ || placing_ || isMinimized()) return;
  QScopedValueRollback<bool> raising(raising_, true);
  if (!embedsPanels()) {
    for (const auto& panel : panels_) {
      if (panel && panel->isWindow() && panel->isVisible() && !panel->isMinimized()) panel->raise();
    }
    raise();
  }
  if (primaryPanel_) primaryPanel_->raise();
}

bool HostShell::eventFilter(QObject* watched, QEvent* event) {
  if (!placing_ && !raising_ && !isMinimized() &&
      (event->type() == QEvent::WindowActivate || event->type() == QEvent::ZOrderChange)) {
    auto* panel = qobject_cast<QWidget*>(watched);
    if (panel && panel->isVisible() && panel->geometry().intersects(geometry())) {
      QScopedValueRollback<bool> raising(raising_, true);
      raise();
    }
  }
  return QWidget::eventFilter(watched, event);
}

void HostShell::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  scheduleCompositorKeepAbove();
}

void HostShell::changeEvent(QEvent* event) {
  QWidget::changeEvent(event);
  if (event->type() == QEvent::WindowStateChange) {
    emit minimizedChanged(isMinimized());
  } else if (event->type() == QEvent::WindowActivate) {
    bringPanelsForward();
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

void HostShell::moveEvent(QMoveEvent* event) {
  QWidget::moveEvent(event);
  if (!embedsPanels() && !placing_ && isVisible()) emit primaryMoved(pos());
}

void HostShell::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (embedsPanels()) notifyBoundsChanged();
}

void HostShell::paintEvent(QPaintEvent* event) {
  if (embedsPanels()) return;
  QPainter painter(this);
  painter.setCompositionMode(QPainter::CompositionMode_Source);
  for (const QRect& rect : event->region()) painter.fillRect(rect, Qt::transparent);
}
