#pragma once

#include "host_shell.h"

#include <QCloseEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QVector>
#include <QWidget>

struct HostPanelPlacement {
  QWidget* widget = nullptr;
  QRect screen;
};

class HostShell : public QWidget {
  Q_OBJECT

 public:
  explicit HostShell(QWidget* parent = nullptr);

  void applyLayout(const tramp::HostShellLayout& layout);
  void placePanels(const QVector<HostPanelPlacement>& panels, bool updatePunch = true);
  void setAlwaysOnTop(bool on);
  void setPrimaryPanel(QWidget* panel);
  QRect virtualDesktop() const;

 signals:
  void minimizedChanged(bool minimized);
  void activated();
  void desktopGeometryChanged();

 protected:
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  void applyStoredMask();
  void bindDesktopScreens();

  tramp::HostShellLayout lastLayout_{};
  QRect lastRequestedVirtual_{};
  QWidget* primaryPanel_ = nullptr;
};
