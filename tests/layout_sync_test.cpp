#include "chrome_layout.h"
#include "layout_sync.h"

#include <QTest>
#include <functional>

using aoide::DockLayout;
using aoide::LayoutSync;
using aoide::WindowId;

namespace {

/// A desktop with no compositor behind it. Everything LayoutSync needs from the
/// outside world is one rectangle, so a monitor going away is a field write.
class FakeDesktop : public aoide::PanelSurfaces {
 public:
  explicit FakeDesktop(QRect host = {}) : host_(host) {}
  QRect hostRect() const override { return host_; }
  void setHostRect(QRect host) { host_ = host; }

  /// Empty until a test says otherwise, which is "no work area known" — the
  /// state every test that is not about zoom availability wants.
  QRect workAreaFor(QRect clusterNative) const override {
    clusterAsked = clusterNative;
    if (!screen_.isEmpty() && clusterNative.intersects(screen_)) return screenWork_;
    return work_;
  }
  void setWorkArea(QRect work) { work_ = work; }
  /// A second display's work area, used when the asked rectangle sits on it.
  /// The default [work_] stays the answer for everything else, including the
  /// primary — so a playlist on the neighbour cannot be measured against the
  /// taskbar of a screen it is not on.
  void setWorkAreaForScreen(QRect screen, QRect work) {
    screen_ = screen;
    screenWork_ = work;
  }
  mutable QRect clusterAsked;

  void placePanels(const QVector<aoide::PanelPlacement>& panels) override {
    ++passes;
    last = panels;
    if (onPlace) onPlace();
  }

  std::function<void()> onPlace;

  aoide::PanelPlacement placementOf(WindowId id) const {
    for (const aoide::PanelPlacement& panel : last) {
      if (panel.id == id) return panel;
    }
    return {};
  }

  int passes = 0;
  QVector<aoide::PanelPlacement> last;

 private:
  QRect host_;
  QRect work_;
  QRect screen_;
  QRect screenWork_;
};

/// Main, EQ and playlist at their defaults: the vertical stack a first launch
/// opens on, 1073 x 1392 logical.
DockLayout defaultCluster() {
  DockLayout dock;
  dock.main = aoide::WindowFrame::mainDefault();
  dock.equalizer = aoide::WindowFrame::equalizerDefault();
  dock.playlist = aoide::WindowFrame::playlistDefault();
  return dock;
}

/// A 1080p display less a desktop panel along the bottom.
constexpr QRect kWorkArea1080p(0, 0, 1920, 1044);

}  // namespace

class LayoutSyncTest : public QObject {
  Q_OBJECT

 private slots:
  void nativeAndLogicalAreInversesAcrossTheZoomLadder();
  void logicalToNativeRoundsRatherThanTruncates();
  void nativeFrameRectZoomsThePanelAndItsOrigin();
  void nativeFrameRectTakesTheStoredPlaylistSizeAndTheShadedHeight();
  void setNativeFrameRoundTripsThroughTheFrame();
  void aScreenRectanglePlacesAPanelWithoutWritingItsSize();
  void anAutomaticClampDoesNotDestroyTheChosenPlaylistSize();
  void aPlaylistGripUnderTheTaskbarIsPulledIntoTheWorkArea();
  void theWorkAreaIsTakenFromTheScreenThePlaylistIsOn();
  void anEmptyWorkAreaWithdrawsNoPlaylistClamp();
  void afterAFitNeitherDockedSiblingOverlapsMain();
  void aFitShrinksThePlaylistTowardItsMinimumToClearMain();
  void aFitNeverAsksForAPlaylistSmallerThanTheMinimumInForce();
  void aHandDragMayLeaveASiblingOverMain();
  void aMoveWithNoSurfacesDoesNotLatchTheNextPlace();
  void aReentrantPlaceLeavesTheLatchForTheOuterPass();
  void aWestResizeKeepsThePinnedEastEdgeThroughPlace();
  void aNorthResizeKeepsThePinnedSouthEdgeThroughPlace();
  void aUserResizeStillPullsTheGripIntoTheWorkArea();
  void aZoomStepThatFitsAtThePlaylistMinimumIsTakenAndThePlaylistShrinks();
  void aZoomStepUpIsRefusedAtTheTrueFloor();
  void clampPullsAPanelBackOntoTheDesktop();
  void clampLeavesEverythingAloneWhenThereIsNoHostYet();
  void clampAcceptsAScreenLeftOfThePrimary();
  void aClusterIsTranslatedWholeOntoAScreenLeftOfThePrimary();
  void clampingOntoAScreenLeftOfThePrimaryKeepsTheEdgesItDoesNotBreak();
  void unpluggingAMonitorTranslatesAClusterThatStillFits();
  void aClusterTooWideForTheDesktopParksThePlaylistBelowMainAndKeepsTheEdges();
  void aFitThatCannotRestoreAFlushPairDropsTheBrokenEdge();
  void aCrawlTooSlowToPeelStillLosesTheEdgeItCrawledAwayFrom();
  void aClusterThatOnlyMovedKeepsEveryEdgeItWasDockedBy();
  void aZoomStepTheWorkAreaCannotHoldIsNotOffered();
  void closingAPanelBringsTheStepsItCrowdedOutBack();
  void theWorkAreaIsAskedForWhereTheClusterActuallyIs();
  void anUnknownWorkAreaWithdrawsNoStep();
  void theLadderRefusesAStepItDoesNotCarry();
  void zoomingBackOutOfAStepTheDisplayOutgrewIsAlwaysOffered();
  void aZoomStepThatOutgrowsTheDesktopShrinksThePlaylistAndKeepsTheEdges();
  void aRefusedZoomStepLeavesTheDockedClusterExactlyWhereItWas();
  void thePanelsAreScaledToTheStepTheLayoutTookAndNotTheOneItWasOffered();
  void everyPanelReachesTheSurfacesIncludingTheHiddenOnes();
  void minimizingMainSuppressesThePanelsWithoutForgettingThem();
  void aShadedPanelKeepsTheCanvasItWillGoBackTo();
  void placeCorrectsTheFrameWhenTheDesktopOverrulesIt();
  void placingRepeatedlyDoesNotWalkTheFrame();
  void theFirstLaunchStackClearsTheReservedTop();
  void aMainDragCannotParkTheClusterUnderTheStrip();
  void anUnknownWorkAreaLeavesTheClusterWhereTheHostPutIt();
  void aWorkAreaFlushWithTheHostTopMovesNothing();
  void theReservedTopIsTakenFromMainsOwnScreen();
  void clampToHostPushesASiblingClearOfTheStrip();
};

