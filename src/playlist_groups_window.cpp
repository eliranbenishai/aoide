#include "playlist_groups_window.h"

#include "aoide_fonts.h"
#include "aoide_metrics.h"
#include "chrome_paint.h"
#include "mockup_draw.h"
#include "playlist_group_colors.h"
#include "window_spec.h"

#include <QCloseEvent>
#include <QDataStream>
#include <QGuiApplication>
#include <QIODevice>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QSignalBlocker>
#include <QWindow>
#include <QtMath>
#include <algorithm>

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

  explicit GroupEditorMetrics(qreal zoom)
      : width(qRound(520 * zoom)),
        pad(qMax(14, qRound(24 * zoom))),
        titleHeight(qRound(kTitleBar * zoom)),
        introHeight(qMax(30, qRound(38 * zoom))),
        rowHeight(qMax(26, qRound(36 * zoom))),
        rowGap(qMax(7, qRound(10 * zoom))),
        swatchSize(qMax(12, qRound(16 * zoom))),
        swatchColumn(qMax(28, qRound(42 * zoom))) {}

  int rowsTop() const { return titleHeight + pad + introHeight; }
  QSize size(int count) const {
    return QSize(width, rowsTop() + count * rowHeight + qMax(0, count - 1) * rowGap + pad);
  }
};

qreal fittedEditorZoom(qreal requested, QSize available, int groupCount) {
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

// Only the tokens this window's frame, labels, and fields paint matter here.
// Session chrome updates also arrive on each name edit and during playback;
// unchanged appearances must not restyle fields or rebuild the frame then.
QByteArray editorAppearanceKey(const ChromeTokens& look) {
  QByteArray key;
  QDataStream stream(&key, QIODevice::WriteOnly);
  stream << look.id << look.chromeFamily << look.lcdFamily << chromeFamily()
         << look.radii.window << look.radii.surface << look.radii.button << look.railStops;
  for (const QColor& color : {
           look.shellHi, look.shell, look.shellMid, look.shellLo, look.shellDeep,
           look.titleBar0, look.titleBar26, look.titleBar62, look.titleBar100,
           look.bevelLight, look.bevelSoft, look.coolSheen, look.metalHi,
           look.phos, look.phosDim, look.accent, look.accentDim, look.hoverLift,
           look.wbtn0, look.wbtn55, look.wbtn100, look.wbtnClose0, look.wbtnClose55,
           look.wbtnClose100, look.closeGlyph, look.glyphInk, look.windowName,
           look.wordmark, look.ink, look.inkDim, look.well}) {
    stream << color;
  }
  return key;
}

class FrameCloseButton final : public QPushButton {
 public:
  explicit FrameCloseButton(QWidget* parent) : QPushButton(parent) {
    setAutoDefault(false);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("Close playlist groups"));
    setToolTip(QStringLiteral("Close"));
  }

 protected:
  // The shared Aoide frame paints the face; this widget supplies its input and
  // accessibility behavior without any native platform decoration.
  void paintEvent(QPaintEvent*) override {}
};

QPoint boundedPosition(QPoint position, QSize size, const QRect& available) {
  if (available.isEmpty()) return position;
  position.setX(std::clamp(position.x(), available.left(),
                          qMax(available.left(), available.right() - size.width() + 1)));
  position.setY(std::clamp(position.y(), available.top(),
                          qMax(available.top(), available.bottom() - size.height() + 1)));
  return position;
}

const QScreen* editorScreen(QWidget* editor, QWidget* owner, bool initial = false) {
  if (!initial && editor->windowHandle()) {
    if (const auto* screen = QGuiApplication::screenAt(editor->frameGeometry().center())) {
      return screen;
    }
    if (editor->screen()) return editor->screen();
  }
  return owner ? owner->screen() : editor->screen();
}

}  // namespace

