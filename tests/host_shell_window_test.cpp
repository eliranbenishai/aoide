#include "compositor_keep_above.h"
#include "host_shell.h"
#include "host_shell_window.h"
#include "main_on_top.h"

#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QTest>
#include <QVector>
#include <QWindow>

namespace {

QWidget* topmostOf(QWidget* host, const QVector<QWidget*>& panels) {
  QWidget* top = nullptr;
  for (QObject* obj : host->children()) {
    auto* w = qobject_cast<QWidget*>(obj);
    if (w && panels.contains(w)) top = w;
  }
  return top;
}

}  // namespace

class HostShellWindowTest : public QObject {
  Q_OBJECT

 private slots:
  void shellIsFramelessToplevelNotTool();
  void shellAdvertisesAppLogoOnTheTaskbar();
  void applyLayoutSetsGeometryAndPunchedMask();
  void emptyLayoutHidesTheShell();
  void placePanelsKeepsMainVisibleInTheMask();
  void movingOnePanelDoesNotMoveItsSibling();
  void hostContainsPanelsAtRequestedScreenPositions();
  void translatingEveryPanelLeavesTheHostPut();
  void movingOnePanelDoesNotResizeTheHost();
  void expandingPastTopLeftKeepsSiblingOnScreen();
  void punchFollowsEveryPlacementWhileMapped();
  void alwaysOnTopSetsWindowStaysOnTopHint();
  void compositorKeepAboveAvailableIsFalseOnOffscreen();
  void keepAboveIsHonouredOnWindows();
  void keepAboveIsHonouredOnCocoa();
  void keepAboveIsHonouredOnXcb();
  void keepAboveIsHonouredOnWaylandWhenKwinIsReachable();
  void keepAboveIsHonouredOnWaylandPrefixedWhenKwinIsReachable();
  void keepAboveIsRefusedOnWaylandWhenKwinIsUnreachable();
  void keepAboveIsRefusedOnWaylandPrefixedWhenKwinIsUnreachable();
  void keepAboveIsRefusedOnOffscreen();
  void keepAboveIsRefusedOnMinimal();
  void keepAboveIsRefusedOnVnc();
  void keepAboveIsRefusedOnUnrecognizedQpa();
  void applyCompositorKeepAboveSetsFlagWhenNotWayland();
  void kwinKeepAboveScriptNamesTheHost();
  void kwinKeepAboveScriptLivesInASharedSubdirectory();
  void mainStaysTopMostAfterEachSiblingIsShown();
  void mainStaysTopMostAfterARequestRaise();
  void mainStaysTopMostAfterActivationAndUnminimize();
};

void HostShellWindowTest::shellIsFramelessToplevelNotTool() {
  HostShell shell;
  QCOMPARE(shell.windowFlags() & Qt::WindowType_Mask, Qt::WindowFlags(Qt::Window));
  QVERIFY(shell.windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(!shell.windowFlags().testFlag(Qt::Tool));
  QVERIFY(!shell.windowFlags().testFlag(Qt::Dialog));
  QVERIFY(!shell.windowFlags().testFlag(Qt::WindowTransparentForInput));
  QVERIFY(shell.testAttribute(Qt::WA_TranslucentBackground));
  QCOMPARE(shell.windowTitle(), QStringLiteral("Aoide"));
}

void HostShellWindowTest::shellAdvertisesAppLogoOnTheTaskbar() {
  HostShell shell;
  const QIcon icon = shell.windowIcon();
  QVERIFY2(!icon.isNull(), "host is the taskbar/pager entry and must carry the app logo");
  const QPixmap px = icon.pixmap(QSize(32, 32));
  QVERIFY(!px.isNull());
  QVERIFY(px.width() >= 16);
  QVERIFY(px.height() >= 16);
  const QImage img = px.toImage().convertToFormat(QImage::Format_ARGB32);
  int opaque = 0;
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      if (qAlpha(img.pixel(x, y)) > 16) ++opaque;
    }
  }
  QVERIFY2(opaque > 0, "taskbar icon must be painted logo pixels, not an empty pixmap");
}

