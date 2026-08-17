#pragma once

#include "host_shell.h"

#include <QCloseEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QWidget>

class HostShell : public QWidget {
  Q_OBJECT

 public:
  explicit HostShell(QWidget* parent = nullptr);

  void applyLayout(const tramp::HostShellLayout& layout);
  void refreshMaskFromChildren();
  void setAlwaysOnTop(bool on);
  void setPrimaryPanel(QWidget* panel);

 signals:
  void minimizedChanged(bool minimized);
  void activated();

 protected:
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  void applyStoredMask();

  tramp::HostShellLayout lastLayout_{};
  QWidget* primaryPanel_ = nullptr;
};
