#include "spectrum.h"

#include "stft.h"

#include <algorithm>
#include <cmath>

namespace tramp {
namespace {

/// The analysis window. `stft.h` takes both as arguments; the choice is ours, and
/// the slicing below has to know it to cut on a frame boundary.
constexpr int kFftSize = 4096;
constexpr int kFramesPerSecond = 30;

/// The same frames one pass over the whole buffer gives, produced a couple of
/// seconds of audio at a time so [stillWanted] gets a say. Every frame is
/// windowed and normalised on its own, so a slice that starts on a hop boundary
/// and carries the window's tail cannot tell it was cut.
///
/// Worth the arithmetic because a five-minute track is the better part of a
/// second of band folding, and a session being torn down waits for it.
QVector<std::array<double, AudioLevels::kBandCount>> foldInSlices(
    const QVector<double>& samples, int sampleRateHz,
    const SpectrumAnalyzer::CancelFn& stillWanted) {
  const StftBandFolder stft(kFftSize, kFramesPerSecond);
  const int hop = std::max(1, sampleRateHz / kFramesPerSecond);
  if (sampleRateHz <= 0 || samples.size() < kFftSize) {
    return stft.analyze(samples, sampleRateHz);
  }
  const int frameCount = (int(samples.size()) - kFftSize) / hop + 1;
  const int perSlice = kFramesPerSecond * 2;
  QVector<std::array<double, AudioLevels::kBandCount>> frames;
  frames.reserve(frameCount);
  for (int first = 0; first < frameCount; first += perSlice) {
    // No frames means cancelled, which is not the same as silent and not a
    // failure either: the caller asked us to stop and will drop the result
    // unread. A decode that fails says so with [Spectrogram::unmeasured]
    // instead, because a broken analyser must not read as a quiet passage.
    if (!stillWanted()) return {};
    const int last = std::min(first + perSlice, frameCount);
    frames += stft.analyze(samples.mid(first * hop, (last - 1 - first) * hop + kFftSize),
                           sampleRateHz);
  }
  return frames;
}

}  // namespace

Spectrogram Spectrogram::silent() {
  Spectrogram spec;
  spec.frames = {{}};
  spec.framesPerSecond = 30;
  spec.sampleRateHz = 44100;
  return spec;
}

Spectrogram Spectrogram::unmeasured() {
  // One flat frame, the same shape silence has, which is the whole reason it has
  // to carry the mark.
  Spectrogram spec = silent();
  spec.synthetic = true;
  return spec;
}

Spectrogram Spectrogram::cancelled() { return {}; }

AudioLevels Spectrogram::levelsAt(qint64 positionMs) const {
  AudioLevels levels;
  levels.synthetic = synthetic;
  if (frames.isEmpty()) return levels;
  const double seconds = double(positionMs) / 1000.0;
  int idx = int(std::floor(seconds * double(framesPerSecond)));
  idx = std::clamp(idx, 0, int(frames.size() - 1));
  levels.bands = frames[idx];
  double rms = 0;
  for (double b : levels.bands) {
    rms += b * b;
  }
  rms /= double(levels.bands.size());
  const double amp = rms > 0 ? std::clamp(rms, 0.0, 1.0) : 0.0;
  levels.leftRms = amp;
  levels.rightRms = amp;
  return levels;
}

AudioLevels spectrumFrame(const Spectrogram& spec, bool playing, qint64 positionMs) {
  if (!playing) return AudioLevels::silent();
  return spec.levelsAt(positionMs);
}

SpectrumAnalyzer::SpectrumAnalyzer(PcmLoader loader) {
  if (!loader) return;
  loader_ = [loader = std::move(loader)](const QString& path, const CancelFn&) {
    return loader(path);
  };
}

SpectrumAnalyzer::SpectrumAnalyzer(CancellablePcmLoader loader) : loader_(std::move(loader)) {}

Spectrogram SpectrumAnalyzer::analyzeMonoPcm(const QVector<double>& samples,
                                             int sampleRateHz) const {
  StftBandFolder stft(kFftSize, kFramesPerSecond);
  Spectrogram spec;
  spec.frames = stft.analyze(samples, sampleRateHz);
  spec.framesPerSecond = stft.framesPerSecond();
  spec.sampleRateHz = sampleRateHz;
  return spec;
}

Spectrogram SpectrumAnalyzer::load(const QString& path) const { return load(path, {}); }

Spectrogram SpectrumAnalyzer::load(const QString& path, const CancelFn& stillWanted) const {
  const auto wanted = [&stillWanted]() { return !stillWanted || stillWanted(); };
  if (!wanted()) return Spectrogram::cancelled();
  // Nothing to ask, so nothing to measure. A build without a decoder is the
  // development case synthetic levels were named for.
  if (!loader_) return Spectrogram::unmeasured();
  try {
    const PcmBuffer pcm = loader_(path, stillWanted);
    if (!wanted()) return Spectrogram::cancelled();
    if (!stillWanted) return analyzeMonoPcm(pcm.samples, pcm.sampleRateHz);
    Spectrogram spec;
    spec.framesPerSecond = kFramesPerSecond;
    spec.sampleRateHz = pcm.sampleRateHz;
    spec.frames = foldInSlices(pcm.samples, pcm.sampleRateHz, stillWanted);
    if (spec.frames.isEmpty()) return Spectrogram::cancelled();
    return spec;
  } catch (...) {
    // The decoder throws for a deadline it never met and for a caller that has
    // moved on, and only the first failed to measure anything. Ask again rather
    // than read the exception: the loader is anyone's, and "does anyone still
    // want this?" is the question that separates the two.
    if (!wanted()) return Spectrogram::cancelled();
    return Spectrogram::unmeasured();
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

void SpectrumHold::release() {
  for (int i = 0; i < AudioLevels::kBandCount; ++i) {
    bars[size_t(i)] *= kRestDecay;
    peaks[size_t(i)] *= kRestDecay;
  }
}

bool SpectrumHold::atRest() const {
  for (int i = 0; i < AudioLevels::kBandCount; ++i) {
    if (bars[size_t(i)] > kRestFloor || peaks[size_t(i)] > kRestFloor) return false;
  }
  return true;
}

void SpectrumHold::reset() {
  bars.fill(0);
  peaks.fill(0);
}

}  // namespace tramp
