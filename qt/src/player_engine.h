#pragma once

#include "track.h"

#include <QString>
#include <functional>
#include <optional>

namespace tramp {

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
  virtual void dispose() = 0;

  std::function<void(bool)> onPlaying;
  std::function<void(qint64)> onPosition;
  std::function<void(qint64)> onDuration;
  std::function<void()> onCompleted;
  std::function<void(AudioFormatInfo)> onFormat;
  std::function<void(QString)> onError;
  std::function<void(QString path, QString title, QString artist, QString album, qint64 durationMs)>
      onMetadata;
};

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

}  // namespace tramp
