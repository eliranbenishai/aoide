#pragma once

#include "chrome_anim.h"
#include "look.h"
#include "persist.h"
#include "title_chrome.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QImage>
#include <QPointer>
#include <QTimer>
#include <QWidget>

class QLineEdit;
class QPushButton;

namespace aoide {

/// An ordinary Aoide window: each valid name edit is applied immediately.
/// The session owns it; the owner passed here only anchors its placement.
class PlaylistGroupsWindow final : public QWidget {
  Q_OBJECT

 public:
  /// With an owner, fit to its screen using a lower supported zoom if needed.
  /// This is local to the editor; ownerless renders keep the requested zoom.
  PlaylistGroupsWindow(const QVector<PlaylistGroup>& groups, const ChromeTokens& look,
                       qreal zoomPercent, QWidget* owner = nullptr);
  ~PlaylistGroupsWindow() override;

  QVector<QString> groupNames() const;
  void setAppearance(const ChromeTokens& look, qreal zoomPercent, QWidget* owner);

 signals:
  void groupNameChanged(int groupId, const QString& name);

 protected:
  bool event(QEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

 private:
  void finishEdit(int index);
  void placeWithinBounds(bool center);
  void rebuildFrame();
  void animateClose();

  ChromeTokens look_;
  QByteArray appearanceKey_;
  QPointer<QWidget> owner_;
  qreal requestedZoom_ = 100;
  qreal zoom_ = 1;
  QSize logicalSize_;
  TitleChromeLayout title_;
  QVector<QLineEdit*> names_;
  QVector<int> groupIds_;
  QVector<QString> validNames_;
  QVector<QRect> swatches_;
  QRect instruction_;
  QPushButton* close_ = nullptr;
  QImage frame_;
  ChromePhases phases_;
  QTimer closeAnimation_;
  QElapsedTimer closeClock_;
  bool wayland_ = false;
  bool appearanceReady_ = false;
  bool dragging_ = false;
  QPoint dragOffset_;
};

}  // namespace aoide
