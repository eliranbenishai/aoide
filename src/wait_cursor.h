#pragma once

#include <QObject>
#include <functional>

namespace aoide {

struct WaitCursorHooks {
  std::function<void()> apply;
  std::function<void()> restore;
};

class WaitCursorScope {
 public:
  WaitCursorScope();
  ~WaitCursorScope();
  WaitCursorScope(const WaitCursorScope&) = delete;
  WaitCursorScope& operator=(const WaitCursorScope&) = delete;

  static void installHooks(WaitCursorHooks hooks);
  static void resetHooks();
  static bool showing();

 private:
  friend class WaitCursorPause;
  static void applyNow();
  static void restoreNow();
  static int depth_;
  static int paused_;
  static WaitCursorHooks hooks_;
};

void withWaitCursor(QObject* context, std::function<void()> work);

class WaitCursorPause {
 public:
  WaitCursorPause();
  ~WaitCursorPause();
  WaitCursorPause(const WaitCursorPause&) = delete;
  WaitCursorPause& operator=(const WaitCursorPause&) = delete;

 private:
  bool active_ = false;
};

}  // namespace aoide
