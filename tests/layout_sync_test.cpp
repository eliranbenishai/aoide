#include "layout_sync.h"

#include <QTest>

using tramp::DockLayout;
using tramp::LayoutSync;
using tramp::WindowId;

namespace {

/// A desktop with no compositor behind it. Everything LayoutSync needs from the
/// outside world is one rectangle, so a monitor going away is a field write.
class FakeDesktop : public tramp::PanelSurfaces {
 public:
  explicit FakeDesktop(QRect host = {}) : host_(host) {}
  QRect hostRect() const override { return host_; }
  void setHostRect(QRect host) { host_ = host; }

  void placePanels(const QVector<tramp::PanelPlacement>& panels) override {
    ++passes;
    last = panels;
  }

  tramp::PanelPlacement placementOf(WindowId id) const {
    for (const tramp::PanelPlacement& panel : last) {
      if (panel.id == id) return panel;
    }
    return {};
  }

  int passes = 0;
  QVector<tramp::PanelPlacement> last;

 private:
  QRect host_;
};

}  // namespace

class LayoutSyncTest : public QObject {
  Q_OBJECT

 private slots:
  void nativeAndLogicalAreInversesAcrossTheZoomLadder();
  void logicalToNativeRoundsRatherThanTruncates();
  void nativeFrameRectZoomsThePanelAndItsOrigin();
  void nativeFrameRectTakesTheStoredPlaylistSizeAndTheShadedHeight();
  void setNativeFrameRoundTripsThroughTheFrame();
  void onlyThePlaylistTakesASizeFromAScreenRectangle();
  void clampPullsAPanelBackOntoTheDesktop();
  void clampLeavesEverythingAloneWhenThereIsNoHostYet();
  void clampAcceptsAScreenLeftOfThePrimary();
  void unpluggingAMonitorTranslatesAClusterThatStillFits();
  void aClusterTooWideForTheDesktopFallsBackToClampingEachPanel();
  void everyPanelReachesTheSurfacesIncludingTheHiddenOnes();
  void minimizingMainSuppressesThePanelsWithoutForgettingThem();
  void aShadedPanelKeepsTheCanvasItWillGoBackTo();
  void placeCorrectsTheFrameWhenTheDesktopOverrulesIt();
  void placingRepeatedlyDoesNotWalkTheFrame();
};

void LayoutSyncTest::nativeAndLogicalAreInversesAcrossTheZoomLadder() {
  for (int percent : {75, 100, 125, 150}) {
    LayoutSync layout({}, percent);
    QCOMPARE(layout.zoomPercent(), percent);
    const QPoint native(400, 240);
    QCOMPARE(layout.logicalToNative(layout.nativeToLogical(native)), native);
  }
}

void LayoutSyncTest::logicalToNativeRoundsRatherThanTruncates() {
  LayoutSync layout({}, 75);
  // 825 * 0.75 = 618.75. Truncating loses most of a pixel per panel, and the
  // cluster walks a pixel left every time a drag round-trips through here.
  QCOMPARE(layout.logicalToNative(QPointF(825, 348)), QPoint(619, 261));
}

void LayoutSyncTest::nativeFrameRectZoomsThePanelAndItsOrigin() {
  DockLayout dock;
  dock.main = {true, false, 100, 200, {}, {}};
  LayoutSync layout(dock, 100);
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(100, 200, 825, 348));

  LayoutSync zoomed(dock, 150);
  QCOMPARE(zoomed.nativeFrameRect(WindowId::main), QRect(150, 300, 1238, 522));
}

void LayoutSyncTest::nativeFrameRectTakesTheStoredPlaylistSizeAndTheShadedHeight() {
  DockLayout dock;
  dock.playlist = {true, false, 0, 0, 900.0, 600.0};
  LayoutSync layout(dock, 100);
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), QRect(0, 0, 900, 600));

  // Windowshade is a layout fact, not a paint one: docking measures the
  // collapsed panel by its title bar.
  dock.playlist.shaded = true;
  LayoutSync shaded(dock, 100);
  QCOMPARE(shaded.nativeFrameRect(WindowId::playlist), QRect(0, 0, 900, tramp::kTitleBar));
}

void LayoutSyncTest::setNativeFrameRoundTripsThroughTheFrame() {
  DockLayout dock;
  dock.playlist = {true, false, 0, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 75);

  const QRect moved(300, 150, 900, 600);
  layout.setNativeFrame(WindowId::playlist, moved);
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), moved);
  QCOMPARE(layout.layout().playlist.left, 400.0);
  QCOMPARE(layout.layout().playlist.top, 200.0);
}

void LayoutSyncTest::onlyThePlaylistTakesASizeFromAScreenRectangle() {
  DockLayout dock;
  LayoutSync layout(dock, 100);
  // Main and EQ never stretch, so a rectangle handed to them moves the origin
  // and nothing else — a clamp that shrank one would otherwise stick.
  layout.setNativeFrame(WindowId::main, QRect(40, 60, 200, 100));
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(40, 60, 825, 348));
  QVERIFY(!layout.layout().main.width.has_value());
}

void LayoutSyncTest::clampPullsAPanelBackOntoTheDesktop() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.about = {true, false, 1900, 1000, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), QRect(1440, 720, 480, 360));
}