void HostShellWindowTest::applyLayoutSetsGeometryAndPunchedMask() {
  HostShell shell;
  aoide::HostShellLayout layout;
  layout.screenRect = QRect(10, 20, 300, 50);
  QRegion mask;
  mask += QRect(0, 0, 100, 50);
  mask += QRect(200, 0, 100, 50);
  layout.localMask = mask;

  shell.applyLayout(layout);

  QCOMPARE(shell.geometry(), QRect(10, 20, 300, 50));
  QVERIFY(shell.mask().contains(QPoint(10, 10)));
  QVERIFY(shell.mask().contains(QPoint(210, 10)));
  QVERIFY(!shell.mask().contains(QPoint(150, 10)));
}

void HostShellWindowTest::emptyLayoutHidesTheShell() {
  HostShell shell;
  aoide::HostShellLayout visible;
  visible.screenRect = QRect(0, 0, 100, 50);
  visible.localMask = QRegion(QRect(0, 0, 100, 50));
  shell.applyLayout(visible);
  shell.show();
  QVERIFY(shell.isVisible());

  shell.applyLayout({});
  QVERIFY(!shell.isVisible());
  QVERIFY(!shell.mask().isEmpty());
}

void HostShellWindowTest::placePanelsKeepsMainVisibleInTheMask() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect mainR(10, 20, 200, 80);
  const QRect eqR(10, 100, 200, 80);
  const QRect plR(220, 20, 200, 160);
  shell.placePanels({{&main, mainR}, {&eq, eqR}, {&pl, plR}});
  shell.show();

  QVERIFY(main.isVisible());
  QCOMPARE(main.size(), mainR.size());
  QVERIFY(shell.mask().contains(main.geometry().center()));
  QVERIFY(shell.mask().contains(eq.geometry().center()));
  QVERIFY(shell.mask().contains(pl.geometry().center()));
}

void HostShellWindowTest::movingOnePanelDoesNotMoveItsSibling() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect eqR(0, 0, 100, 50);
  const QRect plR(200, 0, 100, 80);
  shell.placePanels({{&eq, eqR}, {&pl, plR}});
  shell.show();
  const QPoint pl0 = pl.mapToGlobal(QPoint(0, 0));

  shell.placePanels({{&eq, QRect(40, 20, 100, 50)}, {&pl, plR}});
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), pl0);
}

void HostShellWindowTest::hostContainsPanelsAtRequestedScreenPositions() {
  HostShell shell;
  QWidget main(&shell);
  const QRect mainR(40, 80, 100, 50);
  shell.placePanels({{&main, mainR}});
  shell.show();

  QVERIFY(shell.isVisible());
  QVERIFY(main.isVisible());
  QVERIFY(shell.rect().contains(main.geometry()));
  QCOMPARE(main.mapToGlobal(QPoint(0, 0)), mainR.topLeft());
  QCOMPARE(shell.geometry(), shell.virtualDesktop());
}

void HostShellWindowTest::translatingEveryPanelLeavesTheHostPut() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect eqR(0, 0, 100, 50);
  const QRect plR(200, 0, 100, 80);
  shell.placePanels({{&eq, eqR}, {&pl, plR}});
  shell.show();
  const QRect host0 = shell.geometry();
  QCOMPARE(host0, shell.virtualDesktop());

  const QRect eq1(30, 40, 100, 50);
  const QRect pl1(230, 40, 100, 80);
  shell.placePanels({{&eq, eq1}, {&pl, pl1}});
  QCOMPARE(shell.geometry(), host0);
  QCOMPARE(eq.mapToGlobal(QPoint(0, 0)), eq1.topLeft());
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), pl1.topLeft());
}

