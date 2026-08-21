#pragma once

#include "wav_reader.h"

#include <QString>
#include <functional>

namespace tramp {

class MpvPcmDecoder {
 public:
  /// False means nobody wants the result any more.
  using CancelFn = std::function<bool()>;

  /// Renders the whole track through a throwaway mpv handle, which takes as long
  /// as the track is long. [stillWanted] is asked on every wait tick, and a no
  /// tears the handle down there and then rather than at the end of the file.
  PcmBuffer decode(const QString& path, const CancelFn& stillWanted = {}) const;
};

}  // namespace tramp
