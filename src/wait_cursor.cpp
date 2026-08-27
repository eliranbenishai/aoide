#include "wait_cursor.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <QWidget>

namespace aoide {
namespace {

struct SavedCursor {
  QPointer<QWidget> widget;
  QCursor cursor;
};

QVector<SavedCursor> gSavedCursors;

/// No event loop is pumped here, deliberately.
///
/// A scope is entered part-way through a session method, so pumping ran timers
/// and queued slots against state that was half changed — a persist of a
/// playlist that was about to be replaced, a probe answer landing in a list the
/// caller still held a copy of. It was only ever there to get the cursor onto
/// the screen before a long blocking call, and the operation that made those
/// calls long — the duration probe on a playlist ingest — is on a worker now.
/// What is left under a wait cursor is skin work and the bootstrap read.
///
/// Chrome published inside a scope still reaches the screen: `HostWindow`
/// repaints synchronously while [showing] is true.
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

}  // namespace aoide
