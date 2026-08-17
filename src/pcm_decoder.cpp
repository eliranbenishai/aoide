#include "pcm_decoder.h"

#include "wav_reader.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QTemporaryDir>
#include <mpv/client.h>
#include <stdexcept>

namespace tramp {
namespace {

void check(int rc, const char* what) {
  if (rc < 0) {
    throw std::runtime_error(QStringLiteral("%1 failed: %2")
                                 .arg(QLatin1String(what))
                                 .arg(QString::fromUtf8(mpv_error_string(rc)))
                                 .toStdString());
  }
}

void renderToWav(const QString& inputPath, const QString& outputPath) {
  mpv_handle* mpv = mpv_create();
  if (!mpv) throw std::runtime_error("mpv_create failed");

  try {
    auto opt = [&](const char* key, const QByteArray& value) {
      check(mpv_set_option_string(mpv, key, value.constData()), key);
    };
    opt("config", "no");
    opt("vo", "null");
    opt("vid", "no");
    opt("audio-display", "no");
    opt("untimed", "yes");
    opt("terminal", "no");
    opt("ao", "pcm");
    opt("ao-pcm-waveheader", "yes");
    opt("ao-pcm-file", QFile::encodeName(outputPath));
    opt("audio-channels", "mono");
    check(mpv_initialize(mpv), "mpv_initialize");

    const QByteArray path = inputPath.toUtf8();
    const char* cmd[] = {"loadfile", path.constData(), "replace", nullptr};
    check(mpv_command(mpv, cmd), "loadfile");

    const int deadlineTicks = 120 * 4;
    for (int i = 0; i < deadlineTicks; ++i) {
      mpv_event* ev = mpv_wait_event(mpv, 0.25);
      if (!ev) continue;
      if (ev->event_id == MPV_EVENT_END_FILE || ev->event_id == MPV_EVENT_SHUTDOWN) {
        mpv_terminate_destroy(mpv);
        return;
      }
    }
    throw std::runtime_error("timeout waiting for PCM end-file");
  } catch (...) {
    mpv_terminate_destroy(mpv);
    throw;
  }
}

}  // namespace

PcmBuffer MpvPcmDecoder::decode(const QString& path) const {
  QTemporaryDir work(QDir::temp().filePath(QStringLiteral("tramp_pcm_XXXXXX")));
  if (!work.isValid()) throw std::runtime_error("PCM temp dir failed");
  const QString outPath = work.filePath(QStringLiteral("out.wav"));
  renderToWav(path, outPath);
  QFile file(outPath);
  if (!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("PCM wav missing");
  }
  return WavReader().read(file.readAll());
}

}  // namespace tramp
