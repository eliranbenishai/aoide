#include "chrome_tooltip.h"

#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace tramp {
namespace {

/// Naming the control is no use on a button that will not take it: "Zoom out"
/// on a dead button says what it would have done, which is the reading the
/// disabled face exists to prevent. So a withdrawn step says why instead, and
/// the two reasons are not the same answer — the floor of the ladder is where
/// Tramp ends, and a step that will not fit is where this display ends. At the
/// shipped default both are out at once on a short screen, and a whole cluster
/// greyed for one unexplained reason is what reads as broken chrome.
QString zoomFloorTip(const SessionView& view) {
  return QStringLiteral("%1% is as small as Tramp goes").arg(view.zoomPercent);
}

/// The step is named rather than called "the next one": the readout between the
/// buttons already says where the listener is, and this says what is out of
/// reach from there. Closing a panel is the way back — but only while one is
/// open to close, or the sentence sends them after something that is not there.
QString zoomNoRoomTip(const SessionView& view) {
  const int step = nextZoomPercent(view.zoomPercent);
  if (!view.eqOn && !view.plOn) {
    return QStringLiteral("No room for %1% on this display").arg(step);
  }
  return QStringLiteral("No room for %1% — close a panel").arg(step);
}

QString titleTip(TitleChromeLayout::Hit title, const SessionView& view) {
  switch (title) {
    case TitleChromeLayout::Hit::minimize:
      return QStringLiteral("Minimize");
    case TitleChromeLayout::Hit::collapse:
      return QStringLiteral("Collapse");
    case TitleChromeLayout::Hit::zoomOut:
      return view.zoomOutEnabled ? QStringLiteral("Zoom out") : zoomFloorTip(view);
    case TitleChromeLayout::Hit::zoomIn:
      return view.zoomInEnabled ? QStringLiteral("Zoom in") : zoomNoRoomTip(view);
    case TitleChromeLayout::Hit::close:
      return QStringLiteral("Close");
    case TitleChromeLayout::Hit::drag:
    case TitleChromeLayout::Hit::none:
      break;
  }
  return {};
}

QString chromeKindTip(const ChromeHit& chrome, const SessionView& view) {
  using K = ChromeHit::Kind;
  switch (chrome.kind) {
    case K::options:
      return QStringLiteral("Options");
    case K::mute:
      return view.muted ? QStringLiteral("Unmute") : QStringLiteral("Mute");
    case K::mono:
      return view.forceMono ? QStringLiteral("Play in stereo")
                            : QStringLiteral("Fold both channels to mono");
    case K::eqToggle:
      return view.eqOn ? QStringLiteral("Hide equalizer") : QStringLiteral("Show equalizer");
    case K::plToggle:
      return view.plOn ? QStringLiteral("Hide Playlist Manager")
                       : QStringLiteral("Show Playlist Manager");
    case K::prev:
    case K::plPrev:
      return QStringLiteral("Previous");
    case K::play:
      return QStringLiteral("Play");
    case K::pause:
      return QStringLiteral("Pause");
    case K::stop:
      return QStringLiteral("Stop");
    case K::next:
    case K::plNext:
      return QStringLiteral("Next");
    case K::eject:
      return QStringLiteral("Open files");
    case K::shuffle:
      return QStringLiteral("Shuffle");
    case K::repeat:
      return QStringLiteral("Repeat");
    case K::eqOn:
      return QStringLiteral("Equalizer on");
    case K::eqAuto:
      return QStringLiteral("Auto");
    case K::eqPresets:
      return QStringLiteral("Presets");
    case K::plCollapse:
      return view.collectionCollapsed ? QStringLiteral("Show playlist collection")
                                      : QStringLiteral("Collapse playlist collection");
    case K::plAddCollection:
      return QStringLiteral("Add playlist to collection");
    case K::plCreate:
      return QStringLiteral("Create playlist");
    case K::plRename:
      return QStringLiteral("Rename playlist");
    case K::plRemoveCollection:
      return QStringLiteral("Remove playlist from collection");
    case K::plAdd:
      return QStringLiteral("Add tracks");
    case K::plRemove:
      return QStringLiteral("Remove selected tracks");
    case K::plSort:
      return QStringLiteral("Sort playlist");
    case K::plOptions:
      return QStringLiteral("Playlist options");
    case K::plPlay:
      return view.playing ? QStringLiteral("Pause") : QStringLiteral("Play");
    case K::plRefresh:
      return QStringLiteral("Refresh playlist");
    case K::settingsGeneral:
      return QStringLiteral("General");
    case K::settingsSkins:
      return QStringLiteral("Skins");
    case K::settingsResume:
      return QStringLiteral("Resume last session");
    case K::settingsConfirm:
      return QStringLiteral("Confirm before quit");
    case K::settingsScroll:
      return QStringLiteral("Scroll title");
    case K::settingsMinimize:
      return QStringLiteral("Minimize hides secondaries");
    case K::settingsSnapOff:
      return QStringLiteral("Dock snap off");
    case K::settingsSnapNormal:
      return QStringLiteral("Dock snap normal");
    case K::settingsSnapStrong:
      return QStringLiteral("Dock snap strong");
    case K::settingsReset:
      return QStringLiteral("Reset Settings");
    case K::settingsInstallZip:
      return QStringLiteral("Install zip");
    case K::settingsInstallFolder:
      return QStringLiteral("Install folder");
    case K::settingsSkinsFolder:
      return QStringLiteral("Skins folder");
    case K::settingsResetSkinsFolder:
      return QStringLiteral("Reset folder");
    case K::aboutWeb:
      return QStringLiteral("Open tramp.music");
    case K::none:
    case K::timeToggle:
    case K::volume:
    case K::seek:
    case K::eqPreamp:
    case K::eqBand:
    case K::plCollectionRow:
    case K::plDivider:
    case K::plTrackRow:
    case K::plResize:
    case K::settingsSkinRow:
      break;
  }
  return {};
}

class ChromeTooltipWindow : public QWidget {
 public:
  ChromeTooltipWindow() {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus |
                   Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
  }

