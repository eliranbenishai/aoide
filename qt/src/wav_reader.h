#pragma once

#include <QByteArray>
#include <QVector>

namespace tramp {

struct PcmBuffer {
  QVector<double> samples;
  int sampleRateHz = 0;
};

class WavReader {
 public:
  PcmBuffer read(const QByteArray& bytes) const;
};

}  // namespace tramp
