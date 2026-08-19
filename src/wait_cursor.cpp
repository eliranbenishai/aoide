#include "wait_cursor.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QEventLoop>
#include <QGuiApplication>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <QWidget>

namespace tramp {
namespace {

struct SavedCursor {
  QPointer<QWidget> widget;
  QCursor cursor;
};

QVector<SavedCursor> gSavedCursors;

void defaultApply() {
  if (!QGuiApplication::instance()) return;
  QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if (auto* app = qobject_cast<QApplication*>(QCoreApplication::instance())) {
    gSavedCursors.clear();
    const auto widgets = app->allWidgets();
    for (QWidget* w : widgets) {
      if (!w || !w->isVisible()) continue;
      gSavedCursors.push_back({w, w->cursor()});
      w->setCursor(Qt::WaitCursor);
    }
  }
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void defaultRestore() {
  if (!QGuiApplication::instance()) return;
  QGuiApplication::restoreOverrideCursor();
  for (const SavedCursor& saved : gSavedCursors) {
    if (saved.widget) saved.widget->setCursor(saved.cursor);
  }
  gSavedCursors.clear();
}

}  // namespace

int WaitCursorScope::depth_ = 0;
int WaitCursorScope::paused_ = 0;
WaitCursorHooks WaitCursorScope::hooks_;

void WaitCursorScope::applyNow() {
  if (hooks_.apply) hooks_.apply();
  else defaultApply();
}

void WaitCursorScope::restoreNow() {
  if (hooks_.restore) hooks_.restore();
  else defaultRestore();
}

bool WaitCursorScope::showing() { return depth_ > 0 && paused_ == 0; }

WaitCursorScope::WaitCursorScope() {
  if (++depth_ != 1) return;
  applyNow();
}

WaitCursorScope::~WaitCursorScope() {
  if (--depth_ != 0) return;
  restoreNow();
}

WaitCursorPause::WaitCursorPause() {
  if (WaitCursorScope::depth_ <= 0) return;
  ++WaitCursorScope::paused_;
  active_ = true;
  WaitCursorScope::restoreNow();
}

WaitCursorPause::~WaitCursorPause() {
  if (!active_) return;
  --WaitCursorScope::paused_;
  if (WaitCursorScope::depth_ > 0 && WaitCursorScope::paused_ == 0) {
    WaitCursorScope::applyNow();
  }
}

void WaitCursorScope::installHooks(WaitCursorHooks hooks) { hooks_ = std::move(hooks); }

void withWaitCursor(QObject* context, std::function<void()> work) {
  QTimer::singleShot(0, context, [work = std::move(work)]() {
    WaitCursorScope wait;
    work();
  });
}

void WaitCursorScope::resetHooks() {
  depth_ = 0;
  paused_ = 0;
  hooks_ = {};
  gSavedCursors.clear();
}

}  // namespace tramp
