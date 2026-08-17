#pragma once

#include "audio_levels.h"
#include "wav_reader.h"

#include <QString>
#include <QtGlobal>
#include <QVector>
#include <array>
#include <functional>

namespace tramp {

struct Spectrogram {
  QVector<std::array<double, AudioLevels::kBandCount>> frames;
  int framesPerSecond = 30;
  int sampleRateHz = 44100;

  AudioLevels levelsAt(qint64 positionMs) const;
  static Spectrogram silent();
};

AudioLevels spectrumFrame(const Spectrogram& spec, bool playing, qint64 positionMs);

class SpectrumAnalyzer {
 public:
  using PcmLoader = std::function<PcmBuffer(const QString& path)>;

  explicit SpectrumAnalyzer(PcmLoader loader = {});

  Spectrogram analyzeMonoPcm(const QVector<double>& samples, int sampleRateHz) const;
  Spectrogram load(const QString& path) const;
  static double bandCenterHz(int index, int sampleRateHz);

 private:
  PcmLoader loader_;
};

struct SpectrumHold {
  static constexpr double kDecay = 0.86;
  static constexpr double kPeakDecay = 0.97;

  std::array<qreal, AudioLevels::kBandCount> bars{};
  std::array<qreal, AudioLevels::kBandCount> peaks{};

  void apply(const AudioLevels& frame);
  void reset();
};

}  // namespace tramp