void LayoutSyncTest::nativeAndLogicalAreInversesAcrossTheZoomLadder() {
  for (qreal percent : aoide::kZoomSteps) {
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
  QCOMPARE(shaded.nativeFrameRect(WindowId::playlist), QRect(0, 0, 900, aoide::kTitleBar));
}

void LayoutSyncTest::setNativeFrameRoundTripsThroughTheFrame() {
  DockLayout dock;
  dock.playlist = {true, false, 0, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 75);

  const QRect moved(300, 150, 900, 600);
  layout.setNativeFrame(WindowId::playlist, moved);
  // Size is not part of the trip: a screen rectangle is a placement, and
  // the chosen canvas has to survive it.
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).topLeft(), QPoint(300, 150));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).size(), QSize(805, 522));
  QCOMPARE(layout.layout().playlist.left, 400.0);
  QCOMPARE(layout.layout().playlist.top, 200.0);
  QCOMPARE(*layout.layout().playlist.width, 1073.0);
  QCOMPARE(*layout.layout().playlist.height, 696.0);
}

void LayoutSyncTest::aScreenRectanglePlacesAPanelWithoutWritingItsSize() {
  DockLayout dock;
  dock.playlist = {true, false, 0, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 100);
  // Main and EQ never stretch, so a rectangle handed to them moves the origin
  // and nothing else — a clamp that shrank one would otherwise stick.
  layout.setNativeFrame(WindowId::main, QRect(40, 60, 200, 100));
  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(40, 60, 825, 348));
  QVERIFY(!layout.layout().main.width.has_value());

  // The playlist's chosen size is what the listener dragged it to. A screen
  // rectangle is a placement, not a resize: the desktop may have to fit a
  // smaller panel right now, and writing that fit back is what made a clamp
  // destroy the width they chose. Only resizePlaylist writes the chosen size.
  layout.setNativeFrame(WindowId::playlist, QRect(80, 90, 400, 200));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).topLeft(), QPoint(80, 90));
  QCOMPARE(*layout.layout().playlist.width, 1073.0);
  QCOMPARE(*layout.layout().playlist.height, 696.0);
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).size(), QSize(1073, 696));
}

void LayoutSyncTest::anAutomaticClampDoesNotDestroyTheChosenPlaylistSize() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.equalizer.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  layout.docking().resizePlaylist(QSizeF(900, 600));
  layout.setNativeFrame(WindowId::playlist, QRect(40, 40, 900, 600));
  layout.place();
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen.size(), QSize(900, 600));
  QCOMPARE(desktop.placementOf(WindowId::playlist).logicalSize, QSize(900, 600));
  QCOMPARE(*layout.layout().playlist.width, 900.0);
  QCOMPARE(*layout.layout().playlist.height, 600.0);

  desktop.setHostRect(QRect(0, 0, 500, 400));
  layout.place();
  // The recorder is the screen: the panel paints at the fitted size it was
  // placed at. The frame still holds the size the listener chose, so giving
  // the desktop its room back can restore it.
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen.size(), QSize(500, 400));
  QCOMPARE(desktop.placementOf(WindowId::playlist).logicalSize, QSize(500, 400));
  QCOMPARE(*layout.layout().playlist.width, 900.0);
  QCOMPARE(*layout.layout().playlist.height, 600.0);

  desktop.setHostRect(QRect(0, 0, 1920, 1080));
  layout.place();
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen.size(), QSize(900, 600));
  QCOMPARE(desktop.placementOf(WindowId::playlist).logicalSize, QSize(900, 600));
  QCOMPARE(*layout.layout().playlist.width, 900.0);
  QCOMPARE(*layout.layout().playlist.height, 600.0);
}