void LayoutSyncTest::clampLeavesEverythingAloneWhenThereIsNoHostYet() {
  DockLayout dock;
  dock.about = {true, false, -4000, -4000, {}, {}};
  LayoutSync layout(dock, 100);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.layout().about.left, -4000.0);

  // An empty rectangle is not a desktop of zero size; it is not knowing yet.
  FakeDesktop none(QRect(0, 0, 0, 0));
  layout.setSurfaces(&none);
  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.layout().about.left, -4000.0);
}

void LayoutSyncTest::clampAcceptsAScreenLeftOfThePrimary() {
  // A monitor placed left of or above the primary gives the virtual desktop a
  // negative origin, and the clamp has to hold the panel inside that rather
  // than against zero.
  FakeDesktop desktop(QRect(-1920, -200, 3840, 1280));
  DockLayout dock;
  dock.about = {true, false, -3000, -900, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), QRect(-1920, -200, 480, 360));
}

void LayoutSyncTest::unpluggingAMonitorTranslatesAClusterThatStillFits() {
  FakeDesktop desktop(QRect(0, 0, 3840, 1080));
  DockLayout dock;
  dock.main = {true, false, 2000, 100, {}, {}};
  dock.equalizer = {true, false, 2000, 448, {}, {}};
  dock.playlist = {false, false, 2000, 796, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(2000, 100, 825, 348));

  desktop.setHostRect(QRect(0, 0, 1920, 1080));
  layout.fitClusterToHost();

  // The cluster still fits, so it is translated whole: the equalizer stays
  // flush under main rather than each panel being pulled back on its own.
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(1095, 100, 825, 348));
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer), QRect(1095, 448, 825, 348));
  // The closed playlist did not decide how far to move, but it rides along, or
  // it would be left on the monitor that has gone.
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).topLeft(), QPoint(1095, 796));
}

void LayoutSyncTest::aClusterTooWideForTheDesktopFallsBackToClampingEachPanel() {
  // Today's answer when the union cannot fit at any origin: each visible panel
  // is pulled back on its own, which stacks panels that were docked apart. What
  // it should do instead is ticket 08's decision, not this seam's — this pins
  // what the code does now so that change is visible when it is made.
  FakeDesktop desktop(QRect(0, 0, 900, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.playlist = {true, false, 825, 0, 1073.0, 696.0};
  dock.equalizer.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(0, 0, 825, 348));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), QRect(0, 0, 900, 696));
}

void LayoutSyncTest::everyPanelReachesTheSurfacesIncludingTheHiddenOnes() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 40, 40, {}, {}};
  dock.equalizer = {true, false, 40, 388, {}, {}};
  dock.playlist.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.place();
  QCOMPARE(desktop.last.size(), 5);
  QCOMPARE(desktop.placementOf(WindowId::main).screen, QRect(40, 40, 825, 348));
  QVERIFY(desktop.placementOf(WindowId::equalizer).visible);
  QVERIFY(!desktop.placementOf(WindowId::playlist).visible);
}

void LayoutSyncTest::minimizingMainSuppressesThePanelsWithoutForgettingThem() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.equalizer = {true, false, 0, 348, {}, {}};
  dock.about = {false, false, 900, 40, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.setMainMinimized(true);
  layout.place();
  QVERIFY(!desktop.placementOf(WindowId::equalizer).visible);
  // The frame is untouched: minimize hides a panel, it does not close it.
  QVERIFY(layout.layout().equalizer.visible);

  layout.setMainMinimized(false);
  layout.place();
  QVERIFY(desktop.placementOf(WindowId::equalizer).visible);
  // A panel that was already closed stays closed through the round trip.
  QVERIFY(!desktop.placementOf(WindowId::about).visible);
}

void LayoutSyncTest::aShadedPanelKeepsTheCanvasItWillGoBackTo() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.playlist = {true, true, 0, 0, 900.0, 600.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.place();
  const tramp::PanelPlacement pl = desktop.placementOf(WindowId::playlist);
  // The screen rectangle collapses to the title bar; the canvas handed to the
  // panel does not, or unshading would restore a 42px playlist.
  QCOMPARE(pl.screen.height(), tramp::kTitleBar);
  QCOMPARE(pl.logicalSize, QSize(900, 600));
  QVERIFY(pl.shaded);
}

void LayoutSyncTest::placeCorrectsTheFrameWhenTheDesktopOverrulesIt() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.about = {true, false, 1800, 40, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.place();
  QCOMPARE(desktop.placementOf(WindowId::about).screen, QRect(1440, 40, 480, 360));
  // The frame agrees with where the panel actually went, so nothing has to read
  // the widget back to find out where it ended up.
  QCOMPARE(layout.layout().about.left, 1440.0);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), desktop.placementOf(WindowId::about).screen);
}

void LayoutSyncTest::placingRepeatedlyDoesNotWalkTheFrame() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 101, 67, {}, {}};
  dock.playlist = {true, false, 200, 500, 1073.0, 696.0};
  LayoutSync layout(dock, 75);
  layout.setSurfaces(&desktop);

  for (int i = 0; i < 20; ++i) layout.place();

  // A drag places on every move. Rounding through native space and back on each
  // pass would creep the cluster a pixel at a time across a long gesture.
  QCOMPARE(layout.layout().main.left, 101.0);
  QCOMPARE(layout.layout().main.top, 67.0);
  QCOMPARE(*layout.layout().playlist.width, 1073.0);
  QCOMPARE(*layout.layout().playlist.height, 696.0);
}

QTEST_APPLESS_MAIN(LayoutSyncTest)
#include "layout_sync_test.moc"
