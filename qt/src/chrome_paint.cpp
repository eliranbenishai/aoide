#include "chrome_paint.h"

#include "mockup_tokens.h"
#include "tramp_fonts.h"
#include "tramp_metrics.h"

#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>

namespace tramp {
namespace {

void drawShell(QPainter& p, const QRectF& rect) {
  QPainterPath path;
  path.addRoundedRect(rect, kShellRadius, kShellRadius);
  QLinearGradient face(rect.topLeft(), rect.bottomLeft());
  face.setColorAt(0, kShellHi);
  face.setColorAt(0.03, kShell);
  face.setColorAt(0.46, kShellMid);
  face.setColorAt(0.92, kShellLo);
  face.setColorAt(1, kShellDeep);
  p.fillPath(path, face);

  p.save();
  p.setClipPath(path);
  p.setPen(QPen(kBevelLight, 1));
  p.drawLine(QPointF(rect.left() + 1, rect.top() + 1),
             QPointF(rect.right() - 1, rect.top() + 1));
  p.setPen(QPen(kBevelSoft, 1));
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
  g.setColorAt(0, QColor(0x5c, 0x63, 0x73));
  g.setColorAt(0.6, QColor(0x26, 0x2b, 0x33));
  g.setColorAt(1, QColor(0x10, 0x12, 0x18));
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(226, 236, 255, 31));
  p.drawEllipse(center + QPointF(0, 0.5), r, r);
  p.setBrush(g);
  p.drawEllipse(center, r, r);
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(0, 0, 0, 204), 0.8));
  p.drawEllipse(center, r, r);
}

void drawTitleFace(QPainter& p, const QRectF& bar) {
  p.save();
  p.setClipRect(bar);
  QLinearGradient face(bar.topLeft(), bar.bottomLeft());
  face.setColorAt(0, kTitleBar0);
  face.setColorAt(0.26, kTitleBar26);
  face.setColorAt(0.62, kTitleBar62);
  face.setColorAt(1, kTitleBar100);
  p.fillRect(bar, face);

  QLinearGradient lift(bar.topLeft(), QPointF(bar.left(), bar.top() + bar.height() * 0.5));
  lift.setColorAt(0, QColor(232, 240, 255, 31));
  lift.setColorAt(1, QColor(232, 240, 255, 0));
  p.fillRect(QRectF(bar.left(), bar.top(), bar.width(), bar.height() * 0.5), lift);

  p.setPen(QPen(QColor(kCoolSheen.red(), kCoolSheen.green(), kCoolSheen.blue(), 56), 1));
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
  QLinearGradient bloom(rail.topLeft(), rail.topRight());
  bloom.setColorAt(0, QColor(61, 231, 255, 0));
  bloom.setColorAt(0.12, QColor(61, 231, 255, 77));
  bloom.setColorAt(0.88, QColor(61, 231, 255, 77));
  bloom.setColorAt(1, QColor(61, 231, 255, 0));
  p.setPen(Qt::NoPen);
  p.setBrush(bloom);
  p.drawRect(rail.adjusted(0, -2, 0, 2));

  QLinearGradient railFill(rail.topLeft(), rail.topRight());
  railFill.setColorAt(0, Qt::transparent);
  railFill.setColorAt(0.12, kPhosDim);
  railFill.setColorAt(0.5, kAccentDim);
  railFill.setColorAt(0.88, kPhosDim);
  railFill.setColorAt(1, Qt::transparent);
  p.setBrush(railFill);
  p.drawRect(rail);

  const QRectF under(rail.left(), rail.bottom() + 2, rail.width(), 1);
  QLinearGradient magenta(under.topLeft(), under.topRight());
  magenta.setColorAt(0, Qt::transparent);
  magenta.setColorAt(0.5, QColor(255, 61, 154, 89));
  magenta.setColorAt(1, Qt::transparent);
  p.setBrush(magenta);
  p.drawRect(under);
}

