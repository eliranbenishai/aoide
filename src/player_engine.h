#pragma once

#include "audio_output.h"
#include "track.h"

#include <QString>
#include <QVector>
#include <functional>
#include <optional>

namespace aoide {

struct AudioFormatInfo {
  std::optional<int> bitrateKbps;
  std::optional<int> sampleRateHz;
  std::optional<int> channels;
};

class PlayerEngine {
 public:
  virtual ~PlayerEngine() = default;
  virtual void open(const Track& track) = 0;
  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void stop() = 0;
  virtual void seekMs(qint64 positionMs) = 0;
  virtual void setVolume(double volume) = 0;
  virtual void setForceMono(bool enabled) = 0;
  virtual void setEqualizerAf(const QString& af) = 0;
  virtual QVector<AudioOutputDevice> listAudioOutputs() { return {}; }
  virtual void setAudioDevice(const QString& name) { Q_UNUSED(name); }
  virtual void setAudioExclusive(bool enabled) { Q_UNUSED(enabled); }
  virtual void dispose() = 0;
  /// Snapshot of the playback clock. `-1` means the engine has no reading.
  virtual qint64 queryPositionMs() { return -1; }

  std::function<void(bool)> onPlaying;
  std::function<void(qint64)> onPosition;
  std::function<void(qint64)> onDuration;
  std::function<void()> onCompleted;
  std::function<void(AudioFormatInfo)> onFormat;
  std::function<void(QString)> onError;
  std::function<void(QString path, QString title, QString artist, QString album, qint64 durationMs)>
      onMetadata;
};

/// Inert stand-in used by tests. It reports playback so transport behaviour can
/// be exercised without an audio backend — see `MissingAudioEngine` for what the
/// application uses when it genuinely cannot play.
class NullEngine : public PlayerEngine {
 public:
  void open(const Track&) override {}
  void play() override {
    if (onPlaying) onPlaying(true);
  }
  void pause() override {
    if (onPlaying) onPlaying(false);
  }
  void stop() override {
    if (onPlaying) onPlaying(false);
    if (onPosition) onPosition(0);
  }
  void seekMs(qint64 positionMs) override {
    if (onPosition) onPosition(positionMs);
  }
  void setVolume(double) override {}
  void setForceMono(bool) override {}
  void setEqualizerAf(const QString&) override {}
  void dispose() override {}
};

/// What the application falls back to when there is no usable audio backend:
/// libmpv absent from the build, or present and refusing to initialise. It
/// refuses the open with a reason, so the transport stays stopped and the panel
/// says why. Reporting playback of silence instead is how a package built
/// without an engine managed to look healthy.
class MissingAudioEngine : public PlayerEngine {
 public:
  explicit MissingAudioEngine(QString reason) : reason_(std::move(reason)) {}

  void open(const Track&) override {
    if (onFormat) onFormat({});
    if (onError) onError(reason_);
  }
  void play() override {}
  void pause() override {}
  void stop() override {
    if (onPlaying) onPlaying(false);
    if (onPosition) onPosition(0);
  }
  void seekMs(qint64) override {}
  void setVolume(double) override {}
  void setForceMono(bool) override {}
  void setEqualizerAf(const QString&) override {}
  void dispose() override {}

 private:
  QString reason_;
};

}  // namespace aoide