void LayoutSyncTest::aPlaylistGripUnderTheTaskbarIsPulledIntoTheWorkArea() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(QRect(0, 0, 1920, 1044));
  DockLayout dock;
  dock.equalizer.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  layout.docking().resizePlaylist(QSizeF(900, 600));
  layout.setNativeFrame(WindowId::playlist, QRect(100, 500, 900, 600));
  layout.place();

  const QRect placed = desktop.placementOf(WindowId::playlist).screen;
  const QRect work(0, 0, 1920, 1044);
  QVERIFY(work.contains(placed.bottomRight()));
  QCOMPARE(placed.size(), QSize(900, 600));
  QCOMPARE(placed.topLeft(), QPoint(100, 444));
  QCOMPARE(*layout.layout().playlist.width, 900.0);
  QCOMPARE(*layout.layout().playlist.height, 600.0);
}

void LayoutSyncTest::theWorkAreaIsTakenFromTheScreenThePlaylistIsOn() {
  FakeDesktop desktop(QRect(0, 0, 3840, 1080));
  desktop.setWorkArea(QRect(0, 0, 1920, 1044));
  desktop.setWorkAreaForScreen(QRect(1920, 0, 1920, 1080), QRect(1920, 0, 1920, 1080));
  DockLayout dock;
  dock.equalizer.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  layout.docking().resizePlaylist(QSizeF(800, 600));
  layout.setNativeFrame(WindowId::playlist, QRect(2000, 200, 800, 600));
  layout.place();

  // The primary's taskbar is not this panel's problem. Measuring the neighbour
  // against that work area would drag it onto the primary just to tuck a grip
  // that was already reachable on its own display.
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen, QRect(2000, 200, 800, 600));
  QCOMPARE(desktop.clusterAsked, QRect(2000, 200, 800, 600));
}

void LayoutSyncTest::anEmptyWorkAreaWithdrawsNoPlaylistClamp() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.equalizer.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  layout.docking().resizePlaylist(QSizeF(900, 560));
  layout.setNativeFrame(WindowId::playlist, QRect(40, 500, 900, 560));
  layout.place();

  // Bottom 1060 sits on the virtual desktop and would be pulled if a 1044-tall
  // work area were invented. Empty means not known, so the host is the last word.
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen, QRect(40, 500, 900, 560));
}

void LayoutSyncTest::afterAFitNeitherDockedSiblingOverlapsMain() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer = {true, false, 0, 0, {}, {}};
  dock.playlist = {true, false, 0, 0, 900.0, 600.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  const QRect mainBefore = layout.nativeFrameRect(WindowId::main);
  layout.fitClusterToHost();
  layout.place();

  const QRect main = desktop.placementOf(WindowId::main).screen;
  const QRect eq = desktop.placementOf(WindowId::equalizer).screen;
  const QRect playlist = desktop.placementOf(WindowId::playlist).screen;
  QCOMPARE(main, mainBefore);
  QVERIFY(!eq.intersects(main));
  QVERIFY(!playlist.intersects(main));
}

void LayoutSyncTest::aFitShrinksThePlaylistTowardItsMinimumToClearMain() {
  // Short enough that parking below main cannot keep the chosen height, so
  // the only room left is a slice to main's right — narrower than chosen,
  // still above the playlist minimum.
  FakeDesktop desktop(QRect(0, 0, 1500, 500));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 0, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  const QRect mainBefore = layout.nativeFrameRect(WindowId::main);
  layout.fitClusterToHost();
  layout.place();

  const QRect main = desktop.placementOf(WindowId::main).screen;
  const QRect playlist = desktop.placementOf(WindowId::playlist).screen;
  QCOMPARE(main, mainBefore);
  QVERIFY(!playlist.intersects(main));
  QCOMPARE(playlist.topLeft(), QPoint(825, 0));
  QCOMPARE(playlist.width(), 675);
  QCOMPARE(desktop.placementOf(WindowId::playlist).logicalSize, QSize(675, 500));
  QCOMPARE(*layout.layout().playlist.width, 1073.0);
  QCOMPARE(*layout.layout().playlist.height, 696.0);
}

void LayoutSyncTest::aFitNeverAsksForAPlaylistSmallerThanTheMinimumInForce() {
  const QSize min =
      aoide::playlistMinLogical(240, aoide::kPlaylistStripTotalReserve);
  FakeDesktop desktop(QRect(0, 0, 1500, 500));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 0, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 100);
  layout.setPlaylistMinLogical(min);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  layout.place();

  const QRect playlist = desktop.placementOf(WindowId::playlist).screen;
  QVERIFY(playlist.width() >= min.width());
  QVERIFY(playlist.height() >= min.height());
}

void LayoutSyncTest::aHandDragMayLeaveASiblingOverMain() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 400, 400, 900.0, 600.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.docking().move(WindowId::playlist, QPointF(0, 0), false, false);
  layout.clampToHost(WindowId::playlist);
  layout.place();

  // A title-bar drag is the listener's. Automatic correction that pulled the
  // playlist off main here would fight the hand that just put it there.
  QVERIFY(desktop.placementOf(WindowId::playlist).screen.intersects(
      desktop.placementOf(WindowId::main).screen));
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen.topLeft(), QPoint(0, 0));
}

