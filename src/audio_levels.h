#pragma once

#include <array>

namespace aoide {

struct AudioLevels {
  static constexpr int kBandCount = 20;

  std::array<double, kBandCount> bands{};
  double leftRms = 0;
  double rightRms = 0;
  /// These bands were not measured — a hard-fail signal, never the normal path.
  /// The bands of a failed measurement are flat, so this flag is the only thing
  /// separating a broken analyser from a quiet passage.
  bool synthetic = false;

  static AudioLevels silent() { return {}; }
};

}  // namespace aoide
