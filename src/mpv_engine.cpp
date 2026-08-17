#include "mpv_engine.h"

#include <QByteArray>
#include <QMetaObject>
#include <algorithm>
#include <cmath>
#include <mpv/client.h>

namespace tramp {
namespace {

QString mpvError(int status) { return QString::fromUtf8(mpv_error_string(status)); }

}  // namespace

MpvEngine::MpvEngine(QObject* parent) : QObject(parent) {
  mpv_ = mpv_create();
  if (!mpv_) return;
  mpv_set_option_string(mpv_, "vo", "null");
  mpv_set_option_string(mpv_, "video", "no");
  mpv_set_option_string(mpv_, "terminal", "no");
  mpv_set_option_string(mpv_, "idle", "yes");
  mpv_set_option_string(mpv_, "keep-open", "no");
  mpv_set_option_string(mpv_, "osc", "no");
  mpv_set_option_string(mpv_, "input-default-bindings", "no");
  mpv_set_option_string(mpv_, "input-vo-keyboard", "no");
  mpv_set_wakeup_callback(
      mpv_,
      [](void* ctx) {
        auto* self = static_cast<MpvEngine*>(ctx);
        if (!self->drainQueued_.exchange(true)) {
          QMetaObject::invokeMethod(self, "drainEvents", Qt::QueuedConnection);
        }
      },
      this);
  if (mpv_initialize(mpv_) < 0) {
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
  if (onFormat) onFormat({});
  const QByteArray path = track.path.toUtf8();
  const char* cmd[] = {"loadfile", path.constData(), "replace", nullptr};
  const int status = mpv_command(mpv_, cmd);
  if (status < 0 && onError) {
    onError(mpvError(status));
  }
  applyPending();
}

void MpvEngine::play() {
  if (!mpv_) return;
  mpv_set_property_string(mpv_, "pause", "no");
}

void MpvEngine::pause() {
  if (!mpv_) return;
  mpv_set_property_string(mpv_, "pause", "yes");
}

void MpvEngine::stop() {
  if (!mpv_) return;
  const char* cmd[] = {"stop", nullptr};
  mpv_command(mpv_, cmd);
  if (onPlaying) onPlaying(false);
  if (onPosition) onPosition(0);
}

void MpvEngine::seekMs(qint64 positionMs) {
  if (!mpv_) return;
  const QByteArray secs = QByteArray::number(positionMs / 1000.0, 'f', 3);
  const char* cmd[] = {"seek", secs.constData(), "absolute", nullptr};
  mpv_command(mpv_, cmd);
}

void MpvEngine::setVolume(double volume) {
  pendingVolume_ = volume;
  if (!mpv_) return;
  double v = std::clamp(volume, 0.0, 1.0) * 100.0;
  mpv_set_property(mpv_, "volume", MPV_FORMAT_DOUBLE, &v);
}

void MpvEngine::setForceMono(bool enabled) {
  pendingMono_ = enabled;
  if (!mpv_) return;
  mpv_set_property_string(mpv_, "audio-channels", enabled ? "mono" : "auto");
}

void MpvEngine::setEqualizerAf(const QString& af) {
  pendingAf_ = af;
  if (!mpv_) return;
  mpv_set_property_string(mpv_, "af", af.toUtf8().constData());
}

void MpvEngine::applyPending() {
  setVolume(pendingVolume_);
  setForceMono(pendingMono_);
  setEqualizerAf(pendingAf_);
}

qint64 MpvEngine::queryPositionMs() {
  if (!mpv_) return -1;
  double secs = 0;
  if (mpv_get_property(mpv_, "time-pos", MPV_FORMAT_DOUBLE, &secs) < 0) return -1;
  if (!std::isfinite(secs) || secs < 0) return -1;
  return qint64(secs * 1000.0);
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
      case MPV_EVENT_END_FILE: {
        const auto* end = static_cast<mpv_event_end_file*>(event->data);
        if (end && end->reason == MPV_END_FILE_REASON_EOF && onCompleted) {
          onCompleted();
        } else if (end && end->reason == MPV_END_FILE_REASON_ERROR && onError) {
          onError(mpvError(end->error));
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
          QString title, artist, album;
          if (node->format == MPV_FORMAT_NODE_MAP) {
            auto take = [&](const char* want, QString& dest) {
              for (int i = 0; i < node->u.list->num; ++i) {
                if (QByteArray(node->u.list->keys[i]).toLower() != want) continue;
                const mpv_node& val = node->u.list->values[i];
                if (val.format == MPV_FORMAT_STRING) dest = QString::fromUtf8(val.u.string);
              }
            };
            take("title", title);
            take("artist", artist);
            take("album", album);
          }
          qint64 duration = 0;
          double secs = 0;
          if (mpv_get_property(mpv_, "duration", MPV_FORMAT_DOUBLE, &secs) >= 0) {
            duration = qint64(secs * 1000.0);
          }
          onMetadata(currentPath_, title, artist, album, duration);
        }
        break;
      }
      default:
        break;
    }
  }
}

}  // namespace tramp
