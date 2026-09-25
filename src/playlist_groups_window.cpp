#include "playlist_groups_window.h"

#include "aoide_metrics.h"
#include "chrome_paint.h"
#include "mockup_draw.h"
#include "playlist_group_colors.h"
#include "window_spec.h"

#include <QGuiApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QWindow>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace aoide {
namespace {

struct GroupEditorMetrics {
  int width;
  int pad;
  int titleHeight;
  int introHeight;
  int rowHeight;
  int rowGap;
  int swatchSize;
  int swatchColumn;
  int buttonHeight;
  int buttonWidth;
  int footerGap;

  explicit GroupEditorMetrics(qreal zoom)
      : width(qRound(520 * zoom)),
        pad(qMax(14, qRound(24 * zoom))),
        titleHeight(qRound(kTitleBar * zoom)),
        introHeight(qMax(30, qRound(38 * zoom))),
        rowHeight(qMax(26, qRound(36 * zoom))),
        rowGap(qMax(7, qRound(10 * zoom))),
        swatchSize(qMax(12, qRound(16 * zoom))),
        swatchColumn(qMax(28, qRound(42 * zoom))),
        buttonHeight(qMax(27, qRound(34 * zoom))),
        buttonWidth(qMax(76, qRound(104 * zoom))),
        footerGap(qMax(18, qRound(24 * zoom))) {}

  int rowsTop() const { return titleHeight + pad + introHeight; }
  int buttonsTop(int count) const {
    return rowsTop() + count * rowHeight + qMax(0, count - 1) * rowGap + footerGap;
  }
  QSize size(int count) const { return QSize(width, buttonsTop(count) + buttonHeight + pad); }
};

qreal fittedEditorZoom(qreal requested, QWidget* parent, int groupCount) {
  // An ownerless editor is also used for exact-zoom renders. Only the live
  // transient has a work area to fit, and this never changes the session zoom.
  if (!parent || !parent->screen()) return requested;
  const QSize available = parent->screen()->availableGeometry().size();
  if (available.isEmpty()) return requested;
  qreal fitted = kZoomSteps[0] / 100;
  for (const qreal percent : kZoomSteps) {
    const qreal zoom = percent / 100;
    if (zoom > requested) break;
    const QSize wanted = GroupEditorMetrics(zoom).size(groupCount);
    if (wanted.width() <= available.width() && wanted.height() <= available.height()) {
      fitted = zoom;
    }
  }
  return fitted;
}

// Keeping QPushButton's input handling preserves Tab, Space, Enter, and the
// accessibility role, while its face uses Aoide's own button painter.
class GroupActionButton final : public QPushButton {
 public:
  GroupActionButton(const QString& label, bool primary, const ChromeTokens& look,
                    qreal zoom, QWidget* parent)
      : QPushButton(label, parent), look_(look), zoom_(zoom), primary_(primary) {
    setAutoDefault(false);
    setDefault(primary);
    setFocusPolicy(Qt::StrongFocus);
    phases_.setLive(true);
    phases_.snapTo(ChromeHit::Kind::none, 0, BtnChannel::on, primary ? 1 : 0);
    animation_.setInterval(16);
    connect(&animation_, &QTimer::timeout, this, [this]() {
      if (!phases_.advance(clock_.restart())) animation_.stop();
      update();
    });
  }

 protected:
  bool event(QEvent* event) override {
    const bool handled = QPushButton::event(event);
    switch (event->type()) {
      case QEvent::Enter:
      case QEvent::Leave:
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::KeyPress:
      case QEvent::KeyRelease:
      case QEvent::EnabledChange:
        phases_.setTarget(ChromeHit::Kind::none, 0, BtnChannel::hover,
                          isEnabled() && underMouse() ? 1 : 0);
        phases_.setTarget(ChromeHit::Kind::none, 0, BtnChannel::press,
                          isEnabled() && isDown() ? 1 : 0);
        if (!animation_.isActive()) {
          clock_.start();
          animation_.start();
        }
        break;
      default:
        break;
    }
    return handled;
  }

