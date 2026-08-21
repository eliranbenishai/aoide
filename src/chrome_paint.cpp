#include "chrome_paint.h"

#include "chrome_bodies.h"
#include "look.h"
#include "mockup_draw.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>

namespace tramp {
namespace {

/// Every painter below leaves the painter as it found it, the same contract
/// `mockup_draw.h` states for the primitives they call — and it is the whole of
/// [PainterState], not the pen, brush and font a check happened to be written
/// for. Nothing here was broken by the ones that did not: `drawTitleContents`
/// happens to set what it needs before each call, so the leaks were invisible.
/// That is exactly how the playlist footer lost three readouts —
/// `drawStatusDot` left `Qt::NoPen` behind, and every `drawText` after it drew
/// nothing — so the rule holds here whether or not a caller currently depends
/// on it.
const ChromeTokens& T() { return currentLook(); }

void drawShell(QPainter& p, const QRectF& rect) {
  QPainterPath path;
  path.addRoundedRect(rect, kShellRadius, kShellRadius);
  QLinearGradient face(rect.topLeft(), rect.bottomLeft());
  face.setColorAt(0, T().shellHi);
  face.setColorAt(0.03, T().shell);
  face.setColorAt(0.46, T().shellMid);
  face.setColorAt(0.92, T().shellLo);
  face.setColorAt(1, T().shellDeep);
  p.fillPath(path, face);

  p.save();
  p.setClipPath(path);
  p.setPen(QPen(T().bevelLight, 1));
  p.drawLine(QPointF(rect.left() + 1, rect.top() + 1),
             QPointF(rect.right() - 1, rect.top() + 1));
  p.setPen(QPen(T().bevelSoft, 1));
  p.drawLine(QPointF(rect.left() + 1, rect.top() + 1),
             QPointF(rect.left() + 1, rect.bottom() - 1));
  p.setPen(QPen(QColor(0, 0, 0, 140), 1));
  p.drawLine(QPointF(rect.right() - 1, rect.top() + 1),
             QPointF(rect.right() - 1, rect.bottom() - 1));
  p.setPen(QPen(QColor(0, 0, 0, 230), 1));
  p.drawLine(QPointF(rect.left() + 1, rect.bottom() - 1),
             QPointF(rect.right() - 1, rect.bottom() - 1));
  p.restore();
}

void drawRivet(QPainter& p, QPointF center) {
  constexpr qreal r = 3.5;
  QRadialGradient g(center + QPointF(-1.0, -1.4), r);
  g.setColorAt(0, T().metalHi);
  g.setColorAt(0.6, T().shell);
  g.setColorAt(1, T().shellDeep);
  p.save();
  p.setPen(Qt::NoPen);
  p.setBrush(withAlpha(T().coolSheen, 31));
  p.drawEllipse(center + QPointF(0, 0.5), r, r);
  p.setBrush(g);
  p.drawEllipse(center, r, r);
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(0, 0, 0, 204), 0.8));
  p.drawEllipse(center, r, r);
  p.restore();
}

void drawTitleFace(QPainter& p, const QRectF& bar) {
  p.save();
  p.setClipRect(bar, Qt::IntersectClip);
  QPainterPath facePath;
  facePath.addRoundedRect(QRectF(bar.left(), bar.top(), bar.width(), bar.height() + kShellRadius),
                          kShellRadius, kShellRadius);
  p.setClipPath(facePath, Qt::IntersectClip);
  QLinearGradient face(bar.topLeft(), bar.bottomLeft());
  face.setColorAt(0, T().titleBar0);
  face.setColorAt(0.26, T().titleBar26);
  face.setColorAt(0.62, T().titleBar62);
  face.setColorAt(1, T().titleBar100);
  p.fillPath(facePath, face);

  QLinearGradient lift(bar.topLeft(), QPointF(bar.left(), bar.top() + bar.height() * 0.5));
  lift.setColorAt(0, withAlpha(T().hoverLift, 31));
  lift.setColorAt(1, withAlpha(T().hoverLift, 0));
  p.fillRect(QRectF(bar.left(), bar.top(), bar.width(), bar.height() * 0.5), lift);

  p.setPen(QPen(withAlpha(T().coolSheen, 56), 1));
  p.drawLine(QPointF(bar.left(), bar.top() + 0.5),
             QPointF(bar.right(), bar.top() + 0.5));
  p.setPen(QPen(QColor(0, 0, 0, 191), 1));
  p.drawLine(QPointF(bar.left(), bar.bottom() - 0.5),
             QPointF(bar.right(), bar.bottom() - 0.5));
  p.restore();
}

