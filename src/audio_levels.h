#pragma once

#include <array>

namespace tramp {

struct AudioLevels {
  static constexpr int kBandCount = 20;

  std::array<double, kBandCount> bands{};
  double leftRms = 0;
  double rightRms = 0;
  bool synthetic = false;

  static AudioLevels silent() { return {}; }
};

}  // namespace tramp
