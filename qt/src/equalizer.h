#pragma once

#include <QPair>
#include <QString>
#include <QVector>
#include <array>

namespace tramp {

struct EqualizerSettings {
  static constexpr int kBandCount = 10;
  static constexpr double kGainLimit = 12;
  static constexpr std::array<int, kBandCount> kBandFrequencies = {
      60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000};

  bool enabled = false;
  bool auto_ = false;
  double preamp = 0;
  std::array<double, kBandCount> gains = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  QString presetName;

  static EqualizerSettings flat() { return {}; }

  static double clampGain(double value);

  EqualizerSettings withGain(int band, double gain) const;
  EqualizerSettings withPreamp(double value) const;
  EqualizerSettings withPreset(const QString& name, const QVector<double>& values) const;
};

struct EqualizerPresets {
  static const QVector<QPair<QString, QVector<double>>>& builtIn();
};

QString buildEqualizerAf(const EqualizerSettings& settings);

}  // namespace tramp
