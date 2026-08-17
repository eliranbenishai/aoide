#include "spectrum.h"

#include "stft.h"

#include <algorithm>
#include <cmath>

namespace tramp {

Spectrogram Spectrogram::silent() {
  Spectrogram spec;
  spec.frames = {{}};
  spec.framesPerSecond = 30;
  spec.sampleRateHz = 44100;
  return spec;
}

AudioLevels Spectrogram::levelsAt(qint64 positionMs) const {
  if (frames.isEmpty()) return AudioLevels::silent();
  const double seconds = double(positionMs) / 1000.0;
  int idx = int(std::floor(seconds * double(framesPerSecond)));
  idx = std::clamp(idx, 0, int(frames.size() - 1));
  AudioLevels levels;
  levels.bands = frames[idx];
  double rms = 0;
  for (double b : levels.bands) {
    rms += b * b;
  }
  rms /= double(levels.bands.size());
  const double amp = rms > 0 ? std::clamp(rms, 0.0, 1.0) : 0.0;
  levels.leftRms = amp;
  levels.rightRms = amp;
  levels.synthetic = false;
  return levels;
}

AudioLevels spectrumFrame(const Spectrogram& spec, bool playing, qint64 positionMs) {
  if (!playing) return AudioLevels::silent();
  return spec.levelsAt(positionMs);
}

SpectrumAnalyzer::SpectrumAnalyzer(PcmLoader loader) : loader_(std::move(loader)) {}

Spectrogram SpectrumAnalyzer::analyzeMonoPcm(const QVector<double>& samples,
                                             int sampleRateHz) const {
  StftBandFolder stft;
  Spectrogram spec;
  spec.frames = stft.analyze(samples, sampleRateHz);
  spec.framesPerSecond = stft.framesPerSecond();
  spec.sampleRateHz = sampleRateHz;
  return spec;
}

Spectrogram SpectrumAnalyzer::load(const QString& path) const {
  if (!loader_) return Spectrogram::silent();
  try {
    const PcmBuffer pcm = loader_(path);
    return analyzeMonoPcm(pcm.samples, pcm.sampleRateHz);
  } catch (...) {
    return Spectrogram::silent();
  }
}

double SpectrumAnalyzer::bandCenterHz(int index, int sampleRateHz) {
  return StftBandFolder::bandCenterHz(index, sampleRateHz);
}

void SpectrumHold::apply(const AudioLevels& frame) {
  for (int i = 0; i < AudioLevels::kBandCount; ++i) {
    const double incoming = std::clamp(frame.bands[size_t(i)], 0.0, 1.0);
    const qreal current = bars[size_t(i)];
    bars[size_t(i)] = incoming > current ? incoming : current * kDecay;
    peaks[size_t(i)] = std::max(double(bars[size_t(i)]), double(peaks[size_t(i)]) * kPeakDecay);
  }
}

void SpectrumHold::reset() {
  bars.fill(0);
  peaks.fill(0);
}

}  // namespace tramp
