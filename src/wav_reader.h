#pragma once

#include <QByteArray>
#include <QIODevice>
#include <QVector>

namespace aoide {

struct PcmBuffer {
  QVector<double> samples;
  int sampleRateHz = 0;
};

class WavReader {
 public:
  explicit WavReader(QIODevice& device);
  qint64 frameCount() const { return frameCount_; }
  int sampleRateHz() const { return sampleRate_; }
  QVector<double> readFrames(qint64 first, qint64 count) const;
  static PcmBuffer read(const QByteArray& bytes);

 private:
  QIODevice& device_;
  qint64 dataOffset_ = -1;
  qint64 frameCount_ = 0;
  int channels_ = 0;
  int sampleRate_ = 0;
  int bitsPerSample_ = 0;
  bool isFloat_ = false;
};

}  // namespace aoide
