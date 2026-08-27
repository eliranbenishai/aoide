#pragma once

#include "player_engine.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <atomic>

struct mpv_handle;

namespace aoide {

class MpvEngine : public QObject, public PlayerEngine {
  Q_OBJECT

 public:
  explicit MpvEngine(QObject* parent = nullptr);
  ~MpvEngine() override;

  bool available() const { return mpv_ != nullptr; }

  void open(const Track& track) override;
  void play() override;
  void pause() override;
  void stop() override;
  void seekMs(qint64 positionMs) override;
  void setVolume(double volume) override;
  void setForceMono(bool enabled) override;
  void setEqualizerAf(const QString& af) override;
  QVector<AudioOutputDevice> listAudioOutputs() override;
  void setAudioDevice(const QString& name) override;
  void setAudioExclusive(bool enabled) override;
  void dispose() override;
  qint64 queryPositionMs() override;

 private slots:
  void drainEvents();

 private:
  void observe(const char* name, int format);
  void applyPending();

  mpv_handle* mpv_ = nullptr;
  std::atomic<bool> drainQueued_{false};
  QString pendingAf_;
  double pendingVolume_ = 1.0;
  bool pendingMono_ = false;
  QString pendingDevice_ = kDefaultAudioDeviceName();
  bool pendingExclusive_ = false;
  QString currentPath_;
};

}  // namespace aoide
