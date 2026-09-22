#include "chrome_tooltip.h"

#include "aoide_fonts.h"
#include "aoide_metrics.h"
#include "track_info.h"

#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QTextLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace aoide {
namespace {

/// Naming the control is no use on a button that will not take it: "Zoom out"
/// on a dead button says what it would have done, which is the reading the
/// disabled face exists to prevent. So a withdrawn step says why instead, and
/// the two reasons are not the same answer — the floor of the ladder is where
/// Aoide ends, and a step that will not fit is where this display ends. On the
/// floor of a short screen both are out at once, and a whole cluster greyed for
/// one unexplained reason is what reads as broken chrome.
QString zoomFloorTip(const SessionView& view) {
  return QStringLiteral("%1% is as small as Aoide goes").arg(zoomLabel(view.zoomPercent));
}

/// The step is named rather than called "the next one": the readout between the
/// buttons already says where the listener is, and this says what is out of
/// reach from there. Closing a panel is the way back — but only while one is
/// open to close, or the sentence sends them after something that is not there.
QString zoomNoRoomTip(const SessionView& view) {
  const qreal step = nextZoomPercent(view.zoomPercent);
  if (!view.eqOn && !view.plOn) {
    return QStringLiteral("No room for %1% on this display").arg(zoomLabel(step));
  }
  return QStringLiteral("No room for %1% — close a panel").arg(zoomLabel(step));
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
    case K::skins:
      return view.skinsOn ? QStringLiteral("Hide Skins") : QStringLiteral("Show Skins");
    case K::trackInfo:
      return view.trackInfoEnabled ? QStringLiteral("Track info")
                                   : QStringLiteral("No track loaded.");
    case K::trackInfoCopy:
      return QStringLiteral("Copy all track details");
    case K::trackInfoField: {
      const auto fields = trackInfoFields(view);
      if (chrome.index < 0 || chrome.index >= fields.size()) return {};
      const auto& field = fields[chrome.index];
      return field.label + QStringLiteral(": ") +
             (field.value.trimmed().isEmpty() ? QStringLiteral("No tag available") : field.value);
    }
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
    case K::plSave:
      return view.playlistAltered ? QStringLiteral("Save playlist")
                                  : QStringLiteral("No changes to save");
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
    case K::settingsAudio:
      return QStringLiteral("Audio");
    case K::settingsResume:
      return resumePlaybackLabel();
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
    case K::settingsAudioDevice:
      return QStringLiteral("Output device");
    case K::settingsExclusive:
      return QStringLiteral("Exclusive output");
    case K::settingsSkinAdd:
      return QStringLiteral("Install skin");
    case K::settingsSkinsFolder:
      return QStringLiteral("Open skins folder");
    case K::settingsSkinsRefresh:
      return QStringLiteral("Refresh skins");
    case K::settingsSkinRemove:
      if (chrome.index >= 0 && chrome.index < view.skins.size()) {
        return QStringLiteral("Remove %1").arg(view.skins[chrome.index].name);
      }
      return QStringLiteral("Remove skin");
    case K::aboutWeb:
      return QStringLiteral("Open aoide.music");
    case K::none:
    case K::timeToggle:
    case K::volume:
    case K::seek:
    case K::eqPreamp:
    case K::eqBand:
    case K::plCollectionRow:
    case K::plCollectionScroll:
    case K::plDivider:
    case K::plTrackRow:
    case K::plResize:
    case K::settingsSkinRow:
    case K::settingsSkinScroll:
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

  void present(QPoint globalAbove, const QString& text, qreal zoomPercent, const ChromeTokens& look) {
    // Tags are untrusted in size. The clipboard keeps the full value; a hover
    // preview need only lay out enough text to fill a screen.
    constexpr qsizetype maxCharacters = 16384;
    text_ = text.left(maxCharacters);
    if (text.size() > maxCharacters) {
      if (text_.back().isHighSurrogate()) text_.chop(1);
      text_ += QChar(0x2026);
    }
    zoom_ = qMax(qreal(1), zoomPercent) / 100.0;
    look_ = look;
    QScreen* screen = QGuiApplication::screenAt(globalAbove);
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect avail = screen ? screen->availableGeometry() : QRect(0, 0, 640, 480);
    const QSize sz = prepareText(avail.size());
    const int margin = int(std::lround(6 * zoom_));
    qint64 y = qint64(globalAbove.y()) - sz.height() - margin;
    if (y < avail.top()) y = qint64(globalAbove.y()) + margin;
    const QPoint pos(
        int(std::clamp<qint64>(qint64(globalAbove.x()) - sz.width() / 2, avail.left(),
                               qint64(avail.right()) - sz.width() + 1)),
        int(std::clamp<qint64>(y, avail.top(), qint64(avail.bottom()) - sz.height() + 1)));
    const QRect geometry(pos, sz);
    if (this->geometry() != geometry) setGeometry(geometry);
    update();
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
    const QRect content = rect().adjusted(padX, padY, -padX, -padY);
    if (wrapped_) {
      p.setClipRect(content);
      textLayout_.draw(&p, QPointF(padX, padY));
    } else {
      p.drawText(content, Qt::AlignCenter, text_);
    }
  }

 private:
  QFont tipFont() const {
    QFont font(look_.chromeFamily.isEmpty() ? chromeFamily() : look_.chromeFamily);
    font.setPixelSize(qMax(1, int(std::lround(11 * zoom_))));
    font.setWeight(QFont::Bold);
    font.setLetterSpacing(QFont::PercentageSpacing, 112);
    return font;
  }

  QSize prepareText(QSize available) {
    const QFontMetrics fm(tipFont());
    const int padX = int(std::lround(9 * zoom_));
    const int padY = int(std::lround(5 * zoom_));
    const int textWidth = qMax(1, qMin(int(std::lround(480 * zoom_)),
                                      available.width() - padX * 2));
    const int textHeight = qMax(1, available.height() - padY * 2);
    const int singleLineWidth = fm.horizontalAdvance(text_);
    wrapped_ = singleLineWidth > textWidth || text_.contains(QLatin1Char('\n'));
    if (!wrapped_) {
      return QSize(singleLineWidth + padX * 2, fm.height() + padY * 2).boundedTo(available);
    }

    text_.replace(QLatin1Char('\n'), QChar::LineSeparator);
    textLayout_.setFont(tipFont());
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout_.setTextOption(option);
    auto layout = [&]() {
      textLayout_.setText(text_);
      textLayout_.beginLayout();
      qreal width = 0;
      qreal height = 0;
      while (true) {
        QTextLine line = textLayout_.createLine();
        if (!line.isValid()) break;
        line.setLineWidth(textWidth);
        line.setPosition(QPointF(0, height));
        width = qMax(width, line.naturalTextWidth());
        height += line.height();
        // Reserve only lines that fit. The final visible line is elided below
        // if another line would run beyond the available screen height.
        if (height + line.height() > textHeight) break;
      }
      textLayout_.endLayout();
      return QSize(int(std::ceil(width)) + padX * 2, int(std::ceil(height)) + padY * 2);
    };
    QSize size = layout();
    const QTextLine last = textLayout_.lineAt(textLayout_.lineCount() - 1);
    if (last.textStart() + last.textLength() < text_.size()) {
      const int start = last.textStart();
      text_ = text_.left(start) +
              fm.elidedText(text_.mid(start).simplified(), Qt::ElideRight, textWidth);
      size = layout();
    }
    return size.boundedTo(available);
  }

  QString text_;
  QTextLayout textLayout_;
  bool wrapped_ = false;
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

void showChromeTooltip(QPoint globalAbove, const QString& text, qreal zoomPercent,
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

}  // namespace aoide
