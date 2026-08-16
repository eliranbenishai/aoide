#pragma once

#include "title_chrome.h"
#include "window_spec.h"

#include <QCloseEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
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
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

 private:
  QPoint logicalFrom(const QPointF& widgetPos) const;
  void applyHitCursor(const QPointF& widgetPos);

  tramp::WindowSpec spec_;
  tramp::TitleChromeLayout title_;
  QImage logo_;
};
