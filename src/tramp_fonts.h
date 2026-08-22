#pragma once

#include <QFont>
#include <QString>

namespace tramp {

void loadTrampFonts();
void setLookFamilies(const QString& chrome, const QString& lcd);
QString lookChromeOverride();
QString lookLcdOverride();
QString bundledDataDir(const char* leaf);
QString bundledSkinsDir();
QString assetPath(const char* relative);
QString chromeFamily();
QString lcdFamily();

/// Largest pixel size ≤ requestedPx whose AlignTop line fits in maxLineHeight.
/// LCD time is drawn AlignTop in a fixed slot; fonts with a taller line
/// (ascent + clock-glyph descent) would otherwise clip at the bottom.
int pixelSizeFittingLineHeight(QFont font, int requestedPx, qreal maxLineHeight);

}  // namespace tramp