void HostShellWindowTest::movingOnePanelDoesNotResizeTheHost() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  shell.placePanels({{&eq, QRect(0, 0, 100, 50)}, {&pl, QRect(200, 0, 100, 80)}});
  shell.show();
  const QSize hostSize = shell.size();

  const QRect plCloser(100, 0, 100, 80);
  shell.placePanels({{&eq, QRect(0, 0, 100, 50)}, {&pl, plCloser}});
  QCOMPARE(shell.size(), hostSize);
  QCOMPARE(shell.geometry(), shell.virtualDesktop());
  QCOMPARE(eq.mapToGlobal(QPoint(0, 0)), QPoint(0, 0));
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), plCloser.topLeft());
}

void HostShellWindowTest::expandingPastTopLeftKeepsSiblingOnScreen() {
  HostShell shell;
  QWidget eq(&shell);
  QWidget pl(&shell);
  const QRect eq0(40, 40, 100, 50);
  const QRect pl0(200, 40, 100, 80);
  shell.placePanels({{&eq, eq0}, {&pl, pl0}});
  shell.show();
  const QPoint plScreen = pl.mapToGlobal(QPoint(0, 0));
  const QPoint hostPos = shell.pos();

  const QRect eq1(10, 10, 100, 50);
  shell.placePanels({{&eq, eq1}, {&pl, pl0}});
  QCOMPARE(pl.mapToGlobal(QPoint(0, 0)), plScreen);
  QCOMPARE(eq.mapToGlobal(QPoint(0, 0)), eq1.topLeft());
  QCOMPARE(shell.pos(), hostPos);
}

// Punch deferral shipped once and left ghost rectangles on KWin, so the punch
// follows the panels on every placement while the host is mapped — including the
// one after the widgets have already moved, which is where a deferral would show
// as a hole in the wrong place.
void HostShellWindowTest::punchFollowsEveryPlacementWhileMapped() {
  HostShell shell;
  QWidget panel(&shell);
  const QRect start(40, 40, 120, 60);
  const QRect end(200, 80, 120, 60);
  shell.placePanels({{&panel, start}});
  shell.show();
  QVERIFY(shell.mask().contains(panel.geometry().center()));

  shell.placePanels({{&panel, end}});
  QCOMPARE(panel.mapToGlobal(QPoint(0, 0)), end.topLeft());
  QVERIFY2(!shell.mask().isEmpty(),
           "empty mask is full input on Wayland; punch must stay a panel union");
  QVERIFY(shell.mask().contains(panel.geometry().center()));
  const QRect vacated = panel.geometry().translated(start.topLeft() - end.topLeft());
  QVERIFY2(!shell.mask().contains(vacated.center()),
           "the vacated rectangle must leave the punch, or it stays on the canvas");

  shell.placePanels({{&panel, end}});
  QVERIFY(shell.mask().contains(panel.geometry().center()));
  QVERIFY(!shell.mask().contains(QPoint(10, 10)));
}

