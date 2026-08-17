#pragma once

#include "audio_levels.h"

#include <QVector>
#include <array>

namespace tramp {

class StftBandFolder {
 public:
  explicit StftBandFolder(int fftSize = 4096, int framesPerSecond = 30);

  QVector<std::array<double, AudioLevels::kBandCount>> analyze(const QVector<double>& samples,
                                                              int sampleRateHz) const;
  static double bandCenterHz(int index, int sampleRateHz);
  int framesPerSecond() const { return framesPerSecond_; }

 private:
  int fftSize_ = 4096;
  int framesPerSecond_ = 30;
};

}  // namespace tramp