  void paintEvent(QPaintEvent*) override {
    const LookPaintScope scope(look_);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.scale(zoom_, zoom_);
    const QRectF face = QRectF(0, 0, width() / zoom_, height() / zoom_)
                            .adjusted(1, 1, -1, -1);
    const BtnFace phase = phases_.face(ChromeHit::Kind::none, 0);
    painter.setOpacity(isEnabled() ? 1 : 0.45);
    drawBtn(painter, face, phase);
    // At the smallest player zoom, editing still needs readable control text.
    painter.setFont(condensedFont(qMax(13, qCeil(11 / zoom_)), 0.12));
    painter.setPen(primary_ ? look_.btnOnInk : look_.btnLabelIdle);
    painter.drawText(face, Qt::AlignCenter, text().toUpper());
    if (hasFocus()) {
      painter.setPen(QPen(look_.phos, 1 / zoom_, Qt::DotLine));
      painter.setBrush(Qt::NoBrush);
      painter.drawRoundedRect(face.adjusted(3, 3, -3, -3), look_.radii.button,
                              look_.radii.button);
    }
  }

 private:
  ChromeTokens look_;
  qreal zoom_;
  bool primary_;
  ChromePhases phases_;
  QTimer animation_;
  QElapsedTimer clock_;
};

// The shared frame paints this button. The real widget supplies its keyboard
// and accessibility behavior, without adding any native platform decoration.
class FrameCloseButton final : public QPushButton {
 public:
  explicit FrameCloseButton(QWidget* parent) : QPushButton(parent) {
    setAutoDefault(false);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Close playlist groups"));
    setToolTip(QStringLiteral("Close"));
  }

 protected:
  void paintEvent(QPaintEvent*) override {}
};

QPoint centeredOnParent(QWidget* parent, QSize size) {
  const QPoint center = parent ? parent->mapToGlobal(parent->rect().center()) : QPoint();
  const QScreen* screen = parent ? parent->screen() : QGuiApplication::primaryScreen();
  if (!screen) return center - QPoint(size.width() / 2, size.height() / 2);
  const QRect available = screen->availableGeometry();
  QPoint position = (parent ? center : available.center()) -
                    QPoint(size.width() / 2, size.height() / 2);
  position.setX(std::clamp(position.x(), available.left(),
                          qMax(available.left(), available.right() - size.width() + 1)));
  position.setY(std::clamp(position.y(), available.top(),
                          qMax(available.top(), available.bottom() - size.height() + 1)));
  return position;
}

}  // namespace

