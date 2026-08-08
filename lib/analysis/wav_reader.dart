import 'dart:typed_data';

/// Mono (or downmixed) PCM samples from a WAV byte buffer.
class WavPcm {
  const WavPcm({
    required this.samples,
    required this.sampleRateHz,
    required this.channels,
  });

  final Float64List samples;
  final int sampleRateHz;
  final int channels;
}

/// Parses PCM and WAVE_FORMAT_EXTENSIBLE WAVs (mpv `ao=pcm` output).
class WavReader {
  const WavReader();

  WavPcm read(Uint8List bytes) {
    if (bytes.length < 12) {
      throw const FormatException('WAV too short');
    }
    if (!_eq(bytes, 0, 'RIFF') || !_eq(bytes, 8, 'WAVE')) {
      throw const FormatException('not a RIFF/WAVE file');
    }

    var offset = 12;
    int? sampleRate;
    int? channels;
    int? bitsPerSample;
    int? audioFormat;
    Uint8List? data;

    while (offset + 8 <= bytes.length) {
      final id = String.fromCharCodes(bytes.sublist(offset, offset + 4));
      final size = ByteData.sublistView(bytes, offset + 4, offset + 8)
          .getUint32(0, Endian.little);
      final body = offset + 8;
      if (body + size > bytes.length) break;

      if (id == 'fmt ') {
        final view = ByteData.sublistView(bytes, body, body + size);
        audioFormat = view.getUint16(0, Endian.little);
        channels = view.getUint16(2, Endian.little);
        sampleRate = view.getUint32(4, Endian.little);
        bitsPerSample = view.getUint16(14, Endian.little);
        // WAVE_FORMAT_EXTENSIBLE: real format is at cbSize offset.
        if (audioFormat == 0xFFFE && size >= 40) {
          audioFormat = view.getUint16(24, Endian.little);
        }
      } else if (id == 'data') {
        data = bytes.sublist(body, body + size);
      }

      offset = body + size + (size.isOdd ? 1 : 0);
    }

    if (sampleRate == null ||
        channels == null ||
        bitsPerSample == null ||
        audioFormat == null ||
        data == null) {
      throw const FormatException('WAV missing fmt/data');
    }
    if (audioFormat != 1 && audioFormat != 3) {
      throw FormatException('unsupported WAV format $audioFormat');
    }
    if (bitsPerSample != 16 && bitsPerSample != 32 && bitsPerSample != 24) {
      throw FormatException('unsupported bits $bitsPerSample');
    }

    final mono = _decodeMono(
      data,
      channels: channels,
      bitsPerSample: bitsPerSample,
      float: audioFormat == 3,
    );
    return WavPcm(
      samples: mono,
      sampleRateHz: sampleRate,
      channels: channels,
    );
  }

  static Float64List _decodeMono(
    Uint8List data, {
    required int channels,
    required int bitsPerSample,
    required bool float,
  }) {
    final bytesPerSample = bitsPerSample ~/ 8;
    final frameSize = bytesPerSample * channels;
    if (frameSize == 0) return Float64List(0);
    final frames = data.length ~/ frameSize;
    final out = Float64List(frames);
    final view = ByteData.sublistView(data);

    for (var i = 0; i < frames; i++) {
      var sum = 0.0;
      for (var ch = 0; ch < channels; ch++) {
        final off = i * frameSize + ch * bytesPerSample;
        if (float && bitsPerSample == 32) {
          sum += view.getFloat32(off, Endian.little);
        } else if (bitsPerSample == 16) {
          sum += view.getInt16(off, Endian.little) / 32768.0;
        } else if (bitsPerSample == 24) {
          final b0 = data[off];
          final b1 = data[off + 1];
          final b2 = data[off + 2];
          var v = b0 | (b1 << 8) | (b2 << 16);
          if (v & 0x800000 != 0) v |= ~0xFFFFFF;
          sum += v / 8388608.0;
        } else if (bitsPerSample == 32) {
          sum += view.getInt32(off, Endian.little) / 2147483648.0;
        }
      }
      out[i] = sum / channels;
    }
    return out;
  }

  static bool _eq(Uint8List bytes, int offset, String ascii) {
    for (var i = 0; i < ascii.length; i++) {
      if (bytes[offset + i] != ascii.codeUnitAt(i)) return false;
    }
    return true;
  }
}
