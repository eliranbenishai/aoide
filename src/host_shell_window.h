#pragma once

#include "host_shell.h"

#include <QCloseEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QShowEvent>
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
  /// Places children. When [updatePunch] is false, widgets move but the input
  /// mask stays put (never emptied). `TRAMP_LEGACY_DRAG=1` punches every call.
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
  void showEvent(QShowEvent* event) override;

 private:
  void applyStoredMask();
  void applyPunch(const QRegion& mask);
  void bindDesktopScreens();

  tramp::HostShellLayout lastLayout_{};
  QRect lastRequestedVirtual_{};
  QWidget* primaryPanel_ = nullptr;
};