PlaylistGroupsWindow::PlaylistGroupsWindow(const QVector<PlaylistGroup>& groups,
                                         const ChromeTokens& look, qreal zoomPercent,
                                         QWidget* parent)
    : QDialog(parent, hostWindowFlags()), look_(look),
      zoom_(fittedEditorZoom(snapZoomPercent(zoomPercent) / 100, parent, groups.size())),
      wayland_(QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
  setObjectName(QStringLiteral("playlistGroupsWindow"));
  setWindowTitle(QStringLiteral("Playlist groups"));
  setWindowModality(Qt::WindowModal);
  if (parent && parent->window()->windowFlags().testFlag(Qt::WindowStaysOnTopHint)) {
    setWindowFlag(Qt::WindowStaysOnTopHint);
  }
  setAttribute(Qt::WA_TranslucentBackground, !wayland_);
  if (wayland_) {
    QPalette colors = palette();
    colors.setColor(QPalette::Window, look_.shellMid);
    setPalette(colors);
    setAutoFillBackground(true);
  }

  const GroupEditorMetrics metrics(zoom_);
  instruction_ = QRect(metrics.pad, metrics.titleHeight + metrics.pad,
                        metrics.width - 2 * metrics.pad, metrics.introHeight);
  int top = metrics.rowsTop();

  const LookPaintScope scope(look_);
  const QFont inputFont = condensedFont(qMax(11, qRound(16 * zoom_)), 0.02);
  const QString fieldStyle = QStringLiteral(
      "QLineEdit { color: %1; background: %2; border: 1px solid %3;"
      " border-radius: %4px; padding: 0px %5px; selection-background-color: %6;"
      " selection-color: %7; }"
      "QLineEdit:focus { border-color: %6; }")
      .arg(look_.ink.name(), look_.well.name(), look_.shellDeep.name())
      .arg(qMax(0, qRound(look_.radii.surface * zoom_)))
      .arg(qMax(6, qRound(10 * zoom_)))
      .arg(look_.phos.name(), look_.well.name());
  for (const auto& group : groups) {
    auto* name = new QLineEdit(group.name, this);
    name->setObjectName(QStringLiteral("groupName_%1").arg(group.id));
    name->setAccessibleName(QStringLiteral("Group %1 name").arg(group.id + 1));
    name->setMaxLength(60);
    name->setFont(inputFont);
    name->setStyleSheet(fieldStyle);
    name->setGeometry(metrics.pad + metrics.swatchColumn, top,
                      metrics.width - 2 * metrics.pad - metrics.swatchColumn, metrics.rowHeight);
    names_.append(name);
    groupIds_.append(group.id);
    swatches_.append(QRect(metrics.pad + (metrics.swatchColumn - metrics.swatchSize) / 2,
                          top + (metrics.rowHeight - metrics.swatchSize) / 2,
                          metrics.swatchSize, metrics.swatchSize));
    connect(name, &QLineEdit::textChanged, this, [this]() { updateSaveEnabled(); });
    top += metrics.rowHeight + metrics.rowGap;
  }
  top = metrics.buttonsTop(groups.size());
  auto* cancel = new GroupActionButton(QStringLiteral("Cancel"), false, look_, zoom_, this);
  cancel->setObjectName(QStringLiteral("cancelGroups"));
  cancel->setGeometry(metrics.width - metrics.pad - 2 * metrics.buttonWidth - metrics.rowGap,
                       top, metrics.buttonWidth, metrics.buttonHeight);
  save_ = new GroupActionButton(QStringLiteral("Save"), true, look_, zoom_, this);
  save_->setObjectName(QStringLiteral("saveGroups"));
  save_->setGeometry(metrics.width - metrics.pad - metrics.buttonWidth, top,
                     metrics.buttonWidth, metrics.buttonHeight);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
  connect(save_, &QPushButton::clicked, this, &PlaylistGroupsWindow::accept);
  updateSaveEnabled();

  setFixedSize(metrics.size(groups.size()));
  logicalSize_ = QSize(qRound(this->width() / zoom_), qRound(height() / zoom_));
  title_ = TitleChromeLayout::forWindow(WindowId::settings, logicalSize_);
  title_.roleName = QStringLiteral("Playlist groups");
  title_.minimize = QRect();
  title_.buttonsLeft = title_.close.left();
  title_.dragRight = title_.buttonsLeft - 12;
  close_ = new FrameCloseButton(this);
  close_->setObjectName(QStringLiteral("closeGroups"));
  close_->setGeometry(QRect(qRound(title_.close.x() * zoom_), qRound(title_.close.y() * zoom_),
                            qRound(title_.close.width() * zoom_),
                            qRound(title_.close.height() * zoom_)));
  close_->installEventFilter(this);
  connect(close_, &QPushButton::clicked, this, &QDialog::reject);
  phases_.setLive(true);
  closeAnimation_.setInterval(16);
  connect(&closeAnimation_, &QTimer::timeout, this, [this]() {
    if (!phases_.advance(closeClock_.restart())) closeAnimation_.stop();
    frame_ = QImage();
    update();
  });
  if (!names_.isEmpty()) {
    names_.first()->setFocus(Qt::OtherFocusReason);
    names_.first()->selectAll();
  }
  // Wayland owns toplevel placement. Native platforms centre against the
  // parent's mapped coordinates once, before the dialog is shown.
  if (!wayland_) move(centeredOnParent(parent, size()));
}

QVector<QString> PlaylistGroupsWindow::groupNames() const {
  QVector<QString> result;
  result.reserve(names_.size());
  for (const auto* field : names_) result.append(field->text().trimmed());
  return result;
}

bool PlaylistGroupsWindow::namesValid() const {
  return std::all_of(names_.cbegin(), names_.cend(), [](const QLineEdit* field) {
    const QString name = field->text().trimmed();
    return !name.isEmpty() && name.size() <= 60;
  });
}

