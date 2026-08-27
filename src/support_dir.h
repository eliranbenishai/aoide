#pragma once

#include <QMap>
#include <QString>
#include <functional>

namespace aoide {

inline constexpr auto kApplicationId = "com.proximamagnifica.aoide";
inline constexpr const char* kLegacyLinuxSupportDirNames[] = {
    "com.proximamagnifica.tramp",
    "tramp",
};
inline constexpr auto kLegacyNonLinuxSupportDirName = "Tramp";

QString resolveLinuxSupportPath(const QMap<QString, QString>& environment,
                                const std::function<bool(const QString&)>& exists);

QString resolveNonLinuxSupportPath(const QString& appDataLocation,
                                   const std::function<bool(const QString&)>& exists);

QString aoideSupportDirectory();

}  // namespace aoide