void drawGlyph(QPainter& p, const QRect& btn, TitleChromeLayout::Hit kind, const QColor& color) {
  p.save();
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setBrush(color);
  const QRectF g = QRectF(btn).adjusted(7, 5, -7, -5);
  switch (kind) {
    case TitleChromeLayout::Hit::minimize:
    case TitleChromeLayout::Hit::collapse:
      p.fillRect(QRectF(g.left(), g.center().y() - 0.8, g.width(), 1.6), color);
      break;
    case TitleChromeLayout::Hit::zoomOut:
      p.fillRect(QRectF(g.left(), g.center().y() - 0.8, g.width(), 1.6), color);
      break;
    case TitleChromeLayout::Hit::zoomIn: {
      const qreal cx = g.center().x();
      const qreal cy = g.center().y();
      p.fillRect(QRectF(g.left(), cy - 0.8, g.width(), 1.6), color);
      p.fillRect(QRectF(cx - 0.8, g.top(), 1.6, g.height()), color);
      break;
    }
    case TitleChromeLayout::Hit::close:
      p.drawLine(g.topLeft(), g.bottomRight());
      p.drawLine(g.topRight(), g.bottomLeft());
      break;
    default:
      break;
  }
  p.restore();
}

void drawWinBtn(QPainter& p, const QRect& btn, bool close, TitleChromeLayout::Hit kind) {
  if (btn.isEmpty()) {
    return;
  }
  const QRectF r = btn;
  QPainterPath path;
  path.addRoundedRect(r, 3, 3);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0, 0, 0, 140));
  p.drawPath(path.translated(0, 1));

  QLinearGradient face(r.topLeft(), r.bottomLeft());
  if (close) {
    face.setColorAt(0, kWbtnClose0);
    face.setColorAt(0.55, kWbtnClose55);
    face.setColorAt(1, kWbtnClose100);
  } else {
    face.setColorAt(0, kWbtn0);
    face.setColorAt(0.55, kWbtn55);
    face.setColorAt(1, kWbtn100);
  }
  p.setBrush(face);
  p.drawPath(path);

  p.save();
  p.setClipPath(path);
  p.fillRect(QRectF(r.left(), r.top(), r.width(), 1), QColor(232, 240, 255, 77));
  p.fillRect(QRectF(r.left(), r.bottom() - 1, r.width(), 1), QColor(0, 0, 0, 153));
  p.restore();

  drawGlyph(p, btn, kind, close ? kCloseGlyph : kGlyphInk);
}

void drawLogo(QPainter& p, const QRectF& disc, const QImage* logo) {
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(kAccent.red(), kAccent.green(), kAccent.blue(), 71));
  p.drawEllipse(disc.adjusted(-4, -4, 4, 4));
  p.setBrush(QColor(0, 0, 0, 140));
  p.drawEllipse(disc.translated(0, 2));
  p.setBrush(kLogoDisc);
  p.drawEllipse(disc);
  if (logo && !logo->isNull()) {
    p.save();
    QPainterPath clip;
    clip.addEllipse(disc);
    p.setClipPath(clip);
    const QRectF dest = disc.adjusted(-disc.width() * 0.06, -disc.height() * 0.06,
                                      disc.width() * 0.06, disc.height() * 0.06);
    p.drawImage(dest, *logo);
    p.restore();
  }
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(0, 0, 0, 166), 1));
  p.drawEllipse(disc);
  QLinearGradient gloss(disc.topLeft(), QPointF(disc.left(), disc.center().y()));
  gloss.setColorAt(0, QColor(255, 255, 255, 128));
  gloss.setColorAt(1, QColor(255, 255, 255, 0));
  p.setPen(Qt::NoPen);
  p.setBrush(gloss);
  p.save();
  QPainterPath clip;
  clip.addEllipse(disc);
  p.setClipPath(clip);
  p.drawRect(QRectF(disc.left(), disc.top(), disc.width(), disc.height() * 0.55));
  p.restore();
}

void drawWordmark(QPainter& p, const QRectF& box) {
  QFont font(chromeFamily());
  font.setPixelSize(24);
  font.setWeight(QFont::Bold);
  font.setLetterSpacing(QFont::AbsoluteSpacing, 24 * 0.2);
  p.setFont(font);
  const QString text = QStringLiteral("TRAMP");
  p.setPen(QColor(0, 0, 0, 217));
  p.drawText(box.translated(0, 1), Qt::AlignVCenter | Qt::AlignLeft, text);
  p.setPen(QColor(226, 236, 255, 77));
  p.drawText(box.translated(0, -1), Qt::AlignVCenter | Qt::AlignLeft, text);
  p.setPen(kWordmark);
  p.drawText(box, Qt::AlignVCenter | Qt::AlignLeft, text);
}

