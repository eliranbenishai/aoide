#pragma once

#include <QByteArray>
#include <QVector>

namespace aoide {

struct PcmBuffer {
  QVector<double> samples;
  int sampleRateHz = 0;
};

class WavReader {
 public:
  PcmBuffer read(const QByteArray& bytes) const;
};

}  // namespace aoide
