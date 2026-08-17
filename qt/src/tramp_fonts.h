#pragma once

#include <QString>

namespace tramp {

void loadTrampFonts();
void setLookFamilies(const QString& chrome, const QString& lcd);
QString assetPath(const char* relative);
QString chromeFamily();
QString lcdFamily();

}  // namespace tramp