void HostShellWindowTest::alwaysOnTopSetsWindowStaysOnTopHint() {
  HostShell shell;
  QVERIFY(!shell.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
  shell.setAlwaysOnTop(true);
  QVERIFY2(shell.windowFlags().testFlag(Qt::WindowStaysOnTopHint),
           "the cog check is settings; the host flag is what X11/Win/macOS stack on");
  shell.setAlwaysOnTop(false);
  QVERIFY(!shell.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
}

void HostShellWindowTest::compositorKeepAboveAvailableIsFalseOnOffscreen() {
  // ctest forces offscreen. That QPA cannot stack above other apps, and this
  // binary has no AOIDE_HAVE_DBUS, so the predicate must be false — a true
  // here would show the options row where keep-above cannot be honoured.
  QCOMPARE(QGuiApplication::platformName(), QStringLiteral("offscreen"));
  QVERIFY2(!aoide::compositorKeepAboveAvailable(),
           "offscreen cannot honour keep-above; do not treat the Qt flag as stacking");
}

void HostShellWindowTest::keepAboveIsHonouredOnWindows() {
  QVERIFY(aoide::compositorKeepAboveAvailableOn(QStringLiteral("windows"), false));
}

void HostShellWindowTest::keepAboveIsHonouredOnCocoa() {
  QVERIFY(aoide::compositorKeepAboveAvailableOn(QStringLiteral("cocoa"), false));
}

void HostShellWindowTest::keepAboveIsHonouredOnXcb() {
  QVERIFY(aoide::compositorKeepAboveAvailableOn(QStringLiteral("xcb"), false));
}

void HostShellWindowTest::keepAboveIsHonouredOnWaylandWhenKwinIsReachable() {
  QVERIFY(aoide::compositorKeepAboveAvailableOn(QStringLiteral("wayland"), true));
}

void HostShellWindowTest::keepAboveIsHonouredOnWaylandPrefixedWhenKwinIsReachable() {
  QVERIFY(aoide::compositorKeepAboveAvailableOn(QStringLiteral("wayland-egl"), true));
}

void HostShellWindowTest::keepAboveIsRefusedOnWaylandWhenKwinIsUnreachable() {
  QVERIFY(!aoide::compositorKeepAboveAvailableOn(QStringLiteral("wayland"), false));
}

void HostShellWindowTest::keepAboveIsRefusedOnWaylandPrefixedWhenKwinIsUnreachable() {
  QVERIFY(!aoide::compositorKeepAboveAvailableOn(QStringLiteral("wayland-egl"), false));
}

void HostShellWindowTest::keepAboveIsRefusedOnOffscreen() {
  QVERIFY(!aoide::compositorKeepAboveAvailableOn(QStringLiteral("offscreen"), true));
}

void HostShellWindowTest::keepAboveIsRefusedOnMinimal() {
  QVERIFY(!aoide::compositorKeepAboveAvailableOn(QStringLiteral("minimal"), true));
}

void HostShellWindowTest::keepAboveIsRefusedOnVnc() {
  QVERIFY(!aoide::compositorKeepAboveAvailableOn(QStringLiteral("vnc"), true));
}

void HostShellWindowTest::keepAboveIsRefusedOnUnrecognizedQpa() {
  QVERIFY(!aoide::compositorKeepAboveAvailableOn(QStringLiteral("directfb"), true));
}

void HostShellWindowTest::applyCompositorKeepAboveSetsFlagWhenNotWayland() {
  QCOMPARE(QGuiApplication::platformName(), QStringLiteral("offscreen"));
  HostShell shell;
  shell.show();
  QWindow* native = shell.windowHandle();
  QVERIFY(native);
  aoide::applyCompositorKeepAbove(native, true);
  QVERIFY2(native->flags().testFlag(Qt::WindowStaysOnTopHint),
           "offscreen is not Wayland; the Qt flag stays the X11/Win/macOS path");
  aoide::applyCompositorKeepAbove(native, false);
  QVERIFY(!native->flags().testFlag(Qt::WindowStaysOnTopHint));
}

void HostShellWindowTest::kwinKeepAboveScriptNamesTheHost() {
  const QString on = aoide::kwinKeepAboveScript(4242, QStringLiteral("Aoide"),
                                               QStringLiteral("com.proximamagnifica.aoide"), true);
  QVERIFY(on.contains(QStringLiteral("const pid = 4242")));
  QVERIFY(on.contains(QStringLiteral("\"Aoide\"")));
  QVERIFY(on.contains(QStringLiteral("com.proximamagnifica.aoide")));
  QVERIFY(on.contains(QStringLiteral("const want = true")));
  QVERIFY(on.contains(QStringLiteral("w.keepAbove = want")));
  const QString off = aoide::kwinKeepAboveScript(1, QStringLiteral("Aoide"),
                                                QStringLiteral("com.proximamagnifica.aoide"), false);
  QVERIFY(off.contains(QStringLiteral("const want = false")));
}

void HostShellWindowTest::kwinKeepAboveScriptLivesInASharedSubdirectory() {
  const QString path = aoide::kwinKeepAboveScriptPath(QStringLiteral("/run/user/1000"));
  // Not the runtime root. KWin opens this path from its own process, and a
  // Flatpak's $XDG_RUNTIME_DIR is a private mount the host cannot see, so a file
  // at the root is unreadable to the reader while looking correct to the writer
  // — loadScript still returns success. The subdirectory is what
  // --filesystem=xdg-run/aoide:create shares at one path on both sides, so the
  // manifest entry and this prefix have to keep agreeing.
  QCOMPARE(path, QStringLiteral("/run/user/1000/aoide/keep-above.js"));
  QVERIFY2(QFileInfo(path).path() != QStringLiteral("/run/user/1000"),
           "a script in the runtime root is invisible to KWin under Flatpak");
  QVERIFY(aoide::kwinKeepAboveScriptPath(QString()).isEmpty());
}

void HostShellWindowTest::mainStaysTopMostAfterEachSiblingIsShown() {
  HostShell shell;
  QWidget main(&shell);
  QWidget equalizer(&shell);
  QWidget playlist(&shell);
  QWidget settings(&shell);
  QWidget about(&shell);
  QWidget skins(&shell);
  const QVector<QWidget*> panels = {&main, &equalizer, &playlist, &settings, &about, &skins};
  const QVector<QWidget*> siblings = {&equalizer, &playlist, &settings, &about, &skins};
  aoide::MainOnTopGuard guard(&shell, &main);

  QVERIFY2(topmostOf(&shell, panels) == &main,
           "main must be the top-most panel after the cluster is constructed");

  for (QWidget* sibling : siblings) {
    sibling->show();
    sibling->raise();
    QVERIFY2(topmostOf(&shell, panels) == &main,
             "main must stay the top-most panel after a sibling is shown");
  }

  QWidget extra(&shell);
  extra.raise();
  QVERIFY2(topmostOf(&shell, QVector<QWidget*>{&main, &extra}) == &main,
           "a later-added sibling must not stack above main");
}

void HostShellWindowTest::mainStaysTopMostAfterARequestRaise() {
  HostShell shell;
  QWidget main(&shell);
  QWidget settings(&shell);
  QWidget about(&shell);
  QWidget skins(&shell);
  const QVector<QWidget*> panels = {&main, &settings, &about, &skins};
  aoide::MainOnTopGuard guard(&shell, &main);

  for (QWidget* sibling : {&settings, &skins, &about}) {
    sibling->raise();
    QVERIFY2(topmostOf(&shell, panels) == &main,
             "main must stay the top-most panel after a requestRaise");
  }
}

void HostShellWindowTest::mainStaysTopMostAfterActivationAndUnminimize() {
  HostShell shell;
  QWidget main(&shell);
  QWidget equalizer(&shell);
  QWidget playlist(&shell);
  QWidget settings(&shell);
  QWidget about(&shell);
  QWidget skins(&shell);
  const QVector<QWidget*> panels = {&main, &equalizer, &playlist, &settings, &about, &skins};
  const QVector<QWidget*> siblings = {&equalizer, &playlist, &settings, &about, &skins};
  aoide::MainOnTopGuard guard(&shell, &main);

  // Un-minimize restores suppressed siblings. Show+raise is what a restore that
  // brought them forward would do; main still has to win the overlap.
  for (QWidget* sibling : siblings) {
    sibling->hide();
  }
  for (QWidget* sibling : siblings) {
    sibling->show();
    sibling->raise();
  }
  QVERIFY2(topmostOf(&shell, panels) == &main,
           "main must stay the top-most panel after un-minimizing");

  // Activation is the host coming forward. Raising settings and skins here is
  // the move that would hide the player; the guard has to reject it.
  settings.raise();
  skins.raise();
  QVERIFY2(topmostOf(&shell, panels) == &main,
           "main must stay the top-most panel after mainActivated");
}

QTEST_MAIN(HostShellWindowTest)
#include "host_shell_window_test.moc"