void LayoutSyncTest::aMoveWithNoSurfacesDoesNotLatchTheNextPlace() {
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 0, 0, 900.0, 600.0};
  LayoutSync layout(dock, 100);

  layout.docking().move(WindowId::playlist, QPointF(0, 0), false, false);
  layout.place();

  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  layout.setSurfaces(&desktop);
  layout.place();

  // The move never landed on a surface, so the latch is stale. Leaving it
  // would skip un-overlap on this automatic pass.
  QVERIFY(!desktop.placementOf(WindowId::playlist).screen.intersects(
      desktop.placementOf(WindowId::main).screen));
}

void LayoutSyncTest::aReentrantPlaceLeavesTheLatchForTheOuterPass() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 400, 400, 900.0, 600.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  desktop.onPlace = [&]() { layout.place(); };

  layout.docking().move(WindowId::playlist, QPointF(0, 0), false, false);
  layout.clampToHost(WindowId::playlist);
  layout.place();

  // A re-entrant place must not consume the latch the outer pass still needs.
  // If it did, this hand-drag would be un-overlapped mid-gesture.
  QVERIFY(desktop.placementOf(WindowId::playlist).screen.intersects(
      desktop.placementOf(WindowId::main).screen));
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen.topLeft(), QPoint(0, 0));
}

void LayoutSyncTest::aWestResizeKeepsThePinnedEastEdgeThroughPlace() {
  // Wide enough that parking on main's right still fits the grown width —
  // otherwise clearKeep falls through to below and the rewrite is a different
  // rectangle than the east-grow the west grip actually produces in the app.
  FakeDesktop desktop(QRect(0, 0, 2560, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 825, 0, 1073.0, 696.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  layout.place();
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen, QRect(825, 0, 1073, 696));

  // West grip: the east edge stays put and the origin walks left. place() has
  // to honour that rectangle — parking back on main's right would grow east
  // instead, which is what made the west grip a no-op in the app.
  layout.docking().resizePlaylist(QPointF(785, 0), QSizeF(1113, 696));
  layout.clampToHost(WindowId::playlist);
  layout.place();

  QCOMPARE(desktop.placementOf(WindowId::playlist).screen, QRect(785, 0, 1113, 696));
  QCOMPARE(*layout.layout().playlist.width, 1113.0);
  QCOMPARE(*layout.layout().playlist.height, 696.0);
}

void LayoutSyncTest::aNorthResizeKeepsThePinnedSouthEdgeThroughPlace() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer.visible = false;
  dock.playlist = {true, false, 0, 348, 1073.0, 696.0};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);
  layout.place();
  QCOMPARE(desktop.placementOf(WindowId::playlist).screen, QRect(0, 348, 1073, 696));

  // North grip: the south edge stays put and the origin walks up, covering
  // main by the drag. Automatic un-overlap would park the panel on its right
  // park-side and throw the pinned edge away.
  layout.docking().resizePlaylist(QPointF(0, 338), QSizeF(1073, 706));
  layout.clampToHost(WindowId::playlist);
  layout.place();

  QCOMPARE(desktop.placementOf(WindowId::playlist).screen, QRect(0, 338, 1073, 706));
  QCOMPARE(*layout.layout().playlist.width, 1073.0);
  QCOMPARE(*layout.layout().playlist.height, 706.0);
}

void LayoutSyncTest::aUserResizeStillPullsTheGripIntoTheWorkArea() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(QRect(0, 0, 1920, 1044));
  DockLayout dock;
  dock.equalizer.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  // A resize is a user gesture, but the grip still has to stay reachable. The
  // latch that stops un-overlap must not skip the work-area clamp.
  layout.docking().resizePlaylist(QPointF(100, 500), QSizeF(900, 600));
  layout.clampToHost(WindowId::playlist);
  layout.place();

  const QRect placed = desktop.placementOf(WindowId::playlist).screen;
  QVERIFY(QRect(0, 0, 1920, 1044).contains(placed.bottomRight()));
  QCOMPARE(placed.size(), QSize(900, 600));
  QCOMPARE(placed.topLeft(), QPoint(100, 444));
  QCOMPARE(*layout.layout().playlist.width, 900.0);
  QCOMPARE(*layout.layout().playlist.height, 600.0);
}

void LayoutSyncTest::aZoomStepThatFitsAtThePlaylistMinimumIsTakenAndThePlaylistShrinks() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  LayoutSync layout(defaultCluster(), 75);
  layout.setSurfaces(&desktop);

  // The default stack is 1392 logical tall: 1044 native at 75% and 1392 at
  // 100%. The playlist can shrink to its minimum and the stack still fits,
  // so the step is taken and the shrink is the fit — not a new chosen size.
  QVERIFY(layout.setZoomPercent(100));
  QCOMPARE(layout.zoomPercent(), qreal(100));
  layout.fitClusterToHost();
  layout.place();

  const QRect main = desktop.placementOf(WindowId::main).screen;
  const QRect playlist = desktop.placementOf(WindowId::playlist).screen;
  QVERIFY(!playlist.intersects(main));
  QVERIFY(kWorkArea1080p.contains(playlist.bottomRight()));
  QCOMPARE(playlist.height(), 348);
  QVERIFY(!layout.layout().playlist.width.has_value());
  QVERIFY(!layout.layout().playlist.height.has_value());
  QCOMPARE(desktop.placementOf(WindowId::playlist).logicalSize.height(), 348);
}

