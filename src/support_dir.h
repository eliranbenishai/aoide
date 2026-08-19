#pragma once

#include <QMap>
#include <QString>
#include <functional>

namespace tramp {

inline constexpr auto kApplicationId = "com.proximamagnifica.tramp";
inline constexpr const char* kLegacyLinuxSupportDirNames[] = {
    "tramp",
};

QString resolveLinuxSupportPath(const QMap<QString, QString>& environment,
                                const std::function<bool(const QString&)>& exists);

QString trampSupportDirectory();

}  // namespace tramp
