#include "mockup_draw.h"

#include "chrome_layout.h"
#include "look.h"
#include "tramp_fonts.h"

#include <QElapsedTimer>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>
#include <QtMath>
#include <cmath>
#include <deque>
#include <functional>
#include <vector>

namespace tramp {
namespace {

const ChromeTokens& T() { return currentLook(); }

QImage g_noise;

BlurCost g_blurCost;

struct FontAccount {
  QElapsedTimer clock;
  FontAccount() {
    clock.start();
    g_blurCost.fonts += 1;
  }
  ~FontAccount() { g_blurCost.fontNanos += clock.nsecsElapsed(); }
};

// Ablation switch for the drag benches: proves how much of a repaint is blur.
const bool g_blurOff = qEnvironmentVariableIsSet("TRAMP_BENCH_NO_BLUR");
// Escape hatch to the exact separable kernel, for fidelity comparison.
const bool g_blurExact = qEnvironmentVariable("TRAMP_BLUR") == QLatin1String("exact");
// Sigma at or above which the box approximation takes over. Overridable so the
// fidelity/cost trade-off can be re-measured rather than argued about.
const qreal kBoxBlurSigma =
    qEnvironmentVariableIsSet("TRAMP_BLUR_BOX_SIGMA")
        ? qEnvironmentVariable("TRAMP_BLUR_BOX_SIGMA").toDouble()
        : 4.0;

// Separable Gaussian. `sigma` is Skia/Flutter MaskFilter sigma.
//
// Weights are fixed-point so the inner loop stays in int32: a tap accumulator
// peaks at 255 * kWeightUnit, and the kernel is forced to sum to kWeightUnit
// exactly so flat regions do not drift. Row pointers are resolved per output
// row rather than per tap, and interior pixels skip edge clamping — the naive
// form spent most of its time on `constScanLine` calls and `qBound` per tap.
constexpr int kWeightShift = 12;
constexpr int kWeightUnit = 1 << kWeightShift;

std::vector<int> gaussianKernel(qreal sigma, int radius) {
  const int size = radius * 2 + 1;
  std::vector<double> weights(static_cast<size_t>(size));
  double sum = 0;
  const double s2 = 2.0 * double(sigma) * double(sigma);
  for (int i = -radius; i <= radius; ++i) {
    const double w = std::exp(-(double(i) * double(i)) / s2);
    weights[static_cast<size_t>(i + radius)] = w;
    sum += w;
  }
  std::vector<int> kernel(static_cast<size_t>(size));
  int total = 0;
  for (int i = 0; i < size; ++i) {
    kernel[static_cast<size_t>(i)] =
        int(std::lround(weights[static_cast<size_t>(i)] / sum * kWeightUnit));
    total += kernel[static_cast<size_t>(i)];
  }
  kernel[static_cast<size_t>(radius)] += kWeightUnit - total;
  return kernel;
}

// Box-blur widths whose n-fold convolution matches a Gaussian of `sigma`
// (Kovesi's approximation). Three passes are within a couple of 255ths of the
// true kernel, and each pass is O(1) per pixel instead of O(radius).
std::vector<int> boxWidthsForGauss(double sigma, int passes) {
  const double ideal = std::sqrt((12.0 * sigma * sigma / passes) + 1.0);
  int lower = int(std::floor(ideal));
  if (lower % 2 == 0) lower -= 1;
  lower = qMax(1, lower);
  const int upper = lower + 2;
  const double mIdeal = (12.0 * sigma * sigma - double(passes) * lower * lower -
                         4.0 * passes * lower - 3.0 * passes) /
                        (-4.0 * lower - 4.0);
  const int m = qBound(0, int(std::lround(mIdeal)), passes);
  std::vector<int> widths(static_cast<size_t>(passes));
  for (int i = 0; i < passes; ++i) {
    widths[static_cast<size_t>(i)] = i < m ? lower : upper;
  }
  return widths;
}

// Fixed-point reciprocal: a per-pixel divide would dominate a box pass.
constexpr int kDivShift = 22;

inline uint boxMean(int sum, qint64 recip) {
  return uint((qint64(sum) * recip + (qint64(1) << (kDivShift - 1))) >> kDivShift);
}

/// Clamp-extended horizontal box pass over every row.
void boxRows(const QRgb* in, int inStride, QRgb* out, int outStride, int w, int h, int radius) {
  const qint64 recip = (qint64(1) << kDivShift) / (radius * 2 + 1);
  for (int y = 0; y < h; ++y) {
    const QRgb* line = in + qsizetype(y) * inStride;
    QRgb* dst = out + qsizetype(y) * outStride;
    const auto tap = [&](int x) { return line[qBound(0, x, w - 1)]; };
    int b = 0, g = 0, r = 0, a = 0;
    for (int k = -radius; k <= radius; ++k) {
      const QRgb v = tap(k);
      b += int(v & 0xffu);
      g += int((v >> 8) & 0xffu);
      r += int((v >> 16) & 0xffu);
      a += int((v >> 24) & 0xffu);
    }
    for (int x = 0; x < w; ++x) {
      dst[x] = QRgb((boxMean(a, recip) << 24) | (boxMean(r, recip) << 16) |
                    (boxMean(g, recip) << 8) | boxMean(b, recip));
      const QRgb drop = tap(x - radius);
      const QRgb gain = tap(x + radius + 1);
      b += int(gain & 0xffu) - int(drop & 0xffu);
      g += int((gain >> 8) & 0xffu) - int((drop >> 8) & 0xffu);
      r += int((gain >> 16) & 0xffu) - int((drop >> 16) & 0xffu);
      a += int((gain >> 24) & 0xffu) - int((drop >> 24) & 0xffu);
    }
  }
}

/// Clamp-extended vertical box pass. Carries one running sum per column so
/// every read and write walks memory forwards — a per-column inner loop with a
/// row stride costs a cache miss per pixel instead.
void boxColumns(const QRgb* in, int inStride, QRgb* out, int outStride, int w, int h, int radius,
                std::vector<int>& scratch) {
  const qint64 recip = (qint64(1) << kDivShift) / (radius * 2 + 1);
  scratch.assign(static_cast<size_t>(w) * 4, 0);
  int* sb = scratch.data();
  int* sg = sb + w;
  int* sr = sg + w;
  int* sa = sr + w;
  const auto row = [&](int y) { return in + qsizetype(qBound(0, y, h - 1)) * inStride; };
  for (int k = -radius; k <= radius; ++k) {
    const QRgb* line = row(k);
    for (int x = 0; x < w; ++x) {
      const QRgb v = line[x];
      sb[x] += int(v & 0xffu);
      sg[x] += int((v >> 8) & 0xffu);
      sr[x] += int((v >> 16) & 0xffu);
      sa[x] += int((v >> 24) & 0xffu);
    }
  }
  for (int y = 0; y < h; ++y) {
    QRgb* dst = out + qsizetype(y) * outStride;
    for (int x = 0; x < w; ++x) {
      dst[x] = QRgb((boxMean(sa[x], recip) << 24) | (boxMean(sr[x], recip) << 16) |
                    (boxMean(sg[x], recip) << 8) | boxMean(sb[x], recip));
    }
    const QRgb* drop = row(y - radius);
    const QRgb* gain = row(y + radius + 1);
    for (int x = 0; x < w; ++x) {
      const QRgb d = drop[x];
      const QRgb a2 = gain[x];
      sb[x] += int(a2 & 0xffu) - int(d & 0xffu);
      sg[x] += int((a2 >> 8) & 0xffu) - int((d >> 8) & 0xffu);
      sr[x] += int((a2 >> 16) & 0xffu) - int((d >> 16) & 0xffu);
      sa[x] += int((a2 >> 24) & 0xffu) - int((d >> 24) & 0xffu);
    }
  }
}

QImage boxBlur(const QImage& src, qreal sigma, int passes) {
  const int w = src.width();
  const int h = src.height();
  QImage front = src.copy();
  QImage back(w, h, QImage::Format_ARGB32_Premultiplied);
  const int frontStride = int(front.bytesPerLine() / qsizetype(sizeof(QRgb)));
  const int backStride = int(back.bytesPerLine() / qsizetype(sizeof(QRgb)));
  QRgb* frontBits = reinterpret_cast<QRgb*>(front.bits());
  QRgb* backBits = reinterpret_cast<QRgb*>(back.bits());
  std::vector<int> scratch;

  for (int width : boxWidthsForGauss(double(sigma), passes)) {
    const int radius = (width - 1) / 2;
    if (radius < 1) continue;  // a one-pixel box is the identity; forcing r=1 over-blurs
    boxRows(frontBits, frontStride, backBits, backStride, w, h, radius);
    boxColumns(backBits, backStride, frontBits, frontStride, w, h, radius, scratch);
  }
  return front;
}

QImage gaussianBlur(QImage src, qreal sigma) {
  if (src.isNull() || sigma < 0.12 || g_blurOff) {
    return src;
  }
  QElapsedTimer blurClock;
  blurClock.start();
  g_blurCost.calls += 1;
  g_blurCost.pixels += qint64(src.width()) * src.height();
  const struct Account {
    QElapsedTimer* clock;
    ~Account() { g_blurCost.nanos += clock->nsecsElapsed(); }
  } account{&blurClock};

  src = src.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  // Tight glows are where the eye can actually see kernel shape, and there the
  // exact kernel is only a handful of taps wide — so approximate only the wide
  // blooms, where three box passes are indistinguishable and radius-independent.
  if (!g_blurExact && sigma >= kBoxBlurSigma) {
    return boxBlur(src, sigma, 3);
  }
  const int radius = qMax(1, int(std::ceil(sigma * 3.0)));
  const int kSize = radius * 2 + 1;
  const std::vector<int> kernel = gaussianKernel(sigma, radius);
  const int* kw = kernel.data();

  const int w = src.width();
  const int h = src.height();
  QImage mid(w, h, QImage::Format_ARGB32_Premultiplied);
  QImage out(w, h, QImage::Format_ARGB32_Premultiplied);

  auto blend = [](int b, int g, int r, int a) {
    constexpr int half = kWeightUnit >> 1;
    return QRgb((uint((a + half) >> kWeightShift) << 24) |
                (uint((r + half) >> kWeightShift) << 16) |
                (uint((g + half) >> kWeightShift) << 8) | uint((b + half) >> kWeightShift));
  };

  for (int y = 0; y < h; ++y) {
    const QRgb* in = reinterpret_cast<const QRgb*>(src.constScanLine(y));
    QRgb* dst = reinterpret_cast<QRgb*>(mid.scanLine(y));
    for (int x = 0; x < w; ++x) {
      int b = 0, g = 0, r = 0, a = 0;
      const int first = x - radius;
      if (first >= 0 && x + radius < w) {
        const QRgb* tap = in + first;
        for (int k = 0; k < kSize; ++k) {
          const QRgb v = tap[k];
          const int weight = kw[k];
          b += int(v & 0xffu) * weight;
          g += int((v >> 8) & 0xffu) * weight;
          r += int((v >> 16) & 0xffu) * weight;
          a += int((v >> 24) & 0xffu) * weight;
        }
      } else {
        for (int k = 0; k < kSize; ++k) {
          const QRgb v = in[qBound(0, first + k, w - 1)];
          const int weight = kw[k];
          b += int(v & 0xffu) * weight;
          g += int((v >> 8) & 0xffu) * weight;
          r += int((v >> 16) & 0xffu) * weight;
          a += int((v >> 24) & 0xffu) * weight;
        }
      }
      dst[x] = blend(b, g, r, a);
    }
  }

  std::vector<const QRgb*> rows(static_cast<size_t>(h));
  for (int y = 0; y < h; ++y) {
    rows[static_cast<size_t>(y)] = reinterpret_cast<const QRgb*>(mid.constScanLine(y));
  }
  std::vector<const QRgb*> taps(static_cast<size_t>(kSize));
  for (int y = 0; y < h; ++y) {
    for (int k = 0; k < kSize; ++k) {
      taps[static_cast<size_t>(k)] = rows[static_cast<size_t>(qBound(0, y - radius + k, h - 1))];
    }
    const QRgb* const* tap = taps.data();
    QRgb* dst = reinterpret_cast<QRgb*>(out.scanLine(y));
    for (int x = 0; x < w; ++x) {
      int b = 0, g = 0, r = 0, a = 0;
      for (int k = 0; k < kSize; ++k) {
        const QRgb v = tap[k][x];
        const int weight = kw[k];
        b += int(v & 0xffu) * weight;
        g += int((v >> 8) & 0xffu) * weight;
        r += int((v >> 16) & 0xffu) * weight;
        a += int((v >> 24) & 0xffu) * weight;
      }
      dst[x] = blend(b, g, r, a);
    }
  }
  return out;
}

QImage noiseTile() {
  if (!g_noise.isNull()) {
    return g_noise;
  }
  constexpr int n = 140;
  g_noise = QImage(n, n, QImage::Format_ARGB32);
  auto hash = [](int x, int y) -> double {
    qint64 v = (qint64(x) * 374761393) ^ (qint64(y) * 668265263) ^
               (qint64(x) * y * 1274126177);
    v = (v ^ (v >> 13)) * 1274126177;
    v = (v ^ (v >> 16)) & 0x7fffffff;
    return double(v) / double(0x7fffffff);
  };
  for (int y = 0; y < n; ++y) {
    for (int x = 0; x < n; ++x) {
      double sum = 0;
      double amp = 1;
      double norm = 0;
      int xo = x;
      int yo = y;
      for (int o = 0; o < 3; ++o) {
        sum += amp * hash(xo, yo);
        norm += amp;
        amp *= 0.5;
        xo = xo * 2 + 17;
        yo = yo * 2 + 31;
      }
      const int v = int(std::round((sum / norm) * 255.0));
      g_noise.setPixel(x, y, qRgba(v, v, v, 255));
    }
  }
  return g_noise;
}

void paintIconPath(QPainter& p, const QRectF& box, qreal view, const QPainterPath& path,
                   const QColor& color) {
  p.save();
  p.translate(box.topLeft());
  p.scale(box.width() / view, box.height() / view);
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawPath(path);
  p.restore();
}

QPainterPath pathPrev() {
  QPainterPath path;
  path.addRect(6, 5, 2.4, 14);
  path.moveTo(20, 5);
  path.lineTo(20, 19);
  path.lineTo(10.4, 12);
  path.closeSubpath();
  return path;
}

QPainterPath pathPlay() {
  QPainterPath path;
  path.moveTo(7, 4.5);
  path.lineTo(20, 12);
  path.lineTo(7, 19.5);
  path.closeSubpath();
  return path;
}

QPainterPath pathPause() {
  QPainterPath path;
  path.addRect(7, 5, 3.6, 14);
  path.addRect(13.4, 5, 3.6, 14);
  return path;
}

QPainterPath pathStop() {
  QPainterPath path;
  path.addRect(6, 6, 12, 12);
  return path;
}

QPainterPath pathNext() {
  QPainterPath path;
  path.addRect(15.6, 5, 2.4, 14);
  path.moveTo(4, 5);
  path.lineTo(13.6, 12);
  path.lineTo(4, 19);
  path.closeSubpath();
  return path;
}

QPainterPath pathEject() {
  QPainterPath path;
  path.moveTo(12, 4.5);
  path.lineTo(19.5, 13);
  path.lineTo(4.5, 13);
  path.closeSubpath();
  path.addRect(4.5, 15.5, 15, 3.5);
  return path;
}

QPainterPath pathAdd() {
  QPainterPath path;
  path.addRect(10.9, 4, 2.2, 6.9);
  path.addRect(4, 10.9, 16, 2.2);
  path.addRect(10.9, 13.1, 2.2, 6.9);
  return path;
}

QPainterPath pathRemove() {
  QPainterPath path;
  path.addRect(4, 10.9, 16, 2.2);
  return path;
}

QPainterPath pathSave() {
  QPainterPath path;
  path.addRect(10.9, 3.5, 2.2, 9.4);
  path.moveTo(12, 16.4);
  path.lineTo(6.4, 10.6);
  path.lineTo(17.6, 10.6);
  path.closeSubpath();
  path.addRect(4.5, 17.8, 15, 2.2);
  path.addRect(4.5, 17.8, 2.2, 4.4);
  path.addRect(17.3, 17.8, 2.2, 4.4);
  return path;
}

QPainterPath pathSort() {
  QPainterPath path;
  path.addRoundedRect(2.6, 5.4, 11, 2.2, 1.1, 1.1);
  path.addRoundedRect(2.6, 10.9, 8, 2.2, 1.1, 1.1);
  path.addRoundedRect(2.6, 16.4, 5, 2.2, 1.1, 1.1);
  path.addRoundedRect(17.9, 5.4, 2.2, 9.4, 0.6, 0.6);
  path.moveTo(19, 19.3);
  path.lineTo(14.9, 14.2);
  path.lineTo(23.1, 14.2);
  path.closeSubpath();
  return path;
}

QPainterPath pathMinimize() {
  QPainterPath path;
  path.addRect(3, 10, 10, 2);
  return path;
}

QPainterPath pathZoomOut() {
  QPainterPath path;
  path.addRect(3, 7, 10, 2);
  return path;
}

QPainterPath pathZoomIn() {
  QPainterPath path;
  path.addRect(7, 3, 2, 4);
  path.addRect(9, 7, 4, 2);
  path.addRect(7, 9, 2, 4);
  path.addRect(3, 7, 4, 2);
  return path;
}

QPainterPath pathClose() {
  QPainterPath path;
  path.moveTo(4.4, 3);
  path.lineTo(8, 6.6);
  path.lineTo(11.6, 3);
  path.lineTo(13, 4.4);
  path.lineTo(9.4, 8);
  path.lineTo(13, 11.6);
  path.lineTo(11.6, 13);
  path.lineTo(8, 9.4);
  path.lineTo(4.4, 13);
  path.lineTo(3, 11.6);
  path.lineTo(6.6, 8);
  path.lineTo(3, 4.4);
  path.closeSubpath();
  return path;
}

void drawMute(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  p.translate(box.topLeft());
  p.scale(box.width() / 24.0, box.height() / 24.0);
  // The cone starts at x=4 but the outer wave's stroke reaches x=26.6, so the
  // mockup's own artwork sits 3.3 units right of its 24-unit box and spills past
  // it. A browser hides the spill by clipping to the viewBox; QPainter does not,
  // which left this the one glyph visibly off its button's axis.
  p.translate(-3.325, 0);
  QPainterPath body;
  body.moveTo(4, 9.4);
  body.lineTo(7.3, 9.4);
  body.lineTo(12, 5);
  body.lineTo(12, 19);
  body.lineTo(7.3, 14.6);
  body.lineTo(4, 14.6);
  body.closeSubpath();
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawPath(body);
  QPen wave(color, 1.7, Qt::SolidLine, Qt::RoundCap);
  p.setPen(wave);
  p.setBrush(Qt::NoBrush);
  p.drawArc(QRectF(11.2, 7.8, 8.4, 8.4), 90 * 16, -180 * 16);
  p.drawArc(QRectF(10.2, 4.2, 15.6, 15.6), 90 * 16, -180 * 16);
  p.restore();
}

void drawCog(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  p.translate(box.center());
  const qreal s = box.width() / 24.0;
  p.scale(s, s);
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  for (int i = 0; i < 8; ++i) {
    p.save();
    p.rotate(i * 45.0);
    p.drawRoundedRect(QRectF(-1.1, -8.8, 2.2, 4.0), 0.5, 0.5);
    p.restore();
  }
  QPainterPath ring;
  ring.setFillRule(Qt::OddEvenFill);
  ring.addEllipse(QRectF(-6.3, -6.3, 12.6, 12.6));
  ring.addEllipse(QRectF(-2.6, -2.6, 5.2, 5.2));
  p.drawPath(ring);
  p.restore();
}

QPainterPath pathSkins() {
  // Upper-face mask: brow, cheeks, nose. A full face with a smile is a
  // smiley at 16px; an eye-bar is goggles. This silhouette is only a mask.
  QPainterPath path;
  path.setFillRule(Qt::OddEvenFill);
  path.moveTo(3.0, 8.6);
  path.cubicTo(3.0, 3.8, 7.0, 2.2, 12.0, 2.2);
  path.cubicTo(17.0, 2.2, 21.0, 3.8, 21.0, 8.6);
  path.cubicTo(21.0, 11.4, 18.8, 13.8, 16.4, 14.4);
  path.lineTo(14.2, 15.0);
  path.lineTo(12.0, 18.8);
  path.lineTo(9.8, 15.0);
  path.lineTo(7.6, 14.4);
  path.cubicTo(5.2, 13.8, 3.0, 11.4, 3.0, 8.6);
  path.closeSubpath();
  path.addEllipse(QRectF(5.4, 6.4, 5.0, 4.2));
  path.addEllipse(QRectF(13.6, 6.4, 5.0, 4.2));
  return path;
}

QPainterPath pathTrackInfo() {
  // Circled lowercase i — the information mark, not a font letter and not the
  // retired clutterbar I. The cut-outs stay fat enough to open at 16px.
  QPainterPath path;
  path.setFillRule(Qt::OddEvenFill);
  path.addEllipse(QRectF(2.2, 2.2, 19.6, 19.6));
  path.addEllipse(QRectF(10.0, 4.8, 4.0, 3.4));
  path.addRoundedRect(QRectF(10.0, 11.2, 4.0, 7.0), 1.8, 1.8);
  return path;
}

}  // namespace

void paintBlurred(QPainter& p, const QRectF& bounds, qreal sigma,
                  const std::function<void(QPainter&)>& paint) {
  if (sigma < 0.12) {
    p.save();
    paint(p);
    p.restore();
    return;
  }
  QElapsedTimer layerClock;
  layerClock.start();
  g_blurCost.layers += 1;
  const struct LayerAccount {
    QElapsedTimer* clock;
    ~LayerAccount() { g_blurCost.layerNanos += clock->nsecsElapsed(); }
  } layerAccount{&layerClock};
  const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
  QImage buf(int(std::ceil(bounds.width())) + pad * 2,
             int(std::ceil(bounds.height())) + pad * 2,
             QImage::Format_ARGB32_Premultiplied);
  buf.fill(Qt::transparent);
  QPainter bp(&buf);
  bp.setRenderHint(QPainter::Antialiasing);
  bp.setRenderHint(QPainter::TextAntialiasing);
  bp.translate(pad - bounds.left(), pad - bounds.top());
  paint(bp);
  bp.end();
  p.drawImage(bounds.topLeft() - QPointF(pad, pad), gaussianBlur(buf, sigma));
}

QFont condensedFont(int px, qreal trackingEm) {
  FontAccount account;
  QFont f(chromeFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Bold);
  f.setHintingPreference(QFont::PreferNoHinting);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

QFont monoFont(int px, qreal trackingEm) {
  FontAccount account;
  QFont f(lcdFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Medium);
  f.setHintingPreference(QFont::PreferNoHinting);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

QFont brandFont(int px, qreal trackingEm) {
  FontAccount account;
  QFont f(brandFamily());
  f.setPixelSize(px);
  f.setWeight(QFont::Normal);
  f.setHintingPreference(QFont::PreferNoHinting);
  f.setStyleStrategy(QFont::PreferAntialias);
  if (trackingEm != 0) {
    f.setLetterSpacing(QFont::AbsoluteSpacing, px * trackingEm);
  }
  return f;
}

qreal textWidth(const QFont& font, const QString& text) {
  return QFontMetricsF(font).horizontalAdvance(text);
}

void fillRound(QPainter& p, const QRectF& r, qreal radius, const QBrush& brush) {
  QPainterPath path;
  path.addRoundedRect(r, radius, radius);
  p.fillPath(path, brush);
}

namespace {

struct CachedWell {
  int w = 0;
  int h = 0;
  QString lookId;
  QImage bloom;
  int bloomPad = 0;
  QImage inner;
  int innerPad = 0;
};

const CachedWell& cachedWell(int w, int h) {
  // Bounded, most-recent-first. Every distinct well size costs two blurred
  // ARGB images, and a playlist resize drag walks a new size per pixel of
  // travel — unbounded this grew by megabytes per gesture.
  constexpr size_t kMaxEntries = 24;
  static std::deque<CachedWell> cache;
  const QString lookId = T().id;
  for (size_t i = 0; i < cache.size(); ++i) {
    const CachedWell& c = cache[i];
    if (c.w == w && c.h == h && c.lookId == lookId) {
      if (i > 0) std::swap(cache[0], cache[i]);
      return cache.front();
    }
  }
  CachedWell c;
  c.w = w;
  c.h = h;
  c.lookId = lookId;
  const QRectF well(0, 0, w, h);
  {
    constexpr qreal sigma = 12;
    const QRectF bounds = well.adjusted(-36, -36, 36, 36);
    const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
    c.bloomPad = pad;
    QImage buf(int(std::ceil(bounds.width())) + pad * 2,
               int(std::ceil(bounds.height())) + pad * 2,
               QImage::Format_ARGB32_Premultiplied);
    buf.fill(Qt::transparent);
    QPainter bp(&buf);
    bp.setRenderHint(QPainter::Antialiasing);
    bp.translate(pad - bounds.left(), pad - bounds.top());
    bp.setPen(Qt::NoPen);
    bp.setBrush(withAlpha(T().phos, 13));
    bp.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), kWellRadius - 0.5, kWellRadius - 0.5);
    bp.end();
    c.bloom = gaussianBlur(buf, sigma);
  }
  {
    constexpr qreal sigma = 3;
    const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
    c.innerPad = pad;
    QImage buf(w + pad * 2, h + pad * 2, QImage::Format_ARGB32_Premultiplied);
    buf.fill(Qt::transparent);
    QPainter bp(&buf);
    bp.setRenderHint(QPainter::Antialiasing);
    bp.setPen(Qt::NoPen);
    bp.setBrush(QColor(0, 0, 0, 0xE6));
    bp.drawRoundedRect(QRectF(pad + 1, pad + 1, w - 2, h - 2), kWellRadius - 1, kWellRadius - 1);
    bp.end();
    QImage blurred = gaussianBlur(buf, sigma);
    QImage mask(buf.size(), QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    QPainter mp(&mask);
    mp.setRenderHint(QPainter::Antialiasing);
    mp.setPen(Qt::NoPen);
    mp.setBrush(Qt::white);
    mp.drawRoundedRect(QRectF(pad, pad, w, h), kWellRadius, kWellRadius);
    mp.end();
    QPainter cut(&blurred);
    cut.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    cut.drawImage(0, 0, mask);
    cut.end();
    c.inner = std::move(blurred);
  }
  cache.push_front(std::move(c));
  while (cache.size() > kMaxEntries) cache.pop_back();
  return cache.front();
}

}  // namespace

void drawScreenWell(QPainter& p, const QRectF& well) {
  QPainterPath path;
  path.addRoundedRect(well, kWellRadius, kWellRadius);
  p.save();
  p.setClipPath(path);
  QRadialGradient wash(QPointF(well.left() + well.width() * 0.18,
                               well.top() - well.height() * 0.20),
                       well.width() * 0.9);
  wash.setColorAt(0, T().screenWash0);
  wash.setColorAt(0.48, T().screenWash1);
  wash.setColorAt(1, T().screenWash2);
  p.fillRect(well, wash);
  p.setPen(QPen(QColor(T().coolSheen.red(), T().coolSheen.green(), T().coolSheen.blue(), 31), 1));
  p.drawLine(QPointF(well.left(), well.top() + 0.5),
             QPointF(well.right(), well.top() + 0.5));
  p.setPen(QPen(withAlpha(T().phos, 26), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), kWellRadius - 0.5, kWellRadius - 0.5);
  p.restore();

  const CachedWell& fx =
      cachedWell(int(std::lround(well.width())), int(std::lround(well.height())));
  p.drawImage(well.topLeft() - QPointF(36 + fx.bloomPad, 36 + fx.bloomPad), fx.bloom);
  p.drawImage(well.topLeft() - QPointF(fx.innerPad, fx.innerPad), fx.inner);
}

void drawScreenOverlay(QPainter& p, const QRectF& well, QColor scan, bool glass) {
  QPainterPath path;
  path.addRoundedRect(well, kWellRadius, kWellRadius);
  p.save();
  p.setClipPath(path);
  for (qreal y = well.top(); y < well.bottom(); y += 3) {
    p.fillRect(QRectF(well.left(), y, well.width(), 1), scan);
  }
  if (glass) {
    QLinearGradient wash(well.topLeft(),
                         QPointF(well.left(), well.top() + well.height() * 0.38));
    wash.setColorAt(0, QColor(255, 255, 255, 13));
    wash.setColorAt(1, QColor(255, 255, 255, 0));
    p.fillRect(well, wash);
  }
  p.restore();
}

void drawScreen(QPainter& p, const QRectF& well) {
  drawScreenWell(p, well);
  drawScreenOverlay(p, well);
}

void drawListWell(QPainter& p, const QRectF& well) {
  QPainterPath path;
  path.addRoundedRect(well, kWellRadius, kWellRadius);
  p.save();
  p.setClipPath(path);
  QRadialGradient wash(QPointF(well.left() + well.width() * 0.2,
                               well.top() - well.height() * 0.10),
                       well.width() * 1.05);
  wash.setColorAt(0, T().listWash0);
  wash.setColorAt(0.7, T().listWash1);
  wash.setColorAt(1, T().listWash2);
  p.fillRect(well, wash);
  constexpr qreal row = 37;
  for (qreal y = well.top(); y < well.bottom(); y += row * 2) {
    p.fillRect(QRectF(well.left(), y, well.width(), row), withAlpha(T().listSheen, 4));
    p.fillRect(QRectF(well.left(), y + row, well.width(), row), QColor(0, 0, 0, 31));
  }
  p.setClipping(false);
  p.setPen(QPen(withAlpha(T().phos, 20), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(well.adjusted(0.5, 0.5, -0.5, -0.5), kWellRadius - 0.5, kWellRadius - 0.5);
  p.restore();
}

void drawBtn(QPainter& p, const QRectF& r, BtnFace face, const QString& label) {
  const qreal on = std::clamp(face.on, qreal(0), qreal(1));
  const qreal hover = std::clamp(face.hover, qreal(0), qreal(1));
  const qreal press = std::clamp(face.press, qreal(0), qreal(1));
  QPainterPath path;
  path.addRoundedRect(r, 4, 4);
  if (on > 0.004) {
    paintBlurred(p, r.adjusted(-18, -18, 18, 18), 4, [&](QPainter& bp) {
      bp.setPen(Qt::NoPen);
      bp.setBrush(withAlpha(T().phos, int(std::lround(77 * on))));
      bp.drawRoundedRect(r.adjusted(-3, -3, 3, 3), 7, 7);
    });
  }
  // The lit and idle faces are different gradients, not one gradient with a
  // brighter stop, so a transition has to mix them stop by stop. Hover lifts the
  // whole face a little; press sinks it, which is the only cue a flat button has.
  // The shell is dark enough that a subtle lift is simply invisible; hover has
  // to be worth about a third of the face's brightness to register at all.
  const qreal lift = (1 + 0.34 * hover) * (1 - 0.30 * press);
  QLinearGradient faceFill(r.topLeft(), r.bottomLeft());
  // The two faces do not even put their middle stop in the same place, so the
  // position travels with the colour.
  faceFill.setColorAt(0, scaled(mix(T().btnIdle0, T().btnOn0, on), lift));
  faceFill.setColorAt(0.48 + (0.45 - 0.48) * on,
                      scaled(mix(T().btnIdle48, T().btnOn1, on), lift));
  faceFill.setColorAt(1, scaled(mix(T().btnIdle100, T().btnOn2, on), lift));
  p.fillPath(path, faceFill);
  p.save();
  p.setClipPath(path);
  if (on > 0.004) {
    p.setPen(QPen(withAlpha(T().btnOnLip, int(std::lround(179 * on))), 1));
    p.drawLine(QPointF(r.left() + 2, r.top() + 1), QPointF(r.right() - 2, r.top() + 1));
    p.fillRect(QRectF(r.left() + 1, r.bottom() - 4, r.width() - 2, 3),
               withAlpha(T().btnOnFoot, int(std::lround(140 * on))));
  }
  if (on < 0.996) {
    const qreal idle = 1 - on;
    QLinearGradient rim(r.topLeft(), r.bottomLeft());
    rim.setColorAt(0, withAlpha(T().hoverLift, int(std::lround((51 + 90 * hover) * idle))));
    rim.setColorAt(0.5, Qt::transparent);
    rim.setColorAt(1, QColor(0, 0, 0, int(std::lround(128 * idle))));
    p.setPen(QPen(QBrush(rim), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  }
  QLinearGradient gloss(r.topLeft(), QPointF(r.left(), r.top() + r.height() * 0.55));
  const qreal glossTop = (31 + 40 * on + 55 * hover) * (1 - 0.7 * press);
  gloss.setColorAt(0, withAlpha(T().hoverLift, int(std::lround(glossTop))));
  gloss.setColorAt(1, withAlpha(T().hoverLift, 0));
  p.fillRect(QRectF(r.left() + 1, r.top() + 1, r.width() - 2, r.height() * 0.5), gloss);
  if (press > 0.004) {
    // Pressed buttons also take a shadow from the top edge, so the face reads as
    // sunk into the chassis rather than merely darker.
    QLinearGradient sink(r.topLeft(), QPointF(r.left(), r.top() + r.height() * 0.6));
    sink.setColorAt(0, QColor(0, 0, 0, int(std::lround(92 * press))));
    sink.setColorAt(1, QColor(0, 0, 0, 0));
    p.fillRect(r, sink);
  }
  p.restore();
  paintBlurred(p, r.adjusted(-4, -2, 4, 6), 1.2, [&](QPainter& bp) {
    bp.setPen(QPen(QColor(0, 0, 0, 153), 0.5));
    bp.setBrush(Qt::NoBrush);
    bp.drawRoundedRect(r.translated(0, 1), 4, 4);
  });
  if (!label.isEmpty()) {
    p.save();
    p.setFont(condensedFont(13, 0.18));
    p.setPen(mix(T().btnLabelIdle, T().btnOnInk, on));
    p.drawText(r, Qt::AlignCenter, label.toUpper());
    p.restore();
  }
}

qreal labelBtnWidth(const QString& label, qreal padL, qreal padR) {
  return padL + textWidth(condensedFont(13, 0.18), label.toUpper()) + padR;
}

void drawIcon(QPainter& p, const QRectF& box, MockupIcon icon, const QColor& color) {
  switch (icon) {
    case MockupIcon::mute:
      drawMute(p, box, color);
      return;
    case MockupIcon::options:
      drawCog(p, box, color);
      return;
    case MockupIcon::skins:
      paintIconPath(p, box, 24, pathSkins(), color);
      return;
    case MockupIcon::trackInfo:
      paintIconPath(p, box, 24, pathTrackInfo(), color);
      return;
    case MockupIcon::previous:
      paintIconPath(p, box, 24, pathPrev(), color);
      return;
    case MockupIcon::play:
      paintIconPath(p, box, 24, pathPlay(), color);
      return;
    case MockupIcon::pause:
      paintIconPath(p, box, 24, pathPause(), color);
      return;
    case MockupIcon::stop:
      paintIconPath(p, box, 24, pathStop(), color);
      return;
    case MockupIcon::next:
      paintIconPath(p, box, 24, pathNext(), color);
      return;
    case MockupIcon::eject:
      paintIconPath(p, box, 24, pathEject(), color);
      return;
    case MockupIcon::add:
      paintIconPath(p, box, 24, pathAdd(), color);
      return;
    case MockupIcon::remove:
      paintIconPath(p, box, 24, pathRemove(), color);
      return;
    case MockupIcon::save:
      paintIconPath(p, box, 24, pathSave(), color);
      return;
    case MockupIcon::sort:
      paintIconPath(p, box, 24, pathSort(), color);
      return;
    case MockupIcon::minimize:
      paintIconPath(p, box, 16, pathMinimize(), color);
      return;
    case MockupIcon::zoomOut:
      paintIconPath(p, box, 16, pathZoomOut(), color);
      return;
    case MockupIcon::zoomIn:
      paintIconPath(p, box, 16, pathZoomIn(), color);
      return;
    case MockupIcon::close:
      paintIconPath(p, box, 16, pathClose(), color);
      return;
  }
}

void drawGlyphBtn(QPainter& p, const QRectF& r, MockupIcon icon, BtnFace face, qreal iconSize,
                  bool enabled) {
  if (!enabled) face = BtnFace{};
  drawBtn(p, r, face, {});
  const QRectF box(r.center().x() - iconSize / 2, r.center().y() - iconSize / 2, iconSize,
                   iconSize);
  QColor ink = mix(T().glyphInk, T().btnOnInk, face.on);
  if (!enabled) ink = withAlpha(T().glyphInk, 77);
  drawIcon(p, box, icon, ink);
}

void drawSlider(QPainter& p, const QRectF& track, qreal t, bool seekStyle, bool glow) {
  t = qBound(0.0, t, 1.0);
  p.save();
  QPainterPath trough;
  trough.addRoundedRect(track, track.height() / 2, track.height() / 2);
  QLinearGradient tg(track.topLeft(), track.bottomLeft());
  tg.setColorAt(0, T().well);
  tg.setColorAt(0.6, T().shellLo);
  tg.setColorAt(1, T().metalLo);
  p.fillPath(trough, tg);
  p.setPen(QPen(withAlpha(T().coolSheen, 20), 1));
  p.setBrush(Qt::NoBrush);
  p.drawPath(trough);

  const qreal fillW = qMax(0.0, (track.width() - 4) * t);
  if (fillW > 0) {
    QRectF fill(track.left() + 2, track.top() + 2, fillW, track.height() - 4);
    QPainterPath fillPath;
    if (seekStyle) {
      const qreal rl = fill.height() / 2;
      const qreal rr = 3;
      fillPath.moveTo(fill.left() + rl, fill.top());
      fillPath.lineTo(fill.right() - rr, fill.top());
      fillPath.quadTo(fill.right(), fill.top(), fill.right(), fill.top() + rr);
      fillPath.lineTo(fill.right(), fill.bottom() - rr);
      fillPath.quadTo(fill.right(), fill.bottom(), fill.right() - rr, fill.bottom());
      fillPath.lineTo(fill.left() + rl, fill.bottom());
      fillPath.quadTo(fill.left(), fill.bottom(), fill.left(), fill.bottom() - rl);
      fillPath.lineTo(fill.left(), fill.top() + rl);
      fillPath.quadTo(fill.left(), fill.top(), fill.left() + rl, fill.top());
      fillPath.closeSubpath();
    } else {
      fillPath.addRoundedRect(fill, fill.height() / 2, fill.height() / 2);
    }
    if (glow) {
      paintBlurred(p, fill.adjusted(-8, -8, 8, 8), 4, [&](QPainter& bp) {
        bp.setPen(Qt::NoPen);
        bp.setBrush(withAlpha(T().phos, 102));
        bp.drawPath(fillPath);
      });
    }
    QLinearGradient g(fill.topLeft(), fill.bottomLeft());
    g.setColorAt(0, T().sliderFillHi);
    g.setColorAt(0.4, T().phos);
    g.setColorAt(1, T().sliderFillLo);
    p.fillPath(fillPath, g);
    p.setPen(QPen(QColor(T().btnOnLip.red(), T().btnOnLip.green(), T().btnOnLip.blue(), 153), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(fillPath);
  }

  const QSizeF thumb = seekStyle ? QSizeF(kSeekThumbW, kSeekThumbH)
                                 : QSizeF(kVolumeThumbW, kVolumeThumbH);
  const qreal x = qBound(track.left() + thumb.width() / 2,
                         track.left() + track.width() * t,
                         track.right() - thumb.width() / 2);
  const QRectF thumbR(x - thumb.width() / 2, track.center().y() - thumb.height() / 2,
                      thumb.width(), thumb.height());
  if (glow) {
    paintBlurred(p, thumbR.adjusted(-4, -2, 4, 6), 2, [&](QPainter& bp) {
      bp.setPen(Qt::NoPen);
      bp.setBrush(QColor(0, 0, 0, 166));
      bp.drawRoundedRect(thumbR.translated(0, 1), 4, 4);
    });
  }
  QLinearGradient face(thumbR.topLeft(), thumbR.bottomLeft());
  face.setColorAt(0, T().metalHi);
  face.setColorAt(0.55, T().metalMid);
  face.setColorAt(1, T().metalLo);
  fillRound(p, thumbR, 4, face);
  QLinearGradient rim(thumbR.topLeft(), thumbR.bottomRight());
  rim.setColorAt(0, withAlpha(T().btnLabelIdle, 140));
  rim.setColorAt(1, T().idleLedLo);
  p.setPen(QPen(QBrush(rim), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(thumbR.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
  const QRectF grip(thumbR.left() + 5, thumbR.top() + 8, thumbR.width() - 10,
                    thumbR.height() - 16);
  p.setPen(QPen(withAlpha(T().hoverLift, 56), 1));
  for (qreal y = grip.top(); y < grip.bottom(); y += 2.2) {
    p.drawLine(QPointF(grip.left(), y), QPointF(grip.right(), y));
  }
  p.restore();
}

namespace {

/// The value point a gain sits at down a band well: full scale at the top.
qreal bandValueY(const QRectF& well, qreal gainDb) {
  const qreal frac = qBound(0.0, (gainDb + 12.0) / 24.0, 1.0);
  return well.top() + (1.0 - frac) * well.height();
}

}  // namespace

QRectF bandThumbRect(const QRectF& well, qreal gainDb) {
  return QRectF(well.center().x() - kEqBandThumbW / 2,
                bandValueY(well, gainDb) - kEqBandThumbH / 2, kEqBandThumbW, kEqBandThumbH);
}

void drawVBand(QPainter& p, const QRectF& column, qreal gainDb) {
  p.save();
  constexpr qreal trackW = 12;
  const QRectF track(column.center().x() - trackW / 2, column.top(), trackW,
                     column.height());
  QPainterPath trough;
  trough.addRoundedRect(track, 6, 6);
  QLinearGradient tg(track.topLeft(), track.topRight());
  tg.setColorAt(0, T().well);
  tg.setColorAt(0.55, T().shellLo);
  tg.setColorAt(1, T().metalLo);
  p.fillPath(trough, tg);

  const qreal thumbY = bandValueY(track, gainDb);
  if (track.bottom() - thumbY > 0.5) {
    p.save();
    p.setClipPath(trough);
    p.setClipRect(QRectF(track.left(), thumbY, trackW, track.bottom() - thumbY),
                  Qt::IntersectClip);
    p.fillRect(track, T().spectrumGradient(track.bottomLeft(), track.topLeft()));
    p.restore();
  }

  p.fillRect(QRectF(track.left() - 13, track.center().y() - 0.5, trackW + 26, 1),
             withAlpha(T().coolSheen, 36));

  const QRectF thumb = bandThumbRect(column, gainDb);
  QLinearGradient face(thumb.topLeft(), thumb.bottomLeft());
  face.setColorAt(0, T().eqThumbHi);
  face.setColorAt(0.42, T().idleLedHi);
  face.setColorAt(1, T().metalLo);
  fillRound(p, thumb, 3, face);
  p.setPen(QPen(withAlpha(T().hoverLift, 89), 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(thumb.adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
  const QRectF line(thumb.center().x() - 11, thumb.center().y() - 1, 22, 2);
  QLinearGradient lg(line.topLeft(), line.topRight());
  lg.setColorAt(0, T().spectrumStops.value(0, T().phosHot));
  lg.setColorAt(1, T().phos);
  fillRound(p, line, 1, lg);
  p.restore();
}

void drawLed(QPainter& p, QPointF c, qreal on, qreal size) {
  on = std::clamp(on, qreal(0), qreal(1));
  const qreal r = size / 2;
  if (on > 0.004) {
    paintBlurred(p, QRectF(c.x() - r - 12, c.y() - r - 12, (r + 12) * 2, (r + 12) * 2), 6,
                 [&](QPainter& bp) {
                   bp.setPen(Qt::NoPen);
                   bp.setBrush(withAlpha(T().accent, int(std::lround(89 * on))));
                   bp.drawEllipse(c, r + 5, r + 5);
                 });
    paintBlurred(p, QRectF(c.x() - r - 8, c.y() - r - 8, (r + 8) * 2, (r + 8) * 2), 3,
                 [&](QPainter& bp) {
                   bp.setPen(Qt::NoPen);
                   bp.setBrush(withAlpha(T().accent, int(std::lround(179 * on))));
                   bp.drawEllipse(c, r + 1.5, r + 1.5);
                 });
  }
  // The dark LED is a plain two-stop gradient and the lit one has a hot centre,
  // so the mid stop has to start where the dark gradient already was or an
  // unlit LED gains a highlight it never had.
  QRadialGradient g(c + QPointF(-r * 0.2, -r * 0.3), r);
  g.setColorAt(0, mix(T().idleLedHi, T().accentHot, on));
  g.setColorAt(0.45, mix(mix(T().idleLedHi, T().idleLedLo, 0.45), T().accent, on));
  g.setColorAt(1, mix(T().idleLedLo, T().accentDim, on));
  p.save();
  p.setBrush(g);
  p.setPen(QPen(mix(QColor(0, 0, 0, 204), withAlpha(T().litLedRim, 153), on), 1));
  p.drawEllipse(c, r, r);
  p.restore();
}

void drawMenuCaret(QPainter& p, const QRectF& btn) {
  const QRectF c(btn.right() - 11, btn.bottom() - 11, 6, 6);
  QPainterPath path;
  path.moveTo(c.left(), c.bottom());
  path.lineTo(c.right(), c.bottom());
  path.lineTo(c.right(), c.top());
  path.closeSubpath();
  p.fillPath(path, withAlpha(T().glyphInk, 115));
}

void drawReload(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  const qreal s = box.width();
  const QPointF c = box.center();
  const qreal radius = s * 0.34;
  const qreal width = qMax(1.2, s * 0.14);
  QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  auto arrow = [&](qreal start, qreal sweep, qreal tipAngle) {
    // Flutter Canvas.drawArc is clockwise; Qt is counter-clockwise.
    const int start16 = int(-start * 180.0 / M_PI * 16);
    const int sweep16 = int(-sweep * 180.0 / M_PI * 16);
    p.drawArc(QRectF(c.x() - radius, c.y() - radius, radius * 2, radius * 2), start16,
              sweep16);
    const QPointF tip(c.x() + radius * std::cos(tipAngle),
                      c.y() + radius * std::sin(tipAngle));
    const qreal tangent = tipAngle + M_PI / 2;
    const qreal head = s * 0.22;
    const qreal wing = s * 0.16;
    QPainterPath path;
    path.moveTo(tip.x() + head * std::cos(tangent), tip.y() + head * std::sin(tangent));
    path.lineTo(tip);
    path.lineTo(tip.x() - wing * std::cos(tipAngle), tip.y() - wing * std::sin(tipAngle));
    p.drawPath(path);
  };
  arrow(-M_PI * 0.85, M_PI * 0.95, M_PI * 0.12);
  arrow(M_PI * 0.15, M_PI * 0.95, M_PI * 1.12);
  p.restore();
}

void drawNoiseOverlay(QPainter& p, const QRectF& rect, qreal radius) {
  const QRectF inner = rect.adjusted(1, 1, -1, -1);
  p.save();
  QPainterPath clip;
  clip.addRoundedRect(inner, qMax(0.0, radius - 1), qMax(0.0, radius - 1));
  p.setClipPath(clip);
  p.setOpacity(0.05);
  p.setCompositionMode(QPainter::CompositionMode_Overlay);
  p.drawTiledPixmap(inner, QPixmap::fromImage(noiseTile()));
  p.restore();
}

void drawStyledText(QPainter& p, const QRectF& box, const QString& text,
                    const QFont& font, const QColor& fill, int flags,
                    const QVector<TextShadow>& shadows) {
  p.save();
  for (const TextShadow& shadow : shadows) {
    const QRectF dest = box.translated(shadow.offset);
    if (shadow.blurRadius > 0.2) {
      const qreal sigma = shadow.blurRadius * 0.57735;
      const int pad = qMax(2, int(std::ceil(sigma * 3)) + 1);
      QImage buf(int(std::ceil(box.width())) + pad * 2,
                 int(std::ceil(box.height())) + pad * 2,
                 QImage::Format_ARGB32_Premultiplied);
      buf.fill(Qt::transparent);
      QPainter bp(&buf);
      bp.setRenderHint(QPainter::TextAntialiasing);
      bp.setFont(font);
      bp.setPen(shadow.color);
      bp.drawText(QRectF(pad, pad, box.width(), box.height()), flags, text);
      bp.end();
      p.drawImage(dest.topLeft() - QPointF(pad, pad), gaussianBlur(buf, sigma));
    } else {
      p.setFont(font);
      p.setPen(shadow.color);
      p.drawText(dest, flags, text);
    }
  }
  p.setFont(font);
  p.setPen(fill);
  p.drawText(box, flags, text);
  p.restore();
}

void drawGlowText(QPainter& p, const QRectF& box, const QString& text, const QFont& font,
                  const QColor& fill, const QColor& glow, qreal blurRadius, int flags) {
  drawStyledText(p, box, text, font, fill, flags, {{glow, {}, blurRadius}});
}

qreal toggleBtnWidth(const QString& label) {
  return 15 + 8 + 9 + textWidth(condensedFont(13, 0.16), label.toUpper()) + 15;
}

void drawToggleBtn(QPainter& p, const QRectF& r, const QString& label, BtnFace face) {
  drawBtn(p, r, BtnFace(0, face.hover, face.press), {});
  drawLed(p, QPointF(r.left() + 15 + 4, r.center().y()), face.on);
  p.save();
  p.setFont(condensedFont(13, 0.16));
  p.setPen(T().btnLabelIdle);
  p.drawText(r.adjusted(15 + 8 + 9, 0, -15, 0), Qt::AlignVCenter | Qt::AlignLeft,
             label.toUpper());
  p.restore();
}

void drawChevron(QPainter& p, const QRectF& box, bool pointsLeft, const QColor& color) {
  p.save();
  QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  const qreal tipX = pointsLeft ? box.left() + 0.5 : box.right() - 0.5;
  const qreal backX = pointsLeft ? box.right() - 0.5 : box.left() + 0.5;
  QPainterPath path;
  path.moveTo(backX, box.top() + 1);
  path.lineTo(tipX, box.center().y());
  path.lineTo(backX, box.bottom() - 1);
  p.drawPath(path);
  p.restore();
}

void drawCreateMark(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  QPen pen(color, 1.4, Qt::SolidLine, Qt::RoundCap);
  p.setPen(pen);
  const qreal w = box.width();
  const qreal h = box.height();
  const qreal ruleWidth = w * 0.62;
  for (int i = 0; i < 3; ++i) {
    const qreal y = box.top() + h * (0.18 + i * 0.32);
    p.drawLine(QPointF(box.left() + 0.7, y), QPointF(box.left() + ruleWidth, y));
  }
  const qreal cx = box.left() + w - w * 0.16;
  const qreal cy = box.top() + h - h * 0.16;
  const qreal reach = w * 0.2;
  p.drawLine(QPointF(cx - reach, cy), QPointF(cx + reach, cy));
  p.drawLine(QPointF(cx, cy - reach), QPointF(cx, cy + reach));
  p.restore();
}

void drawRenameMark(QPainter& p, const QRectF& box, const QColor& color) {
  p.save();
  QPen pen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
  p.setPen(pen);
  const qreal w = box.width();
  const qreal h = box.height();
  const qreal l = box.left();
  const qreal t = box.top();
  p.drawLine(QPointF(l + w * 0.2, t + h * 0.68), QPointF(l + w * 0.84, t + h * 0.06));
  p.drawLine(QPointF(l + w * 0.2, t + h * 0.68), QPointF(l + w * 0.36, t + h * 0.8));
  p.drawLine(QPointF(l + w * 0.36, t + h * 0.8), QPointF(l + w * 0.86, t + h * 0.28));
  p.drawLine(QPointF(l + w * 0.06, t + h * 0.96), QPointF(l + w * 0.94, t + h * 0.96));
  p.restore();
}

void drawFooterSep(QPainter& p, const QRectF& r) {
  QLinearGradient g(r.topLeft(), r.bottomLeft());
  g.setColorAt(0, QColor(0, 0, 0, 179));
  g.setColorAt(0.5, withAlpha(T().coolSheen, 31));
  g.setColorAt(1, QColor(0, 0, 0, 179));
  p.fillRect(r, g);
}

void drawStatusDot(QPainter& p, QPointF c) {
  p.save();
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(T().inkFaint.red(), T().inkFaint.green(), T().inkFaint.blue(), 160));
  p.drawEllipse(c, 1.5, 1.5);
  p.restore();
}

void drawScrollbar(QPainter& p, const QRectF& track, qreal thumbTop, qreal thumbH) {
  p.save();
  QPainterPath trough;
  trough.addRoundedRect(track, track.width() / 2, track.width() / 2);
  QLinearGradient tg(track.topLeft(), track.topRight());
  tg.setColorAt(0, T().well);
  tg.setColorAt(0.6, T().shellLo);
  tg.setColorAt(1, T().metalLo);
  p.fillPath(trough, tg);
  QLinearGradient inset(track.topLeft(), track.topRight());
  inset.setColorAt(0, QColor(0, 0, 0, 242));
  inset.setColorAt(0.55, Qt::transparent);
  p.fillPath(trough, inset);
  p.setPen(QPen(withAlpha(T().coolSheen, 26), 1));
  p.drawLine(QPointF(track.right() - 0.5, track.top() + 1),
             QPointF(track.right() - 0.5, track.bottom() - 1));
  const QRectF thumb(track.left() + 1, track.top() + thumbTop, track.width() - 2, thumbH);
  QLinearGradient face(thumb.topLeft(), thumb.topRight());
  face.setColorAt(0, T().scrollThumbHi);
  face.setColorAt(0.52, T().scrollThumbMid);
  face.setColorAt(1, T().idleLedLo);
  fillRound(p, thumb, thumb.width() / 2, face);
  QLinearGradient gloss(thumb.topLeft(), thumb.bottomLeft());
  gloss.setColorAt(0, withAlpha(T().hoverLift, 128));
  gloss.setColorAt(0.35, withAlpha(T().hoverLift, 0));
  QPainterPath clip;
  clip.addRoundedRect(thumb, thumb.width() / 2, thumb.width() / 2);
  p.setClipPath(clip);
  p.fillRect(thumb, gloss);
  const QRectF ridge(thumb.left() + 3, thumb.center().y() - 4, thumb.width() - 6, 8);
  for (qreal y = ridge.top(); y < ridge.bottom(); y += 2) {
    p.fillRect(QRectF(ridge.left(), y, ridge.width(), 1), QColor(0, 0, 0, 128));
    p.fillRect(QRectF(ridge.left(), y + 1, ridge.width(), 1), withAlpha(T().coolSheen, 61));
  }
  p.restore();
}

void drawDiscLogo(QPainter& p, const QRectF& disc, const QImage* logo, bool insets) {
  p.save();
  // Title bar: BoxShadow blurRadius 12 alpha 0x47. About badge: size*0.42 @ 0.3.
  const qreal bloomRadius = disc.width() <= 32 ? 12.0 : disc.width() * 0.42;
  const qreal bloomSigma = bloomRadius * 0.57735;
  const int bloomA = disc.width() <= 32 ? 0x47 : int(0.3 * 255);
  paintBlurred(p, disc.adjusted(-bloomRadius, -bloomRadius, bloomRadius, bloomRadius),
               bloomSigma, [&](QPainter& bp) {
                 bp.setPen(Qt::NoPen);
                 bp.setBrush(QColor(T().accent.red(), T().accent.green(), T().accent.blue(), bloomA));
                 bp.drawEllipse(disc);
               });
  const qreal dropY = disc.width() <= 32 ? 2.0 : 3.0;
  const qreal dropBlur = disc.width() <= 32 ? 4.0 : 7.0;
  paintBlurred(p, disc.adjusted(-dropBlur, -2, dropBlur, dropY + dropBlur),
               dropBlur * 0.57735, [&](QPainter& bp) {
                 bp.setPen(Qt::NoPen);
                 bp.setBrush(QColor(0, 0, 0, disc.width() <= 32 ? 0x8C : 0x99));
                 bp.drawEllipse(disc.translated(0, dropY));
               });
  p.setPen(Qt::NoPen);
  p.setBrush(T().logoDisc);
  p.drawEllipse(disc);
  if (logo && !logo->isNull()) {
    p.save();
    QPainterPath clip;
    clip.addEllipse(disc);
    p.setClipPath(clip);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRectF dest = disc.adjusted(-disc.width() * 0.06, -disc.height() * 0.06,
                                      disc.width() * 0.06, disc.height() * 0.06);
    p.drawImage(dest, *logo);
    p.restore();
  }
  p.setBrush(Qt::NoBrush);
  p.setPen(QPen(QColor(0, 0, 0, 166), 1));
  p.drawEllipse(disc);
  if (insets) {
    QPainterPath clip;
    clip.addEllipse(disc);
    p.setClipPath(clip);
    QLinearGradient sheen(disc.topLeft(), QPointF(disc.left(), disc.top() + disc.height() * 0.55));
    sheen.setColorAt(0, QColor(255, 255, 255, 128));
    sheen.setColorAt(1, QColor(255, 255, 255, 0));
    p.fillRect(QRectF(disc.left(), disc.top(), disc.width(), disc.height() * 0.55), sheen);
    QLinearGradient shade(QPointF(disc.left(), disc.top() + disc.height() * 0.45),
                          disc.bottomLeft());
    shade.setColorAt(0, withAlpha(T().phosDeep, 0));
    shade.setColorAt(1, withAlpha(T().phosDeep, 89));
    p.fillRect(QRectF(disc.left(), disc.top() + disc.height() * 0.45, disc.width(),
                      disc.height() * 0.55),
               shade);
    QLinearGradient gloss(disc.topLeft() + QPointF(disc.width() * 0.2, 0),
                          disc.bottomRight() - QPointF(disc.width() * 0.1, disc.height() * 0.2));
    gloss.setColorAt(0, QColor(255, 255, 255, 115));
    gloss.setColorAt(0.46, QColor(255, 255, 255, 0));
    p.fillRect(disc, gloss);
  }
  p.restore();
}

QImage loadTrampLogo() {
  QImage img(assetPath("branding/logo.png"));
  if (img.isNull()) {
    img.load(assetPath("branding/app_icon.png"));
  }
  return img;
}

QImage loadProximaMark() {
  // Static asset — decoding it per paint showed up in the about panel's cost.
  static const QImage mark(assetPath("branding/proxima_mark.png"));
  return mark;
}

BlurCost blurCost() { return g_blurCost; }

void resetBlurCost() { g_blurCost = {}; }

}  // namespace tramp