void LayoutSyncTest::aZoomStepUpIsRefusedAtTheTrueFloor() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  LayoutSync layout(defaultCluster(), 100);
  layout.setSurfaces(&desktop);

  // At 125% even a playlist at its minimum leaves the default stack taller
  // than 1044: that is the true floor, and the step stays withdrawn.
  QVERIFY(!layout.zoomStepAvailable(125));
  QVERIFY(!layout.zoomStepUp().has_value());
  QVERIFY(!layout.setZoomPercent(125));
  QCOMPARE(layout.zoomPercent(), qreal(100));
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

  // And the far edge is a distance from that origin rather than from zero, so a
  // panel overshooting the right-hand monitor comes back to the same place it
  // would on a desktop that started at zero.
  layout.setNativeFrame(WindowId::about, QRect(5000, 5000, 480, 360));
  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), QRect(1440, 720, 480, 360));
}

void LayoutSyncTest::aClusterIsTranslatedWholeOntoAScreenLeftOfThePrimary() {
  // A monitor left of and above the primary puts the whole cluster in negative
  // coordinates. Every unit test before this one used a desktop starting at
  // zero, where a sign error and a correct answer look the same.
  FakeDesktop desktop(QRect(-1920, -1080, 3840, 2160));
  DockLayout dock;
  dock.main = {true, false, -4000, -3000, {}, {}};
  dock.equalizer = {true, false, -4000, -2652, {}, {}};
  dock.playlist.visible = false;
  dock.dockEdges = {{WindowId::equalizer, WindowId::main, aoide::DockSide::top}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  layout.place();

  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(-1920, -1080, 825, 348));
  // Translated whole, so the equalizer is still flush under main and its edge
  // survives the trip: the cluster moved, the arrangement did not.
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer), QRect(-1920, -732, 825, 348));
  QCOMPARE(layout.layout().dockEdges.size(), 1);
}

void LayoutSyncTest::clampingOntoAScreenLeftOfThePrimaryKeepsTheEdgesItDoesNotBreak() {
  // The same fallback as a desktop starting at zero, on the monitor that is left
  // of the primary: each panel pulled inside. The playlist cannot sit on main,
  // so it parks below. That is still a contact, so the edges stay.
  FakeDesktop desktop(QRect(-1920, -200, 900, 1080));
  DockLayout dock;
  dock.main = {true, false, -1920, -200, {}, {}};
  dock.playlist = {true, false, -1095, -200, 1073.0, 696.0};
  dock.equalizer.visible = false;
  dock.dockEdges = {
      {WindowId::playlist, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::top},
  };
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  layout.place();

  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(-1920, -200, 825, 348));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), QRect(-1920, 148, 900, 696));
  QVERIFY(!desktop.placementOf(WindowId::playlist).screen.intersects(
      desktop.placementOf(WindowId::main).screen));
  // Flush under main is still a contact, so the edges stay — they would drop
  // only if the pair had been stacked on top of each other.
  QVERIFY(!layout.layout().dockEdges.isEmpty());
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

void LayoutSyncTest::aClusterTooWideForTheDesktopParksThePlaylistBelowMainAndKeepsTheEdges() {
  // A union that cannot fit at any origin is pulled back panel by panel, which
  // is what a monitor going away is promised to do. The playlist cannot cover
  // main, so it parks below — still flush, so the edges stay.
  FakeDesktop desktop(QRect(0, 0, 900, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.playlist = {true, false, 825, 0, 1073.0, 696.0};
  dock.equalizer.visible = false;
  dock.dockEdges = {
      {WindowId::playlist, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::top},
  };
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  layout.place();

  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(0, 0, 825, 348));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), QRect(0, 348, 900, 696));
  QVERIFY(!desktop.placementOf(WindowId::playlist).screen.intersects(
      desktop.placementOf(WindowId::main).screen));
  QVERIFY(!layout.layout().dockEdges.isEmpty());
}

void LayoutSyncTest::aFitThatCannotRestoreAFlushPairDropsTheBrokenEdge() {
  // Short and narrow enough that no park-side has room for the playlist at
  // its minimum, so clearKeep cannot restore a flush pair. A stale edge
  // would keep main inside the playlist's snap group and exclude it as a
  // target — the panel could neither hold the dock nor get it back.
  FakeDesktop desktop(QRect(0, 0, 900, 500));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.playlist = {true, false, 825, 0, 1073.0, 696.0};
  dock.equalizer.visible = false;
  dock.dockEdges = {
      {WindowId::playlist, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::top},
  };
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  layout.place();

  QVERIFY(layout.layout().dockEdges.isEmpty());
}

