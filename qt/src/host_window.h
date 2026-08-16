#pragma once

#include "window_spec.h"

#include <QShowEvent>
#include <QWidget>

class HostWindow : public QWidget {
  Q_OBJECT

 public:
  explicit HostWindow(const tramp::WindowSpec& spec, QWidget* parent = nullptr);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  tramp::WindowSpec spec_;
};
