#pragma once

#include "track.h"

#include <QVector>
#include <optional>

namespace tramp {

std::optional<int> nextIndex(int current, int length, bool shuffle, RepeatMode repeat,
                             const QVector<int>& shuffledOrder);
std::optional<int> previousIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                 const QVector<int>& shuffledOrder);
QVector<int> shuffledOrder(int length, int seed);

}  // namespace tramp
