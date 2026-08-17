#include "skip_taskbar.h"

#include <QGuiApplication>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#if defined(Q_OS_LINUX)
#if defined(TRAMP_HAVE_X11)
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#endif
#endif

namespace tramp {
namespace {

#ifdef Q_OS_WIN
void skipTaskbarWin32(QWindow* window) {
  const HWND hwnd = reinterpret_cast<HWND>(window->winId());
  LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  ex |= WS_EX_TOOLWINDOW;
  ex &= ~WS_EX_APPWINDOW;
  SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
  SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}
#endif

#if defined(TRAMP_HAVE_X11)
void skipTaskbarX11(QWindow* window) {
  auto* x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
  if (x11 == nullptr || x11->display() == nullptr) {
    return;
  }
  Display* display = static_cast<Display*>(x11->display());
  const Window xid = static_cast<Window>(window->winId());
  const Atom netWmState = XInternAtom(display, "_NET_WM_STATE", False);
  const Atom skipTaskbar =
      XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
  const Atom skipPager = XInternAtom(display, "_NET_WM_STATE_SKIP_PAGER", False);
  const Atom netWmType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  const Atom utility =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_UTILITY", False);

  XChangeProperty(display, xid, netWmType, XA_ATOM, 32, PropModeReplace,
                  reinterpret_cast<const unsigned char*>(&utility), 1);

  const Atom states[] = {skipTaskbar, skipPager};
  XChangeProperty(display, xid, netWmState, XA_ATOM, 32, PropModeReplace,
                  reinterpret_cast<const unsigned char*>(states), 2);

  XEvent event{};
  event.xclient.type = ClientMessage;
  event.xclient.window = xid;
  event.xclient.message_type = netWmState;
  event.xclient.format = 32;
  event.xclient.data.l[0] = 1;  // _NET_WM_STATE_ADD
  event.xclient.data.l[1] = static_cast<long>(skipTaskbar);
  event.xclient.data.l[2] = static_cast<long>(skipPager);
  event.xclient.data.l[3] = 1;
  XSendEvent(display, DefaultRootWindow(display), False,
             SubstructureRedirectMask | SubstructureNotifyMask, &event);
  XFlush(display);
}
#endif

}  // namespace

void applySkipTaskbar(QWindow* window) {
  if (window == nullptr) {
    return;
  }
#ifdef Q_OS_WIN
  skipTaskbarWin32(window);
#endif
#if defined(TRAMP_HAVE_X11)
  skipTaskbarX11(window);
#endif
}

void attachExtraWindow(QWindow* extra, QWindow* main) {
  if (extra == nullptr || main == nullptr) return;
  extra->setTransientParent(main);
  applySkipTaskbar(extra);
}

}  // namespace tramp
