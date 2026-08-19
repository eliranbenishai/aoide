#pragma once

#include <functional>

namespace tramp {

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

 private:
  friend class WaitCursorPause;
  static void applyNow();
  static void restoreNow();
  static int depth_;
  static WaitCursorHooks hooks_;
};

class WaitCursorPause {
 public:
  WaitCursorPause();
  ~WaitCursorPause();
  WaitCursorPause(const WaitCursorPause&) = delete;
  WaitCursorPause& operator=(const WaitCursorPause&) = delete;

 private:
  bool active_ = false;
};

}  // namespace tramp