PlaylistGroupsWindow::PlaylistGroupsWindow(const QVector<PlaylistGroup>& groups,
                                         const ChromeTokens& look, qreal zoomPercent,
                                         QWidget* owner)
    : QWidget(nullptr, hostWindowFlags()),
      wayland_(QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
  setObjectName(QStringLiteral("playlistGroupsWindow"));
  setWindowTitle(QStringLiteral("Playlist groups"));
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_QuitOnClose, false);
  setFocusPolicy(Qt::StrongFocus);
  if (owner && owner->window()->windowFlags().testFlag(Qt::WindowStaysOnTopHint)) {
    setWindowFlag(Qt::WindowStaysOnTopHint);
  }
  for (const auto& group : groups) {
    auto* name = new QLineEdit(group.name, this);
    name->setObjectName(QStringLiteral("groupName_%1").arg(group.id));
    name->setAccessibleName(QStringLiteral("Group %1 name").arg(group.id + 1));
    name->setMaxLength(60);
    name->installEventFilter(this);
    const int index = names_.size();
    names_.append(name);
    groupIds_.append(group.id);
    validNames_.append(name->text().trimmed());
    connect(name, &QLineEdit::textChanged, this, [this, index](const QString& text) {
      const QString value = text.trimmed();
      if (value.isEmpty() || value == validNames_[index]) return;
      validNames_[index] = value;
      emit groupNameChanged(groupIds_[index], value);
    });
    connect(name, &QLineEdit::editingFinished, this, [this, index]() { finishEdit(index); });
  }
  close_ = new FrameCloseButton(this);
  close_->setObjectName(QStringLiteral("closeGroups"));
  close_->installEventFilter(this);
  connect(close_, &QPushButton::clicked, this, &QWidget::close);
  phases_.setLive(true);
  closeAnimation_.setInterval(16);
  connect(&closeAnimation_, &QTimer::timeout, this, [this]() {
    if (!phases_.advance(closeClock_.restart())) closeAnimation_.stop();
    frame_ = QImage();
    update();
  });
  setAppearance(look, zoomPercent, owner);
  if (!names_.isEmpty()) {
    names_.first()->setFocus(Qt::OtherFocusReason);
    names_.first()->selectAll();
  }
}

PlaylistGroupsWindow::~PlaylistGroupsWindow() {
  // QWidget clears focus during its base destructor, after our field state has
  // gone. A focused edit must not call editingFinished back into that state.
  for (auto* field : names_) disconnect(field, nullptr, this, nullptr);
}

void PlaylistGroupsWindow::setAppearance(const ChromeTokens& look, qreal zoomPercent,
                                         QWidget* owner) {
  owner_ = owner;
  requestedZoom_ = zoomPercent;
  QSize available;
  if (!isWindow() && parentWidget()) available = parentWidget()->contentsRect().size();
  else if (owner && owner->screen()) {
    if (const auto* display = editorScreen(this, owner)) {
      available = display->availableGeometry().size();
    }
  }
  const qreal nextZoom = fittedEditorZoom(snapZoomPercent(zoomPercent) / 100, available,
                                          names_.size());
  const QByteArray nextKey = editorAppearanceKey(look);
  const bool firstAppearance = !appearanceReady_;
  if (appearanceReady_ && nextKey == appearanceKey_ && qFuzzyCompare(nextZoom, zoom_)) {
    placeWithinBounds(false);
    return;
  }
  look_ = look;
  zoom_ = nextZoom;
  appearanceKey_ = nextKey;
  const GroupEditorMetrics metrics(zoom_);
  instruction_ = QRect(metrics.pad, metrics.titleHeight + metrics.pad,
                        metrics.width - 2 * metrics.pad, metrics.introHeight);
  swatches_.clear();
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
  int top = metrics.rowsTop();
  for (auto* name : names_) {
    name->setFont(inputFont);
    if (name->styleSheet() != fieldStyle) name->setStyleSheet(fieldStyle);
    name->setGeometry(metrics.pad + metrics.swatchColumn, top,
                      metrics.width - 2 * metrics.pad - metrics.swatchColumn, metrics.rowHeight);
    swatches_.append(QRect(metrics.pad + (metrics.swatchColumn - metrics.swatchSize) / 2,
                          top + (metrics.rowHeight - metrics.swatchSize) / 2,
                          metrics.swatchSize, metrics.swatchSize));
    top += metrics.rowHeight + metrics.rowGap;
  }
  if (size() != metrics.size(names_.size())) setFixedSize(metrics.size(names_.size()));
  logicalSize_ = QSize(qRound(width() / zoom_), qRound(height() / zoom_));
  title_ = TitleChromeLayout::forWindow(WindowId::settings, logicalSize_);
  title_.roleName = QStringLiteral("Playlist groups");
  title_.minimize = QRect();
  title_.buttonsLeft = title_.close.left();
  title_.dragRight = title_.buttonsLeft - 12;
  close_->setGeometry(QRect(qRound(title_.close.x() * zoom_), qRound(title_.close.y() * zoom_),
                            qRound(title_.close.width() * zoom_),
                            qRound(title_.close.height() * zoom_)));
  frame_ = QImage();
  appearanceReady_ = true;
  placeWithinBounds(firstAppearance);
  update();
}

