#pragma once

#include "track.h"

#include <QVector>
#include <functional>
#include <optional>

namespace tramp {

std::optional<int> nextIndex(int current, int length, bool shuffle, RepeatMode repeat,
                             const QVector<int>& shuffledOrder);
std::optional<int> previousIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                 const QVector<int>& shuffledOrder);
std::optional<int> nextPlayableIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                     const QVector<int>& shuffledOrder,
                                     const std::function<bool(int)>& playable);
std::optional<int> previousPlayableIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                         const QVector<int>& shuffledOrder,
                                         const std::function<bool(int)>& playable);
/// A fresh permutation of `0..length-1`. Drawn from the global generator, not
/// a seed the caller can reconstruct: shuffle is meant to be unpredictable.
QVector<int> shuffledOrder(int length);

}  // namespace tramp
