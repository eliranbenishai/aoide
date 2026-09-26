#pragma once

#include "host_shell.h"

#include <QPointer>
#include <QWidget>

struct HostPanelPlacement {
  QWidget* widget = nullptr;
  // Screen coordinates for native windows; content coordinates when embedded.
  QRect screen;
};

/// Native desktops get a primary window the size of main and independent
/// secondary windows. Wayland gets one bounded container with local panels.
/// The session uses the same placement interface in either presentation.
class HostShell : public QWidget {
  Q_OBJECT

 public:
  explicit HostShell(QWidget* parent = nullptr);
  explicit HostShell(aoide::PanelPresentation presentation, QWidget* parent = nullptr);

  bool embedsPanels() const { return presentation_ == aoide::PanelPresentation::embedded; }
  void preparePanel(QWidget* panel, bool primary = false);
  void setPrimaryPanel(QWidget* panel) { preparePanel(panel, true); }
  void placePanels(const QVector<HostPanelPlacement>& panels);
  void setAlwaysOnTop(bool on);
  void bringPanelsForward();
  QRect layoutBounds() const;
  QRect virtualDesktop() const;

 signals:
  void minimizedChanged(bool minimized);
  void activated();
  void primaryMoved(QPoint position);
  void desktopGeometryChanged();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  void bindDesktopScreens();
  void notifyBoundsChanged();
  void scheduleCompositorKeepAbove();
  void applyTopHint(QWidget* window);
  void updateEmbeddedMinimum();

  aoide::PanelPresentation presentation_;
  QPointer<QWidget> primaryPanel_;
  QVector<QPointer<QWidget>> panels_;
  QVector<QPointer<QWidget>> placedPanels_;
  bool alwaysOnTop_ = false;
  bool placing_ = false;
  bool raising_ = false;
  bool boundsPending_ = false;
  bool minimumUpdating_ = false;
};
