#pragma once

#include "chrome_hits.h"
#include "session_view.h"
#include "title_chrome.h"
#include "tramp_metrics.h"
#include "window_spec.h"

#include <QCloseEvent>
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

class HostWindow : public QWidget {
  Q_OBJECT

 public:
  explicit HostWindow(const tramp::WindowSpec& spec, QWidget* parent = nullptr);

  tramp::WindowId id() const { return spec_.id; }
  void setZoomPercent(int percent);
  void setShaded(bool shaded);
  void setSessionView(const tramp::SessionView& view);
  void applyLiveReadouts(const tramp::MainLiveReadouts& live);
  void applyEqualizer(const tramp::EqualizerSettings& eq);
  void setPlaylistLogicalSize(QSize logical);
  void setQuitConfirmer(std::function<bool()> fn) { quitConfirmer_ = std::move(fn); }
  void setAlwaysOnTop(bool on);
  bool shaded() const { return shaded_; }
  QPoint nativeTopLeft() const;
  QRect widgetRectFromLogical(const QRect& logical) const;

 signals:
  void zoomOutRequested();
  void zoomInRequested();
  void extraHidden();
  void extraMapped();
  void chromePressed(tramp::ChromeHit hit, Qt::KeyboardModifiers mods, QPoint logical);
  void chromeDragged(tramp::ChromeHit hit, QPoint logical);
  void chromeReleased();
  void wheelScrolled(int delta);
  void nativeMoved(QPoint pos);
  void nativeResized(QSize size);
  void filesDropped(QStringList paths);
  void aboutToQuit();
  void shadedChanged(bool shaded);
  void mainMinimized(bool minimized);
  void mainActivated();
  void trackActivated(int index);
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
  void applyNativeSize();
  QPoint logicalFrom(const QPointF& widgetPos) const;
  void applyHitCursor(const QPointF& widgetPos);
  void applyChromeTooltip(const QPointF& widgetPos);
  void hideChromeTooltipNow();
  QRect tooltipAnchorRect(tramp::TitleChromeLayout::Hit title, const tramp::ChromeHit& chrome) const;
  void invalidateChassis();
  void rebuildChassis();
  void grabPointerIfAllowed();
  void releasePointerIfHeld();

  tramp::WindowSpec spec_;
  tramp::TitleChromeLayout title_;
  tramp::SessionView view_;
  QImage logo_;
  QImage chassis_;
  bool chassisValid_ = false;
  bool titleMarqueeLive_ = false;
  int zoomPercent_ = tramp::kDefaultZoomPercent;
  bool shaded_ = false;
  bool draggingChrome_ = false;
  bool draggingTitle_ = false;
  bool resizingPlaylist_ = false;
  bool grabbedPointer_ = false;
  QPoint grabOffset_;
  tramp::ChromeHit dragHit_;
  QTimer tooltipTimer_;
  QString tooltipCandidate_;
  QPoint tooltipGlobal_;
  tramp::TitleChromeLayout::Hit tooltipTitle_ = tramp::TitleChromeLayout::Hit::none;
  tramp::ChromeHit tooltipChrome_;
  std::function<bool()> quitConfirmer_;
};