void LayoutSyncTest::aCrawlTooSlowToPeelStillLosesTheEdgeItCrawledAwayFrom() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.equalizer = {true, false, 0, 348, {}, {}};
  dock.playlist.visible = false;
  dock.dockEdges = {{WindowId::equalizer, WindowId::main, aoide::DockSide::top}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  // Two logical pixels per move event is under the peel delta, so nothing in
  // the drag itself ever breaks the edge, however far the crawl goes.
  for (int step = 1; step <= 30; ++step) {
    layout.docking().move(WindowId::equalizer, QPointF(0, 348 + 2 * step), false, false);
    layout.place();
  }
  QCOMPARE(layout.layout().equalizer.top, 408.0);
  QVERIFY(layout.layout().dockEdges.isEmpty());

  // Losing the edge is what lets the panel back: while main was still in the
  // dragged panel's group it was excluded as a snap target, so a drop this
  // close to it did nothing at all.
  layout.docking().move(WindowId::equalizer, QPointF(0, 352), false, true);
  QCOMPARE(layout.layout().equalizer.top, 348.0);
  QVERIFY(!layout.layout().dockEdges.isEmpty());
}

void LayoutSyncTest::aClusterThatOnlyMovedKeepsEveryEdgeItWasDockedBy() {
  // The cluster goes through native pixels and back on every fit, and 825
  // logical px is 618.75 native at the default step, so a snapped pair can come
  // back a third of a pixel out of true. That is rounding, not an undock.
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  DockLayout dock;
  dock.main = {true, false, 2000, 0, {}, {}};
  dock.equalizer = {true, false, 2000, 348, {}, {}};
  dock.playlist = {true, false, 2825, 0, 1073.0, 696.0};
  dock.dockEdges = {
      {WindowId::equalizer, WindowId::main, aoide::DockSide::top},
      {WindowId::equalizer, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::top},
  };
  LayoutSync layout(dock, 75);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();
  layout.place();

  QCOMPARE(layout.layout().dockEdges.size(), 4);
  QVERIFY(layout.layout().playlist.left - layout.layout().main.left - 825.0 <=
          aoide::DockingCoordinator::kEdgeSlack);
}

void LayoutSyncTest::aZoomStepTheWorkAreaCannotHoldIsNotOffered() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  LayoutSync layout(defaultCluster(), 75);
  layout.setSurfaces(&desktop);

  // The default stack is 1392 logical tall. The playlist can shrink to its
  // minimum and still fit 100%, so that step is offered. 125% is the true
  // floor — even a minimum playlist leaves the stack taller than 1044 — and
  // handing that over changes nothing.
  QCOMPARE(layout.clusterLogicalSize(), QSizeF(1073, 1392));
  QVERIFY(layout.zoomStepAvailable(100));
  QVERIFY(!layout.zoomStepAvailable(125));
  QVERIFY(!layout.setZoomPercent(125));
  QCOMPARE(layout.zoomPercent(), qreal(75));
}

void LayoutSyncTest::closingAPanelBringsTheStepsItCrowdedOutBack() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  DockLayout dock = defaultCluster();
  dock.playlist.visible = false;
  LayoutSync layout(dock, 75);
  layout.setSurfaces(&desktop);

  // Availability is a question about what is open, not about the ladder, so a
  // step withdrawn while the playlist was up comes back when it is closed.
  QCOMPARE(layout.clusterLogicalSize(), QSizeF(825, 696));
  QCOMPARE(layout.zoomStepUp().value_or(0), qreal(100));
  QVERIFY(layout.zoomStepAvailable(150));
}

void LayoutSyncTest::theWorkAreaIsAskedForWhereTheClusterActuallyIs() {
  FakeDesktop desktop(QRect(-1920, 0, 3840, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  DockLayout dock = defaultCluster();
  dock.playlist.visible = false;
  dock.main.left = -1600;
  dock.equalizer.left = -1600;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.zoomStepUp();
  // Which display the ladder is measured against is a question about where the
  // panels are, so the cluster's own rectangle is what gets asked — not the
  // desktop, and not the primary screen.
  QCOMPARE(desktop.clusterAsked, QRect(-1600, 0, 825, 696));
}

void LayoutSyncTest::anUnknownWorkAreaWithdrawsNoStep() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  LayoutSync layout(defaultCluster(), 75);
  layout.setSurfaces(&desktop);

  // No work area is not a display of no size: it is a question nobody could
  // answer, and a step is only withdrawn on evidence.
  for (qreal step : {qreal(100), qreal(125), qreal(150)}) {
    QVERIFY(layout.zoomStepAvailable(step));
  }
  QVERIFY(layout.setZoomPercent(150));
  QCOMPARE(layout.zoomPercent(), qreal(150));
}

void LayoutSyncTest::theLadderRefusesAStepItDoesNotCarry() {
  LayoutSync layout(defaultCluster(), 75);
  // Nothing off the ladder reaches here today, and every caller walks the steps
  // — but a setter that took any number was one careless call from a zoom the
  // chrome has no readout for.
  QVERIFY(!layout.setZoomPercent(137));
  QVERIFY(layout.setZoomPercent(50));
  QCOMPARE(layout.zoomPercent(), qreal(50));
  QVERIFY(layout.setZoomPercent(125));
}

void LayoutSyncTest::zoomingBackOutOfAStepTheDisplayOutgrewIsAlwaysOffered() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  LayoutSync layout(defaultCluster(), 150);
  layout.setSurfaces(&desktop);

  // A layout persisted on a bigger display restores at a step this one cannot
  // hold — 150% of the default stack is 2088 native tall against 1044 of work
  // area. Withdrawing the way out would strand the listener at it, so a step at
  // or below the one in force is always carried.
  QVERIFY(!aoide::zoomStepFits(layout.clusterLogicalSize(), kWorkArea1080p.size(), 150));
  QCOMPARE(layout.zoomStepDown().value_or(0), qreal(125));
  QVERIFY(layout.setZoomPercent(125));
  QCOMPARE(layout.zoomStepDown().value_or(0), qreal(100));
  QVERIFY(layout.setZoomPercent(100));
  QCOMPARE(layout.zoomStepDown().value_or(0), qreal(75));
  QVERIFY(layout.setZoomPercent(75));
  QCOMPARE(layout.zoomStepDown().value_or(0), qreal(62.5));
  QVERIFY(layout.setZoomPercent(62.5));
  QCOMPARE(layout.zoomStepDown().value_or(0), qreal(50));

  LayoutSync floor(defaultCluster(), 50);
  floor.setSurfaces(&desktop);
  QVERIFY(!floor.zoomStepDown().has_value());
}

