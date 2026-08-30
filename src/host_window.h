#pragma once

#include "chrome_anim.h"
#include "chrome_hits.h"
#include "session_view.h"
#include "title_chrome.h"
#include "aoide_metrics.h"
#include "window_spec.h"

#include <QCloseEvent>
#include <QElapsedTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QHideEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <functional>

class QPainter;

class HostWindow : public QWidget {
  Q_OBJECT

 public:
  explicit HostWindow(const aoide::WindowSpec& spec, QWidget* parent = nullptr);

  aoide::WindowId id() const { return spec_.id; }
  void setZoomPercent(qreal percent);
  void setShaded(bool shaded);
  void setSessionView(const aoide::SessionView& view);
  void applyLiveReadouts(const aoide::MainLiveReadouts& live);
  void applyEqualizer(const aoide::EqualizerSettings& eq);
  void setPlaylistLogicalSize(QSize logical);
  void setQuitConfirmer(std::function<bool()> fn) { quitConfirmer_ = std::move(fn); }
  void setAlwaysOnTop(bool on);
  bool shaded() const { return shaded_; }
  QPoint nativeTopLeft() const;
  QRect widgetRectFromLogical(const QRect& logical) const;

  /// Repaint accounting for `--bench-drag`. `chassisBuilds` counts the
  /// expensive static rebuilds, which must not happen while a drag runs.
  struct PaintStats {
    int paints = 0;
    int chassisBuilds = 0;
    qint64 nanos = 0;
    qint64 blurCalls = 0;
    qint64 blurNanos = 0;
    qint64 blurPixels = 0;
    qint64 layers = 0;
    qint64 layerNanos = 0;
    qint64 fonts = 0;
    qint64 fontNanos = 0;
  };
  PaintStats paintStats() const { return paintStats_; }
  void resetPaintStats() { paintStats_ = {}; }

 signals:
  void zoomOutRequested();
  void zoomInRequested();
  void extraHidden();
  void extraMapped();
  void chromePressed(aoide::ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical);
  void chromeDragged(aoide::ChromeHit hit, QPoint logical);
  void chromeReleased();
  void wheelScrolled(int delta);
  void nativeMoved(QPoint pos);
  void nativeResized(QRect nativeRect);
  void filesDropped(QStringList paths);
  void aboutToQuit();
  void shadedChanged(bool shaded);
  void mainMinimized(bool minimized);
  void mainActivated();
  void trackActivated(int index);
  void collectionRowActivated(int index);
  void titleDragStarted();
  void titleDragFinished();

 protected:
  void paintEvent(QPaintEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void changeEvent(QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void leaveEvent(QEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void moveEvent(QMoveEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;

 private:
  QSize paintLogical() const;
  void paintChrome(QPainter& painter);
  void applyNativeSize();
  QPoint logicalFrom(const QPointF& widgetPos) const;
  void applyHitCursor(const QPointF& widgetPos);
  void applyChromeTooltip(const QPointF& widgetPos);
  void hideChromeTooltipNow();
  QRect tooltipAnchorRect(aoide::TitleChromeLayout::Hit title, const aoide::ChromeHit& chrome) const;
  bool hasLiveBody() const;
  void invalidateChassis();
  void ensureChassis();
  void rebuildChassis();
  void grabPointerIfAllowed();
  void releasePointerIfHeld();
  /// Point the pointer channels at whatever is under [widgetPos]. Passing an
  /// empty optional means the pointer left, so everything cools off.
  void trackPointer(std::optional<QPointF> widgetPos, bool pressed);
  /// Aim the latched channels at the state this view describes. [snap] skips the
  /// animation, for the first view a panel is ever handed.
  void syncLatchedPhases(bool snap);
  void stepButtonAnimation();
  void startButtonAnimation();
  /// Emit a collection-row press that was waiting to see if a double-click
  /// would cancel it. Does not take the pointer: the caller owns that.
  void deliverPendingCollectionClick();
  /// Timer path: deliver the stored press, and take the pointer only while the
  /// button is still down so a later release matches.
  void onDeferredCollectionClick();

  aoide::WindowSpec spec_;
  aoide::TitleChromeLayout title_;
  aoide::SessionView view_;
  QImage logo_;
  QImage chassis_;
  bool chassisValid_ = false;
  bool chassisIsFullPaint_ = false;
  bool titleMarqueeLive_ = false;
  qreal zoomPercent_ = aoide::kDefaultZoomPercent;
  bool shaded_ = false;
  bool draggingChrome_ = false;
  bool draggingTitle_ = false;
  bool resizingPlaylist_ = false;
  aoide::PlaylistResizeEdges playlistResizeEdges_;
  QRect playlistResizeStart_;
  QRect playlistResizeLast_;
  QPoint playlistResizePress_;
  bool grabbedPointer_ = false;
  int wheelPixelCarry_ = 0;
  QPoint grabOffset_;
  aoide::ChromeHit dragHit_;
  aoide::ChromePhases phases_;
  QTimer animTimer_;
  QElapsedTimer animClock_;
  bool sawFirstView_ = false;
  QTimer tooltipTimer_;
  QString tooltipCandidate_;
  QPoint tooltipGlobal_;
  aoide::TitleChromeLayout::Hit tooltipTitle_ = aoide::TitleChromeLayout::Hit::none;
  aoide::ChromeHit tooltipChrome_;
  QTimer collectionClickTimer_;
  bool pendingCollectionClick_ = false;
  aoide::ChromeHit pendingCollectionHit_;
  Qt::KeyboardModifiers pendingCollectionMods_ = Qt::NoModifier;
  QPoint pendingCollectionLogical_;
  PaintStats paintStats_;
  std::function<bool()> quitConfirmer_;
};
