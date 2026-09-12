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
#include <QSignalSpy>
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
  void nativeDesktopHasOnlyPanelSizedWindows();
  void nativeWindowsAreExposedAndRestorable();
  void shellAdvertisesAppLogoOnTheTaskbar();
  void nativeClusterTranslationMovesEachWindow();
  void nativeSiblingMoveLeavesPrimaryPut();
  void nativeAlwaysOnTopIncludesHiddenSecondaries();
  void nativePlacementDoesNotRestoreMinimizedPrimary();
  void nativeWindowMoveReportsPrimaryPosition();
  void embeddedPanelsUseContainerCoordinates();
  void embeddedContainerHasNoDesktopMask();
  void embeddedResizeReportsNewLayoutBounds();
  void presentationMatchesDesktopCapabilities_data();
  void presentationMatchesDesktopCapabilities();
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

void HostShellWindowTest::nativeDesktopHasOnlyPanelSizedWindows() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  shell.setPrimaryPanel(&main);
  const QRect mainRect(40, 80, 200, 100);
  const QRect eqRect(280, 80, 200, 100);
  shell.placePanels({{&main, mainRect}, {&eq, eqRect}});
  QCOMPARE(shell.geometry(), mainRect);
  QVERIFY(eq.isWindow());
  QCOMPARE(eq.geometry(), eqRect);
  QCOMPARE(main.mapToGlobal(QPoint()), mainRect.topLeft());
}

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

void HostShellWindowTest::nativeWindowsAreExposedAndRestorable() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  shell.setPrimaryPanel(&main);
  const QRect mainRect(100, 150, 200, 100);
  const QRect eqRect(350, 150, 200, 100);
  shell.placePanels({{&main, mainRect}, {&eq, eqRect}});
  QVERIFY(QTest::qWaitForWindowExposed(&shell));
  QVERIFY(QTest::qWaitForWindowExposed(&eq));
  shell.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&shell));
  eq.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&eq));
  shell.showMinimized();
  QTRY_VERIFY(shell.isMinimized());
  shell.showNormal();
  shell.activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(&shell));
  QVERIFY(QTest::qWaitForWindowExposed(&shell));
  QCOMPARE(shell.size(), mainRect.size());
  QVERIFY(shell.rect().contains(main.geometry()));
  QVERIFY(!shell.isFullScreen());
  QVERIFY(shell.mask().isEmpty());
  QCOMPARE(eq.size(), eqRect.size());
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

void HostShellWindowTest::nativeClusterTranslationMovesEachWindow() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  shell.setPrimaryPanel(&main);
  QRect mainRect(40, 80, 200, 100);
  QRect eqRect(280, 80, 200, 100);
  shell.placePanels({{&main, mainRect}, {&eq, eqRect}});
  QSignalSpy moved(&shell, &HostShell::primaryMoved);
  mainRect.translate(20, 30);
  eqRect.translate(20, 30);
  shell.placePanels({{&main, mainRect}, {&eq, eqRect}});
  QCOMPARE(shell.geometry(), mainRect);
  QCOMPARE(eq.geometry(), eqRect);
  QCOMPARE(main.mapToGlobal(QPoint()), mainRect.topLeft());
  QCOMPARE(moved.count(), 0); // A placement must not feed back as another drag.
}

void HostShellWindowTest::nativeSiblingMoveLeavesPrimaryPut() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  const QRect mainRect(40, 80, 200, 100);
  const QRect eqRect(280, 80, 200, 100);
  shell.setPrimaryPanel(&main);
  shell.placePanels({{&main, mainRect}, {&eq, eqRect}});
  shell.placePanels({{&main, mainRect}, {&eq, eqRect.translated(-30, 40)}});
  QCOMPARE(shell.geometry(), mainRect);
  QCOMPARE(main.mapToGlobal(QPoint()), mainRect.topLeft());
  QCOMPARE(eq.geometry(), eqRect.translated(-30, 40));
}

void HostShellWindowTest::nativeAlwaysOnTopIncludesHiddenSecondaries() {
  HostShell shell;
  QWidget main(&shell);
  QWidget eq(&shell);
  shell.preparePanel(&main, true);
  shell.preparePanel(&eq);
  shell.placePanels({{&main, QRect(40, 80, 200, 100)}});
  for (bool on : {true, false}) {
    shell.setAlwaysOnTop(on);
    QCOMPARE(shell.windowFlags().testFlag(Qt::WindowStaysOnTopHint), on);
    QCOMPARE(eq.windowFlags().testFlag(Qt::WindowStaysOnTopHint), on);
    QVERIFY(eq.isHidden());
    QVERIFY(shell.isVisible());
    QCOMPARE(shell.size(), QSize(200, 100));
  }
}