void drawGrip(QPainter& p, const QRectF& slot) {
  if (slot.width() < 8) {
    return;
  }
  const QRectF rail(slot.left(), slot.top() + (slot.height() - 2) / 2, slot.width(), 2);
  p.save();
  paintBlurred(p, rail.adjusted(0, -8, 0, 10), 3.5, [&](QPainter& bp) {
    QLinearGradient bloom(rail.topLeft(), rail.topRight());
    bloom.setColorAt(0, withAlpha(T().phos, 0));
    bloom.setColorAt(0.12, withAlpha(T().phos, 77));
    bloom.setColorAt(0.88, withAlpha(T().phos, 77));
    bloom.setColorAt(1, withAlpha(T().phos, 0));
    bp.setPen(Qt::NoPen);
    bp.setBrush(bloom);
    bp.drawRect(rail);
  });

  const QColor rail0 = T().railStops.value(0, T().phosDim);
  const QColor rail1 = T().railStops.value(1, T().accentDim);
  const QColor rail2 = T().railStops.value(2, T().phosDim);
  QLinearGradient railFill(rail.topLeft(), rail.topRight());
  railFill.setColorAt(0, Qt::transparent);
  railFill.setColorAt(0.12, rail0);
  railFill.setColorAt(0.5, rail1);
  railFill.setColorAt(0.88, rail2);
  railFill.setColorAt(1, Qt::transparent);
  p.setPen(Qt::NoPen);
  p.setBrush(railFill);
  p.drawRect(rail);

  const QRectF under(rail.left(), rail.bottom() + 2, rail.width(), 1);
  QLinearGradient magenta(under.topLeft(), under.topRight());
  magenta.setColorAt(0, Qt::transparent);
  magenta.setColorAt(0.5, withAlpha(T().accent, 89));
  magenta.setColorAt(1, Qt::transparent);
  p.setBrush(magenta);
  p.drawRect(under);
  p.restore();
}

void drawGlyph(QPainter& p, const QRect& btn, TitleChromeLayout::Hit kind, const QColor& color) {
  MockupIcon icon = MockupIcon::minimize;
  switch (kind) {
    case TitleChromeLayout::Hit::minimize:
    case TitleChromeLayout::Hit::collapse:
      icon = MockupIcon::minimize;
      break;
    case TitleChromeLayout::Hit::zoomOut:
      icon = MockupIcon::zoomOut;
      break;
    case TitleChromeLayout::Hit::zoomIn:
      icon = MockupIcon::zoomIn;
      break;
    case TitleChromeLayout::Hit::close:
      icon = MockupIcon::close;
      break;
    default:
      return;
  }
  const QRectF g = QRectF(btn).adjusted(7, 5, -7, -5);
  drawIcon(p, g, icon, color);
}

void drawWinBtn(QPainter& p, const QRect& btn, bool close, TitleChromeLayout::Hit kind,
                BtnFace state) {
  if (btn.isEmpty()) {
    return;
  }
  const qreal hover = std::clamp(state.hover, qreal(0), qreal(1));
  const qreal press = std::clamp(state.press, qreal(0), qreal(1));
  const QRectF r = btn;
  QPainterPath path;
  path.addRoundedRect(r, 3, 3);
  paintBlurred(p, r.adjusted(-4, -2, 4, 6), 1.2, [&](QPainter& bp) {
    bp.setPen(Qt::NoPen);
    bp.setBrush(QColor(0, 0, 0, 140));
    bp.drawPath(path.translated(0, 1));
  });

  // Close is already the loud one, so it lifts less than the neutral buttons or
  // it goes past the accent and stops reading as a warning.
  const qreal lift = 1 + (close ? 0.16 : 0.24) * hover - 0.18 * press;
  QLinearGradient face(r.topLeft(), r.bottomLeft());
  if (close) {
    face.setColorAt(0, scaled(T().wbtnClose0, lift));
    face.setColorAt(0.55, scaled(T().wbtnClose55, lift));
    face.setColorAt(1, scaled(T().wbtnClose100, lift));
  } else {
    face.setColorAt(0, scaled(T().wbtn0, lift));
    face.setColorAt(0.55, scaled(T().wbtn55, lift));
    face.setColorAt(1, scaled(T().wbtn100, lift));
  }
  p.fillPath(path, face);

  p.save();
  p.setClipPath(path);
  p.fillRect(QRectF(r.left(), r.top(), r.width(), 1),
             withAlpha(T().hoverLift, int(std::lround(77 + 70 * hover))));
  p.fillRect(QRectF(r.left(), r.bottom() - 1, r.width(), 1), QColor(0, 0, 0, 153));
  if (press > 0.004) {
    p.fillRect(r, QColor(0, 0, 0, int(std::lround(46 * press))));
  }
  p.restore();

  drawGlyph(p, btn, kind, close ? T().closeGlyph : T().glyphInk);
}

void drawLogo(QPainter& p, const QRectF& disc, const QImage* logo) {
  drawDiscLogo(p, disc, logo);
}

void drawWordmark(QPainter& p, const QRectF& box) {
  QFont font = condensedFont(24, 0.2);
  const QString text = QStringLiteral("TRAMP");
  drawStyledText(p, box, text, font, T().wordmark, Qt::AlignVCenter | Qt::AlignLeft,
                 {
                     {withAlpha(T().hoverLift, 77), QPointF(0, -1), 0},
                     {QColor(0, 0, 0, 217), QPointF(0, 1), 0},
                     {withAlpha(T().phos, 77), QPointF(), 5},
                 });
}

