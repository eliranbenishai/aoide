#include "wait_cursor.h"

#include <QCoreApplication>
#include <QCursor>
#include <QEventLoop>
#include <QGuiApplication>

namespace tramp {
namespace {

void defaultApply() {
  if (!QGuiApplication::instance()) return;
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void defaultRestore() {
  if (!QGuiApplication::instance()) return;
  QGuiApplication::restoreOverrideCursor();
}

}  // namespace

int WaitCursorScope::depth_ = 0;
WaitCursorHooks WaitCursorScope::hooks_;

void WaitCursorScope::applyNow() {
  if (hooks_.apply) hooks_.apply();
  else defaultApply();
}

void WaitCursorScope::restoreNow() {
  if (hooks_.restore) hooks_.restore();
  else defaultRestore();
}

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
  active_ = true;
  WaitCursorScope::restoreNow();
}

WaitCursorPause::~WaitCursorPause() {
  if (!active_ || WaitCursorScope::depth_ <= 0) return;
  WaitCursorScope::applyNow();
}

void WaitCursorScope::installHooks(WaitCursorHooks hooks) { hooks_ = std::move(hooks); }

void WaitCursorScope::resetHooks() {
  depth_ = 0;
  hooks_ = {};
}

}  // namespace tramp