void HostShellWindowTest::nativePlacementDoesNotRestoreMinimizedPrimary() {
  HostShell shell;
  QWidget main(&shell);
  const QRect mainRect(40, 80, 200, 100);
  shell.setPrimaryPanel(&main);
  shell.placePanels({{&main, mainRect}});
  shell.showMinimized();
  shell.placePanels({{&main, mainRect}});
  QVERIFY(shell.isMinimized());
  shell.setAlwaysOnTop(true);
  QVERIFY(shell.isMinimized());
  shell.showNormal();
  QVERIFY(!shell.isMinimized());
  QVERIFY(main.isVisible());
  QCOMPARE(shell.size(), mainRect.size());
}

void HostShellWindowTest::nativeWindowMoveReportsPrimaryPosition() {
  HostShell shell;
  QWidget main(&shell);
  shell.setPrimaryPanel(&main);
  shell.placePanels({{&main, QRect(40, 80, 200, 100)}});
  QSignalSpy moved(&shell, &HostShell::primaryMoved);
  shell.move(80, 120);
  QCOMPARE(moved.count(), 1);
  QCOMPARE(moved.front().front().toPoint(), QPoint(80, 120));
  QCOMPARE(main.mapToGlobal(QPoint()), QPoint(80, 120));
}

void HostShellWindowTest::embeddedPanelsUseContainerCoordinates() {
  HostShell shell(aoide::PanelPresentation::embedded);
  QWidget main(&shell);
  QWidget eq(&shell);
  shell.resize(600, 400);
  shell.move(100, 150);
  shell.setPrimaryPanel(&main);
  const QRect mainRect(20, 30, 200, 100);
  const QRect eqRect(250, 30, 200, 100);
  shell.placePanels({{&main, mainRect}, {&eq, eqRect}});
  const QRect host = shell.geometry();
  QVERIFY(!main.isWindow());
  QVERIFY(!eq.isWindow());
  QCOMPARE(main.geometry(), mainRect);
  QCOMPARE(eq.geometry(), eqRect);
  QCOMPARE(shell.layoutBounds(), QRect(0, 0, 600, 400));
  shell.placePanels({{&main, mainRect}, {&eq, eqRect.translated(-30, 40)}});
  QCOMPARE(shell.geometry(), host);
  QCOMPARE(main.geometry(), mainRect);
  shell.move(200, 250); // A compositor move must not alter panel layout.
  QCOMPARE(main.geometry(), mainRect);
  QCOMPARE(eq.geometry(), eqRect.translated(-30, 40));
}

void HostShellWindowTest::embeddedContainerHasNoDesktopMask() {
  HostShell shell(aoide::PanelPresentation::embedded);
  QWidget main(&shell);
  shell.setPrimaryPanel(&main);
  shell.placePanels({{&main, QRect(20, 30, 200, 100)}});
  QVERIFY(!shell.testAttribute(Qt::WA_TranslucentBackground));
  QVERIFY(!shell.windowFlags().testFlag(Qt::FramelessWindowHint));
  QVERIFY(shell.mask().isEmpty());
  QVERIFY(shell.windowHandle()->mask().isEmpty());
  QVERIFY(shell.width() < shell.virtualDesktop().width());
  QVERIFY(shell.height() < shell.virtualDesktop().height());
}

void HostShellWindowTest::embeddedResizeReportsNewLayoutBounds() {
  HostShell shell(aoide::PanelPresentation::embedded);
  shell.show();
  QCoreApplication::processEvents();
  QSignalSpy changed(&shell, &HostShell::desktopGeometryChanged);
  shell.resize(550, 350);
  QTRY_COMPARE(changed.count(), 1);
  QCOMPARE(shell.layoutBounds(), QRect(0, 0, 550, 350));
}

void HostShellWindowTest::presentationMatchesDesktopCapabilities_data() {
  QTest::addColumn<QString>("platform");
  QTest::addColumn<bool>("embedded");
  for (const char* name : {"cocoa", "windows", "xcb", "offscreen"})
    QTest::newRow(name) << QString::fromLatin1(name) << false;
  for (const char* name : {"wayland", "wayland-egl"})
    QTest::newRow(name) << QString::fromLatin1(name) << true;
}

void HostShellWindowTest::presentationMatchesDesktopCapabilities() {
  QFETCH(QString, platform);
  QFETCH(bool, embedded);
  QCOMPARE(aoide::panelPresentationFor(platform) == aoide::PanelPresentation::embedded, embedded);
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
  HostShell shell(aoide::PanelPresentation::embedded);
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
  HostShell shell(aoide::PanelPresentation::embedded);
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
  HostShell shell(aoide::PanelPresentation::embedded);
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
