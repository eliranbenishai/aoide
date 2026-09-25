#pragma once

#include "chrome_anim.h"
#include "look.h"
#include "persist.h"
#include "title_chrome.h"

#include <QDialog>
#include <QElapsedTimer>
#include <QImage>
#include <QTimer>

class QLineEdit;
class QPushButton;

namespace aoide {

/// A transient editor with the same chassis as the player. Its draft stays in
/// the fields until accepted; closing it never changes the collection.
class PlaylistGroupsWindow final : public QDialog {
 public:
  /// With a parent, fit to its screen using a lower supported zoom if needed.
  /// This is local to the editor; ownerless renders keep the requested zoom.
  PlaylistGroupsWindow(const QVector<PlaylistGroup>& groups, const ChromeTokens& look,
                       qreal zoomPercent, QWidget* parent = nullptr);

  QVector<QString> groupNames() const;
  void accept() override;

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

 private:
  bool namesValid() const;
  void updateSaveEnabled();
  void rebuildFrame();
  void animateClose();

  ChromeTokens look_;
  qreal zoom_ = 1;
  QSize logicalSize_;
  TitleChromeLayout title_;
  QVector<QLineEdit*> names_;
  QVector<int> groupIds_;
  QVector<QRect> swatches_;
  QRect instruction_;
  QPushButton* save_ = nullptr;
  QPushButton* close_ = nullptr;
  QImage frame_;
  ChromePhases phases_;
  QTimer closeAnimation_;
  QElapsedTimer closeClock_;
  bool wayland_ = false;
  bool dragging_ = false;
  QPoint dragOffset_;
};

}  // namespace aoide