void PlaylistGroupsWindow::placeWithinBounds(bool center) {
  QPoint next = pos();
  const QPoint half(width() / 2, height() / 2);
  if (!isWindow() && parentWidget()) {
    const QRect bounds = parentWidget()->contentsRect();
    if (center) {
      const QPoint anchor = owner_ ? parentWidget()->mapFromGlobal(
                                       owner_->mapToGlobal(owner_->rect().center()))
                                   : bounds.center();
      next = anchor - half;
    }
    next = boundedPosition(next, size(), bounds);
  } else {
    // A top-level Wayland surface is positioned by the compositor. The live
    // editor becomes an embedded widget before it is mapped there.
    if (wayland_) return;
    const QScreen* display = editorScreen(this, owner_, center);
    const QRect bounds = display ? display->availableGeometry() : QRect();
    if (center) {
      const QPoint anchor = owner_ ? owner_->mapToGlobal(owner_->rect().center()) : bounds.center();
      next = anchor - half;
    }
    next = boundedPosition(next, size(), bounds);
  }
  if (next != pos()) move(next);
}

QVector<QString> PlaylistGroupsWindow::groupNames() const { return validNames_; }

void PlaylistGroupsWindow::finishEdit(int index) {
  auto* field = names_[index];
  // The valid value was already emitted as it was typed. Finishing an edit
  // merely normalises surrounding spaces or restores a temporarily blank box.
  if (field->text() != validNames_[index]) {
    const QSignalBlocker block(field);
    field->setText(validNames_[index]);
  }
}

void PlaylistGroupsWindow::closeEvent(QCloseEvent* event) {
  for (int index = 0; index < names_.size(); ++index) finishEdit(index);
  dragging_ = false;
  QWidget::closeEvent(event);
}

void PlaylistGroupsWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape) {
    close();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

bool PlaylistGroupsWindow::event(QEvent* event) {
  const bool handled = QWidget::event(event);
  if (appearanceReady_ && event->type() == QEvent::ParentChange) {
    frame_ = QImage();
    setAppearance(look_, requestedZoom_, owner_);
    placeWithinBounds(true);
  }
  return handled;
}

void PlaylistGroupsWindow::animateClose() {
  if (!closeAnimation_.isActive()) {
    closeClock_.start();
    closeAnimation_.start();
  }
}

bool PlaylistGroupsWindow::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    auto* key = static_cast<QKeyEvent*>(event);
    const auto found = std::find(names_.cbegin(), names_.cend(), watched);
    if (found != names_.cend()) {
      if (key->key() == Qt::Key_Escape) {
        close();
        return true;
      }
      if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
        finishEdit(int(found - names_.cbegin()));
        setFocus(Qt::OtherFocusReason);
        return true;
      }
    }
  }
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
  return QWidget::eventFilter(watched, event);
}

void PlaylistGroupsWindow::rebuildFrame() {
  const qreal dpr = devicePixelRatioF();
  frame_ = QImage(chromePaintBufferSize(size(), dpr), QImage::Format_ARGB32_Premultiplied);
  frame_.setDevicePixelRatio(dpr);
  frame_.fill(wayland_ && isWindow() ? look_.shellMid : QColor(Qt::transparent));
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
    if (!isWindow() && parentWidget()) {
      dragging_ = true;
      dragOffset_ = parentWidget()->mapFromGlobal(event->globalPosition().toPoint()) - pos();
    } else if (wayland_) {
      if (windowHandle()) windowHandle()->startSystemMove();
    } else {
      dragging_ = true;
      dragOffset_ = event->globalPosition().toPoint() - pos();
    }
    event->accept();
    return;
  }
  QWidget::mousePressEvent(event);
}

void PlaylistGroupsWindow::mouseMoveEvent(QMouseEvent* event) {
  if (dragging_ && (event->buttons() & Qt::LeftButton)) {
    QPoint next;
    if (!isWindow() && parentWidget()) {
      next = parentWidget()->mapFromGlobal(event->globalPosition().toPoint()) - dragOffset_;
      next = boundedPosition(next, size(), parentWidget()->contentsRect());
    } else {
      next = event->globalPosition().toPoint() - dragOffset_;
    }
    if (next != pos()) move(next);
    event->accept();
    return;
  }
  QWidget::mouseMoveEvent(event);
}

void PlaylistGroupsWindow::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) dragging_ = false;
  QWidget::mouseReleaseEvent(event);
}

}  // namespace aoide