  void present(QPoint globalAbove, const QString& text, int zoomPercent, const ChromeTokens& look) {
    text_ = text;
    zoom_ = qMax(1, zoomPercent) / 100.0;
    look_ = look;
    const QSize sz = tipSize();
    const int margin = int(std::lround(6 * zoom_));
    QPoint pos(globalAbove.x() - sz.width() / 2, globalAbove.y() - sz.height() - margin);
    if (QScreen* screen = QGuiApplication::screenAt(globalAbove)) {
      const QRect avail = screen->availableGeometry();
      pos.setX(std::clamp(pos.x(), avail.left(), avail.right() - sz.width() + 1));
      if (pos.y() < avail.top()) pos.setY(globalAbove.y() + margin);
    }
    setGeometry(QRect(pos, sz));
    show();
    raise();
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const qreal r = 3 * zoom_;
    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    QColor sheen = look_.coolSheen;
    sheen.setAlpha(0x33);
    p.setBrush(look_.shellMid);
    p.setPen(QPen(sheen, 1));
    p.drawRoundedRect(box, r, r);
    p.setFont(tipFont());
    p.setPen(look_.ink);
    const int padX = int(std::lround(9 * zoom_));
    const int padY = int(std::lround(5 * zoom_));
    p.drawText(rect().adjusted(padX, padY, -padX, -padY), Qt::AlignCenter, text_);
  }

 private:
  QFont tipFont() const {
    QFont font(look_.chromeFamily.isEmpty() ? chromeFamily() : look_.chromeFamily);
    font.setPixelSize(qMax(1, int(std::lround(11 * zoom_))));
    font.setWeight(QFont::Bold);
    font.setLetterSpacing(QFont::PercentageSpacing, 112);
    return font;
  }

  QSize tipSize() const {
    const QFontMetrics fm(tipFont());
    const int padX = int(std::lround(9 * zoom_));
    const int padY = int(std::lround(5 * zoom_));
    return QSize(fm.horizontalAdvance(text_) + padX * 2, fm.height() + padY * 2);
  }

  QString text_;
  qreal zoom_ = 0.75;
  ChromeTokens look_{};
};

ChromeTooltipWindow* g_tip = nullptr;

}  // namespace

QString chromeTooltip(TitleChromeLayout::Hit title, const ChromeHit& chrome,
                      const SessionView& view) {
  if (title == TitleChromeLayout::Hit::drag) return {};
  if (const QString named = titleTip(title, view); !named.isEmpty()) return named;
  return chromeKindTip(chrome, view);
}

TooltipMotion tooltipMotion(const QString& previous, const QString& next, bool busy,
                            bool sameControl) {
  if (busy || next.isEmpty()) return TooltipMotion::hide;
  if (!sameControl || previous != next) return TooltipMotion::restartWait;
  return TooltipMotion::keep;
}

void showChromeTooltip(QPoint globalAbove, const QString& text, int zoomPercent,
                       const ChromeTokens& look) {
  if (text.trimmed().isEmpty()) {
    hideChromeTooltip();
    return;
  }
  if (!g_tip) g_tip = new ChromeTooltipWindow;
  g_tip->present(globalAbove, text, zoomPercent, look);
}

void hideChromeTooltip() {
  if (g_tip) g_tip->hide();
}

}  // namespace tramp
