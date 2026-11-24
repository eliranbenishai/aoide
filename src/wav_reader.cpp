#include "wav_reader.h"

#include <QBuffer>
#include <QByteArray>
#include <QtEndian>
#include <cstring>
#include <stdexcept>

namespace aoide {
namespace {

quint32 u32le(const QByteArray& bytes, qsizetype offset) {
  return qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
}

quint16 u16le(const QByteArray& bytes, qsizetype offset) {
  return qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(bytes.constData() + offset));
}

qint32 i32le(const QByteArray& bytes, qsizetype offset) {
  return qint32(u32le(bytes, offset));
}

qint16 i16le(const QByteArray& bytes, qsizetype offset) {
  return qint16(u16le(bytes, offset));
}

float f32le(const QByteArray& bytes, qsizetype offset) {
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
  const qsizetype frames = data.size() / frameSize;
  QVector<double> out(frames);
  for (qsizetype i = 0; i < frames; ++i) {
    double sum = 0;
    for (int ch = 0; ch < channels; ++ch) {
      const qsizetype off = i * frameSize + ch * bytesPerSample;
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

WavReader::WavReader(QIODevice& device) : device_(device) {
  const QByteArray header = device.read(12);
  if (!eq4(header, 0, "RIFF") || !eq4(header, 8, "WAVE")) {
    throw std::runtime_error("not a RIFF/WAVE file");
  }
  int format = 0;
  qint64 dataBytes = 0;
  const qint64 fileSize = device.size();
  while (device.pos() + 8 <= fileSize) {
    const QByteArray chunk = device.read(8);
    if (chunk.size() != 8) throw std::runtime_error("incomplete WAV chunk");
    const qint64 size = u32le(chunk, 4);
    const qint64 body = device.pos();
    if (size > fileSize - body) throw std::runtime_error("incomplete WAV chunk");
    if (eq4(chunk, 0, "fmt ")) {
      const QByteArray fmt = device.read(qMin<qint64>(size, 40));
      if (fmt.size() < 16) throw std::runtime_error("incomplete WAV format");
      format = u16le(fmt, 0);
      channels_ = u16le(fmt, 2);
      sampleRate_ = int(u32le(fmt, 4));
      bitsPerSample_ = u16le(fmt, 14);
      if (format == 0xfffe && fmt.size() >= 40) format = u16le(fmt, 24);
    } else if (eq4(chunk, 0, "data")) {
      dataOffset_ = body;
      dataBytes = size;
    }
    if (dataOffset_ >= 0 && format != 0) break;
    if (!device.seek(body + size + size % 2)) throw std::runtime_error("invalid WAV chunk");
  }
  if (dataOffset_ < 0 || channels_ < 1 || sampleRate_ < 1 ||
      (format != 1 && format != 3) || (format == 3 && bitsPerSample_ != 32) ||
      (bitsPerSample_ != 16 && bitsPerSample_ != 24 && bitsPerSample_ != 32)) {
    throw std::runtime_error("invalid or unsupported WAV format");
  }
  isFloat_ = format == 3;
  frameCount_ = dataBytes / (channels_ * (bitsPerSample_ / 8));
}

QVector<double> WavReader::readFrames(qint64 first, qint64 count) const {
  const int stride = channels_ * (bitsPerSample_ / 8);
  if (first < 0 || count < 0 || first > frameCount_ || count > frameCount_ - first ||
      !device_.seek(dataOffset_ + first * stride)) throw std::runtime_error("invalid WAV range");
  const QByteArray bytes = device_.read(count * stride);
  if (bytes.size() != count * stride) throw std::runtime_error("incomplete PCM samples");
  return decodeMono(bytes, channels_, bitsPerSample_, isFloat_);
}

PcmBuffer WavReader::read(const QByteArray& bytes) {
  QBuffer input;
  input.setData(bytes);
  input.open(QIODevice::ReadOnly);
  WavReader reader(input);
  return {reader.readFrames(0, reader.frameCount()), reader.sampleRateHz()};
}

}  // namespace aoide
