#pragma once

#include <QString>

namespace tramp {

void loadTrampFonts();
QString assetPath(const char* relative);
QString chromeFamily();
QString lcdFamily();

}  // namespace tramp
