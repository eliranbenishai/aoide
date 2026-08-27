#pragma once

#include "look.h"

#include <QString>
#include <QVector>

namespace aoide {

/// Paint the golden-demo main player in [id]'s look and write a 1× PNG.
/// Does not apply the skin to the live session.
bool writeSkinPreviewPng(const QString& id, const QVector<LookManifest>& installed,
                         const QString& path, QString* error = nullptr);

}  // namespace aoide
