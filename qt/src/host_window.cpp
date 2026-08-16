#include "host_window.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>

namespace {

constexpr int kTitleHeight = 28;
constexpr int kCorner = 10;

class TitleStrip : public QWidget {
 public:
  explicit TitleStrip(const QString& title, QWidget* parent = nullptr)
      : QWidget(parent), title_(title) {
    setFixedHeight(kTitleHeight);
    setCursor(Qt::OpenHandCursor);
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(220, 220, 220, 220));
    p.setFont(QFont(QStringLiteral("Sans Serif"), 10, QFont::DemiBold));
    p.drawText(rect().adjusted(12, 0, -12, 0), Qt::AlignVCenter | Qt::AlignLeft,
               title_);
  }

  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() != Qt::LeftButton) {
      return;
    }
    if (QWindow* win = window()->windowHandle()) {
      win->startSystemMove();
    }
    event->accept();
  }

 private:
  QString title_;
};

}  // namespace

HostWindow::HostWindow(const tramp::WindowSpec& spec, QWidget* parent)
    : QWidget(parent), spec_(spec) {
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowTitle(spec.title);

  Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::Window;
  if (spec.skipTaskbar) {
    flags |= Qt::Tool;
  }
  setWindowFlags(flags);

  resize(spec.size);
  move(spec.origin);

  auto* title = new TitleStrip(spec.title, this);
  title->setGeometry(0, 0, spec.size.width(), kTitleHeight);

  // Force a platform window so extras are not glued as transients of main.
  winId();
  if (QWindow* native = windowHandle()) {
    native->setTransientParent(nullptr);
  }
}

void HostWindow::paintEvent(QPaintEvent*) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QRectF panel = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
  QPainterPath path;
  path.addRoundedRect(panel, kCorner, kCorner);

  p.setPen(QPen(QColor(255, 255, 255, 40), 1));
  p.setBrush(spec_.panel);
  p.drawPath(path);
}

void HostWindow::closeEvent(QCloseEvent* event) {
  if (spec_.id == tramp::WindowId::main) {
    QCoreApplication::quit();
  }
  event->accept();
}
