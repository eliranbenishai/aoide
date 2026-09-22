#include "mpv_engine.h"
#include "mpv_metadata.h"

#include "audio_output.h"
#include "equalizer.h"

#include <QByteArray>
#include <QDebug>
#include <QMetaObject>
#include <algorithm>
#include <cmath>
#include <mpv/client.h>

namespace aoide {
namespace {

QString mpvError(int status) { return QString::fromUtf8(mpv_error_string(status)); }

bool checkMpv(int status, const QString& operation) {
  if (status >= 0) return true;
  qWarning().noquote() << "Aoide audio:" << operation << "failed:" << mpvError(status)
                       << QStringLiteral("(%1)").arg(status);
  return false;
}

int setString(mpv_handle* mpv, const char* name, const QByteArray& value) {
  const int status = mpv_set_property_string(mpv, name, value.constData());
  checkMpv(status, QStringLiteral("set %1=%2").arg(QLatin1String(name),
                                                 QString::fromUtf8(value)));
  return status;
}

}  // namespace

MpvEngine::MpvEngine(QObject* parent) : QObject(parent) {
  mpv_ = mpv_create();
  if (!mpv_) {
    qWarning() << "Aoide audio: mpv_create failed";
    return;
  }
  auto option = [&](const char* name, const char* value) {
    checkMpv(mpv_set_option_string(mpv_, name, value),
             QStringLiteral("option %1=%2").arg(QLatin1String(name), QLatin1String(value)));
  };
  option("vo", "null");
  option("video", "no");
  option("terminal", "no");
  option("idle", "yes");
  option("keep-open", "no");
  option("osc", "no");
  option("input-default-bindings", "no");
  option("input-vo-keyboard", "no");
  // +12 dB at full slider volume needs 158.49 on mpv's cubic volume scale.
  option("volume-max", "160");
  checkMpv(mpv_request_log_messages(mpv_, "warn"), QStringLiteral("request mpv logs"));
  mpv_set_wakeup_callback(
      mpv_,
      [](void* ctx) {
        auto* self = static_cast<MpvEngine*>(ctx);
        if (!self->drainQueued_.exchange(true)) {
          QMetaObject::invokeMethod(self, "drainEvents", Qt::QueuedConnection);
        }
      },
      this);
  if (!checkMpv(mpv_initialize(mpv_), QStringLiteral("mpv_initialize"))) {
    mpv_destroy(mpv_);
    mpv_ = nullptr;
    return;
  }
  observe("pause", MPV_FORMAT_FLAG);
  observe("duration", MPV_FORMAT_DOUBLE);
  observe("eof-reached", MPV_FORMAT_FLAG);
  observe("audio-params", MPV_FORMAT_NODE);
  observe("metadata", MPV_FORMAT_NODE);
}

MpvEngine::~MpvEngine() { dispose(); }

void MpvEngine::observe(const char* name, int format) {
  mpv_observe_property(mpv_, 0, name, mpv_format(format));
}

void MpvEngine::open(const Track& track) {
  if (!mpv_) return;
  currentPath_ = track.path;
  lastPositionMs_ = 0;
  restoringAfterEqFailure_ = false;
  retryWithoutEq_ = false;
  if (onFormat) onFormat({});
  applyPending();
  loadCurrent();
}

void MpvEngine::loadCurrent() {
  const QByteArray path = currentPath_.toUtf8();
  const char* cmd[] = {"loadfile", path.constData(), "replace", nullptr};
  const int status = mpv_command(mpv_, cmd);
  if (!checkMpv(status, QStringLiteral("loadfile"))) {
    restoringAfterEqFailure_ = false;
    if (onError) onError(mpvError(status));
  }
}

void MpvEngine::play() {
  paused_ = false;
  if (!mpv_) return;
  setString(mpv_, "pause", "no");
}

void MpvEngine::pause() {
  paused_ = true;
  if (!mpv_) return;
  setString(mpv_, "pause", "yes");
}

void MpvEngine::stop() {
  currentPath_.clear();
  lastPositionMs_ = 0;
  restoringAfterEqFailure_ = false;
  retryWithoutEq_ = false;
  if (!mpv_) return;
  const char* cmd[] = {"stop", nullptr};
  mpv_command(mpv_, cmd);
  if (onPlaying) onPlaying(false);
  if (onPosition) onPosition(0);
}

void MpvEngine::seekMs(qint64 positionMs) {
  lastPositionMs_ = std::max(qint64(0), positionMs);
  if (!mpv_) return;
  const QByteArray secs = QByteArray::number(positionMs / 1000.0, 'f', 3);
  const char* cmd[] = {"seek", secs.constData(), "absolute", nullptr};
  mpv_command(mpv_, cmd);
}

void MpvEngine::setVolume(double volume) {
  pendingVolume_ = std::isfinite(volume) ? std::clamp(volume, 0.0, 1.0) : 1.0;
  applyVolume();
}

void MpvEngine::applyVolume() {
  if (!mpv_) return;
  // mpv cubes volume/100 to get amplitude. Preserve the user's slider value
  // and multiply by the cube root of the preamp's amplitude gain.
  double v = pendingVolume_ * 100.0 * std::pow(10.0, pendingPreampDb_ / 60.0);
  const int status = mpv_set_property(mpv_, "volume", MPV_FORMAT_DOUBLE, &v);
  if (!checkMpv(status, QStringLiteral("volume=%1").arg(v)) && !pendingAf_.isEmpty())
    bypassEqualizer(QStringLiteral("preamp: %1 (%2)").arg(mpvError(status)).arg(status));
}

void MpvEngine::setForceMono(bool enabled) {
  pendingMono_ = enabled;
  if (!mpv_) return;
  setString(mpv_, "audio-channels", enabled ? "mono" : "auto");
}

void MpvEngine::setEqualizerAf(const QString& af, double preampDb) {
  pendingAf_ = af;
  pendingPreampDb_ = af.isEmpty() ? 0 : EqualizerSettings::clampGain(preampDb);
  if (!mpv_) return;
  const int status = setString(mpv_, "af", af.toUtf8());
  qInfo().noquote() << "Aoide EQ: af=" << af << "result=" << status << mpvError(status);
  if (status < 0 && !pendingAf_.isEmpty()) {
    bypassEqualizer(QStringLiteral("setting af: %1 (%2)").arg(mpvError(status)).arg(status));
  } else {
    applyVolume();
  }
}

void MpvEngine::bypassEqualizer(const QString& reason) {
  qWarning().noquote() << "Aoide EQ: bypassing equalizer; continuing without EQ."
                       << reason << "af=" << pendingAf_;
  // Runtime state only: never erase the listener's saved gains, preamp or enabled flag.
  // Clearing this also bounds an asynchronous load retry to one attempt.
  retryWithoutEq_ = !pendingAf_.isEmpty() && !currentPath_.isEmpty();
  pendingAf_.clear();
  pendingPreampDb_ = 0;
  setString(mpv_, "af", "");
  applyVolume();
}

QVector<AudioOutputDevice> MpvEngine::listAudioOutputs() {
  if (!mpv_) return {};
  mpv_node root{};
  if (mpv_get_property(mpv_, "audio-device-list", MPV_FORMAT_NODE, &root) < 0) return {};
  QVector<AudioOutputDevice> devices;
  if (root.format == MPV_FORMAT_NODE_ARRAY && root.u.list) {
    const mpv_node_list* list = root.u.list;
    for (int i = 0; i < list->num; ++i) {
      const mpv_node& item = list->values[i];
      if (item.format != MPV_FORMAT_NODE_MAP || !item.u.list) continue;
      AudioOutputDevice device;
      const mpv_node_list* map = item.u.list;
      for (int k = 0; k < map->num; ++k) {
        const char* key = map->keys[k];
        const mpv_node& val = map->values[k];
        if (!key || val.format != MPV_FORMAT_STRING || !val.u.string) continue;
        if (QByteArray(key) == "name") device.name = QString::fromUtf8(val.u.string);
        if (QByteArray(key) == "description") device.description = QString::fromUtf8(val.u.string);
      }
      if (!device.name.isEmpty()) devices.push_back(device);
    }
  }
  mpv_free_node_contents(&root);
  return devices;
}

void MpvEngine::setAudioDevice(const QString& name) {
  pendingDevice_ = normalizeAudioDeviceName(name);
  if (!mpv_) return;
  setString(mpv_, "audio-device", pendingDevice_.toUtf8());
}

void MpvEngine::setAudioExclusive(bool enabled) {
  pendingExclusive_ = enabled;
  if (!mpv_) return;
  setString(mpv_, "audio-exclusive", enabled ? "yes" : "no");
}

void MpvEngine::applyPending() {
  setForceMono(pendingMono_);
  setEqualizerAf(pendingAf_, pendingPreampDb_);
  setAudioDevice(pendingDevice_);
  setAudioExclusive(pendingExclusive_);
}

qint64 MpvEngine::queryPositionMs() {
  if (!mpv_) return -1;
  double secs = 0;
  if (mpv_get_property(mpv_, "time-pos", MPV_FORMAT_DOUBLE, &secs) < 0) return -1;
  if (!std::isfinite(secs) || secs < 0) return -1;
  const qint64 position = qint64(secs * 1000.0);
  // The replacement file briefly reports zero before FILE_LOADED restores
  // the seek. A UI clock poll must not erase the position we are restoring.
  if (!restoringAfterEqFailure_) lastPositionMs_ = position;
  return position;
}

void MpvEngine::dispose() {
  if (!mpv_) return;
  mpv_set_wakeup_callback(mpv_, nullptr, nullptr);
  mpv_destroy(mpv_);
  mpv_ = nullptr;
}

void MpvEngine::drainEvents() {
  drainQueued_.store(false);
  if (!mpv_) return;
  while (true) {
    mpv_event* event = mpv_wait_event(mpv_, 0);
    if (event->event_id == MPV_EVENT_NONE) break;
    switch (event->event_id) {
      case MPV_EVENT_LOG_MESSAGE: {
        const auto* log = static_cast<mpv_event_log_message*>(event->data);
        if (log) qWarning().noquote() << "Aoide mpv:" << log->prefix
                                     << QString::fromUtf8(log->text).trimmed();
        // mpv can disable a graph after negotiating its first audio frame,
        // without returning an error from set_property or END_FILE. Clear
        // the whole EQ, including preamp, when it reports that runtime failure.
        if (log && !pendingAf_.isEmpty()) {
          const QByteArray prefix(log->prefix), message(log->text);
          const bool disabled = prefix == "af" && message.contains("Disabling filter") &&
                                message.contains("because it has failed");
          const bool rejected = prefix == "cplayer" &&
                                message.contains("Audio filter initialized failed!");
          if (disabled || rejected) bypassEqualizer(QString::fromUtf8(message).trimmed());
        }
        break;
      }
      case MPV_EVENT_FILE_LOADED:
        if (restoringAfterEqFailure_) {
          restoringAfterEqFailure_ = false;
          if (lastPositionMs_ > 0) seekMs(lastPositionMs_);
          setString(mpv_, "pause", paused_ ? "yes" : "no");
        }
        break;
      case MPV_EVENT_END_FILE: {
        const auto* end = static_cast<mpv_event_end_file*>(event->data);
        if (end && end->reason == MPV_END_FILE_REASON_EOF && onCompleted) {
          onCompleted();
        } else if (end && end->reason == MPV_END_FILE_REASON_ERROR) {
          if (!pendingAf_.isEmpty() && !currentPath_.isEmpty()) {
            bypassEqualizer(QStringLiteral("playback: %1 (%2); retrying track once")
                                .arg(mpvError(end->error)).arg(end->error));
          }
          if (retryWithoutEq_ && !currentPath_.isEmpty()) {
            retryWithoutEq_ = false;
            restoringAfterEqFailure_ = true;
            loadCurrent();
            break;
          }
          checkMpv(end->error, QStringLiteral("playback"));
          if (onError) onError(mpvError(end->error));
        }
        break;
      }
      case MPV_EVENT_PROPERTY_CHANGE: {
        const auto* prop = static_cast<mpv_event_property*>(event->data);
        if (!prop || !prop->name) break;
        const QByteArray name(prop->name);
        if (name == "pause" && prop->format == MPV_FORMAT_FLAG && prop->data) {
          const bool paused = *static_cast<int*>(prop->data) != 0;
          if (onPlaying) onPlaying(!paused);
        } else if (name == "duration" && prop->format == MPV_FORMAT_DOUBLE && prop->data) {
          const double secs = *static_cast<double*>(prop->data);
          if (std::isfinite(secs) && secs >= 0 && onDuration) {
            onDuration(qint64(secs * 1000.0));
          }
        } else if (name == "audio-params" && prop->format == MPV_FORMAT_NODE && prop->data) {
          const auto* node = static_cast<mpv_node*>(prop->data);
          AudioFormatInfo info;
          if (node->format == MPV_FORMAT_NODE_MAP) {
            for (int i = 0; i < node->u.list->num; ++i) {
              const char* key = node->u.list->keys[i];
              const mpv_node& val = node->u.list->values[i];
              if (QByteArray(key) == "samplerate" && val.format == MPV_FORMAT_INT64) {
                info.sampleRateHz = int(val.u.int64);
              }
              if (QByteArray(key) == "channel-count" && val.format == MPV_FORMAT_INT64) {
                info.channels = int(val.u.int64);
              }
            }
          }
          double br = 0;
          if (mpv_get_property(mpv_, "audio-bitrate", MPV_FORMAT_DOUBLE, &br) >= 0 &&
              std::isfinite(br) && br > 0) {
            info.bitrateKbps = int(std::round(br / 1000.0));
          }
          if (onFormat) onFormat(info);
        } else if (name == "metadata" && prop->format == MPV_FORMAT_NODE && prop->data &&
                   onMetadata) {
          const auto* node = static_cast<mpv_node*>(prop->data);
          TrackMetadata metadata = metadataFromMpvNode(*node);
          double secs = 0;
          if (mpv_get_property(mpv_, "duration", MPV_FORMAT_DOUBLE, &secs) >= 0 &&
              std::isfinite(secs) && secs > 0) {
            metadata.durationMs = qint64(secs * 1000.0);
          }
          onMetadata(currentPath_, metadata);
        }
        break;
      }
      default:
        break;
    }
  }
}

}  // namespace aoide
