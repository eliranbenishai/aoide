#pragma once

#include "audio_levels.h"
#include "wav_reader.h"

#include <QString>
#include <QtGlobal>
#include <QVector>
#include <array>
#include <functional>

namespace tramp {

/// Three things this can be, and two of them have flat frames, so the frames are
/// not what tells them apart:
///   - a measurement, quiet or not — [silent] is the quiet one
///   - no measurement, with someone still waiting for it — [unmeasured]
///   - no measurement, and nobody waiting — [cancelled], which has no frames
struct Spectrogram {
  QVector<std::array<double, AudioLevels::kBandCount>> frames;
  int framesPerSecond = 30;
  int sampleRateHz = 44100;
  /// Nothing here was measured. Every frame handed out is marked synthetic, so
  /// a broken analyser cannot be read as a quiet passage.
  bool synthetic = false;

  AudioLevels levelsAt(qint64 positionMs) const;
  /// Measured, and quiet.
  static Spectrogram silent();
  /// Could not be measured: the decode failed, or it ran past its deadline.
  static Spectrogram unmeasured();
  /// Cancelled, so no frames — nobody is left to read them. Neither quiet nor
  /// broken, and marking it either would make a normal quit look like a fault.
  static Spectrogram cancelled();
};

AudioLevels spectrumFrame(const Spectrogram& spec, bool playing, qint64 positionMs);

class SpectrumAnalyzer {
 public:
  using PcmLoader = std::function<PcmBuffer(const QString& path)>;
  /// Answers "does anyone still want this?". False means drop the work.
  using CancelFn = std::function<bool()>;
  using CancellablePcmLoader =
      std::function<PcmBuffer(const QString& path, const CancelFn& stillWanted)>;

  explicit SpectrumAnalyzer(PcmLoader loader = {});
  explicit SpectrumAnalyzer(CancellablePcmLoader loader);

  Spectrogram analyzeMonoPcm(const QVector<double>& samples, int sampleRateHz) const;
  Spectrogram load(const QString& path) const;
  /// Decoding a track takes seconds, and a session being torn down waits for it.
  /// [stillWanted] reaches the decoder's own wait loop, so the wait is a tick
  /// rather than the rest of the track.
  Spectrogram load(const QString& path, const CancelFn& stillWanted) const;
  static double bandCenterHz(int index, int sampleRateHz);

 private:
  CancellablePcmLoader loader_;
};

struct SpectrumHold {
  static constexpr double kDecay = 0.86;
  static constexpr double kPeakDecay = 0.97;
  /// Playback stopped. Bars and peaks fall together: the musical release keeps
  /// peaks near the top for seconds, which reads as a track still sounding.
  static constexpr double kRestDecay = 0.72;
  /// Below this a bar is sub-pixel in the display well.
  static constexpr double kRestFloor = 0.004;

  std::array<qreal, AudioLevels::kBandCount> bars{};
  std::array<qreal, AudioLevels::kBandCount> peaks{};

  void apply(const AudioLevels& frame);
  /// One frame of the fall to rest, for when nothing is playing.
  void release();
  bool atRest() const;
  void reset();
};

}  // namespace tramp