void PlaylistGroupsWindow::updateSaveEnabled() {
  if (save_) save_->setEnabled(namesValid());
}

void PlaylistGroupsWindow::accept() {
  if (!namesValid()) {
    for (auto* field : names_) {
      if (field->text().trimmed().isEmpty()) {
        field->setFocus(Qt::OtherFocusReason);
        break;
      }
    }
    return;
  }
  QDialog::accept();
}

void PlaylistGroupsWindow::animateClose() {
  if (!closeAnimation_.isActive()) {
    closeClock_.start();
    closeAnimation_.start();
  }
}

bool PlaylistGroupsWindow::eventFilter(QObject* watched, QEvent* event) {
  if (watched == close_) {
    switch (event->type()) {
      case QEvent::Enter:
        phases_.setTitleTarget(TitleChromeLayout::Hit::close, BtnChannel::hover, 1);
        animateClose();
        break;
      case QEvent::Leave:
        phases_.setTitleTarget(TitleChromeLayout::Hit::close, BtnChannel::hover, 0);
        animateClose();
        break;
      case QEvent::MouseButtonPress:
        phases_.setTitleTarget(TitleChromeLayout::Hit::close, BtnChannel::press, 1);
        animateClose();
        break;
      case QEvent::MouseButtonRelease:
        phases_.setTitleTarget(TitleChromeLayout::Hit::close, BtnChannel::press, 0);
        animateClose();
        break;
      default:
        break;
    }
  }
  return QDialog::eventFilter(watched, event);
}

void PlaylistGroupsWindow::rebuildFrame() {
  const qreal dpr = devicePixelRatioF();
  frame_ = QImage(chromePaintBufferSize(size(), dpr), QImage::Format_ARGB32_Premultiplied);
  frame_.setDevicePixelRatio(dpr);
  frame_.fill(wayland_ ? look_.shellMid : QColor(Qt::transparent));
  const LookPaintScope scope(look_);
  QPainter painter(&frame_);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);
  {
    const PainterStateScope hold(painter);
    painter.scale(zoom_, zoom_);
    paintWindowFrame(painter, logicalSize_, title_, look_, phases_);
  }
  painter.setFont(condensedFont(qMax(11, qRound(14 * zoom_)), 0.04));
  painter.setPen(look_.inkDim);
  painter.drawText(instruction_, Qt::AlignLeft | Qt::AlignTop,
                   QStringLiteral("Name your playlist groups."));
  for (int i = 0; i < swatches_.size(); ++i) {
    const QRectF swatch = swatches_[i];
    fillRound(painter, swatch.translated(0, 1), qMax(qreal(2), 3 * zoom_),
               withAlpha(look_.shellDeep, 190));
    fillRound(painter, swatch, qMax(qreal(2), 3 * zoom_), playlistGroupColor(groupIds_[i]));
  }
}

void PlaylistGroupsWindow::paintEvent(QPaintEvent*) {
  if (frame_.isNull() || !qFuzzyCompare(frame_.devicePixelRatio(), devicePixelRatioF())) {
    rebuildFrame();
  }
  QPainter painter(this);
  painter.drawImage(QPoint(), frame_);
}

void PlaylistGroupsWindow::mousePressEvent(QMouseEvent* event) {
  const QPoint logical(qFloor(event->position().x() / zoom_),
                       qFloor(event->position().y() / zoom_));
  if (event->button() == Qt::LeftButton && title_.inDragRegion(logical)) {
    if (wayland_) {
      if (windowHandle()) windowHandle()->startSystemMove();
    } else {
      dragging_ = true;
      dragOffset_ = event->globalPosition().toPoint() - pos();
    }
    event->accept();
    return;
  }
  QDialog::mousePressEvent(event);
}

void PlaylistGroupsWindow::mouseMoveEvent(QMouseEvent* event) {
  if (dragging_ && (event->buttons() & Qt::LeftButton)) {
    const QPoint next = event->globalPosition().toPoint() - dragOffset_;
    if (next != pos()) move(next);
    event->accept();
    return;
  }
  QDialog::mouseMoveEvent(event);
}

void PlaylistGroupsWindow::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) dragging_ = false;
  QDialog::mouseReleaseEvent(event);
}

}  // namespace aoide
