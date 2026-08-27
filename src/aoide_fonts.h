#pragma once

#include <QFont>
#include <QString>

namespace aoide {

void loadAoideFonts();
void setLookFamilies(const QString& chrome, const QString& lcd);
QString lookChromeOverride();
QString lookLcdOverride();
QString bundledDataDir(const char* leaf);
QString bundledSkinsDir();
QString assetPath(const char* relative);
QString chromeFamily();
QString lcdFamily();
QString brandFamily();

/// Largest pixel size ≤ requestedPx whose AlignTop line fits in maxLineHeight.
/// LCD time is drawn AlignTop in a fixed slot; fonts with a taller line
/// (ascent + clock-glyph descent) would otherwise clip at the bottom.
int pixelSizeFittingLineHeight(QFont font, int requestedPx, qreal maxLineHeight);

/// Narrow a chrome face until [text] lays out inside [maxWidth]. Tracking
/// (absolute letter-spacing) comes off first so the designed size stays; pixel
/// size only drops if even unspaced glyphs still overflow. Empty-well headings
/// sit in a box that some skin faces overshoot at the collection min width.
void fitFontToWidth(QFont& font, const QString& text, qreal maxWidth);

}  // namespace aoide