namespace {

/// Main with the playlist docked flush against its right edge, and the two dock
/// edges a two-axis snap leaves behind.
DockLayout playlistDockedRightOfMain() {
  DockLayout dock;
  dock.main = {true, false, 0, 0, {}, {}};
  dock.playlist = {true, false, 825, 0, 1073.0, 696.0};
  dock.equalizer.visible = false;
  dock.dockEdges = {
      {WindowId::playlist, WindowId::main, aoide::DockSide::left},
      {WindowId::playlist, WindowId::main, aoide::DockSide::top},
  };
  return dock;
}

}  // namespace

void LayoutSyncTest::aZoomStepThatOutgrowsTheDesktopShrinksThePlaylistAndKeepsTheEdges() {
  // No work area to consult, so the step is taken — and a step that is taken
  // has to leave the layout honest. The playlist absorbs the extra width; main
  // keeps its rectangle and the dock holds if the contact is still flush.
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  LayoutSync layout(playlistDockedRightOfMain(), 75);
  layout.setSurfaces(&desktop);
  layout.fitClusterToHost();
  layout.place();
  QCOMPARE(layout.layout().dockEdges.size(), 2);

  QVERIFY(layout.setZoomPercent(125));
  layout.fitClusterToHost();
  layout.place();

  const QRect main = desktop.placementOf(WindowId::main).screen;
  const QRect playlist = desktop.placementOf(WindowId::playlist).screen;
  QVERIFY(!playlist.intersects(main));
  QCOMPARE(playlist.left(), 1031);
  QVERIFY(playlist.width() < 1341);
  QCOMPARE(*layout.layout().playlist.width, 1073.0);
  QCOMPARE(layout.layout().dockEdges.size(), 2);
}

void LayoutSyncTest::aRefusedZoomStepLeavesTheDockedClusterExactlyWhereItWas() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  LayoutSync layout(playlistDockedRightOfMain(), 75);
  layout.setSurfaces(&desktop);
  layout.fitClusterToHost();
  layout.place();

  // 150% is the true floor for this pair: even a playlist at its minimum is
  // 2115 native, past 1920. A refusal is not a quiet half-application.
  QVERIFY(!layout.setZoomPercent(150));
  layout.place();
  QCOMPARE(layout.zoomPercent(), qreal(75));
  QCOMPARE(layout.layout().playlist.left, 825.0);
  QCOMPARE(layout.layout().dockEdges.size(), 2);
}

void LayoutSyncTest::thePanelsAreScaledToTheStepTheLayoutTookAndNotTheOneItWasOffered() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(kWorkArea1080p);
  LayoutSync layout(playlistDockedRightOfMain(), 75);
  layout.setSurfaces(&desktop);
  layout.fitClusterToHost();
  layout.place();

  const QRect mainFrame = desktop.placementOf(WindowId::main).screen;
  const QRect playlistFrame = desktop.placementOf(WindowId::playlist).screen;

  // A panel's frame comes from `place`, while the scale its chrome is painted at
  // is pushed separately by whoever asked for the step. The two agree only if
  // the pusher reads the step back instead of reusing the one it offered: a
  // refusal that leaves the frames at 75% while the chrome is scaled to 150%
  // paints every panel's contents outside the panel.
  layout.setZoomPercent(150);
  const qreal scaledTo = layout.zoomPercent();
  QCOMPARE(scaledTo, qreal(75));

  const LayoutSync asPainted(layout.layout(), scaledTo);
  QCOMPARE(asPainted.nativeFrameRect(WindowId::main), mainFrame);
  QCOMPARE(asPainted.nativeFrameRect(WindowId::playlist), playlistFrame);
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
  QCOMPARE(desktop.last.size(), aoide::kPanelCount);
  QCOMPARE(desktop.placementOf(WindowId::main).screen, QRect(40, 40, 825, 348));
  QVERIFY(desktop.placementOf(WindowId::equalizer).visible);
  QVERIFY(!desktop.placementOf(WindowId::playlist).visible);
  QCOMPARE(desktop.placementOf(WindowId::skins).id, WindowId::skins);
  QVERIFY(!desktop.placementOf(WindowId::skins).visible);
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
  const aoide::PanelPlacement pl = desktop.placementOf(WindowId::playlist);
  // The screen rectangle collapses to the title bar; the canvas handed to the
  // panel does not, or unshading would restore a 42px playlist.
  QCOMPARE(pl.screen.height(), aoide::kTitleBar);
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

