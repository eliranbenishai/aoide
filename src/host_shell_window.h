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
  /// Places children and punches the host to their union, on every call. The
  /// punch is not optional and must not be deferred: on KWin the mask is the
  /// hole the compositor actually shows and hits, so a mask left on the old
  /// rectangle leaves the vacated pixels on the canvas, and an empty one gives
  /// the whole desktop-sized surface the clicks. Deferring it shipped once and
  /// was undone — `docs/agents/title-bar-drag.md`.
  void placePanels(const QVector<HostPanelPlacement>& panels);
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
