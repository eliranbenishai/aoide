#include "wav_reader.h"

#include <QByteArray>
#include <QtEndian>
#include <cstring>
#include <stdexcept>

namespace tramp {
namespace {

quint32 u32le(const QByteArray& bytes, int offset) {
  return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
}

quint16 u16le(const QByteArray& bytes, int offset) {
  return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
}

qint32 i32le(const QByteArray& bytes, int offset) {
  return qint32(u32le(bytes, offset));
}

qint16 i16le(const QByteArray& bytes, int offset) {
  return qint16(u16le(bytes, offset));
}

float f32le(const QByteArray& bytes, int offset) {
  const quint32 bits = u32le(bytes, offset);
  float value = 0;
  static_assert(sizeof(float) == 4, "float must be 32-bit");
  memcpy(&value, &bits, 4);
  return value;
}

bool eq4(const QByteArray& bytes, int offset, const char* ascii) {
  return bytes.size() >= offset + 4 && bytes[offset] == ascii[0] && bytes[offset + 1] == ascii[1] &&
         bytes[offset + 2] == ascii[2] && bytes[offset + 3] == ascii[3];
}

QVector<double> decodeMono(const QByteArray& data, int channels, int bitsPerSample, bool isFloat) {
  const int bytesPerSample = bitsPerSample / 8;
  const int frameSize = bytesPerSample * channels;
  if (frameSize <= 0) return {};
  const int frames = data.size() / frameSize;
  QVector<double> out(frames);
  for (int i = 0; i < frames; ++i) {
    double sum = 0;
    for (int ch = 0; ch < channels; ++ch) {
      const int off = i * frameSize + ch * bytesPerSample;
      if (isFloat && bitsPerSample == 32) {
        sum += double(f32le(data, off));
      } else if (bitsPerSample == 16) {
        sum += double(i16le(data, off)) / 32768.0;
      } else if (bitsPerSample == 24) {
        const int b0 = uchar(data[off]);
        const int b1 = uchar(data[off + 1]);
        const int b2 = uchar(data[off + 2]);
        int v = b0 | (b1 << 8) | (b2 << 16);
        if (v & 0x800000) v |= ~0xFFFFFF;
        sum += double(v) / 8388608.0;
      } else if (bitsPerSample == 32) {
        sum += double(i32le(data, off)) / 2147483648.0;
      }
    }
    out[i] = sum / channels;
  }
  return out;
}

}  // namespace

PcmBuffer WavReader::read(const QByteArray& bytes) const {
  if (bytes.size() < 12 || !eq4(bytes, 0, "RIFF") || !eq4(bytes, 8, "WAVE")) {
    throw std::runtime_error("not a RIFF/WAVE file");
  }

  int offset = 12;
  int sampleRate = 0;
  int channels = 0;
  int bitsPerSample = 0;
  int audioFormat = 0;
  QByteArray data;
  bool haveFmt = false;
  bool haveData = false;

  while (offset + 8 <= bytes.size()) {
    const int size = int(u32le(bytes, offset + 4));
    const int body = offset + 8;
    if (body + size > bytes.size()) break;

    if (eq4(bytes, offset, "fmt ")) {
      audioFormat = u16le(bytes, body);
      channels = u16le(bytes, body + 2);
      sampleRate = int(u32le(bytes, body + 4));
      bitsPerSample = u16le(bytes, body + 14);
      if (audioFormat == 0xFFFE && size >= 40) {
        audioFormat = u16le(bytes, body + 24);
      }
      haveFmt = true;
    } else if (eq4(bytes, offset, "data")) {
      data = bytes.mid(body, size);
      haveData = true;
    }

    offset = body + size + (size % 2);
  }

  if (!haveFmt || !haveData) {
    throw std::runtime_error("WAV missing fmt/data");
  }
  if (audioFormat != 1 && audioFormat != 3) {
    throw std::runtime_error("unsupported WAV format");
  }
  if (bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32) {
    throw std::runtime_error("unsupported bits");
  }

  PcmBuffer pcm;
  pcm.samples = decodeMono(data, channels, bitsPerSample, audioFormat == 3);
  pcm.sampleRateHz = sampleRate;
  return pcm;
}

}  // namespace tramp
