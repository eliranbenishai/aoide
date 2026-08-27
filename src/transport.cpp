#include "transport.h"

#include <QRandomGenerator>
#include <functional>

namespace aoide {

namespace {

bool indexPlayable(int i, int length, const std::function<bool(int)>& playable) {
  if (i < 0 || i >= length) return false;
  return !playable || playable(i);
}

}  // namespace

std::optional<int> nextIndex(int current, int length, bool shuffle, RepeatMode repeat,
                             const QVector<int>& shuffledOrder) {
  if (length == 0) {
    return std::nullopt;
  }
  if (shuffle && !shuffledOrder.isEmpty()) {
    const int position = shuffledOrder.indexOf(current);
    // Not in this pass: the open file outlived the list it came from. Start the
    // pass rather than handing back the track that is already playing.
    if (position == -1) {
      return shuffledOrder.first();
    }
    if (position < shuffledOrder.size() - 1) {
      return shuffledOrder[position + 1];
    }
    if (repeat == RepeatMode::all) {
      return shuffledOrder.first();
    }
    return std::nullopt;
  }
  if (current < length - 1) {
    return current + 1;
  }
  if (repeat == RepeatMode::all) {
    return 0;
  }
  return std::nullopt;
}

std::optional<int> previousIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                 const QVector<int>& shuffledOrder) {
  if (length == 0) {
    return std::nullopt;
  }
  if (shuffle && !shuffledOrder.isEmpty()) {
    const int position = shuffledOrder.indexOf(current);
    if (position == -1) {
      return shuffledOrder.last();
    }
    if (position > 0) {
      return shuffledOrder[position - 1];
    }
    if (repeat == RepeatMode::all) {
      return shuffledOrder.last();
    }
    return std::nullopt;
  }
  if (current > 0) {
    return current - 1;
  }
  if (repeat == RepeatMode::all) {
    return length - 1;
  }
  return std::nullopt;
}

std::optional<int> nextPlayableIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                     const QVector<int>& shuffledOrder,
                                     const std::function<bool(int)>& playable) {
  std::optional<int> idx = nextIndex(current, length, shuffle, repeat, shuffledOrder);
  for (int guard = 0; idx && guard <= length; ++guard) {
    if (indexPlayable(*idx, length, playable)) return idx;
    if (*idx == current) return std::nullopt;
    idx = nextIndex(*idx, length, shuffle, repeat, shuffledOrder);
  }
  return std::nullopt;
}

std::optional<int> previousPlayableIndex(int current, int length, bool shuffle, RepeatMode repeat,
                                         const QVector<int>& shuffledOrder,
                                         const std::function<bool(int)>& playable) {
  std::optional<int> idx = previousIndex(current, length, shuffle, repeat, shuffledOrder);
  for (int guard = 0; idx && guard <= length; ++guard) {
    if (indexPlayable(*idx, length, playable)) return idx;
    if (*idx == current) return std::nullopt;
    idx = previousIndex(*idx, length, shuffle, repeat, shuffledOrder);
  }
  return std::nullopt;
}

QVector<int> shuffledOrder(int length) {
  QVector<int> order(length);
  for (int i = 0; i < length; ++i) {
    order[i] = i;
  }
  QRandomGenerator* rng = QRandomGenerator::global();
  for (int i = length - 1; i > 0; --i) {
    const int j = int(rng->bounded(i + 1));
    std::swap(order[i], order[j]);
  }
  return order;
}

}  // namespace aoide
