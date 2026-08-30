#pragma once

#include <QChildEvent>
#include <QEvent>
#include <QObject>
#include <QWidget>

namespace aoide {

/// Child-widget stacking choke point: main stays the top-most panel. A sibling
/// raise — including a seventh panel added later as a host child — cannot stay
/// above the player. Installed on the host so a new child is watched
/// automatically. This is not compositor stacking; it is Z-order inside the
/// host, which is what decides a click on an overlap.
class MainOnTopGuard : public QObject {
 public:
  MainOnTopGuard(QWidget* host, QWidget* main, QObject* parent = nullptr)
      : QObject(parent), host_(host), main_(main) {
    if (!host_ || !main_) return;
    host_->installEventFilter(this);
    for (QObject* child : host_->children()) {
      if (child->isWidgetType()) child->installEventFilter(this);
    }
    main_->raise();
  }

  bool eventFilter(QObject* watched, QEvent* event) override {
    if (!host_ || !main_) return false;
    if (event->type() == QEvent::ChildAdded && watched == host_) {
      if (QObject* child = static_cast<QChildEvent*>(event)->child()) {
        if (child->isWidgetType()) child->installEventFilter(this);
      }
      // Construction appends the child above its siblings. raise() on a widget
      // already on top sends no ZOrderChange, so a seventh panel would sit
      // above main unless we restore the stack here.
      main_->raise();
    }
    if (event->type() == QEvent::ZOrderChange && watched != main_) {
      // A sibling moved in the stack. Main has to win, or a click on the
      // overlap hits the panel that just raised.
      main_->raise();
    }
    return false;
  }

 private:
  QWidget* host_ = nullptr;
  QWidget* main_ = nullptr;
};

}  // namespace aoide
