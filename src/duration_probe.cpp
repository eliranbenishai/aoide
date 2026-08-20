#include "duration_probe.h"

#include <QFile>
#ifdef TRAMP_HAVE_MPV
#include <mpv/client.h>
#include <QByteArray>
#endif
#include <QtEndian>

namespace tramp {
namespace {

quint32 u32le(const QByteArray& bytes, int offset) {
  return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
}

quint16 u16le(const QByteArray& bytes, int offset) {
  return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
}

bool eq4(const QByteArray& bytes, int offset, const char* ascii) {
  return bytes.size() >= offset + 4 && bytes[offset] == ascii[0] && bytes[offset + 1] == ascii[1] &&
         bytes[offset + 2] == ascii[2] && bytes[offset + 3] == ascii[3];
}

#ifdef TRAMP_HAVE_MPV
void check(int rc) {
  if (rc < 0) throw rc;
}

mpv_handle* createProbeMpv() {
  mpv_handle* mpv = mpv_create();
  if (!mpv) return nullptr;
  auto opt = [&](const char* key, const char* value) {
    check(mpv_set_option_string(mpv, key, value));
  };
  opt("config", "no");
  opt("vo", "null");
  opt("ao", "null");
  opt("vid", "no");
  opt("pause", "yes");
  opt("idle", "yes");
  opt("terminal", "no");
  opt("audio-display", "no");
  if (mpv_initialize(mpv) < 0) {
    mpv_terminate_destroy(mpv);
    return nullptr;
  }
  mpv_observe_property(mpv, 0, "metadata", MPV_FORMAT_NODE);
  return mpv;
}

void readMpvMetadata(mpv_handle* mpv, QString& title, QString& artist, QString& album) {
  mpv_node node{};
  if (mpv_get_property(mpv, "metadata", MPV_FORMAT_NODE, &node) < 0) return;
  if (node.format == MPV_FORMAT_NODE_MAP && node.u.list) {
    auto take = [&](const char* want, QString& dest) {
      for (int i = 0; i < node.u.list->num; ++i) {
        if (QByteArray(node.u.list->keys[i]).toLower() != want) continue;
        const mpv_node& val = node.u.list->values[i];
        if (val.format == MPV_FORMAT_STRING && val.u.string) {
          dest = QString::fromUtf8(val.u.string);
        }
      }
    };
    take("title", title);
    take("artist", artist);
    take("album", album);
  }
  mpv_free_node_contents(&node);
}

std::optional<ProbedAudio> probeWithMpv(mpv_handle* mpv, const QString& path) {
  const QByteArray encoded = path.toUtf8();
  const char* cmd[] = {"loadfile", encoded.constData(), "replace", nullptr};
  if (mpv_command(mpv, cmd) < 0) return std::nullopt;
  bool loaded = false;
  for (int i = 0; i < 80; ++i) {
    mpv_event* ev = mpv_wait_event(mpv, 0.25);
    if (!ev) continue;
    if (ev->event_id == MPV_EVENT_FILE_LOADED) {
      loaded = true;
      break;
    }
    if (ev->event_id == MPV_EVENT_END_FILE || ev->event_id == MPV_EVENT_SHUTDOWN) break;
  }
  if (!loaded) return std::nullopt;
  ProbedAudio out;
  double secs = 0;
  if (mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &secs) >= 0 && secs > 0) {
    out.durationMs = qint64(secs * 1000.0);
  }
  readMpvMetadata(mpv, out.title, out.artist, out.album);
  if (out.title.isEmpty()) {
    for (int i = 0; i < 20; ++i) {
      mpv_event* ev = mpv_wait_event(mpv, 0.1);
      if (!ev) continue;
      if (ev->event_id == MPV_EVENT_END_FILE || ev->event_id == MPV_EVENT_SHUTDOWN) break;
      readMpvMetadata(mpv, out.title, out.artist, out.album);
      if (!out.title.isEmpty()) break;
    }
  }
  if (!out.durationMs && out.title.isEmpty() && out.artist.isEmpty() && out.album.isEmpty()) {
    return std::nullopt;
  }
  return out;
}
#endif

}  // namespace

std::optional<qint64> probeWavDurationMs(const QByteArray& bytes) {
  if (bytes.size() < 44 || !eq4(bytes, 0, "RIFF") || !eq4(bytes, 8, "WAVE")) return std::nullopt;
  int offset = 12;
  int sampleRate = 0;
  int channels = 0;
  int bitsPerSample = 0;
  int dataBytes = -1;
  while (offset + 8 <= bytes.size()) {
    const quint32 size = u32le(bytes, offset + 4);
    if (eq4(bytes, offset, "fmt ") && size >= 16 && offset + 8 + 16 <= bytes.size()) {
      channels = u16le(bytes, offset + 10);
      sampleRate = int(u32le(bytes, offset + 12));
      bitsPerSample = u16le(bytes, offset + 22);
    } else if (eq4(bytes, offset, "data")) {
      dataBytes = int(size);
      break;
    }
    const int step = 8 + int(size) + int(size % 2);
    if (step <= 0) break;
    offset += step;
  }
  if (sampleRate <= 0 || channels <= 0 || bitsPerSample <= 0 || dataBytes < 0) return std::nullopt;
  const qint64 frameBytes = qint64(channels) * (bitsPerSample / 8);
  if (frameBytes <= 0) return std::nullopt;
  return (qint64(dataBytes) * 1000) / (frameBytes * sampleRate);
}

std::optional<qint64> probeAudioDurationMs(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) return std::nullopt;
  const QByteArray head = file.read(64 * 1024);
  if (const auto wav = probeWavDurationMs(head)) return wav;
#ifdef TRAMP_HAVE_MPV
  mpv_handle* mpv = createProbeMpv();
  if (!mpv) return std::nullopt;
  std::optional<qint64> ms;
  try {
    if (const auto probed = probeWithMpv(mpv, path)) ms = probed->durationMs;
  } catch (...) {
    ms = std::nullopt;
  }
  mpv_terminate_destroy(mpv);
  return ms;
#else
  return std::nullopt;
#endif
}

void probeAudioDurations(const QStringList& paths, const std::function<bool()>& stillWanted,
                         const std::function<void(const QString&, const ProbedAudio&)>& onProbed) {
#ifdef TRAMP_HAVE_MPV
  mpv_handle* mpv = nullptr;
#endif
  for (const QString& path : paths) {
    if (stillWanted && !stillWanted()) break;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
      if (const auto wav = probeWavDurationMs(file.read(64 * 1024))) {
        ProbedAudio probed;
        probed.durationMs = wav;
        onProbed(path, probed);
        continue;
      }
    }
#ifdef TRAMP_HAVE_MPV
    if (!mpv) mpv = createProbeMpv();
    if (!mpv) continue;
    try {
      if (const auto probed = probeWithMpv(mpv, path)) onProbed(path, *probed);
    } catch (...) {
    }
#endif
  }
#ifdef TRAMP_HAVE_MPV
  if (mpv) mpv_terminate_destroy(mpv);
#endif
}

}  // namespace tramp
