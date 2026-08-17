#pragma once

#include "wav_reader.h"

#include <QString>

namespace tramp {

class MpvPcmDecoder {
 public:
  PcmBuffer decode(const QString& path) const;
};

}  // namespace tramp
