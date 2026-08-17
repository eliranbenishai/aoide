#pragma once

#include <QString>

namespace tramp {

void loadTrampFonts();
void setLookFamilies(const QString& chrome, const QString& lcd);
QString bundledDataDir(const char* leaf);
QString bundledSkinsDir();
QString assetPath(const char* relative);
QString chromeFamily();
QString lcdFamily();

}  // namespace tramp
