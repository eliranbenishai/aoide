#pragma once

#include "track.h"

#include <QStringList>
#include <QVector>

namespace aoide {

bool isPlaylistPath(const QString& path);
bool isAudioPath(const QString& path);
QVector<Track> tracksFromPaths(const QStringList& paths);
QStringList audioExtensions();
QStringList playlistExtensions();

}  // namespace aoide