QFont roleFont() {
  return condensedFont(13, 0.26);
}

void drawRole(QPainter& p, const QRectF& box, const QString& name) {
  p.save();
  p.setFont(roleFont());
  const QString text = name.toUpper();
  p.setPen(QColor(0, 0, 0, 179));
  p.drawText(box.translated(0, 1), Qt::AlignCenter, text);
  p.setPen(T().windowName);
  p.drawText(box, Qt::AlignCenter, text);
  p.restore();
}

void drawTitleContents(QPainter& p, const TitleChromeLayout& title, const QImage* logo,
                       int zoomPercent, const ChromePhases& phases) {
  constexpr int padL = 10;
  p.save();
  const QRect bar = title.titleBar;
  qreal x = padL;
  qreal brandRight = padL;
  if (title.showBrand) {
    const QRectF disc(x, (bar.height() - 30) / 2.0, 30, 30);
    drawLogo(p, disc, logo);
    x = disc.right() + 12;
    const QFont wm = condensedFont(24, 0.2);
    const QFontMetrics fm(wm);
    const int wmW = fm.horizontalAdvance(QStringLiteral("TRAMP"));
    drawWordmark(p, QRectF(x + 2, 0, wmW, bar.height()));
    brandRight = x + wmW + 12;
  }

  p.setFont(roleFont());
  const QFontMetrics nfm(p.font());
  const QString role = title.roleName.toUpper();
  const int nameW = nfm.horizontalAdvance(role);
  const qreal availLeft = brandRight;
  const qreal availRight = title.dragRight;
  const qreal avail = availRight - availLeft;
  const qreal grips = (avail - nameW - 24) / 2.0;
  const QRectF leftGrip(availLeft, 0, qMax<qreal>(0, grips), bar.height());
  const QRectF nameBox(leftGrip.right() + 12, 0, nameW, bar.height());
  const QRectF rightGrip(nameBox.right() + 12, 0, qMax<qreal>(0, grips), bar.height());
  drawGrip(p, leftGrip);
  drawRole(p, nameBox, title.roleName);
  drawGrip(p, rightGrip);

  if (title.showZoom && !title.zoomReadout.isEmpty()) {
    const QString label = QString::number(zoomPercent) + QLatin1Char('%');
    p.setFont(condensedFont(11, 0.12));
    p.setPen(QColor(0, 0, 0, 179));
    p.drawText(title.zoomReadout.translated(0, 1), Qt::AlignCenter, label);
    p.setPen(T().inkDim);
    p.drawText(title.zoomReadout, Qt::AlignCenter, label);
  }

  using Hit = TitleChromeLayout::Hit;
  if (title.showZoom) {
    drawWinBtn(p, title.minimize, false, Hit::minimize, phases.titleFace(Hit::minimize));
    drawWinBtn(p, title.zoomOut, false, Hit::zoomOut, phases.titleFace(Hit::zoomOut));
    drawWinBtn(p, title.zoomIn, false, Hit::zoomIn, phases.titleFace(Hit::zoomIn));
  } else {
    drawWinBtn(p, title.minimize, false, Hit::collapse, phases.titleFace(Hit::collapse));
  }
  drawWinBtn(p, title.close, true, Hit::close, phases.titleFace(Hit::close));
  p.restore();
}

}  // namespace

void paintMockupWindow(QPainter& painter,
                       QSize logical,
                       WindowId id,
                       const TitleChromeLayout& title,
                       const QImage* logo,
                       const SessionView& view,
                       BodyPaint pass,
                       const ChromePhases& phases) {
  LookPaintScope scope(view.look);
  if (pass == BodyPaint::live) {
    // The chassis pass paints inside the clip below, which is saved either way.
    // The live pass has no such wrapper, and this is the module's front door:
    // whatever the body layer does behind it, a caller's painter comes back
    // untouched.
    const PainterStateScope hold(painter);
    paintWindowBody(painter, id, logical, logo, view, pass, phases);
    return;
  }
  const QRectF rect(0, 0, logical.width(), logical.height());
  QPainterPath shell;
  shell.addRoundedRect(rect, kShellRadius, kShellRadius);
  drawShell(painter, rect);
  painter.save();
  painter.setClipPath(shell);
  drawNoiseOverlay(painter, rect, kShellRadius);
  drawTitleFace(painter, QRectF(title.titleBar));
  drawTitleContents(painter, title, logo, view.zoomPercent, phases);
  if (logical.height() > kTitleBar) {
    paintWindowBody(painter, id, logical, logo, view, pass, phases);
  }
  drawRivet(painter, QPointF(9 + 3.5, logical.height() - 8 - 3.5));
  drawRivet(painter, QPointF(logical.width() - 9 - 3.5, logical.height() - 8 - 3.5));
  painter.restore();
}

}  // namespace tramp