QFont roleFont() {
  QFont font(chromeFamily());
  font.setPixelSize(13);
  font.setWeight(QFont::Bold);
  font.setLetterSpacing(QFont::AbsoluteSpacing, 13 * 0.26);
  return font;
}

void drawRole(QPainter& p, const QRectF& box, const QString& name) {
  p.setFont(roleFont());
  const QString text = name.toUpper();
  p.setPen(QColor(0, 0, 0, 179));
  p.drawText(box.translated(0, 1), Qt::AlignCenter, text);
  p.setPen(kWindowName);
  p.drawText(box, Qt::AlignCenter, text);
}

void drawTitleContents(QPainter& p, const TitleChromeLayout& title, const QImage* logo) {
  constexpr int padL = 10;
  const QRect bar = title.titleBar;
  qreal x = padL;
  qreal brandRight = padL;
  if (title.showBrand) {
    const QRectF disc(x, (bar.height() - 30) / 2.0, 30, 30);
    drawLogo(p, disc, logo);
    x = disc.right() + 12;
    QFont wm(chromeFamily());
    wm.setPixelSize(24);
    wm.setWeight(QFont::Bold);
    wm.setLetterSpacing(QFont::AbsoluteSpacing, 24 * 0.2);
    const QFontMetrics fm(wm);
    const int wmW = fm.horizontalAdvance(QStringLiteral("TRAMP"));
    drawWordmark(p, QRectF(x, 0, wmW, bar.height()));
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

  if (title.showZoom) {
    drawWinBtn(p, title.minimize, false, TitleChromeLayout::Hit::minimize);
    drawWinBtn(p, title.zoomOut, false, TitleChromeLayout::Hit::zoomOut);
    drawWinBtn(p, title.zoomIn, false, TitleChromeLayout::Hit::zoomIn);
  } else {
    drawWinBtn(p, title.minimize, false, TitleChromeLayout::Hit::collapse);
  }
  drawWinBtn(p, title.close, true, TitleChromeLayout::Hit::close);
}

void drawScreen(QPainter& p, const QRectF& well) {
  QPainterPath path;
  path.addRoundedRect(well, 3, 3);
  p.save();
  p.setClipPath(path);
  QRadialGradient wash(QPointF(well.left() + well.width() * 0.18,
                               well.top() - well.height() * 0.20),
                       well.width() * 0.9);
  wash.setColorAt(0, QColor(0x0f, 0x1c, 0x2a));
  wash.setColorAt(0.48, QColor(0x07, 0x10, 0x18));
  wash.setColorAt(1, QColor(0x04, 0x07, 0x0c));
  p.fillRect(well, wash);

  p.setPen(QPen(QColor(61, 231, 255, 26), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);

  for (qreal y = well.top(); y < well.bottom(); y += 3) {
    p.fillRect(QRectF(well.left(), y, well.width(), 1), QColor(0, 0, 0, 82));
  }
  QLinearGradient glass(well.topLeft(), QPointF(well.left(), well.top() + well.height() * 0.38));
  glass.setColorAt(0, QColor(255, 255, 255, 13));
  glass.setColorAt(1, QColor(255, 255, 255, 0));
  p.fillRect(well, glass);
  p.restore();

  p.setPen(QPen(QColor(226, 236, 255, 31), 1));
  p.drawLine(QPointF(well.left() + 1, well.top()), QPointF(well.right() - 1, well.top()));
}

}  // namespace

void paintMockupWindow(QPainter& painter,
                       QSize logical,
                       const TitleChromeLayout& title,
                       const QImage* logo) {
  const QRectF rect(0, 0, logical.width(), logical.height());
  QPainterPath shell;
  shell.addRoundedRect(rect, kShellRadius, kShellRadius);
  drawShell(painter, rect);
  painter.save();
  painter.setClipPath(shell);
  drawTitleFace(painter, QRectF(title.titleBar));
  drawTitleContents(painter, title, logo);

  const QRectF body(0, kTitleBar, logical.width(), logical.height() - kTitleBar);
  const QRectF well = body.adjusted(10, 8, -10, -16);
  drawScreen(painter, well);

  drawRivet(painter, QPointF(9 + 3.5, logical.height() - 8 - 3.5));
  drawRivet(painter, QPointF(logical.width() - 9 - 3.5, logical.height() - 8 - 3.5));
  painter.restore();
}

}  // namespace tramp