void LayoutSyncTest::theFirstLaunchStackClearsTheReservedTop() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(QRect(0, 25, 1920, 1055));
  LayoutSync layout(defaultCluster(), 75);
  layout.setSurfaces(&desktop);

  const int eqOffset = layout.nativeFrameRect(WindowId::equalizer).top() -
                       layout.nativeFrameRect(WindowId::main).top();
  const int playlistOffset = layout.nativeFrameRect(WindowId::playlist).top() -
                             layout.nativeFrameRect(WindowId::main).top();

  layout.fitClusterToHost();

  QCOMPARE(layout.nativeFrameRect(WindowId::main).top(), 25);
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer).top() -
               layout.nativeFrameRect(WindowId::main).top(),
           eqOffset);
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).top() -
               layout.nativeFrameRect(WindowId::main).top(),
           playlistOffset);
}

void LayoutSyncTest::aMainDragCannotParkTheClusterUnderTheStrip() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(QRect(0, 25, 1920, 1055));
  DockLayout dock;
  dock.main = {true, false, 80, 40, {}, {}};
  dock.equalizer = {true, false, 80, 388, {}, {}};
  dock.playlist.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.setNativeFrame(WindowId::main, QRect(80, 0, 825, 348));
  layout.setNativeFrame(WindowId::equalizer, QRect(80, 348, 825, 348));
  layout.fitClusterToHost();
  QCOMPARE(layout.nativeFrameRect(WindowId::main).top(), 25);
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer).top(), 373);

  layout.setNativeFrame(WindowId::main, QRect(80, -10, 825, 348));
  layout.setNativeFrame(WindowId::equalizer, QRect(80, 338, 825, 348));
  layout.fitClusterToHost();
  QCOMPARE(layout.nativeFrameRect(WindowId::main).top(), 25);
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer).top(), 373);
}

void LayoutSyncTest::anUnknownWorkAreaLeavesTheClusterWhereTheHostPutIt() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  LayoutSync layout(defaultCluster(), 75);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();

  QCOMPARE(layout.nativeFrameRect(WindowId::main), QRect(0, 0, 619, 261));
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer), QRect(0, 261, 619, 261));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist), QRect(0, 522, 805, 522));
}

void LayoutSyncTest::aWorkAreaFlushWithTheHostTopMovesNothing() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(QRect(0, 0, 1920, 1044));
  LayoutSync layout(defaultCluster(), 75);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();

  QCOMPARE(layout.nativeFrameRect(WindowId::main).topLeft(), QPoint(0, 0));
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer).topLeft(), QPoint(0, 261));
  QCOMPARE(layout.nativeFrameRect(WindowId::playlist).topLeft(), QPoint(0, 522));
}

void LayoutSyncTest::theReservedTopIsTakenFromMainsOwnScreen() {
  // Primary at (0, 0); a monitor stacked above it. Host top is negative. Main
  // sits on the upper display, and the work area asked must be the one under
  // main — the cluster union's centre can land on the primary and would pull
  // the stack down onto that screen's strip.
  FakeDesktop desktop(QRect(0, -1080, 1920, 2160));
  desktop.setWorkArea(QRect(0, -1055, 1920, 1055));
  DockLayout dock;
  dock.main = {true, false, 40, -1080, {}, {}};
  dock.equalizer = {true, false, 40, -732, {}, {}};
  dock.playlist.visible = false;
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.fitClusterToHost();

  QCOMPARE(layout.nativeFrameRect(WindowId::main).top(), -1055);
  QCOMPARE(layout.nativeFrameRect(WindowId::equalizer).top(), -707);
  QCOMPARE(desktop.clusterAsked, QRect(40, -1080, 825, 348));
}

void LayoutSyncTest::clampToHostPushesASiblingClearOfTheStrip() {
  FakeDesktop desktop(QRect(0, 0, 1920, 1080));
  desktop.setWorkArea(QRect(0, 25, 1920, 1055));
  DockLayout dock;
  dock.about = {true, false, 200, 0, {}, {}};
  LayoutSync layout(dock, 100);
  layout.setSurfaces(&desktop);

  layout.clampToHost(WindowId::about);
  QCOMPARE(layout.nativeFrameRect(WindowId::about), QRect(200, 25, 480, 360));
}

QTEST_APPLESS_MAIN(LayoutSyncTest)
#include "layout_sync_test.moc"
