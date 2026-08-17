#include "transport.h"

#include <QRandomGenerator>

namespace tramp {

std::optional<int> nextIndex(int current, int length, bool shuffle, RepeatMode repeat,
                             const QVector<int>& shuffledOrder) {
  if (length == 0) {
    return std::nullopt;
  }
  if (shuffle) {
    const int position = shuffledOrder.indexOf(current);
    if (position == -1) {
      return qBound(0, current, length - 1);
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
  if (shuffle) {
    const int position = shuffledOrder.indexOf(current);
    if (position == -1) {
      return qBound(0, current, length - 1);
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

QVector<int> shuffledOrder(int length, int seed) {
  QVector<int> order(length);
  for (int i = 0; i < length; ++i) {
    order[i] = i;
  }
  QRandomGenerator rng{quint32(seed)};
  for (int i = length - 1; i > 0; --i) {
    const int j = int(rng.bounded(i + 1));
    std::swap(order[i], order[j]);
  }
  return order;
}

}  // namespace tramp
